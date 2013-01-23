#include <cstdio>
#include <cxxabi.h>
#include <stdexcept>
#include "Utils.h"

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
static string makeDemangledStackTraceString(string &stackTraceLine)
{
	StringVector stringsHead;
	StringUtils::split(stringsHead, stackTraceLine, '(');
	if (stringsHead.size() != 2)
		return stackTraceLine;

	StringVector stringsTail;
	StringUtils::split(stringsTail, stringsHead[1], ')');
	if (stringsTail.size() != 2)
		return stackTraceLine;

	string &libName    = stringsHead[0];
	string &symbolName = stringsTail[0];
	string &addr       = stringsTail[1];

	StringVector stringsSymbol;
	StringUtils::split(stringsSymbol, symbolName, '+');
	if (stringsSymbol.size() != 2)
		return stackTraceLine;

	int status;
	char *demangled = abi::__cxa_demangle(stringsSymbol[0].c_str(),
	                                      0, 0, &status);
	string demangledStr;
	if (demangled) {
		demangledStr = demangled;
		free(demangled);
	}

	string returnStr = libName;
	if (!demangledStr.empty())
		returnStr += ", " + demangledStr + " +" + stringsSymbol[1];
	returnStr += ", " + symbolName + addr;
	return returnStr;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
string Utils::makeDemangledStackTraceLines(void **trace, int num)
{
	char **symbols = backtrace_symbols(trace, num);
	string str;
	for (int i = 0; i < num; i++) {
		string line = symbols[i];
		str += makeDemangledStackTraceString(line);
		str += "\n";
	}
	free(symbols);
	return str;
}

void Utils::assertNotNull(void *ptr)
{
	if (ptr)
		return;
	string msg;
	TRMSG(msg, "pointer: NULL.");
	throw logic_error(msg);
}
