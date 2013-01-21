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

const string &SQLWhereColumn::getValue(void)
{
	return m_columnName;
}

// ---------------------------------------------------------------------------
// class: SQLWhereNumber
// ---------------------------------------------------------------------------
SQLWhereNumber::SQLWhereNumber(double value)
: SQLWhereElement(SQL_WHERE_ELEM_NUMBER),
  m_value(value)
{
}

SQLWhereNumber::~SQLWhereNumber()
{
}

double SQLWhereNumber::getValue(void)
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

const string &SQLWhereString::getValue(void)
{
	return m_str;
}

