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

#ifndef StringUtils_h
#define StringUtils_h

#include <vector>
#include <list>
#include <string>
using namespace std;

#include <stdarg.h>

namespace mlpl {

class StringUtils;
typedef list<string> StringList;
typedef vector<string> StringVector;
typedef StringList::iterator StringListIterator;
typedef StringVector::iterator StringVectorIterator;

class StringUtils {
public:
	static const string EMPTY_STRING;

	static const size_t SPRINTF_BUF_ON_STACK_LENGTH = 0x400;
	static void split(StringList &stringList, const string &target,
	                  const char separator, bool doMerge = true);
	static void split(StringVector &stringVector, const string &target,
	                  const char separator, bool doMerget = true);
	static string &getAt(StringList &stringList, size_t index);
	static bool casecmp(const char *str1, const char *str2);
	static bool casecmp(string &str1, const char *str2);
	static bool casecmp(const char *str1, string &str2);
	static bool casecmp(string &str1, string &str2);
	static string sprintf(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
	static string sprintf(const char *fmt, va_list ap);
	static bool isNumber(const char *str, bool *isFloat = NULL);
	static bool isNumber(const string &str, bool *isFloat = NULL);
	static string toString(int number);
	static string toLower(string str);
	static string stripBothEndsSpaces(const string &str);
	static string eraseChars(const string &source,
	                         const string &eraseChars);
	/**
	 * Replace specified characters with a word.
	 *
	 * Example:
	 * string source = "ABCDEFG";
	 * string targetChars = "BD";
	 * string newWord = "dog";
	 * string s = StringUtils::replace(source, targetChars, newWord);
	 *
	 * The 's' should have "AdogCdogEFG"
	 *
	 * @param source
	 *  A source string. A UTF-8 string is acceptable.
	 *
	 * @param targetChars
	 * Characters to be replaced. Multi-byte characters cannot not be used. They
	 * are treated as multiple single-byte characters.
	 *
	 * @param newWord
	 * The characters in 'targetChars' are replaced with 'newWord'.
	 *
	 */
	static string replace(const string &source, const string &targetChars,
	                      const string &newWord);
};

} // namespace mlpl

#endif // StringUtils_h
