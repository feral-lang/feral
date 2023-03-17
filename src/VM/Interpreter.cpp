#include "VM/Interpreter.hpp"

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/DynLib.hpp"

namespace fer
{

size_t MAX_RECURSE_COUNT = DEFAULT_MAX_RECURSE_COUNT;

Interpreter::Interpreter(RAIIParser &parser)
	: selfbin(env::getProcPath()), parser(parser), c(parser.getContext()),
	  argparser(parser.getCommandArgs()), tru(makeVarWithRef<VarBool>(nullptr, true)),
	  fals(makeVarWithRef<VarBool>(nullptr, false)), nil(makeVarWithRef<VarNil>(nullptr)),
	  exitcode(0), recurse_count(0), exit_called(false), recurse_count_exceeded(false)
{
	initTypeNames();

	// set core modules
	coremods.emplace_back("Core");
	coremods.emplace_back("Utils");

	// set feral base-directory
	selfbase = fs::parentDir(fs::parentDir(selfbin));
	selfbase = fs::absPath(selfbase.c_str());

	Span<StringRef> _cmdargs = argparser.getCodeExecArgs();
	cmdargs			 = makeVarWithRef<VarVec>(nullptr, _cmdargs.size(), false);
	for(auto &a : _cmdargs) {
		cmdargs->get().push_back(makeVarWithRef<VarStr>(nullptr, a));
	}

	includelocs.push_back(selfbase + "/include/feral");
	dlllocs.push_back(selfbase + "/lib/feral");

	String feral_paths = env::get("FERAL_PATHS");
	for(auto &path : stringDelim(feral_paths, ";")) {
		includelocs.push_back(String(path) + "/include/feral");
		dlllocs.push_back(String(path) + "/lib/feral");
	}
}

// TODO:
Interpreter::~Interpreter()
{
	decref(nil);
	decref(fals);
	decref(tru);
	decref(cmdargs);
	for(auto &typefn : typefns) {
		delete typefn.second;
	}
	for(auto &deinitfn : dlldeinitfns) {
		deinitfn.second();
	}
	for(auto &mod : allmodules) decref(mod.second);
}

int Interpreter::compileAndRun(const ModuleLoc *loc, const String &file, bool main_module)
{
	Module *mod = parser.createModule(file, main_module);

	if(!mod) return 1;
	if(!mod->tokenize()) return 1;
	if(argparser.has("lex")) mod->dumpTokens();
	if(!mod->parseTokens()) return 1;
	if(argparser.has("parse")) mod->dumpParseTree();
	if(!mod->executeDefaultParserPasses()) {
		err::out(loc, "Failed to apply default parser passes on module: ", mod->getPath());
		return 1;
	}
	if(argparser.has("optparse")) mod->dumpParseTree();
	if(!mod->genCode()) return 1;
	if(argparser.has("ir")) mod->dumpCode();

	pushModule(loc, mod);
	if(main_module) {
		for(auto m : coremods) {
			if(!loadNativeModule(loc, String(m))) {
				popModule();
				return 1;
			}
		}
	}
	int res = execute();
	popModule();

	return res;
}

void Interpreter::pushModule(const ModuleLoc *loc, Module *mod)
{
	auto mloc	= allmodules.find(mod->getPath());
	VarModule *mvar = nullptr;
	if(mloc == allmodules.end()) {
		mvar = makeVarWithRef<VarModule>(loc, mod);
		allmodules.insert({mod->getPath(), mvar});
	} else {
		mvar = mloc->second;
	}
	incref(mvar);
	modulestack.push_back(mvar);
}
void Interpreter::pushModule(StringRef path)
{
	auto mloc = allmodules.find(path);
	incref(mloc->second);
	modulestack.push_back(mloc->second);
}
void Interpreter::popModule()
{
	decref(modulestack.back());
	modulestack.pop_back();
}

bool Interpreter::findFileIn(Span<String> dirs, String &name, StringRef ext)
{
	size_t count = name.size();
	static char testpath[MAX_PATH_CHARS];
	if(name.front() != '~' && name.front() != '/' && name.front() != '.') {
		for(auto loc : dirs) {
			strncpy(testpath, loc.data(), loc.size());
			testpath[loc.size()] = '\0';
			strcat(testpath, "/");
			strcat(testpath, name.c_str());
			if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
			if(fs::exists(testpath)) {
				name = fs::absPath(testpath);
				return true;
			}
		}
	} else {
		if(name.front() == '~') {
			name.erase(name.begin());
			static String home = env::get("HOME");
			name.insert(name.begin(), home.begin(), home.end());
		} else if(name.front() == '.') {
			assert(modulestack.size() > 0 &&
			       "dot based module search cannot be done on empty modulestack");
			StringRef dir = modulestack.back()->getMod()->getPath();
			name.erase(name.begin());
			name.insert(name.begin(), dir.begin(), dir.end());
		}
		strcpy(testpath, name.c_str());
		if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
		if(fs::exists(testpath)) {
			name = fs::absPath(testpath);
			return true;
		}
	}
	return false;
}

bool Interpreter::loadNativeModule(const ModuleLoc *loc, String modfile)
{
	String mod	 = modfile.substr(modfile.find_last_of('/') + 1);
	StringRef moddir = fs::parentDir(modfile);
	modfile.insert(modfile.find_last_of('/') + 1, "libferal");
	if(!findModule(modfile)) {
		fail(loc, "module: ", modfile, " not found in locs: ", vecToStr(dlllocs));
		return false;
	}

	DynLib &dlibs = DynLib::getInstance();
	if(dlibs.exists(modfile)) return true;

	if(!dlibs.load(modfile)) {
		fail(loc, "unable to load module file: ", modfile);
		return false;
	}

	String tmp = "Init";
	tmp += mod;
	ModInitFn initfn = (ModInitFn)dlibs.get(modfile, tmp.c_str());
	if(initfn == nullptr) {
		fail(loc, "unable to load init function '", tmp, "' from module file: ", modfile);
		dlibs.unload(modfile);
		return false;
	}
	if(!initfn(*this, loc)) {
		fail(loc, "init function in module: ", modfile, " failed to execute");
		dlibs.unload(modfile);
		return false;
	}
	// set deinit function if available
	tmp = "deinit_";
	tmp += mod;
	ModDeinitFn deinitfn = (ModDeinitFn)dlibs.get(modfile, tmp.c_str());
	if(deinitfn) dlldeinitfns[modfile] = deinitfn;
	return true;
}

void Interpreter::addGlobal(StringRef name, Var *val, bool iref)
{
	if(globals.exists(name)) return;
	globals.add(name, val, iref);
}
void Interpreter::addNativeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
			      bool is_va)
{
	VarFn *f = makeVarWithRef<VarFn>(loc, modulestack.back()->getMod()->getPath(), "",
					 is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
	for(size_t i = 0; i < args; ++i) f->pushParam("");
	addGlobal(name, f, false);
}
void Interpreter::addTypeFn(uiptr _typeid, StringRef name, Var *fn, bool iref)
{
	auto loc    = typefns.find(_typeid);
	VarFrame *f = nullptr;
	if(loc == typefns.end()) {
		typefns[_typeid] = f = new VarFrame;
	} else {
		f = loc->second;
	}
	if(f->exists(name)) {
		err::out(nullptr, "type function: ", name, " already exists");
		assert(false);
	}
	f->add(name, fn, iref);
}
Var *Interpreter::getTypeFn(Var *var, StringRef name)
{
	auto loc = typefns.find(var->getTypeFnID());
	Var *res = nullptr;
	if(loc != typefns.end()) {
		res = loc->second->get(name);
		if(res) return res;
	} else if(var->isAttrBased()) {
		loc = typefns.find(var->getType());
		if(loc != typefns.end()) {
			res = loc->second->get(name);
			if(res) return res;
		}
	}
	return typefns[typeID<VarAll>()]->get(name);
}

StringRef Interpreter::getTypeName(uiptr _typeid)
{
	auto loc = typenames.find(_typeid);
	if(loc == typenames.end()) {
		typenames.insert({_typeid, "typeID<"});
		loc = typenames.find(_typeid);
		loc->second += std::to_string(_typeid);
		loc->second += ">";
	}
	return loc->second;
}

Var *Interpreter::getConst(const ModuleLoc *loc, Data &d, DataType dataty)
{
	switch(dataty) {
	case DataType::BOOL: return d.b ? tru : fals;
	case DataType::NIL: return nil;
	case DataType::INT: return makeVar<VarInt>(loc, d.i);
	case DataType::FLT: return makeVar<VarFlt>(loc, d.d);
	case DataType::STR: return makeVar<VarStr>(loc, d.s);
	default: err::out(loc, "internal error: invalid data type encountered");
	}
	return nullptr;
}

bool Interpreter::callFn(const ModuleLoc *loc, StringRef name, Var *&retdata, Span<Var *> args,
			 const Map<StringRef, AssnArgData> &assn_args)
{
	assert(!modulestack.empty() && "cannot perform a call with empty modulestack");
	bool memcall = args[0] != nullptr;
	Var *fn	     = nullptr;
	if(memcall) {
		if(args[0]->isAttrBased()) fn = args[0]->getAttr(name);
		if(!fn) fn = getTypeFn(args[0], name);
	} else {
		Vars *vars = getCurrModule()->getVars();
		fn	   = vars->get(name);
		if(!fn) fn = getGlobal(name);
	}
	if(!fn) {
		if(memcall) {
			fail(loc, "callable '", name,
			     "' does not exist for type: ", getTypeName(args[0]));
		} else {
			fail(loc, "callable '", name, "' does not exist");
		}
		return false;
	}
	if(!fn->call(*this, loc, args, assn_args)) {
		if(memcall) {
			fail(loc, "call to '", name, "' failed for type: ", getTypeName(args[0]));
		} else {
			fail(loc, "call to '", name, "' failed");
		}
		return false;
	}
	retdata = popExecStack(false);
	return true;
}

void Interpreter::initTypeNames()
{
	registerType<VarAll>(nullptr, "All");

	registerType<VarNil>(nullptr, "Nil");
	registerType<VarBool>(nullptr, "Bool");
	registerType<VarInt>(nullptr, "Int");
	registerType<VarFlt>(nullptr, "Flt");
	registerType<VarStr>(nullptr, "Str");
	registerType<VarVec>(nullptr, "Vec");
	registerType<VarMap>(nullptr, "Map");
	registerType<VarFn>(nullptr, "Func");
	registerType<VarModule>(nullptr, "Module");
	registerType<VarTypeID>(nullptr, "TypeID");
}

} // namespace fer