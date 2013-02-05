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
#include "SQLWhereParser.h"
#include "FormulaOperator.h"
#include "FormulaTestUtils.h"
#include "Asura.h"

namespace testSQLWhereParser {

static FormulaVariableDataGetter *columnDataGetter(string &name, void *priv)
{
	return NULL;
}

static void _assertInputStatement(SQLWhereParser &whereParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  whereParser.getSeparatorChecker();
	whereParser.setColumnDataGetterFactory(columnDataGetter, NULL);

	while (!statement.finished()) {
		string word = statement.readWord(*separator);
		string lower = word;
		transform(lower.begin(), lower.end(),
		          lower.begin(), ::tolower);
		whereParser.add(word, lower);
	}
	whereParser.flush();
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_whereEqNumber(void)
{
	const char *leftHand = "a";
	int rightHand = 1;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaValue(formula->getRightHand(), rightHand);
}

void test_whereEqString(void)
{
	const char *leftHand = "a";
	const char *rightHand = "foo XYZ";
	ParsableString statement(
	  StringUtils::sprintf("%s='%s'", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaValue(formula->getRightHand(), rightHand);
}

void test_whereEqColumnColumn(void)
{
	const char *leftHand = "a";
	const char *rightHand = "b";
	ParsableString statement(
	  StringUtils::sprintf("%s=%s", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaVariable(formula->getRightHand(), rightHand);
}

void test_whereAnd(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s='%s'",
	                       leftHand0, rightHand0, leftHand1, rightHand1));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	// left child
	FormulaElement *leftElem = formula->getLeftHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// right child
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(rightElem);
	assertFormulaVariable(rightElem->getLeftHand(), leftHand1);
	assertFormulaValue(rightElem->getRightHand(), rightHand1);
}

} // namespace testSQLWhereParser

