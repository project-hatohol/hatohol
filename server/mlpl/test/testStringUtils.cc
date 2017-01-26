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

#include <string>
#include <vector>
using namespace std;

#include <cutter.h>
#include <cppcutter.h>
#include <gcutter.h>

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

void test_splitOneWord(void)
{
	string target = "ABC";
	StringList words;
	StringUtils::split(words, target, ' ', false);
	cppcut_assert_equal((size_t)1, words.size());
	StringListIterator it = words.begin();
	cppcut_assert_equal(target, *it);
}

void test_casecmp_char_char(void)
{
	cppcut_assert_equal(true, StringUtils::casecmp("abc", "ABC"));
	cppcut_assert_equal(false, StringUtils::casecmp("abc", "ABX"));
}

void test_casecmp_char_str(void)
{
	string capitalStr = "ABC";
	cppcut_assert_equal(true, StringUtils::casecmp("abc", capitalStr));
	string differentStr = "ABX";
	cppcut_assert_equal(false, StringUtils::casecmp("abc", differentStr));
}

void test_casecmp_str_char(void)
{
	string capitalStr = "ABC";
	cppcut_assert_equal(true, StringUtils::casecmp(capitalStr, "abc"));
	string differentStr = "ABX";
	cppcut_assert_equal(false, StringUtils::casecmp(differentStr, "abc"));
}

void test_casecmp_str_str(void)
{
	string lowerStr = "abc";
	string capitalStr = "ABC";
	string differentStr = "ABX";
	cppcut_assert_equal(true, StringUtils::casecmp(lowerStr, capitalStr));
	cppcut_assert_equal(false, StringUtils::casecmp(lowerStr, differentStr));
}

void test_casecmp_char_rval_str(void)
{
	cppcut_assert_equal(true, StringUtils::casecmp("abc", string("ABC")));
	cppcut_assert_equal(false, StringUtils::casecmp("abc", string("ABX")));
}

void test_casecmp_rval_str_char(void)
{
	cppcut_assert_equal(true, StringUtils::casecmp(string("abc"), "ABC"));
	cppcut_assert_equal(false, StringUtils::casecmp(string("abc"), "ABX"));
}

void test_casecmp_rval_str_rval_str(void)
{
	cppcut_assert_equal(
	  true, StringUtils::casecmp(string("abc"), string("ABC")));
	cppcut_assert_equal(
	  false, StringUtils::casecmp(string("abc"), string("ABX")));
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

void test_shouldNotReplaceUTF8(void)
{
	string src = "これがエスケープされるなんてそんなバカな！";
	string expected = src;
	string actual = StringUtils::replace(src, "'", "''");
	cppcut_assert_equal(expected, actual);
}

void data_toUint64(void)
{
	gcut_add_datum("Zero",
	               "val", G_TYPE_STRING, "0",
	               "expect", G_TYPE_UINT64, (guint64)0,
	               NULL);
	gcut_add_datum("Positive within 32bit",
	               "val", G_TYPE_STRING, "3456",
	               "expect", G_TYPE_UINT64, (guint64)3456,
	               NULL);
	gcut_add_datum("Positive 32bit Max",
	               "val", G_TYPE_STRING, "2147483647",
	               "expect", G_TYPE_UINT64, (guint64)2147483647,
	               NULL);
	gcut_add_datum("Positive 32bit Max + 1",
	               "val", G_TYPE_STRING, "2147483648",
	               "expect", G_TYPE_UINT64, (guint64)G_MAXINT32 + 1,
	               NULL);
	gcut_add_datum("Positive 64bit Poistive Max",
	               "val", G_TYPE_STRING, "9223372036854775807",
	               "expect", G_TYPE_UINT64, 9223372036854775807UL,
	               NULL);
	gcut_add_datum("Positive 64bit Poistive Max+1",
	               "val", G_TYPE_STRING, "9223372036854775808",
	               "expect", G_TYPE_UINT64, 9223372036854775808UL,
	               NULL);
	gcut_add_datum("Positive 64bit Max",
	               "val", G_TYPE_STRING, "18446744073709551615",
	               "expect", G_TYPE_UINT64, 18446744073709551615UL,
	               NULL);
}

void test_toUint64(gconstpointer data)
{
	uint64_t actual =
	   StringUtils::toUint64(gcut_data_get_string(data, "val"));
	uint64_t expect = gcut_data_get_uint64(data, "expect");
	cppcut_assert_equal(expect, actual);
}

void data_hasPrefix(void)
{
	gcut_add_datum("has prefix",
	               "str", G_TYPE_STRING, "image.png",
	               "prefix", G_TYPE_STRING, "image",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("different prefix",
	               "str", G_TYPE_STRING, "image.png",
	               "prefix", G_TYPE_STRING, "foobar",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("prefix with different case",
	               "str", G_TYPE_STRING, "IMAGE.png",
	               "prefix", G_TYPE_STRING, "image",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("same string",
	               "str", G_TYPE_STRING, "image",
	               "prefix", G_TYPE_STRING, "image",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("shorter string",
	               "str", G_TYPE_STRING, "ima",
	               "prefix", G_TYPE_STRING, "image",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("empty string",
	               "str", G_TYPE_STRING, "",
	               "prefix", G_TYPE_STRING, "image",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("empty prefix",
	               "str", G_TYPE_STRING, "image.png",
	               "prefix", G_TYPE_STRING, "",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
}

void test_hasPrefix(gconstpointer data)
{
	const char *str = gcut_data_get_string(data, "str");
	const char *prefix = gcut_data_get_string(data, "prefix");
	bool expect = gcut_data_get_boolean(data, "expect");
	bool actual = StringUtils::hasPrefix(str, prefix);
	cppcut_assert_equal(expect, actual);
}

void test_hasPrefixWithCaseInsensitive(void)
{
	bool actual = StringUtils::hasPrefix("image.png", "IMAGE", false);
	cppcut_assert_equal(true, actual);
}

void data_hasSuffix(void)
{
	gcut_add_datum("has suffix",
	               "str", G_TYPE_STRING, "image.png",
	               "suffix", G_TYPE_STRING, "png",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("different suffix",
	               "str", G_TYPE_STRING, "image.png",
	               "suffix", G_TYPE_STRING, "jpg",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("suffix with different case",
	               "str", G_TYPE_STRING, "image.PNG",
	               "suffix", G_TYPE_STRING, "png",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("same string",
	               "str", G_TYPE_STRING, "png",
	               "suffix", G_TYPE_STRING, "png",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
	gcut_add_datum("shorter string",
	               "str", G_TYPE_STRING, "ng",
	               "suffix", G_TYPE_STRING, "png",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("empty string",
	               "str", G_TYPE_STRING, "",
	               "suffix", G_TYPE_STRING, "png",
	               "expect", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("empty suffix",
	               "str", G_TYPE_STRING, "image.png",
	               "suffix", G_TYPE_STRING, "",
	               "expect", G_TYPE_BOOLEAN, TRUE,
	               NULL);
}

void test_hasSuffix(gconstpointer data)
{
	const char *str = gcut_data_get_string(data, "str");
	const char *suffix = gcut_data_get_string(data, "suffix");
	bool expect = gcut_data_get_boolean(data, "expect");
	bool actual = StringUtils::hasSuffix(str, suffix);
	cppcut_assert_equal(expect, actual);
}

void test_hasSuffixWithCaseInsensitive(void)
{
	bool actual = StringUtils::hasSuffix("image.PNG", "png", false);
	cppcut_assert_equal(true, actual);
}

} // namespace testStringUtils
