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

#include <stdexcept>

#include "Logger.h"
using namespace mlpl;

#include "FormulaElement.h"
#include "ItemEnum.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FormulaElement::FormulaElement(FormulaElementPriority priority, bool unary,
                               bool priorityBarrier)
: m_unary(unary),
  m_leftHand(NULL),
  m_rightHand(NULL),
  m_parent(NULL),
  m_priority(priority),
  m_priorityBarrier(priorityBarrier),
  m_terminalElement(false)
{
}

FormulaElement::~FormulaElement()
{
	if (m_leftHand)
		delete m_leftHand;
	if (m_rightHand)
		delete m_rightHand;
}

void FormulaElement::setLeftHand(FormulaElement *elem)
{
	m_leftHand = elem;
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

FormulaElement *FormulaElement::findInsertPoint(FormulaElement *insertElem)
{
	FormulaElement *prev = NULL;
	FormulaElement *elem = this;
	while (elem) {
		if (elem->m_priorityBarrier)
			break;
		if (insertElem->priorityOver(elem))
			break;
		prev = elem;
		elem = elem->m_parent;
	}
	return prev;
}

bool FormulaElement::getLeftHandDataWithCheck(ItemDataPtr &dataPtr)
{
	FormulaElement *leftHand = getLeftHand();
	if (!leftHand) {
		MLPL_DBG("Left Hand: NULL.\n");
		return false;
	}
	dataPtr = leftHand->evaluate(); 
	if (!dataPtr.hasData())
		return false;
	return true;
}

bool FormulaElement::getRightHandDataWithCheck(ItemDataPtr &dataPtr)
{
	FormulaElement *rightHand = getRightHand();
	if (!rightHand) {
		MLPL_DBG("Right Hand: NULL.\n");
		return false;
	}
	dataPtr = rightHand->evaluate(); 
	if (!dataPtr.hasData())
		return false;
	return true;
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
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	setTerminalElement();
}

FormulaValue::FormulaValue(string &str)
: FormulaElement(FORMULA_ELEM_PRIO_VALUE)
{
	setTerminalElement();
	m_itemDataPtr = ItemDataPtr(new ItemString(ITEM_ID_NOBODY, str), false);
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
FormulaVariable::FormulaVariable(string &name,
                                 FormulaVariableDataGetter *variableDataGetter)
: FormulaElement(FORMULA_ELEM_PRIO_VARIABLE),
  m_name(name),
  m_variableGetter(variableDataGetter)
{
	setTerminalElement();
}

FormulaVariable::~FormulaVariable()
{
	if (m_variableGetter)
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
