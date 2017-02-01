/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#pragma once
#include <vector>
#include <list>
#include <string>

#include <stdarg.h>
#include <stdint.h>

namespace mlpl {

typedef std::list<std::string> StringList;
typedef std::vector<std::string> StringVector;
typedef StringList::iterator StringListIterator;
typedef StringVector::iterator StringVectorIterator;

namespace StringUtils {
	const std::string EMPTY_STRING;

	const size_t SPRINTF_BUF_ON_STACK_LENGTH = 0x400;
	void split(StringList &stringList, const std::string &target,
		   const char separator, bool doMerge = true);
	void split(StringVector &stringVector, const std::string &target,
		   const char separator, bool doMerget = true);
	std::string &getAt(StringList &stringList, size_t index);
	bool casecmp(const char *str1, const char *str2);
	bool casecmp(const std::string &str1, const char *str2);
	bool casecmp(const char *str1, const std::string &str2);
	bool casecmp(const std::string &str1, const std::string &str2);
	bool hasPrefix(const std::string &str,
		       const std::string &prefix,
		       bool caseSensitive = true);
	bool hasSuffix(const std::string &str,
		       const std::string &suffix,
		       bool caseSensitive = true);
	std::string sprintf(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
	std::string vsprintf(const char *fmt, va_list ap);
	bool isNumber(const char *str, bool *isFloat = NULL);
	bool isNumber(const std::string &str, bool *isFloat = NULL);
	std::string toLower(std::string str);
	std::string stripBothEndsSpaces(const std::string &str);

	/**
	 * Convert a decimal number string to uint64_t.
	 *
	 * @param numStr A string of a decimal number
	 *
	 * @return the converted number.
	 */
	uint64_t toUint64(const std::string &numStr);

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
	std::string eraseChars(const std::string &source,
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
	std::string replace(const std::string &source,
			    const std::string &targetChars,
			    const std::string &newWord);
};

} // namespace mlpl

