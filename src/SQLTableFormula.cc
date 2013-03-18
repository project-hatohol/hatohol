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
// SQLTableProcessContext
// ---------------------------------------------------------------------------
SQLTableProcessContext::SQLTableProcessContext(void)
: id(-1),
  tableElement(NULL),
  equalBoundTableCtx(NULL),
  equalBoundColumnIndex(-1),
  equalBoundMyIndex(-1),
  itemDataIndex(NULL)
{
}

void SQLTableProcessContext::clearIndexingVariables(void)
{
	indexMatchedItems.clear();
	indexMatchedItemsIndex = 0;
}

// ---------------------------------------------------------------------------
// SQLTableProcessContextIndex
// ---------------------------------------------------------------------------
SQLTableProcessContextIndex::~SQLTableProcessContextIndex()
{
	clear();
}

void SQLTableProcessContextIndex::clear(void)
{
	for (size_t i = 0; i < tableCtxVector.size(); i++)
		delete tableCtxVector[i];
	tableCtxVector.clear();
	tableNameCtxMap.clear();
	tableCtxVector.clear();
}

SQLTableProcessContext *SQLTableProcessContextIndex::getTableContext(const string &name)
{
	map<string, SQLTableProcessContext *>::iterator it =
	  tableVarCtxMap.find(name);
	if (it != tableVarCtxMap.end())
		return it->second;
	it = tableNameCtxMap.find(name);
	if (it != tableNameCtxMap.end())
		return it->second;
	return NULL;
}

// ---------------------------------------------------------------------------
// SQLTableFormula
// ---------------------------------------------------------------------------
SQLTableFormula::~SQLTableFormula()
{
	TableSizeInfoVectorIterator it = m_tableSizeInfoVector.begin();
	for (; it != m_tableSizeInfoVector.end(); ++it)
		delete *it;
}

void SQLTableFormula::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
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
  m_columnIndexResolver(resolver),
  m_tableProcessCtx(NULL)
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

// This function is called from SQLProcessorSelect::makeItemTables()
void SQLTableElement::setItemTable(ItemTablePtr itemTablePtr)
{
	m_itemTablePtr = itemTablePtr;
}

void SQLTableElement::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
	if (m_tableProcessCtx->equalBoundMyIndex < 0)
		return;

	// This function is called from
	//   SQLProcessorSelect::doJoinWithFromParser
	//     -> SQLFromParser::doJoin()
	// So m_itemTablePtr must have a valid value here.
	if (!m_itemTablePtr->hasIndex())
		return;
	size_t columnIndex = m_tableProcessCtx->equalBoundMyIndex;
	const ItemDataIndexVector &indexVector =
	  m_itemTablePtr->getIndexVector();
	if (indexVector.size() < columnIndex) {
		THROW_ASURA_EXCEPTION(
		  "indexVector.size (%zd) < equalBoundMyIndex (%zd)",
		  indexVector.size(), columnIndex);
	}
	ItemDataIndex *itemIndex = indexVector[columnIndex];
	if (itemIndex->getIndexType() == ITEM_DATA_INDEX_TYPE_NONE)
		return;
	m_tableProcessCtx->itemDataIndex = itemIndex;
}

ItemTablePtr SQLTableElement::getTable(void)
{
	return m_itemTablePtr;
}

ItemGroupPtr SQLTableElement::getActiveRow(void)
{
	// Non-indexing mode
	if (!isIndexingMode())
		return *m_currSelectedGroup;

	// Indexing mode
	size_t index = m_tableProcessCtx->indexMatchedItemsIndex;
	ItemDataPtrForIndex &dataForIndex =
	  m_tableProcessCtx->indexMatchedItems[index];
	return dataForIndex.itemGroupPtr;
}

void SQLTableElement::startRowIterator(void)
{
	m_currSelectedGroup = m_itemTablePtr->getItemGroupList().begin();

	// Non-indexing mode
	if (!isIndexingMode())
		return;

	// Indexing mode
	SQLTableElement *leftTable =
	  m_tableProcessCtx->equalBoundTableCtx->tableElement;
	ItemGroupPtr leftRow = leftTable->getActiveRow();
	ItemDataPtr leftItem =
	  leftRow->getItemAt(m_tableProcessCtx->equalBoundColumnIndex);
	m_tableProcessCtx->clearIndexingVariables();
	m_tableProcessCtx->itemDataIndex
	  ->find(leftItem, m_tableProcessCtx->indexMatchedItems);
	if (m_tableProcessCtx->indexMatchedItems.empty()) {
		// not found
		m_currSelectedGroup = m_itemTablePtr->getItemGroupList().end();
	}
}

bool SQLTableElement::rowIteratorEnd(void)
{
	return m_currSelectedGroup == m_itemTablePtr->getItemGroupList().end();
}

