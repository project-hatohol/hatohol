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

#ifndef SQLTableFormula_h
#define SQLTableFormula_h

#include <vector>
#include <map>
#include <string>
#include <ParsableString.h>
#include <StringUtils.h>
#include "SQLProcessorTypes.h"
#include "ItemTablePtr.h"
#include "ItemGroupPtr.h"

// ---------------------------------------------------------------------------
// SQLRowIterator
// ---------------------------------------------------------------------------
struct SQLTableProcessContext;

class SQLTableRowIterator {
public:
	virtual ~SQLTableRowIterator();
	virtual bool isIndexed(const ItemDataIndexVector &indexVector) = 0;
	virtual int getNumberOfRows(void) = 0;
	virtual bool start(void) = 0;
	virtual ItemGroupPtr getRow(void) = 0;
	virtual bool increment(void) = 0;
};

class SQLTableRowIteratorColumnsEqual : public SQLTableRowIterator {

public:
	SQLTableRowIteratorColumnsEqual(SQLTableProcessContext *otherTableCtx,
	                                int otherIndex, int myIndex);
	virtual bool isIndexed(const ItemDataIndexVector &indexVector);
	virtual int getNumberOfRows(void);
	virtual bool start(void);
	ItemGroupPtr getRow(void);
	virtual bool increment(void);

protected:
	void clearIndexingVariables(void);

private:
	SQLTableProcessContext *m_otherTableCtx;
	size_t                  m_otherIndex;    // in the other table
	size_t                  m_myIndex;       // in this table

	// The body of 'itemDataIndex' in ItemTable instance
	ItemDataIndex          *m_itemDataIndex;

	std::vector<ItemDataPtrForIndex> m_indexMatchedItems;
	size_t                           m_indexMatchedItemsIndex;

};

class SQLTableRowIteratorConstants : public SQLTableRowIterator {
public:
	SQLTableRowIteratorConstants(size_t columnIndex,
	                             const ItemGroupPtr &values);
	virtual bool isIndexed(const ItemDataIndexVector &indexVector);
	virtual int getNumberOfRows(void);
	virtual bool start(void);
	ItemGroupPtr getRow(void);
	virtual bool increment(void);

private:
	size_t             m_columnIndex;
	const ItemGroupPtr m_values;
};

// ---------------------------------------------------------------------------
// SQLTableProcessContext
// ---------------------------------------------------------------------------
class SQLTableElement;
struct SQLTableProcessContext {
	// assigned from 0 in order of appearance in a statement
	size_t                  id;

	// SQLTableElement instance corresponding to this instance
	SQLTableElement        *tableElement;

	// SQLTableRowIterator instances and a pointer to the selected one
	std::vector<SQLTableRowIterator *> rowIteratorVector;

	// methods
	SQLTableProcessContext(void);
	virtual ~SQLTableProcessContext();
};

// ---------------------------------------------------------------------------
// SQLTableProcessContextIndex
// ---------------------------------------------------------------------------
struct SQLTableProcessContextIndex {
	std::map<std::string, SQLTableProcessContext *> tableNameCtxMap;
	std::map<std::string, SQLTableProcessContext *> tableVarCtxMap;

	// This vector is the owner of SQLTableProcessContext instances.
	std::vector<SQLTableProcessContext *>      tableCtxVector;

	// methods
	virtual ~SQLTableProcessContextIndex();
	void clear(void);
	SQLTableProcessContext *
	  getTableContext(const std::string &name,
	                  bool throwExceptionIfNotFound = true);
};

// ---------------------------------------------------------------------------
// SQLTableFormula
// ---------------------------------------------------------------------------
class SQLTableFormula
{
public:
	struct TableSizeInfo {
		std::string name;
		std::string varName;
		size_t      numColumns;
		size_t      accumulatedColumnOffset;
	};
	typedef std::vector<TableSizeInfo *>  TableSizeInfoVector;
	typedef TableSizeInfoVector::iterator TableSizeInfoVectorIterator;
	typedef TableSizeInfoVector::const_iterator
	  TableSizeInfoVectorConstIterator;
	typedef std::map<std::string, TableSizeInfo *> TableSizeInfoMap;
	typedef TableSizeInfoMap::iterator             TableSizeInfoMapIterator;

	virtual ~SQLTableFormula();
	virtual ItemTablePtr getTable(void) = 0;
	virtual void prepareJoin(SQLTableProcessContextIndex *ctxIndex);

