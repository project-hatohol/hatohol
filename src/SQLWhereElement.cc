#include <cstdio>
#include <string>
#include <stdexcept>

#include "Utils.h"
#include "SQLWhereElement.h"

#include "StringUtils.h"
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods (SQLWhereOperator)
// ---------------------------------------------------------------------------
SQLWhereOperator::SQLWhereOperator(SQLWhereOperatorType type)
: m_type(type)
{
}

SQLWhereOperator::~SQLWhereOperator()
{
}

const SQLWhereOperatorType SQLWhereOperator::getType(void) const
{
	return m_type;
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorEqual
// ---------------------------------------------------------------------------
SQLWhereOperatorEqual::SQLWhereOperatorEqual()
: SQLWhereOperator(SQL_WHERE_OP_EQ)
{
}

SQLWhereOperatorEqual::~SQLWhereOperatorEqual()
{
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorAnd
// ---------------------------------------------------------------------------
SQLWhereOperatorAnd::SQLWhereOperatorAnd()
: SQLWhereOperator(SQL_WHERE_OP_EQ)
{
}

SQLWhereOperatorAnd::~SQLWhereOperatorAnd()
{
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorBetween
// ---------------------------------------------------------------------------
SQLWhereOperatorBetween::SQLWhereOperatorBetween()
: SQLWhereOperator(SQL_WHERE_OP_BETWEEN)
{
}

SQLWhereOperatorBetween::~SQLWhereOperatorBetween()
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLWhereElement)
// ---------------------------------------------------------------------------
SQLWhereElement::SQLWhereElement(SQLWhereElementType elemType)
: m_type(elemType),
  m_leftHand(NULL),
  m_operator(NULL),
  m_rightHand(NULL),
  m_parent(NULL)
{
}

SQLWhereElement::~SQLWhereElement()
{
	if (m_leftHand)
		delete m_leftHand;
	if (m_operator)
		delete m_operator;
	if (m_rightHand)
		delete m_rightHand;
}

SQLWhereElementType SQLWhereElement::getType(void) const
{
	return m_type;
}

SQLWhereElement *SQLWhereElement::getLeftHand(void) const
{
	return m_leftHand;
}

SQLWhereElement *SQLWhereElement::getRightHand(void) const
{
	return m_rightHand;
}

SQLWhereOperator *SQLWhereElement::getOperator(void) const
{
	return m_operator;
}

SQLWhereElement *SQLWhereElement::getParent(void) const
{
	return m_parent;
}

void SQLWhereElement::setLeftHand(SQLWhereElement *whereElem)
{
	m_leftHand = whereElem;
}

void SQLWhereElement::setRightHand(SQLWhereElement *whereElem)
{
	m_rightHand = whereElem;
}

void SQLWhereElement::setOperator(SQLWhereOperator *whereOp)
{
	m_operator = whereOp;
}

void SQLWhereElement::setParent(SQLWhereElement *whereElem)
{
	m_parent = whereElem;
}

bool SQLWhereElement::isFull(void)
{
	if (!m_leftHand)
		return false;
	if (!m_operator)
		return false;
	if (!m_rightHand)
		return false;
	return true;
}

// ---------------------------------------------------------------------------
// class: SQLWhereColumn
// ---------------------------------------------------------------------------
SQLWhereColumn::SQLWhereColumn(string &columnName)
: SQLWhereElement(SQL_WHERE_ELEM_COLUMN),
  m_columnName(columnName)
{
}

SQLWhereColumn::~SQLWhereColumn()
{
}

const string &SQLWhereColumn::getValue(void) const
{
	return m_columnName;
}

// ---------------------------------------------------------------------------
// class: SQLWhereNumber
// ---------------------------------------------------------------------------
SQLWhereNumber::SQLWhereNumber(const PolytypeNumber &value)
: SQLWhereElement(SQL_WHERE_ELEM_NUMBER),
  m_value(value)
{
}

SQLWhereNumber::SQLWhereNumber(int value)
: SQLWhereElement(SQL_WHERE_ELEM_NUMBER),
  m_value(value)
{
}

SQLWhereNumber::SQLWhereNumber(double value)
: SQLWhereElement(SQL_WHERE_ELEM_NUMBER),
  m_value(value)
{
}

SQLWhereNumber::~SQLWhereNumber()
{
}

const PolytypeNumber &SQLWhereNumber::getValue(void) const
{
	return m_value;
}

// ---------------------------------------------------------------------------
// class: SQLWhereString
// ---------------------------------------------------------------------------
SQLWhereString::SQLWhereString(string &str)
: SQLWhereElement(SQL_WHERE_ELEM_STRING),
  m_str(str)
{
}

SQLWhereString::~SQLWhereString()
{
}

const string &SQLWhereString::getValue(void) const
{
	return m_str;
}

// ---------------------------------------------------------------------------
// class: SQLWherePairedNumber
// ---------------------------------------------------------------------------
SQLWherePairedNumber::SQLWherePairedNumber(const PolytypeNumber &v0,
                                           const PolytypeNumber &v1)
: SQLWhereElement(SQL_WHERE_ELEM_PAIRED_NUMBER),
  m_value0(v0),
  m_value1(v1)
{
}

SQLWherePairedNumber::~SQLWherePairedNumber()
{
}

const PolytypeNumber &SQLWherePairedNumber::getFirstValue(void) const
{
	return m_value0;
}

const PolytypeNumber &SQLWherePairedNumber::getSecondValue(void) const
{
	return m_value1;
}
