#include <cutter.h>
#include <cppcutter.h>

#include "FormulaElement.h"

namespace testFormulaElement {

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];
static FormulaElement *&z_elem = g_elem[2];

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
void test_constructor(void)
{
	FormulaElement elem;
	cppcut_assert_equal((FormulaElement *)NULL, elem.getLeftHand());
	cppcut_assert_equal((FormulaElement *)NULL, elem.getRightHand());
}

void test_setGetLeftHand(void)
{
	FormulaElement elem;
	x_elem = new FormulaElement();
	elem.setLeftHand(x_elem);
	cppcut_assert_equal(x_elem, elem.getLeftHand());
	x_elem = NULL; // to avoid destructor from being called directly
}

void test_setGetRightHand(void)
{
	FormulaElement elem;
	x_elem = new FormulaElement();
	elem.setRightHand(x_elem);
	cppcut_assert_equal(x_elem, elem.getRightHand());
	x_elem = NULL; // to avoid destructor from being called directly
}

} // testFormulaElement
