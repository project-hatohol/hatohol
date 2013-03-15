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
#include "SQLProcessorSelect.h"
#include "SQLProcessorException.h"

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

FormulaOptimizationResult FormulaComparatorEqual::optimize(void)
{
	FormulaOptimizationResult result;
	FormulaOptimizationResult resultLeft;
	FormulaOptimizationResult resultRight;
	FormulaElement *leftHand  = getLeftHand();
	FormulaElement *rightHand = getRightHand();
	if (leftHand == NULL)
		THROW_SQL_PROCESSOR_EXCEPTION("Left hand is NULL.");
	if (rightHand == NULL)
		THROW_SQL_PROCESSOR_EXCEPTION("Left hand is NULL.");
	resultLeft  = leftHand->optimize();
	resultRight = rightHand->optimize();
	if (resultLeft.type == FORMULA_ALWAYS_TRUE) {
		if (resultRight.type == FORMULA_ALWAYS_TRUE)
			resultLeft.type = FORMULA_ALWAYS_TRUE;
		else if (resultRight.type == FORMULA_ALWAYS_FALSE)
			resultLeft.type = FORMULA_ALWAYS_FALSE;
	} else if (resultLeft.type == FORMULA_ALWAYS_FALSE) {
		if (resultRight.type == FORMULA_ALWAYS_TRUE)
			resultLeft.type = FORMULA_ALWAYS_FALSE;
		else if (resultRight.type == FORMULA_ALWAYS_FALSE)
			resultLeft.type = FORMULA_ALWAYS_TRUE;
	} else if (resultLeft.type == FORMULA_ALWAYS_CONST &&
	           resultRight.type == FORMULA_ALWAYS_CONST) {
		bool isEqual = (*resultLeft.itemData == *resultRight.itemData);
		if (isEqual)
			result.type = FORMULA_ALWAYS_TRUE;
		else
			result.type = FORMULA_ALWAYS_FALSE;
	}
	return setOptimizationResult(result);
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
: FormulaElement(FORMULA_ELEM_PRIO_GT)
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
	return ItemDataPtr(new ItemBool(*v0 > *v1), false);
}

// ---------------------------------------------------------------------------
// class: FormulaGreaterOrEqual
// ---------------------------------------------------------------------------
FormulaGreaterOrEqual::FormulaGreaterOrEqual(void)
: FormulaElement(FORMULA_ELEM_PRIO_GE)
{
}

FormulaGreaterOrEqual::~FormulaGreaterOrEqual()
{
}

ItemDataPtr FormulaGreaterOrEqual::evaluate(void)
{
	ItemDataPtr v0, v1;
	if (!getLeftHandDataWithCheck(v0) || !getRightHandDataWithCheck(v1))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(*v0 >= *v1), false);
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
// FormulaOperatorNot
// ---------------------------------------------------------------------------
FormulaOperatorNot::FormulaOperatorNot(void)
: FormulaElement(FORMULA_ELEM_PRIO_NOT)
{
}

FormulaOperatorNot::~FormulaOperatorNot()
{
}

ItemDataPtr FormulaOperatorNot::evaluate(void)
{
	ItemDataPtr v0;
	if (!getLeftHandDataWithCheck(v0))
		return ItemDataPtr();
	ItemDataPtr dataPtrTrue(new ItemBool(true), false);
	ItemDataPtr dataPtrFalse(new ItemBool(false), false);
	if (*v0 == *dataPtrFalse)
		return dataPtrFalse;
	return dataPtrTrue;
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

FormulaOptimizationResult FormulaOperatorAnd::optimize(void)
{
	FormulaElement *leftHand = getLeftHand();
	if (leftHand == NULL)
		THROW_SQL_PROCESSOR_EXCEPTION("Left hand is NULL.");
	FormulaOptimizationResult resultLeft = leftHand->optimize();
	if (resultLeft.type == FORMULA_ALWAYS_FALSE)
		return setOptimizationResult(resultLeft);

	FormulaElement *rightHand = getRightHand();
	if (rightHand == NULL)
		THROW_SQL_PROCESSOR_EXCEPTION("Left hand is NULL.");
	FormulaOptimizationResult resultRight = rightHand->optimize();
	if (resultRight.type == FORMULA_ALWAYS_FALSE)
		return setOptimizationResult(resultRight);

	bool leftIsTrue  = (resultLeft.type  == FORMULA_ALWAYS_TRUE);
	bool rightIsTrue = (resultRight.type == FORMULA_ALWAYS_TRUE);
	if (leftIsTrue && rightIsTrue)
		return setOptimizationResult(resultLeft);

	return getOptimizationResult();
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
FormulaExists::FormulaExists(const string &statement,
                             SQLProcessorSelectFactory &procSelectFactory)
: FormulaElement(FORMULA_ELEM_PRIO_EXISTS),
  m_processorSelectFactory(procSelectFactory),
  m_statement(statement),
  m_processorSelect(NULL),
  m_selectInfo(NULL)
{
}

FormulaExists::~FormulaExists()
{
	if (m_processorSelect)
		delete m_processorSelect;
	if (m_selectInfo)
		delete m_selectInfo;
}

const string &FormulaExists::getStatement(void) const
{
	return m_statement;
}

ItemDataPtr FormulaExists::evaluate(void)
{
	if (!m_processorSelect) {
		m_processorSelect = m_processorSelectFactory();
		ParsableString parsableStatement(m_statement);
		m_selectInfo = new SQLSelectInfo(parsableStatement);
        }
	bool exists = m_processorSelect->runForExists(*m_selectInfo);
	return ItemDataPtr(new ItemBool(exists), false);
}

// ---------------------------------------------------------------------------
// FormulaIsNull
// ---------------------------------------------------------------------------
FormulaIsNull::FormulaIsNull(void)
: FormulaElement(FORMULA_ELEM_PRIO_IS_NULL, true)
{
}

FormulaIsNull::~FormulaIsNull()
{
}

ItemDataPtr FormulaIsNull::evaluate(void)
{
	ItemDataPtr dataPtr;
	if (!getLeftHandDataWithCheck(dataPtr))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(dataPtr->isNull()), false);
}

// ---------------------------------------------------------------------------
// FormulaIsNotNull
// ---------------------------------------------------------------------------
FormulaIsNotNull::FormulaIsNotNull(void)
: FormulaElement(FORMULA_ELEM_PRIO_IS_NOT_NULL, true)
{
}

FormulaIsNotNull::~FormulaIsNotNull()
{
}

ItemDataPtr FormulaIsNotNull::evaluate(void)
{
	ItemDataPtr dataPtr;
	if (!getLeftHandDataWithCheck(dataPtr))
		return ItemDataPtr();
	return ItemDataPtr(new ItemBool(!dataPtr->isNull()), false);
}
