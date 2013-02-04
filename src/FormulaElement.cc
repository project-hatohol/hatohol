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

#include "Logger.h"
using namespace mlpl;

#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FormulaElement::FormulaElement(void)
: m_leftHand(NULL),
  m_rightHand(NULL),
  m_operator(NULL),
  m_parent(NULL)
{
}

FormulaElement::~FormulaElement()
{
	if (m_leftHand)
		delete m_leftHand;
	if (m_rightHand)
		delete m_rightHand;
	if (m_operator)
		delete m_operator;
}

void FormulaElement::setLeftHand(FormulaElement *elem)
{
	m_leftHand = elem;
	m_leftHand->m_parent = this;
}

void FormulaElement::setRightHand(FormulaElement *elem)
{
	m_rightHand = elem;
	m_rightHand->m_parent = this;
}

void FormulaElement::setOperator(FormulaOperator *op)
{
	m_operator = op;
}

FormulaElement *FormulaElement::getLeftHand(void) const
{
	return m_leftHand;
}

FormulaElement *FormulaElement::getRightHand(void) const
{
	return m_rightHand;
}

ItemDataPtr FormulaElement::evaluate(void)
{
	if (!m_operator) {
		MLPL_DBG("m_operator: NULL.\n");
		return ItemDataPtr();
	}

	return m_operator->evaluate(m_leftHand, m_rightHand);
}

// ---------------------------------------------------------------------------
// class: FormulaCompareEqual
// ---------------------------------------------------------------------------
ItemDataPtr FormulaCompareEqual::evaluate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// class: FormulaNumber
// ---------------------------------------------------------------------------
ItemDataPtr FormulaNumber::evaluate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// class: FormulaVariable
// ---------------------------------------------------------------------------
FormulaVariable::FormulaVariable(string &name,
                                 FormulaVariableDataGetter *variableDataGetter)
: m_name(name),
  m_variableGetter(variableDataGetter)
{
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
