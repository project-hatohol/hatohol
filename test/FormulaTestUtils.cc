#include <cppcutter.h>
#include "FormulaTestUtils.h"

void _assertFormulaVariable(FormulaElement *elem, const char *expected)
{
	assertTypeFormulaVariable(elem);
	FormulaVariable *formulaVariable =
	  dynamic_cast<FormulaVariable *>(elem);
	cut_assert_not_null(formulaVariable);
	cppcut_assert_equal(string(expected), formulaVariable->getName());
}

void _assertFormulaValue(FormulaElement *elem, int expected)
{
	assertTypeFormulaValue(elem);
	FormulaValue *formulaValue = dynamic_cast<FormulaValue *>(elem);
	cut_assert_not_null(formulaValue);
	ItemDataPtr dataPtr = formulaValue->evaluate();
	cppcut_assert_equal(ITEM_TYPE_INT, dataPtr->getItemType());
	int actual;
	dataPtr->get(&actual);
	cppcut_assert_equal(expected, actual);
}

void _assertFormulaValue(FormulaElement *elem, const char *expected)
{
	assertTypeFormulaValue(elem);
	FormulaValue *formulaValue = dynamic_cast<FormulaValue *>(elem);
	cut_assert_not_null(formulaValue);
	ItemDataPtr dataPtr = formulaValue->evaluate();
	cppcut_assert_equal(ITEM_TYPE_STRING, dataPtr->getItemType());
	string actual;
	dataPtr->get(&actual);
	cppcut_assert_equal(string(expected), actual);
}

void _assertFormulaBetween(FormulaElement *elem, int v0, int v1)
{
	assertTypeFormulaBetween(elem);
}

void showTreeInfo(FormulaElement *formulaElement)
{
	string str;
	formulaElement->getTreeInfo(str);
	printf("\n%s\n", str.c_str());
}
