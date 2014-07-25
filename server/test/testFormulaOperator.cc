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

#include <stdexcept>
#include <cppcutter.h>
#include "FormulaOperator.h"
#include "FormulaTestUtils.h"
using namespace std;

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

void _assertFormulaOperatorAndOrVal(bool v0, bool v1, bool opAnd)
{
	bool expectedVal;
	if (opAnd) {
		x_elem = new FormulaOperatorAnd();
		expectedVal = (v0 && v1);
	} else {
		x_elem = new FormulaOperatorOr();
		expectedVal = (v0 || v1);
	}
	FormulaValue *val0 = new FormulaValue(v0);
	FormulaValue *val1 = new FormulaValue(v1);
	x_elem->setLeftHand(val0);
	x_elem->setRightHand(val1);
	ItemDataPtr expected(new ItemBool(expectedVal), false);
	cppcut_assert_equal(*expected, *x_elem->evaluate());
}

#define assertFormulaOperatorAndVal(V0, V1) \
cut_trace(_assertFormulaOperatorAndOrVal(V0, V1, true))

#define assertFormulaOperatorOrVal(V0, V1) \
cut_trace(_assertFormulaOperatorAndOrVal(V0, V1, false))

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

void cut_teardown()
{
	for (int i = 0; i < NUM_ELEM_POOL; i++) {
		delete g_elem[i];
		g_elem[i] = NULL;
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

//
// FormulaOperatorAnd
//
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

//
// FormulaOperatorOr
//
void test_formulaOperatorOrFF(void)
{
	assertFormulaOperatorOrVal(false, false);
}

void test_formulaOperatorOrFT(void)
{
	assertFormulaOperatorOrVal(false, true);
}

void test_formulaOperatorOrTF(void)
{
	assertFormulaOperatorOrVal(true, false);
}

void test_formulaOperatorOrTT(void)
{
	assertFormulaOperatorOrVal(true, true);
}

//
// FormulaBetween
//
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

//
// FormulaOperatorDiv
//
void test_formulaOperatorDiv(void)
{
	double v0 = 1;
	double v1 = 1;
	FormulaOperatorDiv formulaDiv;
	FormulaValue *val0 = new FormulaValue(v0);
	FormulaValue *val1 = new FormulaValue(v1);
	formulaDiv.setLeftHand(val0);
	formulaDiv.setRightHand(val1);
	double expectedVal = v0 / v1;
	ItemDataPtr expected(new ItemDouble(expectedVal), false);
	cppcut_assert_equal(*expected, *formulaDiv.evaluate());
}

} // namespace testFormulaOperator

