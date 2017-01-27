/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "StringUtils.h"
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

// ---------------------------------------------------------------------------
// Private Methods
// ---------------------------------------------------------------------------
template <class T>
static void splitTemplate(T &strings, const string &target,
                          const char separator, bool doMerge)
{
	size_t len = 0;
	const char *currWordHead = target.c_str();
	for (const char *ptr = target.c_str(); *ptr; ptr++) {
		if (*ptr != separator) {
			len++;
			continue;
		}

		if (len > 0 || !doMerge) {
			strings.push_back(string(currWordHead, len));
			len = 0;
		}
		currWordHead = ptr + 1;
	}
	if (len > 0 || !doMerge)
		strings.push_back(string(currWordHead, len));
}

// ---------------------------------------------------------------------------
// Public Functions
// ---------------------------------------------------------------------------
void StringUtils::split(StringList &stringList, const string &target,
                        const char separator, bool doMerge)
{
	splitTemplate<StringList>(stringList, target, separator, doMerge);
}

void StringUtils::split(StringVector &stringVector, const string &target,
                        const char separator, bool doMerge)
{
	splitTemplate<StringVector>(stringVector, target, separator, doMerge);
}

string &StringUtils::getAt(StringList &stringList, size_t index)
{
	if (index >= stringList.size()) {
		string msg = sprintf("index (%zd) > stringList.size (%zd)\n",
		                     index, stringList.size());
		throw out_of_range(msg);
	}
	StringListIterator it = stringList.begin();
	for (size_t i = 0; i < index; i++)
		++it;
	return *it;
}

bool StringUtils::casecmp(const char *str1, const char *str2)
{
	return (strcasecmp(str1, str2) == 0);
}

bool StringUtils::casecmp(const string &str1, const char *str2)
{
	return casecmp(str1.c_str(), str2);
}

bool StringUtils::casecmp(const char *str1, const string &str2)
{
	return casecmp(str1, str2.c_str());
}

bool StringUtils::casecmp(const string &str1, const string &str2)
{
	return casecmp(str1.c_str(), str2.c_str());
}

bool StringUtils::hasPrefix(const string &str, const string &prefix,
			    bool caseSensitive)
{
	if (str.size() < prefix.size())
		return false;
	const char *str1 = str.c_str();
	const char *str2 = prefix.c_str();
	size_t len = prefix.size();
	if (caseSensitive)
		return (strncmp(str1, str2, len) == 0);
	else
		return (strncasecmp(str1, str2, len) == 0);
}

bool StringUtils::hasSuffix(const string &str, const string &suffix,
			    bool caseSensitive)
{
	if (str.size() < suffix.size())
		return false;
	const char *str1 = str.c_str() + str.size() - suffix.size();
	const char *str2 = suffix.c_str();
	if (caseSensitive)
		return (strcmp(str1, str2) == 0);
	else
		return (strcasecmp(str1, str2) == 0);
}

string StringUtils::sprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	string str = vsprintf(fmt, ap);
	va_end(ap);
	return str;
}

string StringUtils::vsprintf(const char *fmt, va_list ap)
{
	char bufOnStack[SPRINTF_BUF_ON_STACK_LENGTH];

	// make make a back up of ap
	va_list copiedAp;
	va_copy(copiedAp, ap);

	int outLen = vsnprintf(bufOnStack,
	                       SPRINTF_BUF_ON_STACK_LENGTH, fmt, ap);

	// In the case, generation of the output string is completed
	if (outLen <= (int)SPRINTF_BUF_ON_STACK_LENGTH) {
		string str(bufOnStack);
		va_end(copiedAp);
		return str;
	}

	// In the case, buffer size is not enough
	outLen += 1; // add length for Null terminator.
	char *outStr = new char[outLen];
	vsnprintf(outStr, outLen, fmt, copiedAp);
	va_end(copiedAp);

	string str(outStr);
	delete [] outStr;
	return str;
}

bool StringUtils::isNumber(const char *str, bool *isFloat)
{
	const char *p = str;
	int countDot = 0;

	// This function allows '+' and '-' at the head.
	if (*p == '+' || *p == '-')
		p++;

	for (; *p; p++) {
		if (*p == '.') {
			countDot++;
			if (countDot >= 2)
				return false;
		} else if (*p < '0' || *p > '9')
			return false;
	}

	if (isFloat)
		*isFloat = (countDot == 1);

	return true;
}

bool StringUtils::isNumber(const string &str, bool *isFloat)
{
	return isNumber(str.c_str(), isFloat);
}

string StringUtils::toLower(string str)
{
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

string StringUtils::stripBothEndsSpaces(const string &str)
{
	int startPosition = 0;
	int length = str.size();
	if (length == 0)
		return "";

	const char *stringPtr = str.c_str();
	for (; startPosition < length; startPosition++, stringPtr++) {
		if (*stringPtr != ' ')
			break;
	}
	if (startPosition == length)
		return "";
	stringPtr = str.c_str();
	stringPtr += (length - 1);

	int bodyLength = length - startPosition;
	for (;bodyLength > 0; bodyLength--, stringPtr--) {
		if (*stringPtr != ' ')
			break;
	}

	return string(str, startPosition, bodyLength);
}

uint64_t StringUtils::toUint64(const string &numStr)
{
	uint64_t valU64;
	sscanf(numStr.c_str(), "%" PRIu64, &valU64);
	return valU64;
}

string StringUtils::eraseChars(const string &source, const string &eraseChars)
{
	static const size_t numArrayChars = 0x100;
	bool eraseCharArray[numArrayChars];
	for (size_t i = 0; i < numArrayChars; i++)
		eraseCharArray[i] = false;
	for (size_t i = 0; i < eraseChars.size(); i++) {
		unsigned char charCode = eraseChars[i];
		size_t idx = static_cast<int>(charCode);
		if (idx >= numArrayChars)
			return "";
		eraseCharArray[idx] = true;
	}

	string erasedString;
	for (size_t i = 0; i < source.size(); i++) {
		unsigned char charCode = source[i];
		size_t idx = static_cast<int>(charCode);
		if (idx >= numArrayChars)
			return "";
		if (eraseCharArray[idx])
			continue;
		erasedString += charCode;
	}

	return erasedString;
}

string StringUtils::replace(const string &source, const string &targetChars,
                            const string &newWord)
{
	static const size_t numArrayChars = 0x100;
	bool targetCharArray[numArrayChars];
	for (size_t i = 0; i < numArrayChars; i++)
		targetCharArray[i] = false;
	for (size_t i = 0; i < targetChars.size(); i++) {
		unsigned char charCode = targetChars[i];
		size_t idx = static_cast<int>(charCode);
		if (idx >= numArrayChars)
			return "";
		targetCharArray[idx] = true;
	}

	string replacedString;
	for (size_t i = 0; i < source.size(); i++) {
		unsigned char charCode = source[i];
		size_t idx = static_cast<int>(charCode);
		if (idx >= numArrayChars)
			return "";
		if (targetCharArray[idx])
			replacedString += newWord;
		else
			replacedString += charCode;
	}

	return replacedString;
}
