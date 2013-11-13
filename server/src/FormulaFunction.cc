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

#include "FormulaFunction.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
// FormulaFunction
// ---------------------------------------------------------------------------
FormulaFunction::FormulaFunction(int numArgument)
: FormulaElement(FORMULA_ELEM_PRIO_FUNCTION, true),
  m_numArgument(numArgument)
{
}

FormulaFunction::~FormulaFunction()
{
	// To avoid double free
	setLeftHand(NULL);

	for (size_t i = 0; i < getNumberOfArguments(); i++)
		delete m_argVector[i];
}

size_t FormulaFunction::getNumberOfArguments(void)
{
	return m_argVector.size();
}

FormulaElement *FormulaFunction::getArgument(size_t index)
{
	if (index >= getNumberOfArguments()) {
		string msg;
		TRMSG(msg, "indxe (%zd) >= array sizei (%zd)",
		      index, getNumberOfArguments());
		throw invalid_argument(msg);
	}
	return m_argVector[index];
}

void FormulaFunction::pushArgument(FormulaElement *formulaElement)
{
	m_argVector.push_back(formulaElement);
}

//
// Virtual methods
//
void FormulaFunction::resetStatistics(void)
{
	for (size_t i = 0; i < m_argVector.size(); i++)
		m_argVector[i]->resetStatistics();
}

bool FormulaFunction::addArgument(FormulaElement *argument)
{
	if (m_numArgument >= 0) {
		if (getNumberOfArguments() >= (size_t)m_numArgument) {
			MLPL_DBG("Too many arguemnts.");
			return false;
		}
		setLeftHand(argument);
		pushArgument(argument);
	}
	return true;
}

bool FormulaFunction::close(void)
{
	if (m_numArgument >= 0) {
		if (getNumberOfArguments() != (size_t)m_numArgument) {
			MLPL_DBG("Number of argument is short: %zd / %d.\n",
			         getNumberOfArguments(), m_numArgument);
			return false;
		}
	}
	return true;
}

//
// Prtected methods
//
const FormulaElementVector &FormulaFunction::getArgVector(void) const
{
	return m_argVector;
}

// ---------------------------------------------------------------------------
// FormulaStatisticalFunc
// ---------------------------------------------------------------------------
FormulaStatisticalFunc::FormulaStatisticalFunc(int numArguments)
: FormulaFunction(numArguments)
{
}

// ---------------------------------------------------------------------------
// FormulaFuncMax
// ---------------------------------------------------------------------------
FormulaFuncMax::FormulaFuncMax(void)
: FormulaStatisticalFunc(NUM_ARGUMENTS_FUNC_MAX)
{
}

FormulaFuncMax::FormulaFuncMax(FormulaElement *arg)
{
	pushArgument(arg);
}

FormulaFuncMax::~FormulaFuncMax()
{
}

ItemDataPtr FormulaFuncMax::evaluate(void)
{
	const FormulaElementVector &elemVector = getArgVector();
	if (elemVector.empty()) {
		MLPL_DBG("argument: empty");
		return ItemDataPtr();
	}
	ItemDataPtr dataPtr = elemVector[0]->evaluate();
	if (!m_maxData.hasData())
		m_maxData = dataPtr;
	else if (*dataPtr > *m_maxData)
		m_maxData = dataPtr;
	return m_maxData;
}

void FormulaFuncMax::resetStatistics(void)
{
	m_maxData = ItemDataPtr();
	FormulaFunction::resetStatistics();
}

// ---------------------------------------------------------------------------
// FormulaFuncCount
// ---------------------------------------------------------------------------
FormulaFuncCount::FormulaFuncCount(void)
: FormulaStatisticalFunc(NUM_ARGUMENTS_FUNC_COUNT),
  m_count(0),
  m_isDistinct(false)
{
}

FormulaFuncCount::~FormulaFuncCount()
{
}

ItemDataPtr FormulaFuncCount::evaluate(void)
{
	if (isDistinct()) {
		const FormulaElementVector &elemVector = getArgVector();
		if (elemVector.empty()) {
			MLPL_DBG("argument: empty");
			return ItemDataPtr();
		}
		ItemDataPtr dataPtr = elemVector[0]->evaluate();
		m_itemDataSet.insert(dataPtr);
		size_t count = m_itemDataSet.size();
		return ItemDataPtr(new ItemInt(count), false);
	}

	m_count++;
	return ItemDataPtr(new ItemInt(m_count), false);
}

void FormulaFuncCount::resetStatistics(void)
{
	m_count = 0;
	m_itemDataSet.clear();
	FormulaFunction::resetStatistics();
}

bool FormulaFuncCount::isDistinct(void) const
{
	return m_isDistinct;
}

void FormulaFuncCount::setDistinct(void)
{
	m_isDistinct = true;
}

// ---------------------------------------------------------------------------
// FormulaFuncSum
// ---------------------------------------------------------------------------
FormulaFuncSum::FormulaFuncSum(void)
: FormulaStatisticalFunc(NUM_ARGUMENTS_FUNC_SUM)
{
}

FormulaFuncSum::~FormulaFuncSum()
{
}

ItemDataPtr FormulaFuncSum::evaluate(void)
{
	const FormulaElementVector &elemVector = getArgVector();
	if (elemVector.empty()) {
		MLPL_DBG("argument: empty");
		return ItemDataPtr();
	}
	ItemDataPtr dataPtr = elemVector[0]->evaluate();
	if (!dataPtr.hasData())
		return dataPtr;

	if (!m_dataPtr.hasData())
		m_dataPtr = VariableItemDataPtr(dataPtr->clone(), false);
	else 
		*m_dataPtr += *dataPtr;

	return ItemDataPtr(m_dataPtr);
}

void FormulaFuncSum::resetStatistics(void)
{
	m_dataPtr = NULL;
	FormulaFunction::resetStatistics();
}