void SQLTableElement::rowIteratorInc(void)
{
	// Non-indexing mode
	if (!isIndexingMode()) {
		++m_currSelectedGroup;
		return;
	}

	// Indexing mode
	size_t numMatchedItems = m_tableProcessCtx->indexMatchedItems.size();
	m_tableProcessCtx->indexMatchedItemsIndex++;
	if (m_tableProcessCtx->indexMatchedItemsIndex >= numMatchedItems)
		m_currSelectedGroup = m_itemTablePtr->getItemGroupList().end();
}

void SQLTableElement::fixupTableSizeInfo(void)
{
	string &name = m_varName.empty() ? m_name : m_varName;
	size_t numColumns = m_columnIndexResolver->getNumberOfColumns(name);
	addTableSizeInfo(m_name, m_varName, numColumns);
}

void SQLTableElement::setSQLTableProcessContext(SQLTableProcessContext *joinedTableCtx)
{
	m_tableProcessCtx = joinedTableCtx;
}

bool SQLTableElement::isIndexingMode(void)
{
	return m_tableProcessCtx->itemDataIndex;
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

void SQLTableJoin::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
	m_leftFormula->prepareJoin(ctxIndex);
	m_rightFormula->prepareJoin(ctxIndex);
}

void SQLTableJoin::setLeftFormula(SQLTableFormula *tableFormula)
{
	m_leftFormula = tableFormula;
}

void SQLTableJoin::setRightFormula(SQLTableFormula *tableFormula)
{
	m_rightFormula = tableFormula;
}

ItemGroupPtr SQLTableJoin::getActiveRow(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}

	ItemGroupPtr itemGroup = ItemGroupPtr(new ItemGroup(), false);

	ItemGroupPtr leftRow = leftFormula->getActiveRow();
	for (size_t i = 0; i < leftRow->getNumberOfItems(); i++)
		itemGroup->add(leftRow->getItemAt(i));
	ItemGroupPtr rightRow = rightFormula->getActiveRow();
	for (size_t i = 0; i < rightRow->getNumberOfItems(); i++)
		itemGroup->add(rightRow->getItemAt(i));

	return itemGroup;
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
  m_columnIndexResolver(resolver),
  m_rightTableElement(NULL)
{
}

void SQLTableInnerJoin::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
	// Current implementation of inner join overwrites equalBoundTableCtx,
	// equalBoundColumnIndex, and equalBoundMyIndex.
	// However, the combination of them may improve performance.
	SQLTableProcessContext *leftTableCtx =
	  ctxIndex->getTableContext(m_leftTableName);
	SQLTableProcessContext *rightTableCtx =
	  ctxIndex->getTableContext(m_rightTableName);
	if (!leftTableCtx || !rightTableCtx) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftTableCtx (%s:%p) or lightTableCtx (%s:%p) is NULL.\n",
		  m_leftTableName.c_str(), leftTableCtx,
		  m_rightTableName.c_str(), rightTableCtx);
	}

	if (rightTableCtx->equalBoundTableCtx) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "rightTableCtx->equalBoundTableCtx: Not null. "
		  "rightTable: %s, innerJoinLeftTable: %s",
		  m_rightTableName.c_str(),
		  rightTableCtx->equalBoundTableCtx
		    ->tableElement->getName().c_str());
	}
	rightTableCtx->equalBoundTableCtx = leftTableCtx;
	rightTableCtx->equalBoundColumnIndex =
	  m_columnIndexResolver->getIndex(m_leftTableName,
	                                  m_leftColumnName);
	rightTableCtx->equalBoundMyIndex =
	  m_columnIndexResolver->getIndex(m_rightTableName,
	                                  m_rightColumnName);
	m_rightTableElement = rightTableCtx->tableElement;
	SQLTableJoin::prepareJoin(ctxIndex);

	//
	// The following lines are for getActiveRows()
	//

	// The following code is copied from getTable(). We want to
	// extract same part as a function.
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
	// Right table is in an indexing mode, we can skip the condition check.
	if (m_rightTableElement->isIndexingMode())
		return  SQLTableJoin::getActiveRow();

	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();

	ItemGroupPtr leftGrpPtr = leftFormula->getActiveRow();
	if (!leftGrpPtr.hasData())
		return leftGrpPtr;
	ItemData *leftData = leftGrpPtr->getItemAt(m_indexLeftJoinColumn);
	ItemGroupPtr rightGrpPtr = rightFormula->getActiveRow();
	ItemData *rightData = rightGrpPtr->getItemAt(m_indexRightJoinColumn);
	if (!rightGrpPtr.hasData())
		return rightGrpPtr;

	if (*leftData != *rightData)
		return ItemGroupPtr(NULL);

	// TODO: The following function also gets active rows of the children.
	//       So it should be fixed more efficiently.
	return  SQLTableJoin::getActiveRow();
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
