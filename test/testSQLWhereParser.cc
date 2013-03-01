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
#include "AsuraException.h"

namespace testSQLWhereParser {

static
FormulaVariableDataGetter *columnDataGetter(const string &name, void *priv)
{
	return NULL;
}

class SQLProcessorSelectFactoryImpl : public SQLProcessorSelectFactory
{
	virtual SQLProcessorSelect * operator()(void) {
		return NULL;
	}
};

static SQLProcessorSelectFactoryImpl procSelectFactory;
static SQLProcessorSelectShareInfo shareInfo(procSelectFactory);

static void _assertInputStatement(SQLWhereParser &whereParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  whereParser.getSeparatorChecker();
	whereParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	shareInfo.statement = &statement;
	whereParser.setShareInfo(&shareInfo);

	try {
		while (!statement.finished()) {
			string word = statement.readWord(*separator);
			string lower = StringUtils::toLower(word);
			whereParser.add(word, lower);
		}
		whereParser.close();
	} catch (const AsuraException &e) {
		cut_fail("Got exception: %s", e.getFancyMessage().c_str());
	}
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

#define DEFINE_PARSER_AND_RUN(WPTHR, FELEM, STATMNT) \
ParsableString _statement(STATMNT); \
SQLWhereParser WPTHR; \
assertInputStatement(WPTHR, _statement); \
FormulaElement *FELEM = WPTHR.getFormula(); \
cppcut_assert_not_null(FELEM);

template<typename T, typename VT>
static void _assertWhereIn(T *expectedValueArray, size_t numValue)
{
	const char *columnName = "testColumnForWhere";
	vector<VT> expectedValues;
	stringstream statementStream;
	statementStream << columnName;
	statementStream << " in (";
	for (size_t i = 0; i < numValue; i++) {
		T &v = expectedValueArray[i];
		statementStream << "'";
		statementStream << v;
		statementStream << "'";
		if (i < numValue - 1)
			statementStream << ",";
		else
			statementStream << ")";

		expectedValues.push_back(v);
	}

	ParsableString statement(statementStream.str());
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaInWithVarName(formula, expectedValues, columnName);
}
#define assertWhereIn(T, VT, EXP, NUM) \
cut_trace((_assertWhereIn<T, VT>(EXP, NUM)))

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

void test_whereInInt(void)
{
	int v[] = {5};
	const size_t numValue = sizeof(v) / sizeof(int);
	assertWhereIn(int, int, v, numValue);
}

void test_whereInMultipleInt(void)
{
	int v[] = {5, -8, 200};
	const size_t numValue = sizeof(v) / sizeof(int);
	assertWhereIn(int, int, v, numValue);
}

void test_whereInString(void)
{
	const char *v[] = {"foo goo XYZ, y = f(x)"};
	const size_t numValue = sizeof(v) / sizeof(const char *);
	assertWhereIn(const char *, string, v, numValue);
}

void test_whereInMultipleString(void)
{
	const char *v[] = {"A A A", "<x,y> = (8,5)", "Isaac Newton"};
	const size_t numValue = sizeof(v) / sizeof(const char *);
	assertWhereIn(const char *, string, v, numValue);
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

	FormulaElement *orElem = innerParenElem->getLeftHand();
	assertFormulaOperatorOr(orElem);
	assertFormulaVariable(orElem->getLeftHand(), elemName0);
	assertFormulaVariable(orElem->getRightHand(), elemName1);

	// The element before closing parenthesis
	assertFormulaVariable(andElem->getRightHand(), elemName2);

	// The most right hand
	assertFormulaVariable(formula->getRightHand(), elemName3);
}

void test_plus(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	string statement =
	  StringUtils::sprintf("%s + %s = %s",
	                       elemName0, elemName1, elemName2);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorEqual(formula);

	FormulaElement *plusElem = formula->getLeftHand();
	assertFormulaOperatorPlus(plusElem);
	assertFormulaVariable(plusElem->getLeftHand(), elemName0);
	assertFormulaVariable(plusElem->getRightHand(), elemName1);
	assertFormulaVariable(formula->getRightHand(), elemName2);
}

void test_greaterThan(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	string statement =
	  StringUtils::sprintf("%s > %s", elemName0, elemName1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaGreaterThan(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
	assertFormulaVariable(formula->getRightHand(), elemName1);
}

void test_notEqGtLt(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	string statement =
	  StringUtils::sprintf("%s <> %s", elemName0, elemName1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorNotEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
	assertFormulaVariable(formula->getRightHand(), elemName1);
}

void test_exists(void)
{
	const char *innerSelect = "select ((a/3)*2+8) from b where (x+5*c)=d";
	string statement =
	  StringUtils::sprintf("exists (%s)", innerSelect);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	FormulaExists *formulaExists = dynamic_cast<FormulaExists *>(formula);
	cppcut_assert_equal(string(innerSelect), formulaExists->getStatement());
}

void test_not(void)
{
	const char *columnName = "col";
	string statement = StringUtils::sprintf("not %s", columnName);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	FormulaOperatorNot *formulaNot =
	  dynamic_cast<FormulaOperatorNot *>(formula);
	assertFormulaOperatorNot(formulaNot);
	assertFormulaVariable(formula->getLeftHand(), columnName);
}

} // namespace testSQLWhereParser

