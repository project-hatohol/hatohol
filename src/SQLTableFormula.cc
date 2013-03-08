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
	TableSizeInfoVectorIterator it = m_tableSizeInfoVector.begin();
	for (; it != m_tableSizeInfoVector.end(); ++it)
		delete *it;
}

size_t SQLTableFormula::getColumnIndexOffset(const string &tableName)
{
	if (m_tableSizeInfoVector.empty())
		fixupTableSizeInfo();

	TableSizeInfoMapIterator it = m_tableVarSizeInfoMap.find(tableName);
	bool found = false;
	if (it != m_tableVarSizeInfoMap.end())
		found = true;
	if (!found) {
		it = m_tableSizeInfoMap.find(tableName);
		if (it != m_tableSizeInfoMap.end())
			found = true;
	}
	if (!found) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found table: %s", tableName.c_str());
	}
	TableSizeInfo *tableSizeInfo = it->second;
	return tableSizeInfo->accumulatedColumnOffset;
}

const SQLTableFormula::TableSizeInfoVector &
  SQLTableFormula::getTableSizeInfoVector(void)
{
	if (m_tableSizeInfoVector.empty())
		fixupTableSizeInfo();
	return m_tableSizeInfoVector;
}

void SQLTableFormula::addTableSizeInfo(const string &tableName,
                                       const string &tableVar,
                                       size_t numColumns)
{
	size_t accumulatedColumnOffset = 0;
	if (!m_tableSizeInfoVector.empty()) {
		TableSizeInfo *prev = m_tableSizeInfoVector.back();
		accumulatedColumnOffset =
		  prev->accumulatedColumnOffset + prev->numColumns;
	}

	TableSizeInfo *tableSizeInfo = new TableSizeInfo();
	tableSizeInfo->name = tableName;
	tableSizeInfo->varName = tableVar;
	tableSizeInfo->numColumns = numColumns;
	tableSizeInfo->accumulatedColumnOffset = accumulatedColumnOffset;

	m_tableSizeInfoVector.push_back(tableSizeInfo);
	m_tableSizeInfoMap[tableName] = tableSizeInfo;
	if (!tableVar.empty())
		m_tableVarSizeInfoMap[tableVar] = tableSizeInfo;
}

void SQLTableFormula::makeTableSizeInfo(const TableSizeInfoVector &leftList,
                                        const TableSizeInfoVector &rightList)
{
	if (!m_tableSizeInfoVector.empty()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "m_tableSizeInfoVector: Not empty.");
	}
	TableSizeInfoVectorConstIterator it = leftList.begin();
	for (; it != leftList.end(); ++it) {
		const TableSizeInfo *tableSizeInfo = *it;
		addTableSizeInfo(tableSizeInfo->name,
		                 tableSizeInfo->varName,
		                 tableSizeInfo->numColumns);
	}
	for (it = rightList.begin(); it != rightList.end(); ++it) {
		const TableSizeInfo *tableSizeInfo = *it;
		addTableSizeInfo(tableSizeInfo->name,
		                 tableSizeInfo->varName,
		                 tableSizeInfo->numColumns);
	}
}

// ---------------------------------------------------------------------------
// SQLTableElement
// ---------------------------------------------------------------------------
SQLTableElement::SQLTableElement(const string &name, const string &varName,
                                 SQLColumnIndexResoveler *resolver)
: m_name(name),
  m_varName(varName),
  m_columnIndexResolver(resolver)
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

ItemGroupPtr SQLTableElement::getActiveRow(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemGroupPtr();
}

void SQLTableElement::startRowIterator(void)
{
	m_currSelectedGroup = m_itemTablePtr->getItemGroupList().begin();
}

bool SQLTableElement::rowIteratorEnd(void)
{
	return m_currSelectedGroup == m_itemTablePtr->getItemGroupList().end();
}

void SQLTableElement::rowIteratorInc(void)
{
	++m_currSelectedGroup;
}

void SQLTableElement::fixupTableSizeInfo(void)
{
	string &name = m_varName.empty() ? m_name : m_varName;
	size_t numColumns = m_columnIndexResolver->getNumberOfColumns(name);
	addTableSizeInfo(m_name, m_varName, numColumns);
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

void SQLTableJoin::fixupTableSizeInfo(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}
	makeTableSizeInfo(leftFormula->getTableSizeInfoVector(),
	                  rightFormula->getTableSizeInfoVector());
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

ItemGroupPtr SQLTableCrossJoin::getActiveRow(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemGroupPtr();
}

// ---------------------------------------------------------------------------
// SQLTableInnerJoin
// ---------------------------------------------------------------------------
SQLTableInnerJoin::SQLTableInnerJoin
  (const string &leftTableName, const string &leftColumnName,
   const string &rightTableName, const string &rightColumnName,
   SQLColumnIndexResoveler *resolver)
: SQLTableJoin(SQL_JOIN_TYPE_INNER),
  m_leftTableName(leftTableName),
  m_leftColumnName(leftColumnName),
  m_rightTableName(rightTableName),
  m_rightColumnName(rightColumnName),
  m_indexLeftJoinColumn(INDEX_NOT_SET),
  m_indexRightJoinColumn(INDEX_NOT_SET),
  m_columnIndexResolver(resolver)
{
}

ItemTablePtr SQLTableInnerJoin::getTable(void)
{
	if (!m_columnIndexResolver)
		THROW_ASURA_EXCEPTION("m_columnIndexResolver: NULL");

	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}

	if (m_indexLeftJoinColumn == INDEX_NOT_SET) {
		m_indexLeftJoinColumn =
		  m_columnIndexResolver->getIndex(m_leftTableName,
		                                  m_leftColumnName);
		m_indexLeftJoinColumn +=
		   leftFormula->getColumnIndexOffset(m_leftTableName);
	}
	if (m_indexRightJoinColumn == INDEX_NOT_SET) {
		m_indexRightJoinColumn =
		  m_columnIndexResolver->getIndex(m_rightTableName,
		                                  m_rightColumnName);
		m_indexRightJoinColumn +=
		  rightFormula->getColumnIndexOffset(m_rightTableName);
	}

	return innerJoin(leftFormula->getTable(), rightFormula->getTable(),
	                 m_indexLeftJoinColumn, m_indexRightJoinColumn);
}

ItemGroupPtr SQLTableInnerJoin::getActiveRow(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemGroupPtr();
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
