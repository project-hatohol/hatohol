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

// ---------------------------------------------------------------------------
// Formula Operator
// ---------------------------------------------------------------------------
FormulaOperator::FormulaOperator(FormulaElementPriority prio)
: FormulaElement(prio)
{
}

FormulaOperator::~FormulaOperator()
{
}

// ---------------------------------------------------------------------------
// class: FormulaComparatorEqual
// ---------------------------------------------------------------------------
FormulaComparatorEqual::FormulaComparatorEqual(void)
: FormulaOperator(FORMULA_ELEM_PRIO_CMP_EQ)
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
// FormulaOperatorAnd
// ---------------------------------------------------------------------------
FormulaOperatorAnd::FormulaOperatorAnd(void)
: FormulaOperator(FORMULA_ELEM_PRIO_AND)
{
}

FormulaOperatorAnd::~FormulaOperatorAnd()
{
}

ItemDataPtr FormulaOperatorAnd::evaluate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// FormulaBetween
// ---------------------------------------------------------------------------
FormulaBetween::FormulaBetween(ItemDataPtr v0, ItemDataPtr v1)
: FormulaOperator(FORMULA_ELEM_PRIO_BETWEEN),
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
