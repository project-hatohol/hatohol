/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FormulaOperator.h"
#include "ItemEnum.h"
#include "AsuraException.h"

// ---------------------------------------------------------------------------
// class: FormulaParenthesis
// ---------------------------------------------------------------------------
FormulaParenthesis::FormulaParenthesis(void)
: FormulaElement(FORMULA_ELEM_PRIO_PARENTHESIS, true)
{
}

FormulaParenthesis::~FormulaParenthesis()
{
}

ItemDataPtr FormulaParenthesis::evaluate(void)
{
	FormulaElement *child = getLeftHand();
	if (!child) {
		MLPL_DBG("child: NULL.\n");
		return ItemDataPtr();
	}
	return child->evaluate();
}

// ---------------------------------------------------------------------------
// class: FormulaComparatorEqual
// ---------------------------------------------------------------------------
FormulaComparatorEqual::FormulaComparatorEqual(void)
: FormulaElement(FORMULA_ELEM_PRIO_CMP_EQ)
{
}

FormulaComparatorEqual::~FormulaComparatorEqual()
{
}

ItemDataPtr FormulaComparatorEqual::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(*v0 == *v1), false);
}

// ---------------------------------------------------------------------------
// class: FormulaComparatorNotEqual
// ---------------------------------------------------------------------------
FormulaComparatorNotEqual::FormulaComparatorNotEqual(void)
: FormulaElement(FORMULA_ELEM_PRIO_CMP_NOT_EQ)
{
}

FormulaComparatorNotEqual::~FormulaComparatorNotEqual()
{
}

ItemDataPtr FormulaComparatorNotEqual::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(*v0 != *v1), false);
}

// ---------------------------------------------------------------------------
// class: FormulaGreaterThan
// ---------------------------------------------------------------------------
FormulaGreaterThan::FormulaGreaterThan(void)
: FormulaElement(FORMULA_ELEM_PRIO_CMP_EQ)
{
}

FormulaGreaterThan::~FormulaGreaterThan()
{
}

ItemDataPtr FormulaGreaterThan::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(*v0 == *v1), false);
}

// ---------------------------------------------------------------------------
// FormulaOperatorPlus
// ---------------------------------------------------------------------------
FormulaOperatorPlus::FormulaOperatorPlus(void)
: FormulaElement(FORMULA_ELEM_PRIO_PLUS)
{
}

FormulaOperatorPlus::~FormulaOperatorPlus()
{
}

ItemDataPtr FormulaOperatorPlus::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(*v0 + *v1, false);
}

// ---------------------------------------------------------------------------
// FormulaOperatorDiv
// ---------------------------------------------------------------------------
FormulaOperatorDiv::FormulaOperatorDiv(void)
: FormulaElement(FORMULA_ELEM_PRIO_DIV)
{
}

FormulaOperatorDiv::~FormulaOperatorDiv()
{
}

ItemDataPtr FormulaOperatorDiv::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(*v0 / *v1, false);
}

// ---------------------------------------------------------------------------
// FormulaOperatorAnd
// ---------------------------------------------------------------------------
FormulaOperatorAnd::FormulaOperatorAnd(void)
: FormulaElement(FORMULA_ELEM_PRIO_AND)
{
}

FormulaOperatorAnd::~FormulaOperatorAnd()
{
}

ItemDataPtr FormulaOperatorAnd::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	ItemDataPtr dataPtrTrue(new ItemBool(true), false);
	ItemDataPtr dataPtrFalse(new ItemBool(false), false);
	if (*v0 != *dataPtrTrue)
		return dataPtrFalse;
	if (*v1 != *dataPtrTrue)
		return dataPtrFalse;
	return dataPtrTrue;
}

// ---------------------------------------------------------------------------
// FormulaOperatorOr
// ---------------------------------------------------------------------------
FormulaOperatorOr::FormulaOperatorOr(void)
: FormulaElement(FORMULA_ELEM_PRIO_OR)
{
}

FormulaOperatorOr::~FormulaOperatorOr()
{
}

ItemDataPtr FormulaOperatorOr::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	ItemDataPtr dataPtrTrue(new ItemBool(true), false);
	ItemDataPtr dataPtrFalse(new ItemBool(false), false);
	if (*v0 == *dataPtrTrue)
		return dataPtrTrue;
	if (*v1 == *dataPtrTrue)
		return dataPtrTrue;
	return dataPtrFalse;
}

// ---------------------------------------------------------------------------
// FormulaBetween
// ---------------------------------------------------------------------------
FormulaBetween::FormulaBetween(ItemDataPtr v0, ItemDataPtr v1)
: FormulaElement(FORMULA_ELEM_PRIO_BETWEEN),
  m_v0(v0),
  m_v1(v1)
{
}

FormulaBetween::~FormulaBetween()
{
}

ItemDataPtr FormulaBetween::getV0(void) const
{
	return m_v0;
}

ItemDataPtr FormulaBetween::getV1(void) const
{
	return m_v1;
}

ItemDataPtr FormulaBetween::evaluate(void)
{
	ItemDataPtr varDataPtr;
	if (!getLeftHandDataWithCheck(varDataPtr))
		return ItemDataPtr();
	bool ret = true;
	if (*varDataPtr < *m_v0 ||*varDataPtr > *m_v1)
		ret = false;
	return ItemDataPtr(new ItemBool(ret), false);
}

// ---------------------------------------------------------------------------
// FormulaIn
// ---------------------------------------------------------------------------
FormulaIn::FormulaIn(ItemGroupPtr values)
: FormulaElement(FORMULA_ELEM_PRIO_IN),
  m_values(values)
{
}

FormulaIn::~FormulaIn()
{
}

const ItemGroupPtr FormulaIn::getValues(void) const
{
	return m_values;
}

ItemDataPtr FormulaIn::evaluate(void)
{
	ItemDataPtr varDataPtr;
	if (!getLeftHandDataWithCheck(varDataPtr))
		return ItemDataPtr();

	// TODO: use more efficient algorithm such as using an index.
	bool found = false;
	size_t num = m_values->getNumberOfItems();
	for (size_t i = 0; i < num; i++) {
		if (*m_values->getItemAt(i) == *varDataPtr) {
			found = true;
			break;
		}
	}
	return ItemDataPtr(new ItemBool(found), false);
}

// ---------------------------------------------------------------------------
// FormulaExists
// ---------------------------------------------------------------------------
FormulaExists::FormulaExists(const string statement)
: FormulaElement(FORMULA_ELEM_PRIO_EXISTS),
  m_statement(statement)
{
}

FormulaExists::~FormulaExists()
{
}

const string &FormulaExists::getStatement(void) const
{
	return m_statement;
}

ItemDataPtr FormulaExists::evaluate(void)
{
	THROW_ASURA_EXCEPTION("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}
