#pragma once

// Implements the basic parse tree simplification - including constant folding

#include "Parser/Passes/Base.hpp"

namespace fer
{

class SimplifyParserPass : public ParserPass
{
	Stmt *applyConstantFolding(StmtSimple *l, StmtSimple *r, const lex::Tok &oper);

public:
	SimplifyParserPass(Context &ctx);
	~SimplifyParserPass() override;

	bool visit(Stmt *stmt, Stmt **source) override;

	bool visit(StmtBlock *stmt, Stmt **source) override;
	bool visit(StmtSimple *stmt, Stmt **source) override;
	bool visit(StmtFnArgs *stmt, Stmt **source) override;
	bool visit(StmtExpr *stmt, Stmt **source) override;
	bool visit(StmtVar *stmt, Stmt **source) override;
	bool visit(StmtFnSig *stmt, Stmt **source) override;
	bool visit(StmtFnDef *stmt, Stmt **source) override;
	bool visit(StmtVarDecl *stmt, Stmt **source) override;
	bool visit(StmtCond *stmt, Stmt **source) override;
	bool visit(StmtFor *stmt, Stmt **source) override;
	bool visit(StmtRet *stmt, Stmt **source) override;
	bool visit(StmtContinue *stmt, Stmt **source) override;
	bool visit(StmtBreak *stmt, Stmt **source) override;
	bool visit(StmtDefer *stmt, Stmt **source) override;
};

} // namespace fer