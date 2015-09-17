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

#include <fstream>
#include <cppcutter.h>
#include <gcutter.h>
#include "JSONParser.h"
#include "Helpers.h"
using namespace std;

namespace testJSONParser {

void _assertReadFile(const string &fileName, string &output)
{
	ifstream ifs(fileName.c_str());
	cppcut_assert_equal(false, ifs.fail());

	string str;
	while (getline(ifs, str))
		output += str;
}
#define assertReadFile(X,Y) cut_trace(_assertReadFile(X,Y))

template<typename T> void _assertReadWord(JSONParser &parser,
                     const string &name, const T &expect)
{
	T actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWord(Z,A,X,Y) cut_trace(_assertReadWord<Z>(A,X,Y))

void _assertReadWordElement(JSONParser &parser,
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
JSONParser parser(_json); \
cppcut_assert_equal(false, PARSER.hasError());

void cut_setup(void)
{
	cut_set_fixture_data_dir(getFixturesDir().c_str(), NULL);
}


// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_parseString(void)
{
	const char *path = cut_build_fixture_path("testJSON01.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);
	assertReadWord(string, parser, "name0", "string value");
	assertReadWord(string, parser, "name1", "123");
}

void test_parseDouble(void)
{
	const char *path = cut_build_fixture_path("testJSON01.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);
	assertReadWord(double, parser, "double0", 0.0);
	assertReadWord(double, parser, "double1", 123456789.0123);
	assertReadWord(double, parser, "double2", -777333.555111);
}

void test_parseStringInObject(void)
{
	const char *path = cut_build_fixture_path("testJSON02.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);

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
	const char *path = cut_build_fixture_path("testJSON03.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);

	cppcut_assert_equal(true, parser.startObject("array0"));
	cppcut_assert_equal(3u, parser.countElements());
	assertReadWordElement(parser, 0, "elem0");
	assertReadWordElement(parser, 1, "elem1");
	assertReadWordElement(parser, 2, "elem2");
	parser.endObject();
}

void test_parseStringInObjectInArray(void)
{
	const char *path = cut_build_fixture_path("testJSON04.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);

	cppcut_assert_equal(true, parser.startObject("array0"));
	cppcut_assert_equal(2u, parser.countElements());

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
	const char *path = cut_build_fixture_path("testJSON05.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);

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
	const char *path = cut_build_fixture_path("testJSON05.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);
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
	const char *path = cut_build_fixture_path("testJSON05.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);
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
	const char *path = cut_build_fixture_path("testJSON05.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);
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
	const char *path = cut_build_fixture_path("testJSON06.json", NULL);
	DEFINE_PARSER_AND_READ(parser, path);

	cppcut_assert_equal(true, parser.isMember("name"));
	cppcut_assert_equal(true, parser.isMember("prefecture"));

	cppcut_assert_equal(true, parser.startObject("data"));
	cppcut_assert_equal(true, parser.isMember("ticketgate"));
	cppcut_assert_equal(true, parser.isMember("greenwindows"));
	parser.endObject();
}

void data_valueType(void)
{
	gcut_add_datum("null",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_NULL,
		       "json", G_TYPE_STRING, "{\"value\":null}",
		       NULL);
	gcut_add_datum("int64",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_INT64,
		       "json", G_TYPE_STRING, "{\"value\":123}",
		       NULL);
	gcut_add_datum("double",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_DOUBLE,
		       "json", G_TYPE_STRING, "{\"value\":123.45}",
		       NULL);
	gcut_add_datum("string",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_STRING,
		       "json", G_TYPE_STRING, "{\"value\":\"abc\"}",
		       NULL);
	gcut_add_datum("true",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_BOOLEAN,
		       "json", G_TYPE_STRING, "{\"value\":true}",
		       NULL);
	gcut_add_datum("false",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_BOOLEAN,
		       "json", G_TYPE_STRING, "{\"value\":false}",
		       NULL);
	gcut_add_datum("array",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_ARRAY,
		       "json", G_TYPE_STRING, "{\"value\":[]}",
		       NULL);
	gcut_add_datum("object",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_OBJECT,
		       "json", G_TYPE_STRING, "{\"value\":{}}",
		       NULL);
	gcut_add_datum("no member",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_UNKNOWN,
		       "json", G_TYPE_STRING, "{\"hoge\":123}",
		       NULL);
	gcut_add_datum("invalid json",
		       "expected", G_TYPE_INT, JSONParser::VALUE_TYPE_UNKNOWN,
		       "json", G_TYPE_STRING, "{hoge:123}",
		       NULL);
}

void test_valueType(gconstpointer data)
{
	JSONParser parser(gcut_data_get_string(data, "json"));
	JSONParser::ValueType expected =
	  static_cast<JSONParser::ValueType>(
	    gcut_data_get_int(data, "expected"));
	cppcut_assert_equal(expected, parser.getValueType("value"));
}

void test_getMemberNames(void)
{
	JSONParser parser("{\"foo\":1, \"dog\":\"cat\", \"book\":[1,2,3]}");
	set<string> names;
	parser.getMemberNames(names);

	const set<string> expect = {"foo", "dog", "book"};
	assertEqual(expect, names);
}

} //namespace testJSONParser
