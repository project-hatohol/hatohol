/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <StringUtils.h>
#include <Logger.h>
using namespace mlpl;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include "Utils.h"
#include "FormulaElement.h"

const static size_t SIZE_JS_METHOD_VALID_CODE_MAP = 0x100;
static bool g_jsMethodValidCodeMap[SIZE_JS_METHOD_VALID_CODE_MAP];


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void Utils::init(void)
{
	for (size_t i = 0; i < SIZE_JS_METHOD_VALID_CODE_MAP; i++)
		g_jsMethodValidCodeMap[i] = false;
	for (size_t i = 'A'; i <= 'Z'; i++)
		g_jsMethodValidCodeMap[i] = true;
	for (size_t i = 'a'; i <= 'z'; i++)
		g_jsMethodValidCodeMap[i] = true;
	for (size_t i = '0'; i <= '9'; i++)
		g_jsMethodValidCodeMap[i] = true;
	g_jsMethodValidCodeMap[static_cast<size_t>('_')] = true;
}

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

void Utils::assertNotNull(const void *ptr)
{
	if (ptr)
		return;
	string msg;
	TRMSG(msg, "pointer: NULL.");
	throw logic_error(msg);
}

string Utils::demangle(string &str)
{
	return Utils::demangle(str.c_str());
}

string Utils::demangle(const char *str)
{
	int status;
	char *demangled = abi::__cxa_demangle(str, 0, 0, &status);
	string demangledStr;
	if (demangled) {
		demangledStr = demangled;
		free(demangled);
	}
	return demangledStr;
}

void Utils::showTreeInfo(FormulaElement *formulaElement, int fd,
                         bool fromRoot, int maxNumElem, int currNum, int depth)
{
	string treeInfo;
	FormulaElement *startElement = formulaElement;
	if (!formulaElement) {
		MLPL_BUG("formulaElement: NULL\n");
		return;
	}
	if (fromRoot) {
		startElement = formulaElement->getRootElement();
		if (!startElement) {
			MLPL_BUG("formulaElement->getRootElement(): NULL\n");
			return;
		}
	}

	startElement->getTreeInfo(treeInfo, maxNumElem, currNum, depth);
	string msg;
	msg = StringUtils::sprintf(
	        "***** FormulaInfo: Tree Information "
	        "(request: %p, fromRoot: %d) ***\n",
	        formulaElement, fromRoot);
	msg += treeInfo;

	size_t remainBytes = msg.size() + 1; /* '+ 1' means NULL term. */
	const char *buf = msg.c_str();
	while (remainBytes > 0) {
		ssize_t writtenBytes = write(fd, buf, remainBytes);
		if (writtenBytes == 0) {
			MLPL_ERR("writtenBytes: 0\n");
			return;
		}
		if (writtenBytes < 0) {
			MLPL_ERR("Failed: errno: %d\n", errno);
			return;
		}
		buf += writtenBytes;
		remainBytes -= writtenBytes;
	}
}

uint64_t Utils::getCurrTimeAsMicroSecond(void)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		MLPL_ERR("Failed to call gettimeofday: %d\n", errno);
		return 0;
	}
	uint64_t currTime = tv.tv_usec;
	currTime += 1000 * 1000 * tv.tv_sec;
	return currTime;
}

bool Utils::isValidPort(int port, bool showErrorMsg)
{
	if (port < 0 || port > 65536) {
		if (showErrorMsg)
			MLPL_ERR("invalid port: %s, %d\n", port);
		return false;
	}
	return true;
}

bool Utils::validateJSMethodName(const string &name, string &errorMessage)
{
	if (name.empty()) {
		errorMessage = "empty name.";
		return false;
	}

	if (name[0] >= '0' && name[0] <= '9') {
		errorMessage = "begins from a number.";
		return false;
	}

	for (size_t i = 0; i < name.size(); i++) {
		int code = name[i];
		if (g_jsMethodValidCodeMap[code])
			continue;
		errorMessage = StringUtils::sprintf(
		  "invalid charactor code: %02x at index: %zd", code, i);
		return false;
	}

	return true;
}

string Utils::getExtension(const string &path)
{
	string ext;
	int len = path.size();
	for (int i = len-1; i >= 0; i--) {
		if (path[i] != '.')
			continue;
		ext = string(path, i+1);
		break;
	}
	return ext;
}

string Utils::getSelfExeDir(void)
{
	char buf[PATH_MAX];
	ssize_t bytesRead = readlink("/proc/self/exe", buf, PATH_MAX);
	if (bytesRead == -1) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to readlink(\"/proc/self/exe\"): %s",
		  strerror(errno));
	}

	int i;
	for (i = bytesRead - 2; i > 0; i--) {
		if (buf[i] == '/')
			break;
	}
	if (i < 1)
		return "/";
	return string(buf, 0, i);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string Utils::makeDemangledStackTraceString(string &stackTraceLine)
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

	string demangledStr = demangle(stringsSymbol[0]);
	string returnStr = libName;
	if (!demangledStr.empty())
		returnStr += ", " + demangledStr + " +" + stringsSymbol[1];
	returnStr += ", " + symbolName + addr;
	return returnStr;
}

