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
		cppcut_assert_equal(true, whereParser.add(word, lower));
	}
	cppcut_assert_equal(true, whereParser.close());
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

#define DEFINE_PARSER_AND_RUN(WPTHR, FELEM, STATMNT) \
ParsableString _statement(STATMNT); \
SQLWhereParser WPTHR; \
assertInputStatement(WPTHR, _statement); \
FormulaElement *FELEM = WPTHR.getFormula(); \
assertFormulaOperatorAnd(FELEM);

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

void test_whereBetween(void)
{
	const char *leftHand = "a";
	int v0 = 0;
	int v1 = 100;
	ParsableString statement(
	  StringUtils::sprintf("%s between %d and %d", leftHand, v0, v1));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaBetweenWithVarName(formula, v0, v1, leftHand);
}

void test_whereIntAndStringAndInt(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s='%s' and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaOperatorAnd(rightElem);

	// Top left child
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// Top right child
	assertFormulaComparatorEqual(rightElem->getLeftHand());
	assertFormulaComparatorEqual(rightElem->getRightHand());

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertFormulaVariable(leftElem->getLeftHand(), leftHand1);
	assertFormulaValue(leftElem->getRightHand(), rightHand1);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertFormulaVariable(rightElem->getLeftHand(), leftHand2);
	assertFormulaValue(rightElem->getRightHand(), rightHand2);
}

void test_whereIntAndColumnAndInt(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "x";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s=%s and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaOperatorAnd(rightElem);

	// Top left child
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// Top right child
	assertFormulaComparatorEqual(rightElem->getLeftHand());
	assertFormulaComparatorEqual(rightElem->getRightHand());

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertFormulaVariable(leftElem->getLeftHand(), leftHand1);
	assertFormulaVariable(leftElem->getRightHand(), rightHand1);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertFormulaVariable(rightElem->getLeftHand(), leftHand2);
	assertFormulaValue(rightElem->getRightHand(), rightHand2);
}

void test_intAndBetween(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;

	const char *btwVar = "b";
	int btwVal0 = 0;
	int btwVal1 = 100;
	string statement =
	  StringUtils::sprintf("%s=%d and %s between %d and %d",
	                       leftHand0, rightHand0,
	                       btwVar, btwVal0, btwVal1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);
	assertFormulaBetweenWithVarName(rightElem, btwVal0, btwVal1, btwVar);
}

void test_parenthesis(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	string statement =
	  StringUtils::sprintf("(%s or %s) and %s",
	                       elemName0, elemName1, elemName2);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);

	// In parenthesis
	FormulaElement *leftElem = formula->getLeftHand();
	assertFormulaParenthesis(leftElem);

	leftElem = leftElem->getLeftHand();
	assertFormulaOperatorOr(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), elemName0);
	assertFormulaVariable(leftElem->getRightHand(), elemName1);

	// right hand
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaVariable(rightElem, elemName2);
}

void test_parenthesisDouble(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	const char *elemName3  = "d";
	string statement =
	  StringUtils::sprintf("((%s or %s) and %s) or %s",
	                       elemName0, elemName1, elemName2, elemName3);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorOr(formula);

	// In the outer parenthesis
	FormulaElement *outerParenElem = formula->getLeftHand();
	assertFormulaParenthesis(outerParenElem);

	FormulaElement *andElem = outerParenElem->getLeftHand();
	assertFormulaOperatorAnd(andElem);

	// Hands of 'and'
	FormulaElement *innerParenElem = andElem->getLeftHand();
	assertFormulaParenthesis(innerParenElem);

	FormulaElement *orElem = outerParenElem->getLeftHand();
	assertFormulaOperatorOr(andElem);
	assertFormulaVariable(orElem->getLeftHand(), elemName0);
	assertFormulaVariable(orElem->getRightHand(), elemName1);

	// The element before closing parenthesis
	assertFormulaVariable(andElem->getRightHand(), elemName2);

	// The most right hand
	assertFormulaVariable(formula->getRightHand(), elemName3);
}

} // namespace testSQLWhereParser

