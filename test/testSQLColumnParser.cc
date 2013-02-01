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

namespace testSQLColumnParser {

static FormulaVariableDataGetter *columnDataGetter(string &name, void *priv)
{
	return NULL;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_add(void)
{
	const char *columnName = "column";
	ParsableString testString(StringUtils::sprintf("%s", columnName));
	SQLColumnParser columnParser;
	SeparatorCheckerWithCallback *separator =
	  columnParser.getSeparatorChecker();
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);

	while (!testString.finished()) {
		string word = testString.readWord(*separator);
		string lower;
		transform(word.begin(), word.end (),
		          lower.begin (), ::tolower); 
		columnParser.add(word, lower);
	}
	columnParser.flush();

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

} // namespace testSQLColumnParser

