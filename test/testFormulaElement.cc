#include <cutter.h>
#include <cppcutter.h>

#include "FormulaElement.h"

namespace testFormulaElement {

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];
static FormulaElement *&z_elem = g_elem[2];

class TestFormulaElement : public FormulaElement
{
public:
	virtual ItemDataPtr evaluate(void)
	{
		return ItemDataPtr();
	}
};

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
	TestFormulaElement elem;
	cppcut_assert_equal((FormulaElement *)NULL, elem.getLeftHand());
	cppcut_assert_equal((FormulaElement *)NULL, elem.getRightHand());
}

void test_setGetLeftHand(void)
{
	TestFormulaElement elem;
	x_elem = new TestFormulaElement();
	elem.setLeftHand(x_elem);
	cppcut_assert_equal(x_elem, elem.getLeftHand());
	x_elem = NULL; // to avoid destructor from being called directly
}

void test_setGetRightHand(void)
{
	TestFormulaElement elem;
	x_elem = new TestFormulaElement();
	elem.setRightHand(x_elem);
	cppcut_assert_equal(x_elem, elem.getRightHand());
	x_elem = NULL; // to avoid destructor from being called directly
}

void test_getParent(void)
{
	TestFormulaElement elem;
	x_elem = new TestFormulaElement();
	elem.setRightHand(x_elem);
	cppcut_assert_equal(static_cast<FormulaElement *>(&elem),
	                    x_elem->getParent());
	x_elem = NULL; // to avoid destructor from being called directly
}

void test_getRootElement(void)
{
	TestFormulaElement elem;
	x_elem = new TestFormulaElement();
	y_elem = new TestFormulaElement();
	elem.setLeftHand(x_elem);
	x_elem->setRightHand(y_elem);
	cppcut_assert_equal(static_cast<FormulaElement *>(&elem),
	                    x_elem->getRootElement());
	cppcut_assert_equal(static_cast<FormulaElement *>(&elem),
	                    y_elem->getRootElement());
	x_elem = NULL; // to avoid destructor from being called directly
	y_elem = NULL; // to avoid destructor from being called directly
}

//
// FormulaValue
//
void test_formulaValueInt(void)
{
	int num = 3;
	FormulaValue formulaValue(num);
	ItemDataPtr itemData = formulaValue.evaluate();
	int actual;
	itemData->get(&actual);
	cppcut_assert_equal(num, actual);
	cppcut_assert_equal(2, itemData->getUsedCount());
}

void test_formulaValueString(void)
{
	string str = "ABcDE XYZ";
	FormulaValue formulaValue(str);
	ItemDataPtr itemData = formulaValue.evaluate();
	string actual;
	itemData->get(&actual);
	cppcut_assert_equal(str, actual);
	cppcut_assert_equal(2, itemData->getUsedCount());
}

} // testFormulaElement
