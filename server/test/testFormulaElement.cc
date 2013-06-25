#include <cutter.h>
#include <cppcutter.h>

#include "FormulaElement.h"
#include "ItemDataUtils.h"

namespace testFormulaElement {

const static int NUM_ELEM_POOL = 10;
static FormulaElement *g_elem[NUM_ELEM_POOL];
static FormulaElement *&x_elem = g_elem[0];
static FormulaElement *&y_elem = g_elem[1];
static FormulaElement *&z_elem = g_elem[2];

class TestFormulaElement : public FormulaElement
{
public:
	TestFormulaElement(void)
	: FormulaElement(static_cast<FormulaElementPriority>(-1))
	{
		setTerminalElement();
	}

	virtual ItemDataPtr evaluate(void)
	{
		return ItemDataPtr();
	}
};

class TestFormulaElementPrio0 : public FormulaElement
{
public:
	TestFormulaElementPrio0(void)
	: FormulaElement(static_cast<FormulaElementPriority>(0))
	{
	}

	virtual ItemDataPtr evaluate(void)
	{
		return ItemDataPtr();
	}

	void callSetTerminalElement(void)
	{
		setTerminalElement();
	}
};

class TestFormulaElementPrio1 : public FormulaElement
{
public:
	TestFormulaElementPrio1(void)
	: FormulaElement(static_cast<FormulaElementPriority>(1))
	{
	}

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

void test_terminalElementInConstructor(void)
{
	TestFormulaElement elem;
	cppcut_assert_equal(true, elem.isTerminalElement());
}

void test_terminalElementAfterCreated(void)
{
	TestFormulaElementPrio0 elem;
	cppcut_assert_equal(false, elem.isTerminalElement());
	elem.callSetTerminalElement();
	cppcut_assert_equal(true, elem.isTerminalElement());
}

void test_findInsertPoint(void)
{
	TestFormulaElement      *elemPrioH = new TestFormulaElement();
	TestFormulaElementPrio0 *elemPrioM = new TestFormulaElementPrio0();
	TestFormulaElementPrio1 *elemPrioL = new TestFormulaElementPrio1();
	x_elem = elemPrioL; // to free the tree in teardown()

	elemPrioL->setLeftHand(elemPrioM);
	elemPrioM->setRightHand(elemPrioH);

	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioL),
	                    elemPrioH->findInsertPoint(elemPrioL));
	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioM),
	                    elemPrioH->findInsertPoint(elemPrioM));
	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioH),
	                    elemPrioH->findInsertPoint(elemPrioH));

	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioL),
	                    elemPrioM->findInsertPoint(elemPrioL));
	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioM),
	                    elemPrioM->findInsertPoint(elemPrioM));
	cppcut_assert_equal(static_cast<FormulaElement *>(NULL),
	                    elemPrioM->findInsertPoint(elemPrioH));

	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioL),
	                    elemPrioL->findInsertPoint(elemPrioL));
	cppcut_assert_equal(static_cast<FormulaElement *>(NULL),
	                    elemPrioL->findInsertPoint(elemPrioM));
	cppcut_assert_equal(static_cast<FormulaElement *>(NULL),
	                    elemPrioL->findInsertPoint(elemPrioH));
}

void test_findInsertPointInsertMid(void)
{
	TestFormulaElement      *elemPrioH2 = new TestFormulaElement();
	TestFormulaElement      *elemPrioH  = new TestFormulaElement();
	TestFormulaElementPrio0 *elemPrioM  = new TestFormulaElementPrio0();
	TestFormulaElementPrio1 *elemPrioL  = new TestFormulaElementPrio1();
	x_elem = elemPrioL; // to free the tree in teardown()
	y_elem = elemPrioM; // to free the tree in teardown()

	elemPrioL->setLeftHand(elemPrioH);
	elemPrioH->setLeftHand(elemPrioH2);

	cppcut_assert_equal(static_cast<FormulaElement *>(elemPrioH),
	                    elemPrioH2->findInsertPoint(elemPrioM));
}

//
// FormulaVariable
//
void test_formulaVariableTerminal(void)
{
	string varName = "a";
	FormulaVariable formulaVariable(varName, NULL);
	cppcut_assert_equal(true, formulaVariable.isTerminalElement());
}

//
// FormulaValue
//
void test_formulaValueInt(void)
{
	int num = 3;
	FormulaValue formulaValue(num);
	cppcut_assert_equal(true, formulaValue.isTerminalElement());
	ItemDataPtr itemData = formulaValue.evaluate();
	int actual = ItemDataUtils::getInt(itemData);
	cppcut_assert_equal(num, actual);
	cppcut_assert_equal(2, itemData->getUsedCount());
}

void test_formulaValueString(void)
{
	string str = "ABcDE XYZ";
	FormulaValue formulaValue(str);
	cppcut_assert_equal(true, formulaValue.isTerminalElement());
	ItemDataPtr itemData = formulaValue.evaluate();
	string actual = ItemDataUtils::getString(itemData);
	cppcut_assert_equal(str, actual);
	cppcut_assert_equal(2, itemData->getUsedCount());
}

} // testFormulaElement
