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

#include <stdarg.h>
#include <stdint.h>

namespace mlpl {

class StringUtils;
typedef std::list<std::string> StringList;
typedef std::vector<std::string> StringVector;
typedef StringList::iterator StringListIterator;
typedef StringVector::iterator StringVectorIterator;

class StringUtils {
public:
	static const std::string EMPTY_STRING;

	static const size_t SPRINTF_BUF_ON_STACK_LENGTH = 0x400;
	static void split(StringList &stringList, const std::string &target,
	                  const char separator, bool doMerge = true);
	static void split(StringVector &stringVector, const std::string &target,
	                  const char separator, bool doMerget = true);
	static std::string &getAt(StringList &stringList, size_t index);
	static bool casecmp(const char *str1, const char *str2);
	static bool casecmp(const std::string &str1, const char *str2);
	static bool casecmp(const char *str1, const std::string &str2);
	static bool casecmp(const std::string &str1, const std::string &str2);
	static std::string sprintf(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
	static std::string vsprintf(const char *fmt, va_list ap);
	static bool isNumber(const char *str, bool *isFloat = NULL);
	static bool isNumber(const std::string &str, bool *isFloat = NULL);
	static std::string toString(int number);
	static std::string toString(uint64_t number);
	static std::string toLower(std::string str);
	static std::string stripBothEndsSpaces(const std::string &str);

	/**
	 * Erase specified characters in a string.
	 *
	 * Example:
	 * string source = "ABCDEFG";
	 * string eraseChars = "BD";
	 * string s = StringUtils::eraseChars(source, eraseChars);
	 *
	 * The 's' should have "ACEFG"
	 *
	 * @param source
	 *  A source string. A UTF-8 string is acceptable.
	 *
	 * @param eraseChars
	 * Characters to be erased. Multi-byte characters cannot not be used. They
	 * are treated as multiple single-byte characters.
	 *
	 * @return
	 * The erased string is returned. If an error is happend, an empty string is
	 * returned.
	 *
	 */
	static std::string eraseChars(const std::string &source,
	                              const std::string &eraseChars);
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
	 * @return
	 * The replaced string is returned. If an error is happend, an empty string is
	 * returned.
	 *
	 */
	static std::string replace(const std::string &source,
	                           const std::string &targetChars,
	                           const std::string &newWord);
};

} // namespace mlpl

#endif // StringUtils_h
