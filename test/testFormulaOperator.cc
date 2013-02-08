#include <cppcutter.h>

#include "FormulaOperator.h"

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

void _assertFormulaBetween(int v, bool expectedValue)
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
#define assertFormulaBetween(V, E) cut_trace(_assertFormulaBetween(V, E))

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
void test_formulaBetween(void)
{
	string varName = "name";
	int v0 = 15;
	int v1 = 2005;
	FormulaVariable *var = new FormulaVariable(varName, NULL);
	ItemDataPtr item0 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v0), false);
	ItemDataPtr item1 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v1), false);
	FormulaBetween between(var, item0, item1);

	FormulaVariable *obtainedVar = between.getVariable();
	cppcut_assert_equal(var, obtainedVar);

	ItemDataPtr dataPtr0 = between.getV0();
	ItemDataPtr dataPtr1 = between.getV1();
	ItemInt *itemInt0 = dynamic_cast<ItemInt *>((ItemData *)dataPtr0);
	ItemInt *itemInt1 = dynamic_cast<ItemInt *>((ItemData *)dataPtr1);
	cppcut_assert_not_null(itemInt0);
	cppcut_assert_not_null(itemInt1);
	cppcut_assert_equal(v0, itemInt0->get());
	cppcut_assert_equal(v1, itemInt1->get());
}

void test_formulaBetweenLower(void)
{
	int v = TestVariableDataGetter::v0;
	assertFormulaBetween(v, true);
}

void test_formulaBetweenMid(void)
{
	int v = (TestVariableDataGetter::v0 + TestVariableDataGetter::v1)/2;
	assertFormulaBetween(v, true);
}

void test_formulaBetweenUpper(void)
{
	int v = TestVariableDataGetter::v1;
	assertFormulaBetween(v, true);
}

void test_formulaBetweenLowerMinus1(void)
{
	int v = TestVariableDataGetter::v0 - 1;
	assertFormulaBetween(v, false);
}

void test_formulaBetweenUpperPlus1(void)
{
	int v = TestVariableDataGetter::v1 + 1;
	assertFormulaBetween(v, false);
}

} // namespace testFormulaOperator

