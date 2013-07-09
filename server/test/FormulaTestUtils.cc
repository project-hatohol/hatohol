/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include "ItemDataUtils.h"
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
	int actual = ItemDataUtils::getInt(dataPtr);
	cppcut_assert_equal(expected, actual);
}

void _assertFormulaValue(FormulaElement *elem, double expected)
{
	assertTypeFormulaValue(elem);
	FormulaValue *formulaValue = dynamic_cast<FormulaValue *>(elem);
	cut_assert_not_null(formulaValue);
	ItemDataPtr dataPtr = formulaValue->evaluate();
	cppcut_assert_equal(ITEM_TYPE_DOUBLE, dataPtr->getItemType());
	double actual = ItemDataUtils::getDouble(dataPtr);;
	cppcut_assert_equal(expected, actual);
}

void _assertFormulaValue(FormulaElement *elem, const char *expected)
{
	assertTypeFormulaValue(elem);
	FormulaValue *formulaValue = dynamic_cast<FormulaValue *>(elem);
	cut_assert_not_null(formulaValue);
	ItemDataPtr dataPtr = formulaValue->evaluate();
	cppcut_assert_equal(ITEM_TYPE_STRING, dataPtr->getItemType());
	string actual = ItemDataUtils::getString(dataPtr);
	cppcut_assert_equal(string(expected), actual);
}

void _assertFormulaBetween(FormulaElement *elem, int v0, int v1)
{
	assertTypeFormulaBetween(elem);
	FormulaBetween *between = dynamic_cast<FormulaBetween *>(elem);

	ItemDataPtr dataPtr0 = between->getV0();
	ItemDataPtr dataPtr1 = between->getV1();
	const ItemInt *itemInt0 = dynamic_cast<const ItemInt *>(&*dataPtr0);
	const ItemInt *itemInt1 = dynamic_cast<const ItemInt *>(&*dataPtr1);
	cppcut_assert_not_null(itemInt0);
	cppcut_assert_not_null(itemInt1);
	cppcut_assert_equal(v0, itemInt0->get());
	cppcut_assert_equal(v1, itemInt1->get());
}

void _assertFormulaBetweenWithVarName(FormulaElement *elem, int v0, int v1,
                                      const char *name)
{
	assertFormulaBetween(elem, v0, v1);
	FormulaElement *leftHand = elem->getLeftHand();
	FormulaVariable *formulaVar = dynamic_cast<FormulaVariable *>(leftHand);
	cppcut_assert_not_null(formulaVar);
	cppcut_assert_equal(string(name), formulaVar->getName());
}


template<typename T, typename ItemDataType>
static void _assertFormulaInWithVarNameT
(FormulaElement *elem, vector<T> &expectedValues, const char *name)
{
	assertTypeFormulaIn(elem);
	FormulaElement *leftHand = elem->getLeftHand();
	FormulaVariable *formulaVar = dynamic_cast<FormulaVariable *>(leftHand);
	cppcut_assert_not_null(formulaVar);
	cppcut_assert_equal(string(name), formulaVar->getName());

	FormulaIn *elemIn = dynamic_cast<FormulaIn *>(elem);
	const ItemGroupPtr grpPtr = elemIn->getValues();
	size_t numExpected = expectedValues.size();
	cppcut_assert_equal(numExpected, grpPtr->getNumberOfItems());
	for (size_t i = 0; i < numExpected; i++) {
		const ItemData *data = grpPtr->getItemAt(i);
		const ItemDataType *item =
		   dynamic_cast<const ItemDataType *>(data);
		cppcut_assert_not_null(item);
		cppcut_assert_equal(expectedValues[i], item->get());
	}
}
#define assertFormulaInWithVarNameT(T, IDT, EL, EXP, N) \
cut_trace((_assertFormulaInWithVarNameT<T, IDT>(EL, EXP, N)))

void _assertFormulaInWithVarName(FormulaElement *elem,
                                 vector<int> &expectedValues, const char *name)
{
	assertFormulaInWithVarNameT(int, ItemInt, elem, expectedValues, name);
}

void _assertFormulaInWithVarName(FormulaElement *elem,
                                 StringVector &expectedValues, const char *name)
{
	assertFormulaInWithVarNameT(string, ItemString, elem,
	                            expectedValues, name);
}

void showTreeInfo(FormulaElement *formulaElement)
{
	string str;
	formulaElement->getTreeInfo(str);
	printf("\n%s\n", str.c_str());
}
