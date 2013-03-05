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

void _assertReadWord(JsonParserAgent &parser,
                     const string &name, const string &expect)
{
	string actual;
	cppcut_assert_equal(true, parser.read(name, actual));
	cppcut_assert_equal(expect, actual);
}
#define assertReadWord(A,X,Y) cut_trace(_assertReadWord(A,X, Y))

#define DEFINE_PARSER_AND_READ(PARTHER, JSON_MATERIAL) \
string _json; \
assertReadFile(JSON_MATERIAL, _json); \
JsonParserAgent parser(_json); \
cppcut_assert_equal(false, parser.hasError());


// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_parseString(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson01.json");
	assertReadWord(parser, "name0", "string value");
	assertReadWord(parser, "name1", "123");
}

void test_parseStringInObject(void)
{
	DEFINE_PARSER_AND_READ(parser, "fixtures/testJson02.json");

	cppcut_assert_equal(true, parser.startObject("object0"));
	assertReadWord(parser, "food", "donuts");
	parser.endObject();
	cppcut_assert_equal(true, parser.startObject("object1"));
	assertReadWord(parser, "name", "dog");
	assertReadWord(parser, "age", "5");
	parser.endObject();
}

} //namespace testJsonParserAgent


