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
		columnParser.add(word, lower);
	}
	columnParser.flush();
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_add(void)
{
	const char *columnName = "column";
	ParsableString statement(StringUtils::sprintf("%s", columnName));
	SQLColumnParser columnParser;
	SeparatorCheckerWithCallback *separator =
	  columnParser.getSeparatorChecker();
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());

	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	FormulaVariable *formulaVariable =
	  dynamic_cast<FormulaVariable *>(formulaInfo->formula);
	cut_assert_not_null(formulaVariable);
	cppcut_assert_equal(string(columnName), formulaVariable->getName());
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

} // namespace testSQLColumnParser

