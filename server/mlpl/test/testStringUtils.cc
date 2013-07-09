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

#include <string>
#include <vector>
using namespace std;

#include <cutter.h>
#include <cppcutter.h>

#include "StringUtils.h"
using namespace mlpl;

namespace testStringUtils {

void test_split(void)
{
	string target = "ABC foo";
	StringList words;
	StringUtils::split(words, target, ' ');
	cut_assert_equal_int(2, words.size());

	StringListIterator it = words.begin();
	cut_assert_equal_string("ABC", it->c_str());
	++it;
	cut_assert_equal_string("foo", it->c_str());
}

void test_splitManySeparators(void)
{
	string target = "  ABC  foo  ";
	StringList words;
	StringUtils::split(words, target, ' ');
	cut_assert_equal_int(2, words.size());

	StringListIterator it = words.begin();
	cut_assert_equal_string("ABC", it->c_str());
	++it;
	cut_assert_equal_string("foo", it->c_str());
}

void test_splitDontMerge(void)
{
	string target = "ABC foo  XY";
	StringList words;
	StringUtils::split(words, target, ' ', false);
	cut_assert_equal_int(4, words.size());

	StringListIterator it = words.begin();
	cut_assert_equal_string("ABC", it->c_str());
	++it;
	cut_assert_equal_string("foo", it->c_str());
	++it;
	cut_assert_equal_string("", it->c_str());
	++it;
	cut_assert_equal_string("XY", it->c_str());
}

void test_splitDontMergeLastIsSeparator(void)
{
	string target = "ABC foo ";
	StringList words;
	StringUtils::split(words, target, ' ', false);
	cut_assert_equal_int(3, words.size());

	StringListIterator it = words.begin();
	cut_assert_equal_string("ABC", it->c_str());
	++it;
	cut_assert_equal_string("foo", it->c_str());
	++it;
	cut_assert_equal_string("", it->c_str());
}

void test_splitDontMergeLastIsDoubleSeparator(void)
{
	string target = "ABC foo  ";
	StringList words;
	StringUtils::split(words, target, ' ', false);
	cut_assert_equal_int(4, words.size());

	StringListIterator it = words.begin();
	cut_assert_equal_string("ABC", it->c_str());
	++it;
	cut_assert_equal_string("foo", it->c_str());
	++it;
	cut_assert_equal_string("", it->c_str());
	++it;
	cut_assert_equal_string("", it->c_str());
}

void test_sprintf(void)
{
	const char *fmt = "ABC %d xyz";
	string str = StringUtils::sprintf(fmt, 10);
	cut_assert_equal_string("ABC 10 xyz", str.c_str());
}

void test_sprintfLongerThanStackBuf(void)
{
	string fmtHead;
	for (size_t i = 0; i < StringUtils::SPRINTF_BUF_ON_STACK_LENGTH; i++)
		fmtHead += "A";

	string fmt = fmtHead + "ABC %d xyz";
	string str = StringUtils::sprintf(fmt.c_str(), 10);

	string expected = fmtHead + "ABC 10 xyz";
	cut_assert_equal_string(expected.c_str(), str.c_str());
}

void test_isNumber(void)
{
	cppcut_assert_equal(true, StringUtils::isNumber("0"));
	cppcut_assert_equal(true, StringUtils::isNumber("1"));
	cppcut_assert_equal(true, StringUtils::isNumber("-10"));
	cppcut_assert_equal(true, StringUtils::isNumber("+51234"));
	cppcut_assert_equal(false, StringUtils::isNumber("10a"));
	cppcut_assert_equal(false, StringUtils::isNumber("-10ab"));
	cppcut_assert_equal(false, StringUtils::isNumber("0x10"));
}

void test_isNumberFloat(void)
{
	bool isFloat;
	cppcut_assert_equal(true, StringUtils::isNumber("0", &isFloat));
	cppcut_assert_equal(false, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("1.2", &isFloat));
	cppcut_assert_equal(true, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("-10", &isFloat));
	cppcut_assert_equal(false, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("-10.5", &isFloat));
	cppcut_assert_equal(true, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("+51234.8", &isFloat));
	cppcut_assert_equal(true, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("+51234", &isFloat));
	cppcut_assert_equal(false, isFloat);
}

void test_isNumberFloatDotMany(void)
{
	bool isFloat;
	cppcut_assert_equal(false, StringUtils::isNumber("0.2.3", &isFloat));
}

void test_isNumberFloatDotFirst(void)
{
	bool isFloat;
	cppcut_assert_equal(true, StringUtils::isNumber(".23", &isFloat));
	cppcut_assert_equal(true, isFloat);
}

void test_isNumberFloatDotLast(void)
{
	bool isFloat;
	cppcut_assert_equal(true, StringUtils::isNumber("23.", &isFloat));
	cppcut_assert_equal(true, isFloat);
}

void test_isNumberFloatSignThenDot(void)
{
	bool isFloat;
	cppcut_assert_equal(true, StringUtils::isNumber("+.23", &isFloat));
	cppcut_assert_equal(true, isFloat);

	cppcut_assert_equal(true, StringUtils::isNumber("-.5", &isFloat));
	cppcut_assert_equal(true, isFloat);
}

void test_isNumberString(void)
{
	string str1 = "10";
	string str2 = "0x10";
	cppcut_assert_equal(true, StringUtils::isNumber(str1));
	cppcut_assert_equal(false, StringUtils::isNumber(str2));
}

void test_toStringFromInt(void)
{
	cppcut_assert_equal(string("57"), StringUtils::toString(57));
	cppcut_assert_equal(string("-205"), StringUtils::toString(-205));
}

void test_toLower(void)
{
	const char *src      = "AbCdEf#2";
	const char *expected = "abcdef#2";
	string testString(src);
	cppcut_assert_equal(string(expected), StringUtils::toLower(testString));
	// confirm that testString is not changed
	cppcut_assert_equal(string(src), testString);
}

void test_stripBothEndsSpaces(void)
{
	const char *body = "AA BC";
	string src = StringUtils::sprintf(" %s  ", body);
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(string(body), stripped);
}
void test_stripLeftEndSpaces(void)
{
	const char *body = "AA BC";
	string src = StringUtils::sprintf("  %s", body);
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(string(body), stripped);
}

void test_stripRigthEndSpaces(void)
{
	const char *body = "AA BC";
	string src = StringUtils::sprintf("%s  ", body);
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(string(body), stripped);
}

void test_stripBothEndsSpacesEmpty(void)
{
	string src = "";
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(string(), stripped);
}

void test_stripBothEndsSpacesOneChar(void)
{
	string src = "A";
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(src, stripped);
}

void test_stripBothEndsSpacesMultChar(void)
{
	string src = "AbCdefG";
	string stripped = StringUtils::stripBothEndsSpaces(src);
	cppcut_assert_equal(src, stripped);
}

void test_removeChars(void)
{
	string src = "AB/DE\nG\t";
	string erasedStr = StringUtils::eraseChars(src, "/\n\t");
	string expected = "ABDEG";
	cppcut_assert_equal(expected, erasedStr);
}

void test_replace(void)
{
	string src = "AB/DE\nG\t";
	string replacedStr = StringUtils::replace(src, "/\n\t", "#!");
	string expected = "AB#!DE#!G#!";
	cppcut_assert_equal(expected, replacedStr);
}

} // namespace testStringUtils
