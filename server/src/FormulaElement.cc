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
#include "Logger.h"
#include "FormulaElement.h"
#include "ItemEnum.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// FormulaOptimizationResult
// ---------------------------------------------------------------------------
FormulaOptimizationResult::FormulaOptimizationResult(void)
: type(FORMULA_UNFIXED)
{
}

FormulaOptimizationResult &
FormulaOptimizationResult::operator=(const FormulaOptimizationResult &rhs)
{
	type = rhs.type;
	itemData = rhs.itemData;
	return *this;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FormulaElement::FormulaElement(FormulaElementPriority priority, bool unary)
: m_unary(unary),
  m_leftHand(NULL),
  m_rightHand(NULL),
  m_parent(NULL),
  m_priority(priority),
  m_terminalElement(false)
{
}

FormulaElement::~FormulaElement()
{
	delete m_leftHand;
	delete m_rightHand;
}

void FormulaElement::setLeftHand(FormulaElement *elem)
{
	m_leftHand = elem;
	if (elem)
		m_leftHand->m_parent = this;
}

void FormulaElement::setRightHand(FormulaElement *elem)
{
	if (m_unary) {
		string msg;
		TRMSG(msg, "m_unary: true.\n");
		throw logic_error(msg);
	}
	m_rightHand = elem;
	m_rightHand->m_parent = this;
}

FormulaElement *FormulaElement::getLeftHand(void) const
{
	return m_leftHand;
}

FormulaElement *FormulaElement::getRightHand(void) const
{
	return m_rightHand;
}

FormulaElement *FormulaElement::getParent(void) const
{
	return m_parent;
}

FormulaElement *FormulaElement::getRootElement(void)
{
	FormulaElement *elem = this;
	while (elem) {
		FormulaElement *nextElement = elem->getParent();
		if (!nextElement)
			break;
		elem = nextElement;
	}
	return elem;
}

bool FormulaElement::isUnary(void) const
{
	return m_unary;
}

bool FormulaElement::isTerminalElement(void) const
{
	return m_terminalElement;
}

bool FormulaElement::priorityOver(FormulaElement *formulaElement)
{
	return m_priority < formulaElement->m_priority;
}

bool FormulaElement::priorityEqual(FormulaElement *formulaElement)
{
	return m_priority == formulaElement->m_priority;
}

FormulaElement *FormulaElement::findInsertPoint(FormulaElement *insertElem,
                                                FormulaElement *upperLimitElem)
{
	FormulaElement *prev = NULL;
	FormulaElement *elem = this;
	while (elem) {
		if (elem == upperLimitElem)
			break;
		if (insertElem->priorityOver(elem))
			break;
		prev = elem;
		elem = elem->m_parent;
	}
	return prev;
}

FormulaOptimizationResult FormulaElement::optimize(void)
{
	FormulaOptimizationResult result;
	result.type = FORMULA_UNFIXED;
	return result;
}

void FormulaElement::removeParenthesis(void)
{
	if (m_leftHand)
		m_leftHand->removeParenthesis();
	if (m_rightHand)
		m_rightHand->removeParenthesis();
}

void FormulaElement::resetStatistics(void)
{
	if (m_leftHand)
		m_leftHand->resetStatistics();
	if (m_rightHand)
		m_rightHand->resetStatistics();
}

bool FormulaElement::getHandDataWithCheck(ItemDataPtr &dataPtr,
                                          FormulaElement *hand,
                                          const char *handName)
{
	if (!hand) {
		MLPL_DBG("%s hand: NULL.\n", handName);
		return false;
	}
	dataPtr = hand->evaluate(); 
	if (!dataPtr.hasData()) {
		MLPL_DBG("evaluate() of %s hand (%p: %s):  no data. \n",
		         handName, hand, Utils::DEMANGLED_TYPE_NAME(*hand));
		return false;
	}
	return true;
}

bool FormulaElement::getLeftHandDataWithCheck(ItemDataPtr &dataPtr)
{
	return getHandDataWithCheck(dataPtr, getLeftHand(), "Left");
}

bool FormulaElement::getRightHandDataWithCheck(ItemDataPtr &dataPtr)
{
	return getHandDataWithCheck(dataPtr, getRightHand(), "Right");
}

void FormulaElement::setTerminalElement(void)
{
	m_terminalElement = true;
}

int FormulaElement::getTreeInfo(string &str, int maxNumElem, int currNum,
                                int depth)
{
	string leftTypeName = "-";
	string rightTypeName = "-";

	if (m_leftHand)
		leftTypeName = DEMANGLED_TYPE_NAME(*m_leftHand);
	if (m_rightHand)
		rightTypeName = DEMANGLED_TYPE_NAME(*m_rightHand);

	string spaces;
	for (int i = 0; i < depth; i++)
		spaces += " ";

	string additionalInfo = getTreeInfoAdditional();
	str += StringUtils::sprintf
	         ("%02d %s[%p] %s, L:%p (%s), R:%p (%s), %s\n",
	          currNum, spaces.c_str(), this, DEMANGLED_TYPE_NAME(*this),
	          m_leftHand, leftTypeName.c_str(),
	          m_rightHand, rightTypeName.c_str(),
	          additionalInfo.c_str());
	currNum++;
	if (maxNumElem >= 0 && currNum >= maxNumElem)
		return currNum;

	if (m_leftHand) {
		currNum = m_leftHand->getTreeInfo(str, maxNumElem, currNum,
		                                  depth + 1);
	}
	if (maxNumElem >= 0 && currNum >= maxNumElem)
		return currNum;

	if (m_rightHand) {
		currNum = m_rightHand->getTreeInfo(str, maxNumElem, currNum,
		                                   depth + 1);
	}
	return currNum;
}

void FormulaElement::setOptimizationResult(FormulaOptimizationResult &result)
{
	m_optimizationResult = result;
}

FormulaOptimizationResult &FormulaElement::getOptimizationResult(void)
{
	return m_optimizationResult;
}

string FormulaElement::getTreeInfoAdditional(void)
{
	return "-";
}

// ---------------------------------------------------------------------------
// class: FormulaValue
// ---------------------------------------------------------------------------
FormulaValue::FormulaValue(bool val)
: FormulaElement(FORMULA_ELEM_PRIO_VALUE)
{
	setTerminalElement();
	m_itemDataPtr = ItemDataPtr(new ItemBool(val), false);
}

FormulaValue::FormulaValue(int number)
: FormulaElement(FORMULA_ELEM_PRIO_VALUE)
{
	setTerminalElement();
	m_itemDataPtr = ItemDataPtr(new ItemInt(ITEM_ID_NOBODY, number), false);
}

FormulaValue::FormulaValue(double number)
: FormulaElement(FORMULA_ELEM_PRIO_VALUE)
{
	setTerminalElement();
	m_itemDataPtr = ItemDataPtr(new ItemDouble(number), false);
}

FormulaValue::FormulaValue(const string &str)
: FormulaElement(FORMULA_ELEM_PRIO_VALUE)
{
	setTerminalElement();
	m_itemDataPtr = ItemDataPtr(new ItemString(ITEM_ID_NOBODY, str), false);
}

FormulaOptimizationResult FormulaValue::optimize(void)
{
	FormulaOptimizationResult result;
	result.type = FORMULA_ALWAYS_CONST;
	result.itemData = m_itemDataPtr;
	setOptimizationResult(result);
	return result;
}

ItemDataPtr FormulaValue::evaluate(void)
{
	return m_itemDataPtr;
}

string FormulaValue::getTreeInfoAdditional(void)
{
	const char *value = "N/A";
	if (m_itemDataPtr.hasData())
		value = m_itemDataPtr->getString().c_str();
	return StringUtils::sprintf("value: %s", value);
}

// ---------------------------------------------------------------------------
// class: FormulaVariable
// ---------------------------------------------------------------------------
FormulaVariable::FormulaVariable(const string &name,
                                 FormulaVariableDataGetter *variableDataGetter)
: FormulaElement(FORMULA_ELEM_PRIO_VARIABLE),
  m_name(name),
  m_variableGetter(variableDataGetter)
{
	setTerminalElement();
}

FormulaVariable::~FormulaVariable()
{
	delete m_variableGetter;
}

ItemDataPtr FormulaVariable::evaluate(void)
{
	return m_variableGetter->getData();
}

const string &FormulaVariable::getName(void) const
{
	return m_name;
}

FormulaVariableDataGetter *FormulaVariable::getFormulaVariableGetter(void) const
{
	return m_variableGetter;
}

string FormulaVariable::getTreeInfoAdditional(void)
{
	return StringUtils::sprintf("name: %s", getName().c_str());
}
