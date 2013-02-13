#include <stdexcept>
#include <cppcutter.h>

#include "FormulaOperator.h"
#include "FormulaTestUtils.h"

namespace testFormulaOperator {

const static int TEST_ITEM_ID = 1;

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];

static const int g_v0 = 15;
static const int g_v1 = 2005;

void _assertFormulaBetweenVal(int v, bool expectedValue)
{
	FormulaBetween between(ItemDataPtr(new ItemInt(g_v0), false),
	                       ItemDataPtr(new ItemInt(g_v1), false));
	ItemDataPtr expected(new ItemBool(expectedValue), false);
	FormulaValue *formulaValue = new FormulaValue(v);
	between.setLeftHand(formulaValue);
	cppcut_assert_equal(*expected, *between.evaluate());
}
#define assertFormulaBetweenVal(V, E) cut_trace(_assertFormulaBetweenVal(V, E))

void _assertFormulaOperatorAndVal(bool v0, bool v1)
{
	FormulaOperatorAnd formulaEq;
	FormulaValue *val0 = new FormulaValue(v0);
	FormulaValue *val1 = new FormulaValue(v1);
	formulaEq.setLeftHand(val0);
	formulaEq.setRightHand(val1);
	bool expectedVal = (v0 && v1);
	ItemDataPtr expected(new ItemBool(expectedVal), false);
	cppcut_assert_equal(*expected, *formulaEq.evaluate());
}
#define assertFormulaOperatorAndVal(V0, V1) \
cut_trace(_assertFormulaOperatorAndVal(V0, V1))

void _assertFormulaComparatorEqualVal(int v0, int v1)
{
	FormulaComparatorEqual formulaEq;
	FormulaValue *val0 = new FormulaValue(v0);
	FormulaValue *val1 = new FormulaValue(v1);
	formulaEq.setLeftHand(val0);
	formulaEq.setRightHand(val1);
	bool expectedVal = (v0 == v1);
	ItemDataPtr expected(new ItemBool(expectedVal), false);
	cppcut_assert_equal(*expected, *formulaEq.evaluate());
}
#define assertFormulaComparatorEqualVal(V0, V1) \
cut_trace(_assertFormulaComparatorEqualVal(V0, V1))

void teardown()
{
	for (int i = 0; i < NUM_ELEM_POOL; i++) {
		if (g_elem[i]) {
			delete g_elem[i];
			g_elem[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

//
// FormulaParenthesis
//
void test_formulaParenthesis(void)
{
	int num = 5;
	FormulaParenthesis formulaParenthesis;
	FormulaElement *formulaValue = new FormulaValue(num);
	formulaParenthesis.setLeftHand(formulaValue);
	cppcut_assert_equal(*formulaValue->evaluate(),
	                    *formulaParenthesis.evaluate());
}

void test_formulaParenthesisSetRightHand(void)
{
	int num = 5;
	FormulaParenthesis formulaParenthesis;
	FormulaElement *formulaValue = new FormulaValue(num);
	bool receivedLogicError = false;
	try {
		formulaParenthesis.setRightHand(formulaValue);
	} catch (logic_error e) {
		receivedLogicError = true;
	}
	cppcut_assert_equal(true, receivedLogicError);
}

//
// FormulaComparatorEqual
//
void test_formulaComparatorEqualTrue(void)
{
	assertFormulaComparatorEqualVal(1, 1);
}

void test_formulaComparatorEqualFalse(void)
{
	assertFormulaComparatorEqualVal(1, 0);
}

void test_formulaComparatorEqualInvalidRight(void)
{
	FormulaComparatorEqual formulaEq;
	formulaEq.setLeftHand(new FormulaValue(1));
	cppcut_assert_equal(false, formulaEq.evaluate().hasData());
}

void test_formulaComparatorEqualInvalidLeft(void)
{
	FormulaComparatorEqual formulaEq;
	formulaEq.setRightHand(new FormulaValue(1));
	cppcut_assert_equal(false, formulaEq.evaluate().hasData());
}

void test_formulaOperatorAndFF(void)
{
	assertFormulaOperatorAndVal(false, false);
}

void test_formulaOperatorAndFT(void)
{
	assertFormulaOperatorAndVal(false, true);
}

void test_formulaOperatorAndTF(void)
{
	assertFormulaOperatorAndVal(true, false);
}

void test_formulaOperatorAndTT(void)
{
	assertFormulaOperatorAndVal(true, true);
}

void test_formulaBetween(void)
{
	int v0 = 15;
	int v1 = 2005;
	ItemDataPtr item0 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v0), false);
	ItemDataPtr item1 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v1), false);
	FormulaBetween between(item0, item1);
	assertFormulaBetween(&between, v0, v1);
}

void test_formulaBetweenLower(void)
{
	assertFormulaBetweenVal(g_v0, true);
}

void test_formulaBetweenMid(void)
{
	assertFormulaBetweenVal((g_v0 + g_v1) / 2, true);
}

void test_formulaBetweenUpper(void)
{
	assertFormulaBetweenVal(g_v1, true);
}

void test_formulaBetweenLowerMinus1(void)
{
	assertFormulaBetweenVal(g_v0 - 1, false);
}

void test_formulaBetweenUpperPlus1(void)
{
	assertFormulaBetweenVal(g_v1 + 1, false);
}

} // namespace testFormulaOperator

