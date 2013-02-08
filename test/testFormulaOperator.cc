#include <cppcutter.h>

#include "FormulaOperator.h"
#include "FormulaTestUtils.h"

namespace testFormulaOperator {

const static int TEST_ITEM_ID = 1;

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];

class TestVariableDataGetter : public FormulaVariableDataGetter {
public:
	static const int v0 = 15;
	static const int v1 = 2005;

	TestVariableDataGetter(int val)
	: m_val(val)
	{
	}

	virtual ItemDataPtr getData(void)
	{
		return ItemDataPtr(new ItemInt(m_val), false);
	}

private:
	int m_val;

};

void _assertFormulaBetweenVal(int v, bool expectedValue)
{
	string varName = "name";
	int v0 = TestVariableDataGetter::v0;
	int v1 = TestVariableDataGetter::v1;
	TestVariableDataGetter dataGetter(v);
	FormulaBetween between(new FormulaVariable(varName, &dataGetter),
	                       ItemDataPtr(new ItemInt(v0), false),
	                       ItemDataPtr(new ItemInt(v1), false));
	ItemDataPtr expected(new ItemBool(expectedValue), false);
	cppcut_assert_equal(true, *expected == *between.evaluate());
}
#define assertFormulaBetweenVal(V, E) cut_trace(_assertFormulaBetweenVal(V, E))

void _assertFormulaComparatorEqualVal(int v0, int v1)
{
	FormulaComparatorEqual formulaEq;
	FormulaValue *val0 = new FormulaValue(v0);
	FormulaValue *val1 = new FormulaValue(v1);
	formulaEq.setLeftHand(val0);
	formulaEq.setRightHand(val1);
	bool expectedVal = (v0 == v1);
	ItemDataPtr expected(new ItemBool(expectedVal), false);
	cppcut_assert_equal(true, *expected == *formulaEq.evaluate());
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

void test_formulaBetween(void)
{
	string varName = "name";
	int v0 = 15;
	int v1 = 2005;
	FormulaVariable *var = new FormulaVariable(varName, NULL);
	ItemDataPtr item0 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v0), false);
	ItemDataPtr item1 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v1), false);
	FormulaBetween between(var, item0, item1);
	assertFormulaBetween(&between, varName, v0, v1);
}

void test_formulaBetweenLower(void)
{
	int v = TestVariableDataGetter::v0;
	assertFormulaBetweenVal(v, true);
}

void test_formulaBetweenMid(void)
{
	int v = (TestVariableDataGetter::v0 + TestVariableDataGetter::v1)/2;
	assertFormulaBetweenVal(v, true);
}

void test_formulaBetweenUpper(void)
{
	int v = TestVariableDataGetter::v1;
	assertFormulaBetweenVal(v, true);
}

void test_formulaBetweenLowerMinus1(void)
{
	int v = TestVariableDataGetter::v0 - 1;
	assertFormulaBetweenVal(v, false);
}

void test_formulaBetweenUpperPlus1(void)
{
	int v = TestVariableDataGetter::v1 + 1;
	assertFormulaBetweenVal(v, false);
}

} // namespace testFormulaOperator

