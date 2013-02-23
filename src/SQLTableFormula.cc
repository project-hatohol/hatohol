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

#include "SQLTableFormula.h"
#include "SQLProcessorException.h"
#include "AsuraException.h"

// ---------------------------------------------------------------------------
// SQLTableFormula
// ---------------------------------------------------------------------------
SQLTableFormula::~SQLTableFormula()
{
}

// ---------------------------------------------------------------------------
// SQLTableElement
// ---------------------------------------------------------------------------
SQLTableElement::SQLTableElement(const string &name, const string &varName)
: m_name(name),
  m_varName(varName)
{
}

const string &SQLTableElement::getName(void) const
{
	return m_name;
}

const string &SQLTableElement::getVarName(void) const
{
	return m_varName;
}

void SQLTableElement::setItemTable(ItemTablePtr itemTablePtr)
{
	m_itemTablePtr = itemTablePtr;
}

ItemTablePtr SQLTableElement::getTable(void)
{
	return m_itemTablePtr;
}

// ---------------------------------------------------------------------------
// SQLTableJoin
// ---------------------------------------------------------------------------
SQLTableJoin::SQLTableJoin(SQLJoinType type)
: m_type(type),
  m_leftFormula(NULL),
  m_rightFormula(NULL)
{
}

SQLTableFormula *SQLTableJoin::getLeftFormula(void) const
{
	return m_leftFormula;
}

SQLTableFormula *SQLTableJoin::getRightFormula(void) const
{
	return m_rightFormula;
}

void SQLTableJoin::setLeftFormula(SQLTableFormula *tableFormula)
{
	m_leftFormula = tableFormula;;
}

void SQLTableJoin::setRightFormula(SQLTableFormula *tableFormula)
{
	m_rightFormula = tableFormula;;
}

// ---------------------------------------------------------------------------
// SQLTableCrossJoin
// ---------------------------------------------------------------------------
SQLTableCrossJoin::SQLTableCrossJoin(void)
: SQLTableJoin(SQL_JOIN_TYPE_CROSS)
{
}

ItemTablePtr SQLTableCrossJoin::getTable(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}
	return crossJoin(leftFormula->getTable(), rightFormula->getTable());
}

// ---------------------------------------------------------------------------
// SQLTableInnerJoin
// ---------------------------------------------------------------------------
SQLTableInnerJoin::SQLTableInnerJoin
  (const string &leftTableName, const string &leftColumnName,
   const string &rightTableName, const string &rightColumnName)
: SQLTableJoin(SQL_JOIN_TYPE_INNER),
  m_leftTableName(leftTableName),
  m_leftColumnName(leftColumnName),
  m_rightTableName(rightTableName),
  m_rightColumnName(rightColumnName),
  m_indexLeftJoinColumn(INDEX_NOT_SET),
  m_indexRightJoinColumn(INDEX_NOT_SET)
{
}

ItemTablePtr SQLTableInnerJoin::getTable(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}
	if (m_indexLeftJoinColumn == INDEX_NOT_SET ||
	    m_indexRightJoinColumn == INDEX_NOT_SET) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "m_indexLeftJoinColumn or/and m_indexRightJoinColumn is "
		  "not set (%zd, %zd)",
		  m_indexLeftJoinColumn, m_indexRightJoinColumn);
	}
	return innerJoin(leftFormula->getTable(), rightFormula->getTable(),
	                 m_indexLeftJoinColumn, m_indexRightJoinColumn);
}

const string &SQLTableInnerJoin::getLeftTableName(void) const
{
	return m_leftTableName;
}

const string &SQLTableInnerJoin::getLeftColumnName(void) const
{
	return m_leftColumnName;
}

const string &SQLTableInnerJoin::getRightTableName(void) const
{
	return m_rightTableName;
}

const string &SQLTableInnerJoin::getRightColumnName(void) const
{
	return m_rightColumnName;
}
