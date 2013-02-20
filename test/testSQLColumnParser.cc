#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <typeinfo>
#include <cutter.h>
#include <cppcutter.h>
#include "SQLColumnParser.h"
#include "FormulaTestUtils.h"
#include "Asura.h"

namespace testSQLColumnParser {

static FormulaVariableDataGetter *columnDataGetter(string &name, void *priv)
{
	return NULL;
}

static void _assertInputStatement(SQLColumnParser &columnParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  columnParser.getSeparatorChecker();
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);

	while (!statement.finished()) {
		string word = statement.readWord(*separator);
		string lower = word;
		transform(lower.begin(), lower.end(),
		          lower.begin(), ::tolower);
		cppcut_assert_equal(true, columnParser.add(word, lower));
	}
	cppcut_assert_equal(true, columnParser.close());
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

void _assertColumn(SQLFormulaInfo *formulaInfo, const char *expectedName)
{
	cppcut_assert_equal(string(expectedName), formulaInfo->expression);
	assertFormulaVariable(formulaInfo->formula, expectedName);
	FormulaVariable *formulaVariable =
	  dynamic_cast<FormulaVariable *>(formulaInfo->formula);
	cut_assert_not_null(formulaVariable);
	cppcut_assert_equal(string(expectedName), formulaVariable->getName());
}
#define assertColumn(X,N) cut_trace(_assertColumn(X,N))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_one(void)
{
	const char *columnName = "column";
	ParsableString statement(StringUtils::sprintf("%s", columnName));
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	assertColumn(formulaInfoVector[0], columnName);
}

void test_multiColumn(void)
{
	const char *names[] = {"c1", "c2", "c3", "c4", "c5"};
	const size_t numNames = sizeof(names) / sizeof(const char *);
	string str;
	for (size_t i = 0; i < numNames; i++) {
		str += names[i];
		if (i != numNames - 1)
			str += ",";
	}
	ParsableString statement(str.c_str());
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal(numNames, formulaInfoVector.size());

	for (int i = 0; i < numNames; i++) 
		assertColumn(formulaInfoVector[i], names[i]);
}

void test_max(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("max(%s)", columnName));
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	string expected = StringUtils::sprintf("max(%s)", columnName);
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(expected, formulaInfo->expression);
	cppcut_assert_equal(true, formulaInfo->hasStatisticalFunc);

	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncMax(formulaElem);
	FormulaFuncMax *formulaFuncMax
	  = dynamic_cast<FormulaFuncMax *>(formulaElem);
	cppcut_assert_equal((size_t)1, formulaFuncMax->getNumberOfArguments());
	FormulaElement *arg = formulaFuncMax->getArgument(0);
	assertFormulaVariable(arg, columnName);
}

void test_as(void)
{
	const char *columnName = "c1";
	const char *aliasName = "dog";
	ParsableString statement(
	  StringUtils::sprintf("%s as %s", columnName, aliasName));
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	cppcut_assert_equal(string(aliasName), formulaInfo->alias);

	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaVariable(formulaElem, columnName);
}

void test_count(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("count(%s)", columnName));
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncCount(formulaElem);
	FormulaFuncCount *formulaFuncCount
	  = dynamic_cast<FormulaFuncCount *>(formulaElem);
	cppcut_assert_equal((size_t)1,
	                    formulaFuncCount->getNumberOfArguments());
	FormulaElement *arg = formulaFuncCount->getArgument(0);
	assertFormulaVariable(arg, columnName);
}

} // namespace testSQLColumnParser

