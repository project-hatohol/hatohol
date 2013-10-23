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

#include <fstream>
#include <cppcutter.h>
#include "JsonParserAgent.h"

namespace testJsonParserAgent {

void _assertReadFile(const string &fileName, string &output)
{
	ifstream ifs(fileName.c_str());
	cppcut_assert_equal(false, ifs.fail());

	string str;
	while (getline(ifs, str))
		output += str;
}
#define assertReadFile(X,Y) cut_trace(_assertReadFile(X, Y))

void _assertReadWordBool(JsonParserAgent &parser,
                     const string &name, const bool &expect)
{
	bool actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWordBool(A,X,Y) cut_trace(_assertReadWordBool(A,X, Y))

void _assertReadWordInt64t(JsonParserAgent &parser,
                     const string &name, const int64_t &expect)
{
	int64_t actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWordInt64t(A,X,Y) cut_trace(_assertReadWordInt64t(A,X, Y))

void _assertReadWordString(JsonParserAgent &parser,
                     const string &name, const string &expect)
{
	string actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWordString(A,X,Y) cut_trace(_assertReadWordString(A,X, Y))

void _assertReadWordElement(JsonParserAgent &parser,
                     int index, const string &expect)
{
	string actual;
	cppcut_assert_equal(true, parser.read(index, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWordElement(A,X,Y) cut_trace(_assertReadWordElement(A,X, Y))

#define DEFINE_PARSER_AND_READ(PARSER, JSON_MATERIAL) \
string _json; \
assertReadFile(JSON_MATERIAL, _json); \
JsonParserAgent parser(_json); \
cppcut_assert_equal(false, PARSER.hasError());


// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_parseString(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson01.json");
	assertReadWordString(parser, "name0", "string value");
	assertReadWordString(parser, "name1", "123");
}

void test_parseStringInObject(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson02.json");

	cppcut_assert_equal(true, parser.startObject("object0"));
	assertReadWordString(parser, "food", "donuts");
	parser.endObject();
	cppcut_assert_equal(true, parser.startObject("object1"));
	assertReadWordString(parser, "name", "dog");
	assertReadWordString(parser, "age", "5");
	parser.endObject();
}

void test_parseStringInArray(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson03.json");

	cppcut_assert_equal(true, parser.startObject("array0"));
	cppcut_assert_equal(3, parser.countElements());
	assertReadWordElement(parser, 0, "elem0");
	assertReadWordElement(parser, 1, "elem1");
	assertReadWordElement(parser, 2, "elem2");
	parser.endObject();
}

void test_parseStringInObjectInArray(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson04.json");

	cppcut_assert_equal(true, parser.startObject("array0"));
	cppcut_assert_equal(2, parser.countElements());

	cppcut_assert_equal(true, parser.startElement(0));
	assertReadWordString(parser, "key0", "value0");
	assertReadWordString(parser, "key1", "value1");
	parser.endElement();

	cppcut_assert_equal(true, parser.startElement(1));
	assertReadWordString(parser, "key0X", "value0Y");
	assertReadWordString(parser, "key1X", "value1Y");
	parser.endElement();

	parser.endObject(); // array0;
}

void test_checkParseSuccess(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson05.json");
}

} //namespace testJsonParserAgent


