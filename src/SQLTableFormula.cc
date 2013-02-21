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
  m_rightColumnName(rightColumnName)
{
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
