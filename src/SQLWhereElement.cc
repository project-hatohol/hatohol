#include <cstdio>
#include <string>
#include <stdexcept>
#include <typeinfo>

#include "StringUtils.h"
using namespace mlpl;

#include "ItemEnum.h"
#include "Utils.h"
#include "SQLWhereElement.h"


// ---------------------------------------------------------------------------
// methods (SQLWhereOperator)
// ---------------------------------------------------------------------------
SQLWhereOperator::SQLWhereOperator(SQLWhereOperatorType type,
                                   SQLWhereOperatorPriority prio)
: m_type(type),
  m_priority(prio)
{
}

SQLWhereOperator::~SQLWhereOperator()
{
}

bool SQLWhereOperator::priorityOver(SQLWhereOperator *whereOp)
{
	return m_priority > whereOp->m_priority;
}

//
// Protected methods
//
const SQLWhereOperatorType SQLWhereOperator::getType(void) const
{
	return m_type;
}

bool SQLWhereOperator::checkType(SQLWhereElement *elem,
                                 SQLWhereElementType type) const
{
	if (!elem) {
		MLPL_DBG("elem is NULL\n");
		return false;
	}
	if (elem->getType() != type) {
		MLPL_DBG("type(%d) is not expected(%d)\n",
		         elem->getType(), type);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorEqual
// ---------------------------------------------------------------------------
SQLWhereOperatorEqual::SQLWhereOperatorEqual()
: SQLWhereOperator(SQL_WHERE_OP_EQ, SQL_WHERE_OP_PRIO_EQ)
{
}

SQLWhereOperatorEqual::~SQLWhereOperatorEqual()
{
}

bool SQLWhereOperatorEqual::evaluate(SQLWhereElement *leftHand,
                                     SQLWhereElement *rightHand)
{
	if (!checkType(leftHand, SQL_WHERE_ELEM_COLUMN))
		return false;
	if (!rightHand)
		return false;
	ItemDataPtr data0 = leftHand->getItemData();
	ItemDataPtr data1 = rightHand->getItemData();
	return (*data0 == *data1);
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorAnd
// ---------------------------------------------------------------------------
SQLWhereOperatorAnd::SQLWhereOperatorAnd()
: SQLWhereOperator(SQL_WHERE_OP_EQ, SQL_WHERE_OP_PRIO_AND)
{
}

SQLWhereOperatorAnd::~SQLWhereOperatorAnd()
{
}

bool SQLWhereOperatorAnd::evaluate(SQLWhereElement *leftHand,
                                   SQLWhereElement *rightHand)
{
	bool ret = leftHand->evaluate() && rightHand->evaluate();
	return ret;
}

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorBetween
// ---------------------------------------------------------------------------
SQLWhereOperatorBetween::SQLWhereOperatorBetween()
: SQLWhereOperator(SQL_WHERE_OP_BETWEEN, SQL_WHERE_OP_PRIO_BETWEEN)
{
}

SQLWhereOperatorBetween::~SQLWhereOperatorBetween()
{
}

bool SQLWhereOperatorBetween::evaluate(SQLWhereElement *leftHand,
                                       SQLWhereElement *rightHand)
{
	if (!checkType(leftHand, SQL_WHERE_ELEM_COLUMN))
		return false;
	if (!checkType(rightHand, SQL_WHERE_ELEM_PAIRED_NUMBER))
		return false;
	SQLWherePairedNumber *pairedElem =
	  dynamic_cast<SQLWherePairedNumber *>(rightHand);
	ItemDataPtr dataColumn = leftHand->getItemData();
	ItemDataPtr dataRight0 = pairedElem->getItemData(0);
	ItemDataPtr dataRight1 = pairedElem->getItemData(1);
	return (*dataColumn >= *dataRight0 && *dataColumn <= *dataRight1);
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
	whereElem->m_parent = this;
}

void SQLWhereElement::setRightHand(SQLWhereElement *whereElem)
{
	m_rightHand = whereElem;
	whereElem->m_parent = this;
}

void SQLWhereElement::setOperator(SQLWhereOperator *whereOp)
{
	m_operator = whereOp;
}

/*
void SQLWhereElement::setParent(SQLWhereElement *whereElem)
{
	m_parent = whereElem;
}*/

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

bool SQLWhereElement::isEmpty(void)
{
	if (m_leftHand)
		return false;
	if (m_operator)
		return false;
	if (m_rightHand)
		return false;
	return true;
}

bool SQLWhereElement::evaluate(void)
{
	if (!m_operator) {
		string msg;
		TRMSG(msg, "m_operator is NULL.");
		throw logic_error(msg);
	}
	bool ret = m_operator->evaluate(m_leftHand, m_rightHand);
	return ret;
}

ItemDataPtr SQLWhereElement::getItemData(int index)
{
	string className = typeid(*this).name();
	MLPL_WARN("This function is typically "
	          "overrided in the sub class: %s (%s) [%p]\n",
	          Utils::demangle(className).c_str(), className.c_str(), this);
	return ItemDataPtr();
}

SQLWhereElement *SQLWhereElement::findInsertPoint(SQLWhereElement *insertElem)
{
	string str;
	SQLWhereOperator *insertElemOp = insertElem->m_operator;
	if (!insertElemOp) {
		TRMSG(str, "Operator of insertElem is NULL.");
		throw logic_error(str);
	}

	SQLWhereElement *whereElem = this;
	for (; whereElem; whereElem = whereElem->m_parent) {
		if (whereElem->getType() != SQL_WHERE_ELEM_ELEMENT)
			continue;
		SQLWhereOperator *whereOp = whereElem->m_operator;
		if (!whereOp) {
			TRMSG(str, "Operator is NULL.");
			throw logic_error(str);
		}
		if (!whereOp->priorityOver(insertElemOp))
			break;
	}
	return whereElem;
}

// ---------------------------------------------------------------------------
// class: SQLWhereColumn
// ---------------------------------------------------------------------------
SQLWhereColumn::SQLWhereColumn
  (string &columnName, SQLWhereColumnDataGetter dataGetter, void *priv,
   SQLWhereColumnPrivDataDestructor privDataDestructor)
: SQLWhereElement(SQL_WHERE_ELEM_COLUMN),
  m_columnName(columnName),
  m_valueGetter(dataGetter),
  m_priv(priv),
  m_privDataDestructor(privDataDestructor)
{
}

SQLWhereColumn::~SQLWhereColumn()
{
	if (m_privDataDestructor)
		(*m_privDataDestructor)(this, m_priv);
}

const string &SQLWhereColumn::getColumnName(void) const
{
	return m_columnName;
}

void *SQLWhereColumn::getPrivateData(void) const
{
	return m_priv;
}

ItemDataPtr SQLWhereColumn::getItemData(int index)
{
	return (*m_valueGetter)(this, m_priv);
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

ItemDataPtr SQLWhereString::getItemData(int index)
{
	return ItemDataPtr(new ItemString(ITEM_ID_NOBODY, m_str), false);
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

ItemDataPtr SQLWherePairedNumber::getItemData(int index)
{
	string msg;
	PolytypeNumber *value = NULL;
	if (index == 0)
		value = &m_value0;
	else if (index == 1)
		value = &m_value1;
	else {
		TRMSG(msg, "Invalid index: %d\n", index);
		throw logic_error(msg);
	}

	PolytypeNumber::NumberType type = value->getType();
	ItemData *item = NULL;
	if (type == PolytypeNumber::TYPE_INT64) {
		item = new ItemInt(ITEM_ID_NOBODY, value->getAsInt64());
	} else {
		TRMSG(msg, "Not supported: type %d\n", type);
		throw logic_error(msg);
	}
	return ItemDataPtr(item, false);
}
