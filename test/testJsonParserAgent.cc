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

// -------------------------------------------------------------------------
// test cases
// -------------------------------------------------------------------------
void test_parseString(void)
{
	string json;
	assertReadFile("fixtures/testJson01.json", json);
	JsonParserAgent parser(json);
	cppcut_assert_equal(false, parser.hasError());
	assertReadWord(parser, "name0", "string value");
	assertReadWord(parser, "name1", "123");
}

} //namespace testJsonParserAgent


