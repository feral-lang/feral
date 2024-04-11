#include "Utils.hpp"

#if defined(FER_OS_WINDOWS)
#include <AtlBase.h>
#include <atlconv.h>
#endif

namespace fer
{

Vector<StringRef> stringDelim(StringRef str, StringRef delim)
{
	Vector<StringRef> res;
	if(str.empty()) return res;

	size_t start = 0;
	size_t end   = str.find(delim);
	while(end != String::npos) {
		res.emplace_back(str.substr(start, end - start));
		start = end + delim.length();
		end   = str.find(delim, start);
	}
	res.emplace_back(str.substr(start, end));

	for(auto &s : res) {
		while(!s.empty() && s.front() == ' ') s = s.substr(0, 1);
		while(!s.empty() && s.back() == ' ') s = s.substr(s.size() - 1);
	}

	return res;
}

String toRawString(StringRef data)
{
	String res(data);
	for(size_t i = 0; i < res.size(); ++i) {
		if(res[i] == '\\') {
			res.insert(i++, "\\");
			continue;
		}
		if(res[i] == '\0') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\0");
			continue;
		}
		if(res[i] == '\a') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\a");
			continue;
		}
		if(res[i] == '\b') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\b");
			continue;
		}
#if !defined(FER_OS_WINDOWS)
		if(res[i] == '\e') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\e");
			continue;
		}
#endif
		if(res[i] == '\f') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\f");
			continue;
		}
		if(res[i] == '\n') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\n");
			continue;
		}
		if(res[i] == '\r') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\r");
			continue;
		}
		if(res[i] == '\t') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\t");
			continue;
		}
		if(res[i] == '\v') {
			res.erase(res.begin() + i);
			res.insert(i++, "\\v");
			continue;
		}
	}
	return res;
}

String fromRawString(StringRef from)
{
	String data(from);
	for(size_t idx = 0; idx < data.size(); ++idx) {
		if(data[idx] != '\\') continue;
		if(idx + 1 >= data.size()) continue;
		data.erase(data.begin() + idx);
		if(data[idx] == '0') data[idx] = '\0';
		else if(data[idx] == 'a') data[idx] = '\a';
		else if(data[idx] == 'b') data[idx] = '\b';
#if !defined(FER_OS_WINDOWS)
		else if(data[idx] == 'e') data[idx] = '\e';
#endif
		else if(data[idx] == 'f') data[idx] = '\f';
		else if(data[idx] == 'n') data[idx] = '\n';
		else if(data[idx] == 'r') data[idx] = '\r';
		else if(data[idx] == 't') data[idx] = '\t';
		else if(data[idx] == 'v') data[idx] = '\v';
	}
	return data;
}

String vecToStr(Span<StringRef> items)
{
	String res = "[";
	for(auto i : items) {
		res += i;
		res += ", ";
	}
	if(!items.empty()) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}

String vecToStr(Span<String> items)
{
	String res = "[";
	for(auto i : items) {
		res += i;
		res += ", ";
	}
	if(!items.empty()) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}

#if defined(FER_OS_WINDOWS)
// Windows' string to wstring functions
WString toWString(StringRef data)
{
	size_t wstrLen = std::mbstowcs(nullptr, data.data(), data.size());
	WString wstr(wstrLen, 0);
	std::mbstowcs(wstr.data(), data.data(), data.size());
	return wstr;
}
#endif

} // namespace fer