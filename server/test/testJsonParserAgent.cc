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
#define assertReadFile(X,Y) cut_trace(_assertReadFile(X,Y))

template<typename T> void _assertReadWord(JsonParserAgent &parser,
                     const string &name, const T &expect)
{
	T actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWord(Z,A,X,Y) cut_trace(_assertReadWord<Z>(A,X,Y))

void _assertReadWordElement(JsonParserAgent &parser,
                     int index, const string &expect)
{
	string actual;
	cppcut_assert_equal(true, parser.read(index, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWordElement(A,X,Y) cut_trace(_assertReadWordElement(A,X,Y))

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
	assertReadWord(string, parser, "name0", "string value");
	assertReadWord(string, parser, "name1", "123");
}

void test_parseStringInObject(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson02.json");

	cppcut_assert_equal(true, parser.startObject("object0"));
	assertReadWord(string, parser, "food", "donuts");
	parser.endObject();
	cppcut_assert_equal(true, parser.startObject("object1"));
	assertReadWord(string, parser, "name", "dog");
	assertReadWord(string, parser, "age", "5");
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
	assertReadWord(string, parser, "key0", "value0");
	assertReadWord(string, parser, "key1", "value1");
	parser.endElement();

	cppcut_assert_equal(true, parser.startElement(1));
	assertReadWord(string, parser, "key0X", "value0Y");
	assertReadWord(string, parser, "key1X", "value1Y");
	parser.endElement();

	parser.endObject(); // array0;
}

void test_checkParseSuccess(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson05.json");

	assertReadWord(bool, parser, "valid", true);
	assertReadWord(int64_t, parser, "id", 1);
	assertReadWord(string, parser, "name", "Hatohol");

	cppcut_assert_equal(true, parser.startObject("object"));
	assertReadWord(bool, parser, "home", true);
	assertReadWord(string, parser, "city", "Tokyo");
	assertReadWord(int64_t, parser, "code", 124);
	parser.endObject();
}

void test_checkResultWhenTrueFalseTrue(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson05.json");
	int64_t value;

	assertReadWord(bool, parser, "valid", true);
	cppcut_assert_equal(false, parser.read("no", value));
	assertReadWord(string, parser, "name", "Hatohol");

	cppcut_assert_equal(true, parser.startObject("object"));
	assertReadWord(bool, parser, "home", true);
	cppcut_assert_equal(false, parser.read("date", value));
	assertReadWord(string, parser, "city", "Tokyo");
	parser.endObject();
}

void test_checkResultWhenFalseTrueFalse(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson05.json");
	string value1;
	bool value2;

	cppcut_assert_equal(false, parser.read("address", value1));
	assertReadWord(string, parser, "name", "Hatohol");
	cppcut_assert_equal(false, parser.read("pay", value2));

	cppcut_assert_equal(true, parser.startObject("object"));
	cppcut_assert_equal(false, parser.read("town", value1));
	assertReadWord(string, parser, "city", "Tokyo");
	cppcut_assert_equal(false, parser.read("foreign", value2));
	parser.endObject();
}

void test_checkResultWhenFalseFalseTrue(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson05.json");
	int64_t value1;
	bool value2;

	cppcut_assert_equal(false, parser.read("no", value1));
	cppcut_assert_equal(false, parser.read("pay", value2));
	assertReadWord(string, parser, "name", "Hatohol");

	cppcut_assert_equal(true, parser.startObject("object"));
	cppcut_assert_equal(false, parser.read("date", value1));
	cppcut_assert_equal(false, parser.read("foreign", value2));
	assertReadWord(string, parser, "city", "Tokyo");
	parser.endObject();
}

void test_checkIsMember(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson06.json");

	cppcut_assert_equal(true, parser.isMember("name"));
	cppcut_assert_equal(true, parser.isMember("prefecture"));

	cppcut_assert_equal(true, parser.startObject("data"));
	cppcut_assert_equal(true, parser.startObject("ticketgate"));
	cppcut_assert_equal(true, parser.startObject("greenwindows"));
	parser.endObject();
}
} //namespace testJsonParserAgent
