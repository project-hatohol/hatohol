#include <cppcutter.h>

#include "FormulaOperator.h"

namespace testFormulaOperator {

const static int TEST_ITEM_ID = 1;

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];

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
	int v0 = 15;
	int v1 = 2005;
	ItemDataPtr item0 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v0), false);
	ItemDataPtr item1 = ItemDataPtr(new ItemInt(TEST_ITEM_ID, v1), false);
	FormulaBetween between(item0, item1);

	ItemDataPtr dataPtr0 = between.getV0();
	ItemDataPtr dataPtr1 = between.getV1();
	ItemInt *itemInt0 = dynamic_cast<ItemInt *>((ItemData *)dataPtr0);
	ItemInt *itemInt1 = dynamic_cast<ItemInt *>((ItemData *)dataPtr1);
	cppcut_assert_not_null(itemInt0);
	cppcut_assert_not_null(itemInt1);
	cppcut_assert_equal(v0, itemInt0->get());
	cppcut_assert_equal(v1, itemInt1->get());
}

} // namespace testFormulaOperator