	/**
	 * Return the active row, which is the currently selected row on
	 * the calculation of join.
	 *
	 * @return An active row on success. If there is no active row,
	 *         return ItemGroupPtr::hasData() is false. This typically
	 *         happens when the condition on the join is not satisfied.
	 */
	virtual ItemGroupPtr getActiveRow(void) = 0;
	virtual size_t getColumnIndexOffset(const std::string &tableName);
	const TableSizeInfoVector &getTableSizeInfoVector(void);

protected:
	virtual void fixupTableSizeInfo(void) = 0;
	void addTableSizeInfo(const std::string &tableName,
	                      const std::string &tableVar, size_t numColumns);
	void makeTableSizeInfo(const TableSizeInfoVector &leftList,
	                       const TableSizeInfoVector &rightList);

private:
	TableSizeInfoVector m_tableSizeInfoVector;
	TableSizeInfoMap    m_tableSizeInfoMap;
	TableSizeInfoMap    m_tableVarSizeInfoMap;
};

// ---------------------------------------------------------------------------
// SQLTableElement
// ---------------------------------------------------------------------------
class SQLTableElement : public SQLTableFormula
{
public:
	SQLTableElement(const std::string &name, const std::string &varName,
	                SQLColumnIndexResoveler *resolver,
	                SQLSubQueryMode subQueryMode);
	const std::string &getName(void) const;
	const std::string &getVarName(void) const;
	void setItemTable(ItemTablePtr itemTablePtr);
	void selectRowIterator(SQLTableProcessContext *tableCtx);
	virtual void prepareJoin(SQLTableProcessContextIndex *ctxIndex);
	virtual ItemTablePtr getTable(void);
	virtual ItemGroupPtr getActiveRow(void);

	void startRowIterator(void);
	bool rowIteratorEnd(void);
	void rowIteratorInc(void);
	void setSQLTableProcessContext(SQLTableProcessContext *joinedTableCtx);
	bool isIndexingMode(void);

	void forceSetRowIterator(SQLTableRowIterator *rowIterator);

protected:
	virtual void fixupTableSizeInfo(void);

private:
	std::string m_name;
	std::string m_varName;
	ItemTablePtr m_itemTablePtr;
	SQLColumnIndexResoveler *m_columnIndexResolver;
	ItemGroupListConstIterator m_currSelectedGroup;
	SQLTableProcessContext    *m_tableProcessCtx;
	SQLTableRowIterator       *m_selectedRowIterator;
	SQLSubQueryMode            m_subQueryMode;
};

typedef std::list<SQLTableElement *>        SQLTableElementList;
typedef SQLTableElementList::iterator       SQLTableElementListIterator;
typedef SQLTableElementList::const_iterator SQLTableElementListConstIterator;

// ---------------------------------------------------------------------------
// SQLTableJoin
// ---------------------------------------------------------------------------
class SQLTableJoin : public SQLTableFormula
{
public:
	SQLTableJoin(SQLJoinType type);
	SQLTableFormula *getLeftFormula(void) const;
	SQLTableFormula *getRightFormula(void) const;
	void setLeftFormula(SQLTableFormula *tableFormula);
	void setRightFormula(SQLTableFormula *tableFormula);
	virtual void prepareJoin(SQLTableProcessContextIndex *ctxIndex);
	virtual ItemGroupPtr getActiveRow(void);

protected:
	virtual void fixupTableSizeInfo(void);

private:
	SQLJoinType      m_type;
	SQLTableFormula *m_leftFormula;
	SQLTableFormula *m_rightFormula;
};

// ---------------------------------------------------------------------------
// SQLTableCorssJoin
// ---------------------------------------------------------------------------
class SQLTableCrossJoin : public SQLTableJoin
{
public:
	SQLTableCrossJoin(void);
	virtual ItemTablePtr getTable(void);

private:
};

// ---------------------------------------------------------------------------
// SQLTableInnerJoin
// ---------------------------------------------------------------------------
class SQLTableInnerJoin : public SQLTableJoin
{
public:
	SQLTableInnerJoin(const std::string &leftTableName,
	                  const std::string &leftColumnName,
	                  const std::string &rightTableName,
	                  const std::string &rightColumnName,
	                  SQLColumnIndexResoveler *resolver);
	virtual void prepareJoin(SQLTableProcessContextIndex *ctxIndex);
	virtual ItemTablePtr getTable(void);
	virtual ItemGroupPtr getActiveRow(void);

	const std::string &getLeftTableName(void) const;
	const std::string &getLeftColumnName(void) const;
	const std::string &getRightTableName(void) const;
	const std::string &getRightColumnName(void) const;

private:
	static const size_t INDEX_NOT_SET = -1;
	std::string m_leftTableName;
	std::string m_leftColumnName;
	std::string m_rightTableName;
	std::string m_rightColumnName;
	size_t m_indexLeftJoinColumn;
	size_t m_indexRightJoinColumn;
	SQLColumnIndexResoveler *m_columnIndexResolver;
	SQLTableElement *m_rightTableElement;
};

#endif // SQLTableFormula_h


