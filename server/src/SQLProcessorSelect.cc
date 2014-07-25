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

#include <Logger.h>
#include <StringUtils.h>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include "SQLProcessorSelect.h"
#include "SQLProcessorException.h"
#include "HatoholException.h"
#include "ItemEnum.h"
#include "SQLUtils.h"
#include "Utils.h"
#include "PrimaryConditionPicker.h"
using namespace std;
using namespace mlpl;

typedef bool (SQLProcessorSelect::*SelectSectionParser)(void);
typedef void (SQLProcessorSelect::*SelectSubParser)(void);

enum BetweenParsingStep {
	BETWEEN_NONE,
	BETWEEN_EXPECT_FIRST,
	BETWEEN_EXPECT_AND,
	BETWEEN_EXPECT_SECOND,
};

enum SelectParseSection {
	SELECT_PARSING_SECTION_COLUMN,
	SELECT_PARSING_SECTION_GROUP_BY,
	SELECT_PARSING_SECTION_FROM,
	SELECT_PARSING_SECTION_WHERE,
	SELECT_PARSING_SECTION_ORDER_BY,
	SELECT_PARSING_SECTION_LIMIT,
	NUM_SELECT_PARSING_SECTION,
};

class SQLProcessorColumnIndexResolver : public SQLColumnIndexResoveler {
public:
	SQLProcessorColumnIndexResolver(TableNameStaticInfoMap &nameInfoMap)
	: m_tableNameStaticInfoMap(nameInfoMap),
	  m_tables(NULL),
	  m_tableVarInfoMap(NULL),
	  m_parentResolver(NULL)
	{
	}

	virtual int getIndex(const string &tableName,
	                     const string &columnName) const
	{
		const SQLTableStaticInfo *staticInfo =
		  getTableStaticInfo(tableName);
		return SQLUtils::getColumnIndex(columnName, staticInfo);
	}

	virtual size_t getNumberOfColumns(const string &tableName) const
	{
		const SQLTableStaticInfo *staticInfo =
		  getTableStaticInfo(tableName);
		return staticInfo->columnDefList.size();
	}

	void setTableInfo(const SQLTableInfoList *tables,
	                  SQLTableVarNameInfoMap *tableVarInfoMap)
	{
		m_tables = tables;
		m_tableVarInfoMap = tableVarInfoMap;
	}

	void setParentResolver(SQLProcessorColumnIndexResolver *parentResolver)
	{
		m_parentResolver = parentResolver;
	}

protected:
	const SQLTableStaticInfo *
	getTableStaticInfo(const string &tableName) const
	{
		if (tableName.empty() && m_tables->size() == 1) {
			return (*m_tables->begin())->staticInfo;
		}

		if (m_tableVarInfoMap) {
			SQLTableVarNameInfoMapIterator it =
			  m_tableVarInfoMap->find(tableName);
			if (it != m_tableVarInfoMap->end()) {
				const SQLTableInfo *tableInfo = it->second;
				return tableInfo->staticInfo;
			}
		}

		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableName);
		if (it != m_tableNameStaticInfoMap.end())
			return it->second;

		// try to find from the parent
		const SQLTableStaticInfo *staticInfo = NULL;
		if (m_parentResolver) {
			staticInfo =
			   m_parentResolver->getTableStaticInfo(tableName);
		}
		if (staticInfo)
			return staticInfo;

		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found table: %s", tableName.c_str());
		return NULL;
	}

private:
	TableNameStaticInfoMap &m_tableNameStaticInfoMap;

	// The following two pointers point members in SelectInfo
	const SQLTableInfoList *m_tables;
	SQLTableVarNameInfoMap *m_tableVarInfoMap;

	SQLProcessorColumnIndexResolver *m_parentResolver;
};

class SQLProcessorSelectFactoryImpl : public SQLProcessorSelectFactory {
public:
	SQLProcessorSelectFactoryImpl(SQLProcessorSelect *parent)
	: m_parent(parent)
	{
	}

	virtual SQLProcessorSelect *create(SQLSubQueryMode subQueryMode)
	{
		return new SQLProcessorSelect(m_parent, subQueryMode);
	}

private:
	SQLProcessorSelect *m_parent;
};

struct SQLProcessorSelect::PrivateContext {
	static const SelectSubParser            selectSubParsers[];
	static map<string, SelectSectionParser> selectSectionParserMap;

	SQLProcessorSelectFactoryImpl procSelectFactory;
	SQLProcessorSelectShareInfo   shareInfo;

	// for internal select
	const SQLProcessorSelect *parentProcessor;

	SQLSelectInfo      *selectInfo;
	string              dbName;

	// The content of m_tableNameStaticInfoMap is typically
	// set in an SQLProcessor's sub class.
	TableNameStaticInfoMap      &tableNameStaticInfoMap;

	SeparatorChecker   *selectSeprators[NUM_SELECT_PARSING_SECTION];
	SeparatorChecker    separatorSpaceComma;

	SelectParseSection  section;
	string              currWord;
	string              currWordLower;

	SQLProcessorColumnIndexResolver columnIndexResolver;
	FormulaOptimizationResult       whereFormulaOptimizationResult;
	PrimaryConditionPicker          primaryConditionPicker;
	SQLTableProcessContextIndex    *tableProcessContextIndex;
	SQLSubQueryMode                 subQueryMode;

	// currently processed Item Group used in selectMatchingRows()
	bool                useEvalTargetItemGroup;
	ItemGroupPtr        evalTargetItemGroup;

	// The number of times to be masked for generating output lines.
	size_t              makeTextRowsWriteMaskCount;
	set<string>         concatOutputTextSet; // for DISTINCT

	// Members for processing GROUP BY
	StringVector        groupByColumns;
	vector<size_t>      groupByColumnIndexes;
	ItemGroupTableMap   groupedTableMap;

	// temporary variable (used in pickupMatchingRows)
	VariableItemTablePtr workTable;

	// methods
	PrivateContext(SQLProcessorSelect *procSelect, const string &_dbName,
	               TableNameStaticInfoMap &nameInfoMap,
	               const SQLProcessorSelect *parentProc = NULL,
	               SQLTableProcessContextIndex *tableProcCtxIdx = NULL,
	               SQLSubQueryMode _subQueryMode = SQL_SUB_QUERY_NONE)
	: procSelectFactory(procSelect),
	  shareInfo(procSelectFactory),
	  parentProcessor(parentProc),
	  selectInfo(NULL),
	  dbName(_dbName),
	  tableNameStaticInfoMap(nameInfoMap),
	  separatorSpaceComma(" ,"),
	  section(SELECT_PARSING_SECTION_COLUMN),
	  columnIndexResolver(nameInfoMap),
	  tableProcessContextIndex(tableProcCtxIdx),
	  subQueryMode(_subQueryMode),
	  useEvalTargetItemGroup(false),
	  evalTargetItemGroup(NULL),
	  makeTextRowsWriteMaskCount(0)
	{
		if (!parentProc) {
			tableProcessContextIndex =
			   new SQLTableProcessContextIndex();
		}
		if (parentProc) {
			columnIndexResolver.setParentResolver
			  (&parentProc->m_ctx->columnIndexResolver);
		}
	}

	virtual ~PrivateContext()
	{
		if (!parentProcessor && tableProcessContextIndex)
			delete tableProcessContextIndex;
	}

	void clear(void)
	{
		shareInfo.clear();
		columnIndexResolver.setTableInfo(NULL, NULL);
		section = SELECT_PARSING_SECTION_COLUMN;
		currWord.clear();
		currWordLower.clear();
		useEvalTargetItemGroup = false;
		if (!parentProcessor)
			tableProcessContextIndex->clear();
		evalTargetItemGroup = NULL;
		makeTextRowsWriteMaskCount = 0;
		concatOutputTextSet.clear();
		groupByColumns.clear();
		groupByColumnIndexes.clear();
		groupedTableMap.clear();
	}

	void setSelectInfo(SQLSelectInfo *_selectInfo) {
		selectInfo = _selectInfo;
		columnIndexResolver.setTableInfo(&selectInfo->tables,
		                                 &selectInfo->tableVarInfoMap);
	};

	void resetStatistics(void) {
		size_t outColumnSize = selectInfo->outputColumnVector.size();
		for (size_t i = 0; i < outColumnSize; i++)
			selectInfo->outputColumnVector[i].resetStatistics();
	}
};

const SelectSubParser SQLProcessorSelect::PrivateContext::selectSubParsers[] = {
	&SQLProcessorSelect::parseSelectedColumns,
	&SQLProcessorSelect::parseGroupBy,
	&SQLProcessorSelect::parseFrom,
	&SQLProcessorSelect::parseWhere,
	&SQLProcessorSelect::parseOrderBy,
	&SQLProcessorSelect::parseLimit,
};

map<string, SelectSectionParser>
  SQLProcessorSelect::PrivateContext::selectSectionParserMap;

class SQLFormulaColumnDataGetter : public FormulaVariableDataGetter {
public:
	SQLFormulaColumnDataGetter(const string &name,
	                           SQLSelectInfo *selectInfo,
	                           bool &useEvalTargetItemGroup)
	: m_columnInfo(NULL),
	  m_columnIndex(-1),
	  m_useEvalTargetItemGroup(useEvalTargetItemGroup)
	{
		SQLColumnNameMapIterator it
		  = selectInfo->columnNameMap.find(name);
		if (it != selectInfo->columnNameMap.end()) {
			m_columnInfo = it->second;
			return;
		}

		// The owner of the SQLColumnInfo object created below
		// is *selectInfo. So its deletion is done
		// in the destructor of the SQLSelectInfo object.
		// No need to delete in this class.
		m_columnInfo = new SQLColumnInfo(name);
		selectInfo->columnNameMap[name] = m_columnInfo;
	}
	
	virtual ~SQLFormulaColumnDataGetter()
	{
	}

	virtual ItemDataPtr getData(void)
	{
		if (m_useEvalTargetItemGroup)
			return getDataFromJoinedTable();
		else
			return getDataOfActiveRow();
	}

	/*
	 * getDataFromJoinedTable() is used when text output is created.
	 * At the one time, it was used to calculation of join. Now it
	 * is replaced by getDataOfActiveRow().
	 */
	virtual ItemDataPtr getDataFromJoinedTable(void)
	{
		if (m_columnInfo->columnType !=
		    SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			THROW_HATOHOL_EXCEPTION(
			  "m_columnInfo->columnType(%d) != TYPE_NORMAL.",
			  m_columnInfo->columnType);
		}
		ItemId itemId = m_columnInfo->columnDef->itemId;
		if (!m_columnInfo->currTargetItemGroupAddr) {
			THROW_HATOHOL_EXCEPTION(
			  "m_columnInfo->currTargetItemGroupAddr: NULL");
		}
		ItemGroupPtr &itemGroupPtr =
		  *m_columnInfo->currTargetItemGroupAddr;
		return itemGroupPtr->getItem(itemId);
	}

	virtual ItemDataPtr getDataOfActiveRow(void)
	{
		if (m_columnIndex < 0)
			m_columnIndex = getColumnIndex();
		SQLTableElement *tableElement =
		  m_columnInfo->tableInfo->tableElement;
		ItemGroupPtr itemGroupPtr = tableElement->getActiveRow();
		return itemGroupPtr->getItemAt(m_columnIndex);
	}

	SQLColumnInfo *getColumnInfo(void) const
	{
		return m_columnInfo;
	}

private:
	SQLColumnInfo *m_columnInfo;
	int            m_columnIndex;
	bool          &m_useEvalTargetItemGroup;

	int getColumnIndex(void)
	{
		const ColumnNameAccessInfoMap &columnAccessInfoMap =
		  m_columnInfo->tableInfo->staticInfo->columnAccessInfoMap;
		ColumnNameAccessInfoMapConstIterator it =
		  columnAccessInfoMap.find(m_columnInfo->baseName);
		if (it == columnAccessInfoMap.end()) {
			THROW_HATOHOL_EXCEPTION(
			  "Not found: %s in columnAccessInfoMap",
		          m_columnInfo->baseName.c_str());
		}
		const ColumnAccessInfo &accessInfo = it->second;
		return accessInfo.index;
	}
};

// ---------------------------------------------------------------------------
// Public methods (SQLColumnDefinitino)
// ---------------------------------------------------------------------------
SQLOutputColumn::SQLOutputColumn(SQLFormulaInfo *_formulaInfo)
: formulaInfo(_formulaInfo),
  columnInfo(NULL),
  columnDef(NULL),
  columnDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::SQLOutputColumn(const SQLColumnInfo *_columnInfo)
: formulaInfo(NULL),
  columnInfo(_columnInfo),
  columnDef(NULL),
  columnDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::~SQLOutputColumn()
{
	if (columnDefDeleteFlag)
		delete columnDef;
}

ItemDataPtr SQLOutputColumn::getItem(const ItemGroup *itemGroup) const
{
	if (formulaInfo)
		return formulaInfo->formula->evaluate();
	if (columnInfo)
		return itemGroup->getItem(columnDef->itemId);
	MLPL_BUG("formulaInfo and columnInfo are both NULL.");
	return ItemDataPtr();
}

void SQLOutputColumn::resetStatistics(void)
{
	if (formulaInfo)
		formulaInfo->formula->resetStatistics();
}

// ---------------------------------------------------------------------------
// Public methods (SQLTableInfo)
// ---------------------------------------------------------------------------
SQLTableInfo::SQLTableInfo(void)
: staticInfo(NULL),
  tableElement(NULL)
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLColumnInfo)
// ---------------------------------------------------------------------------
SQLColumnInfo::SQLColumnInfo(const string &_name)
: name(_name),
  tableInfo(NULL),
  columnDef(NULL),
  columnType(COLUMN_TYPE_UNKNOWN),
  currTargetItemGroupAddr(NULL)
{
}

void SQLColumnInfo::associate(SQLTableInfo *_tableInfo)
{
	tableInfo = _tableInfo;
	_tableInfo->columnList.push_back(this);
}

void SQLColumnInfo::setColumnType(void)
{
	if (name == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL;
	else if (baseName == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL_OF_TABLE;
	else
		columnType = SQLColumnInfo::COLUMN_TYPE_NORMAL;
}

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(const ParsableString &_statement)
: SQLProcessorInfo(_statement),
  useIndex(false),
  itemFalsePtr(new ItemBool(false), false)
{
}

SQLSelectInfo::~SQLSelectInfo()
{
	SQLColumnNameMapIterator columnIt = columnNameMap.begin();
	for (; columnIt != columnNameMap.end(); ++columnIt)
		delete columnIt->second;

	SQLTableInfoListIterator tableIt = tables.begin();
	for (; tableIt != tables.end(); ++tableIt)
		delete *tableIt;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorSelect::init(void)
{
	PrivateContext::selectSectionParserMap["select"] =
	  &SQLProcessorSelect::parseSectionColumn;
	PrivateContext::selectSectionParserMap["from"] =
	  &SQLProcessorSelect::parseSectionFrom;
	PrivateContext::selectSectionParserMap["where"] =
	  &SQLProcessorSelect::parseSectionWhere;
	PrivateContext::selectSectionParserMap["order"] =
	  &SQLProcessorSelect::parseSectionOrder;
	PrivateContext::selectSectionParserMap["group"] =
	  &SQLProcessorSelect::parseSectionGroup;
	PrivateContext::selectSectionParserMap["limit"] =
	  &SQLProcessorSelect::parseSectionLimit;

	// check the size of m_selectSubParsers
	size_t size =
	  sizeof(PrivateContext::selectSubParsers) / sizeof(SelectSubParser);
	if (size != NUM_SELECT_PARSING_SECTION) {
		THROW_HATOHOL_EXCEPTION(
		  "sizeof(m_selectSubParsers) is invalid: "
		  "(expcect/actual: %d/%zd).",
		  NUM_SELECT_PARSING_SECTION, size);
	}
}

bool SQLProcessorSelect::select(SQLSelectInfo &selectInfo)
{
	try {
		// basic setup
		setupForSelect(selectInfo);

		// join tables
		doJoinWithFromParser();

		/*  doJoin() and selectMatchingRows() is not used now.
		 *  But it may be faster than doJoinWithFromParser(),
		 *  we keep them with comment out.
		--------------------------------------------------------
		// join tables
		doJoin();

		// pickup matching rows
		selectMatchingRows();
		-------------------------------------------------------- */

		// grouping if needed
		makeGroups();

		// convert data to string
		makeTextOutputForAllGroups();

	} catch (const SQLProcessorException &e) {
		MLPL_DBG("Got SQLProcessorException: %s",
		         e.getFancyMessage().c_str());
		return false;
	}

	return true;
}

bool SQLProcessorSelect::runForExists(SQLSelectInfo &selectInfo)
{
	if (selectInfo.tables.empty()) {
		 // Should be the first call
		setupForSelect(selectInfo);
	}

	bool exists = false;
	try {
		doJoinWithFromParser();
	} catch (const SQLFoundRowOnJoinException &e) {
		exists = true;
	}
	return exists;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessorSelect::SQLProcessorSelect
  (const string &dbName, TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(this, dbName, tableNameStaticInfoMap);
	setup();
}

SQLProcessorSelect::SQLProcessorSelect(const SQLProcessorSelect *parent,
                                       SQLSubQueryMode subQueryMode)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(this, parent->m_ctx->dbName,
	                           parent->m_ctx->tableNameStaticInfoMap,
	                           parent,
	                           parent->m_ctx->tableProcessContextIndex,
	                           subQueryMode);
	setup();
}

SQLProcessorSelect::~SQLProcessorSelect()
{
	delete m_ctx;
}

bool SQLProcessorSelect::checkSelectedAllColumns
  (const SQLSelectInfo &selectInfo, const SQLColumnInfo &columnInfo) const
{
	if (columnInfo.name == "*")
		return true;
	if (!columnInfo.tableInfo)
		return false;
	if (columnInfo.baseName == "*")
		return true;
	return false;
}

void SQLProcessorSelect::setupForSelect(SQLSelectInfo &selectInfo)
{
	m_ctx->clear();
	setSelectInfoToPrivateContext(selectInfo);

	// disassemble the query statement
	parseSelectStatement();
	makeTableInfo();
	checkParsedResult();

	// set members in SQLFormulaColumnDataGetter
	fixupColumnNameMap();

	// associate each table with static table information
	associateTableWithStaticInfo();

	// associate each column with the table
	associateColumnWithTable();

	// make ItemTable objects for all specified tables
	setColumnTypeAndDefInColumnInfo();
	makeColumnDefs();
	makeItemTables();

	// optimization of the formula
	optimizeFormula();

	// pickup column comparison components
	pickupColumnComparisons();
}

void SQLProcessorSelect::setSelectInfoToPrivateContext(SQLSelectInfo &selectInfo)
{
	m_ctx->setSelectInfo(&selectInfo);
}

void SQLProcessorSelect::parseSelectStatement(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	MLPL_DBG("<%s> %s\n", __func__, selectInfo->statement.getString());
	SelectSubParser subParser = NULL;

	// set ColumnDataGetterFactory
	selectInfo->columnParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);
	selectInfo->whereParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);

	// set parsable string
	m_ctx->shareInfo.statement = &selectInfo->statement;
	selectInfo->whereParser.setShareInfo(&m_ctx->shareInfo);

	// callback function for column and where section
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_COLUMN]
	  = selectInfo->columnParser.getSeparatorChecker();

	m_ctx->selectSeprators[SELECT_PARSING_SECTION_FROM]
	  = selectInfo->fromParser.getSeparatorChecker();

	m_ctx->selectSeprators[SELECT_PARSING_SECTION_WHERE]
	  = selectInfo->whereParser.getSeparatorChecker();

	while (!selectInfo->statement.finished()) {
		m_ctx->currWord = readNextWord();
		if (m_ctx->currWord.empty())
			continue;
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);

		// check if this is a section keyword.
		if (m_ctx->shareInfo.allowSectionParserChange) {
			if (checkSectionParserChange())
				continue;
		}

		// parse each component
		if (m_ctx->section >= NUM_SELECT_PARSING_SECTION) {
			THROW_HATOHOL_EXCEPTION(
			  "section(%d) >= NUM_SELECT_PARSING_SECTION\n",
			  m_ctx->section);
		}
		subParser = m_ctx->selectSubParsers[m_ctx->section];
		(this->*subParser)();
	}
	selectInfo->columnParser.close();
	selectInfo->whereParser.close();
	selectInfo->fromParser.close();
}

void SQLProcessorSelect::makeTableInfo(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLFromParser &fromParser = selectInfo->fromParser;
	SQLTableElementListConstIterator it =
	  fromParser.getTableElementList().begin();
	for (; it != fromParser.getTableElementList().end(); ++it) {
		// make selectInfo->tables
		SQLTableElement *tableElem = *it;
		SQLTableInfo *tableInfo = new SQLTableInfo();
		tableInfo->name    = tableElem->getName();
		tableInfo->varName = tableElem->getVarName();
		tableInfo->tableElement = tableElem;
		selectInfo->tables.push_back(tableInfo);

		// make selectInfo->tableVarInfoMap
		string &varName = tableInfo->varName;
		if (varName.empty())
			continue;
		pair<SQLTableVarNameInfoMapIterator, bool> ret =
		  selectInfo->tableVarInfoMap.insert
		    (pair<string, SQLTableInfo *>(varName, tableInfo));
		if (!ret.second) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to insert: table name: %s, %s.",
			  varName.c_str(), selectInfo->statement.getString());
		}
	}
}

void SQLProcessorSelect::checkParsedResult(void) const
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	if (selectInfo->columnNameMap.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: columns.");

	if (selectInfo->tables.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: tables.");
}

void SQLProcessorSelect::fixupColumnNameMap(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		parseColumnName(columnInfo->name, columnInfo->baseName,
		                columnInfo->tableVar);
		columnInfo->setColumnType();
	}
}

void SQLProcessorSelect::associateColumnWithTable(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		if (columnInfo->columnType == SQLColumnInfo::COLUMN_TYPE_ALL)
			continue;
		SQLTableInfo *tableInfo =
		  getTableInfoFromColumnInfo(columnInfo);
		columnInfo->associate(tableInfo);
	}
}

void SQLProcessorSelect::associateTableWithStaticInfo(void)
{
	SQLTableInfoListIterator tblInfoIt = m_ctx->selectInfo->tables.begin();
	for (; tblInfoIt != m_ctx->selectInfo->tables.end(); ++tblInfoIt) {
		SQLTableInfo *tableInfo = *tblInfoIt;
		TableNameStaticInfoMapIterator it;
		it = m_ctx->tableNameStaticInfoMap.find(tableInfo->name);
		if (it == m_ctx->tableNameStaticInfoMap.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s", tableInfo->name.c_str());
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
}

void SQLProcessorSelect::setColumnTypeAndDefInColumnInfo(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;

		// columnDef
		if (columnInfo->columnType != SQLColumnInfo::COLUMN_TYPE_NORMAL)
			continue;

		if (!columnInfo->tableInfo) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "columnInfo->stableInfo is NULL.");
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo)
			THROW_SQL_PROCESSOR_EXCEPTION("staticInfo is NULL.");

		columnInfo->columnDef =
		  SQLUtils::getColumnDef(columnInfo->baseName, staticInfo);
	}
}

void SQLProcessorSelect::addOutputColumn
  (SQLSelectInfo *selectInfo, const SQLColumnInfo *columnInfo,
   const ColumnDef *columnDef, const SQLFormulaInfo *formulaInfo)
{
	const SQLTableInfo *tableInfo = columnInfo->tableInfo;
	selectInfo->outputColumnVector.push_back(SQLOutputColumn(columnInfo));
	SQLOutputColumn &outCol = selectInfo->outputColumnVector.back();
	outCol.columnDef = columnDef;
	outCol.tableInfo = tableInfo;
	outCol.schema    = m_ctx->dbName;
	outCol.table     = tableInfo->name;
	outCol.tableVar  = tableInfo->varName;
	outCol.column    = columnDef->columnName;
	if (!formulaInfo) // when 'select * or n.*'
		outCol.columnVar = columnDef->columnName;
	else if (formulaInfo->alias.empty())
		outCol.columnVar = columnDef->columnName;
	else
		outCol.columnVar = formulaInfo->alias;
}

void SQLProcessorSelect::addOutputColumn(SQLSelectInfo *selectInfo,
                                         SQLFormulaInfo *formulaInfo)
{
	selectInfo->outputColumnVector.push_back(SQLOutputColumn(formulaInfo));
	SQLOutputColumn &outCol = selectInfo->outputColumnVector.back();
	outCol.columnDef = makeColumnDefForFormula(formulaInfo);
	//outCol.tableInfo     =
	outCol.schema        = m_ctx->dbName;
	//outCol.table         =
	//outCol.tableVar      =
	outCol.column        = formulaInfo->expression;
	if (formulaInfo->alias.empty())
		outCol.columnVar = formulaInfo->expression;
	else
		outCol.columnVar = formulaInfo->alias;
}

void SQLProcessorSelect::addOutputColumnsOfAllTables
  (SQLSelectInfo *selectInfo, const SQLColumnInfo *_columnInfo)
{
	// Note: We can just use the argument type w/o 'const'.
	// Or change the algorithm more elegant.
	SQLColumnInfo *columnInfo = const_cast<SQLColumnInfo *>(_columnInfo);

	SQLTableInfoListIterator it = selectInfo->tables.begin();
	for (; it != selectInfo->tables.end(); ++it) {
		columnInfo->tableInfo = *it;
		addOutputColumnsOfOneTable(selectInfo, columnInfo);
	}
}

void SQLProcessorSelect::addOutputColumnsOfOneTable(SQLSelectInfo *selectInfo,
                                              const SQLColumnInfo *columnInfo)
{
	const SQLTableInfo *tableInfo = columnInfo->tableInfo;
	if (!tableInfo->staticInfo) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "tableInfo->staticInfo is NULL.");
	}

	ColumnDefListConstIterator it;
	it = tableInfo->staticInfo->columnDefList.begin();
	for (; it != tableInfo->staticInfo->columnDefList.end(); ++it) {
		const ColumnDef *columnDef = &(*it);
		addOutputColumn(selectInfo, columnInfo, columnDef);
	}
}

const ColumnDef *
SQLProcessorSelect::makeColumnDefForFormula(SQLFormulaInfo *formulaInfo)
{
	ColumnDef *columnDef = new ColumnDef();
	columnDef->itemId = SYSTEM_ITEM_ID_ANONYMOUS;
	columnDef->tableName = "";
	columnDef->columnName = "";
	columnDef->type = SQL_COLUMN_TYPE_BIGUINT;
	columnDef->columnLength = 20;
	MLPL_BUG("Tentative implementation: type and columnLength must be "
	         "calculated by analyzing used columns.\n");
	columnDef->flags = 0; // TODO: substitute the appropriate value.
	return columnDef;
}

void SQLProcessorSelect::makeColumnDefs(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	const SQLFormulaInfoVector &formulaInfoVector
	  = selectInfo->columnParser.getFormulaInfoVector();
	for (size_t i = 0; i < formulaInfoVector.size(); i++) {
		SQLFormulaInfo *formulaInfo = formulaInfoVector[i];
		FormulaVariable *formulaVariable =
		  dynamic_cast<FormulaVariable *>(formulaInfo->formula);
		if (!formulaVariable) {
			// When the column formula is not single column.
			addOutputColumn(selectInfo, formulaInfo);
			continue;
		}

		// search the ColumnInfo instance
		FormulaVariableDataGetter *dataGetter =
			formulaVariable->getFormulaVariableGetter();
		SQLFormulaColumnDataGetter *sqlDataGetter =
		  dynamic_cast<SQLFormulaColumnDataGetter *>(dataGetter);
		SQLColumnInfo *columnInfo = sqlDataGetter->getColumnInfo();

		int columnType = columnInfo->columnType;
		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL) {
			addOutputColumnsOfAllTables(selectInfo, columnInfo);
			continue;
		} else if (!columnInfo->tableInfo) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "columnInfo->tableInfo is NULL.");
		}

		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL_OF_TABLE) {
			addOutputColumnsOfOneTable(selectInfo, columnInfo);
		} else if (columnType == SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			if (!columnInfo->columnDef) {
				THROW_SQL_PROCESSOR_EXCEPTION(
				  "columnInfo.columnDef is NULL.");
			}
			addOutputColumn(selectInfo, columnInfo,
			                columnInfo->columnDef,
			                formulaInfo);
		} else {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Invalid columnType: %d", columnType);
		}
	}
}

void SQLProcessorSelect::makeItemTables(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLTableInfoList &tables = selectInfo->tables;
	SQLTableElementList &tableElemList =
	  selectInfo->fromParser.getTableElementList();
	if (tableElemList.size() != tables.size()) {
		THROW_HATOHOL_EXCEPTION(
		  "tableElemList.size() != tables.size() (%zd, %zd)",
		  tableElemList.size(), tables.size());
	}
	SQLTableElementListIterator tblElemIt = tableElemList.begin();
	SQLTableInfoListIterator tblInfoIt = tables.begin();
	for (; tblInfoIt != tables.end(); ++tblInfoIt, ++tblElemIt) {
		const SQLTableInfo *tableInfo = *tblInfoIt;
		SQLTableGetFunc func = tableInfo->staticInfo->tableGetFunc;
		ItemTablePtr tablePtr = (*func)();
		if (!tablePtr.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "ItemTable: table has no data. name: %s, var: %s",
			  (*tblInfoIt)->name.c_str(),
			  (*tblInfoIt)->varName.c_str());
		}
		(*tblElemIt)->setItemTable(tablePtr);
	}
}

void SQLProcessorSelect::optimizeFormula(void)
{
	SQLWhereParser &whereParser = m_ctx->selectInfo->whereParser;
	FormulaElement *whereFormula = whereParser.getFormula();
	if (!whereFormula) {
		m_ctx->whereFormulaOptimizationResult.type =
		   FORMULA_ALWAYS_TRUE;
		return;
	}
	m_ctx->whereFormulaOptimizationResult = whereFormula->optimize();

	// remove FormulaParenthesis elements from the tree
	whereFormula->removeParenthesis();
}

void SQLProcessorSelect::pickupColumnComparisons(void)
{
	if (m_ctx->whereFormulaOptimizationResult.type == FORMULA_ALWAYS_FALSE)
		return;
	SQLWhereParser &whereParser = m_ctx->selectInfo->whereParser;
	FormulaElement *formula = whereParser.getFormula();
	m_ctx->primaryConditionPicker.pickupPrimary(formula);
	
	// prepare join with the pickup columns
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	selectInfo->fromParser.prepareJoin(
	  m_ctx->primaryConditionPicker.getPrimaryConditionList(),
	  m_ctx->tableProcessContextIndex);
}

void SQLProcessorSelect::doJoinWithFromParser(void)
{
	FormulaOptimizationResultType type =
	   m_ctx->whereFormulaOptimizationResult.type;
	if (type == FORMULA_ALWAYS_FALSE) {
		if (m_ctx->subQueryMode == SQL_SUB_QUERY_NONE) {
			// selectInfo->selectedTable should be an empty table.
			// So nothing to do.
			return;
		} else {
			THROW_HATOHOL_EXCEPTION(
			  "Not implemented: %s", __PRETTY_FUNCTION__);
		}
	}

	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	FormulaElement *whereFormula = selectInfo->whereParser.getFormula();
	selectInfo->selectedTable =
	  m_ctx->selectInfo->fromParser.doJoin(whereFormula);
}

void SQLProcessorSelect::doJoin(void)
{
	m_ctx->selectInfo->joinedTable =
	  m_ctx->selectInfo->fromParser.getTableFormula()->getTable();

}

void SQLProcessorSelect::selectMatchingRows(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	FormulaElement *formula = selectInfo->whereParser.getFormula();
	if (!formula) {
		selectInfo->selectedTable = selectInfo->joinedTable;
		return;
	}

	m_ctx->workTable = VariableItemTablePtr();
	selectInfo->joinedTable->foreach<SQLProcessorSelect *>
	  (pickupMatchingRows, this);
	m_ctx->selectInfo->selectedTable = m_ctx->workTable;
}

void SQLProcessorSelect::makeGroups(void)
{
	ItemTablePtr &selectedTable = m_ctx->selectInfo->selectedTable;
	if (m_ctx->groupByColumns.empty()) {
		m_ctx->selectInfo->groupedTables.push_back(selectedTable);
		return;
	}

	// When 'GROUP BY' is specified
	makeGroupByTables();
}

void SQLProcessorSelect::makeTextOutputForAllGroups(void)
{
	m_ctx->useEvalTargetItemGroup = true;
	ItemTablePtrListIterator it = m_ctx->selectInfo->groupedTables.begin();
	for (; it != m_ctx->selectInfo->groupedTables.end(); ++it) {
		makeTextOutput(*it);
		m_ctx->resetStatistics();
	}
}

void SQLProcessorSelect::makeTextOutput(ItemTablePtr &tablePtr)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	// check if statistical function is included
	bool hasStatisticalFunc = false;
	const SQLFormulaInfoVector &formulaInfoVector
	  = selectInfo->columnParser.getFormulaInfoVector();
	for (size_t i = 0; i < formulaInfoVector.size(); i++) {
		if (formulaInfoVector[i]->hasStatisticalFunc) {
			hasStatisticalFunc = true;
			break;
		}
	}

	if (hasStatisticalFunc || !m_ctx->groupByColumns.empty()) {
		m_ctx->makeTextRowsWriteMaskCount =
		  tablePtr->getNumberOfRows() - 1;
	}
	tablePtr->foreach<SQLProcessorSelect *>(makeTextRows, this);
}

bool SQLProcessorSelect::pickupMatchingRows(const ItemGroup *itemGroup,
                                            SQLProcessorSelect *sqlProcSelect)
{
	PrivateContext *ctx = sqlProcSelect->m_ctx;
	ctx->evalTargetItemGroup = itemGroup;
	FormulaElement *formula = ctx->selectInfo->whereParser.getFormula();
	ItemDataPtr result = formula->evaluate();
	if (!result.hasData())
		THROW_SQL_PROCESSOR_EXCEPTION("result has no data.");
	if (*result == *ctx->selectInfo->itemFalsePtr)
		return true;
	ctx->workTable->add(itemGroup);
	return true;
}

bool SQLProcessorSelect::makeGroupedTable(const ItemGroup *itemGroup,
                                          SQLProcessorSelect *sqlProcSelect)
{
	PrivateContext *ctx = sqlProcSelect->m_ctx;

	// make an ItemGroup for the target columns of GROUP BY
	VariableItemGroupPtr _targetItemGroup;
	for (size_t i = 0; i < ctx->groupByColumnIndexes.size(); i++) {
		size_t targetIndex = ctx->groupByColumnIndexes[i];
		_targetItemGroup->add(itemGroup->getItemAt(targetIndex));
	}

	// If the first combination of ItemData (targetItemGroup) comes,
	// we insert a new table into groupedTableMap.
	ItemGroupPtr targetItemGroup(_targetItemGroup);
	ItemGroupTableMap &groupedTableMap = ctx->groupedTableMap;
	ItemGroupTableMapIterator it = groupedTableMap.find(targetItemGroup);
	if (it == groupedTableMap.end()) {
		VariableItemTablePtr tablePtr;
		tablePtr->add(itemGroup);
		groupedTableMap[targetItemGroup] = tablePtr;
	} else {
		VariableItemTablePtr &tablePtr = it->second;
		tablePtr->add(itemGroup);
	}
	return true;
}

bool SQLProcessorSelect::makeTextRows(const ItemGroup *itemGroup,
                                      SQLProcessorSelect *sqlProcSelect)
{
	PrivateContext *ctx = sqlProcSelect->m_ctx;
	SQLSelectInfo *selectInfo = ctx->selectInfo;
	bool doOutput = false;
	if (ctx->makeTextRowsWriteMaskCount == 0)
		doOutput = true;
	else
		ctx->makeTextRowsWriteMaskCount--;

	ctx->evalTargetItemGroup = itemGroup;

	StringVector textVector;
	for (size_t i = 0; i < selectInfo->outputColumnVector.size(); i++) {
		const SQLOutputColumn &outputColumn =
		  selectInfo->outputColumnVector[i];
		const ItemDataPtr itemPtr = outputColumn.getItem(itemGroup);
		if (!itemPtr.hasData()) {
			THROW_HATOHOL_EXCEPTION(
			  "Failed to get item data: index: %zd\n", i);
		}
		if (!doOutput)
			continue;
		textVector.push_back(itemPtr->getString());
	}

	// check duplication when DISTINCT is given
	if (!textVector.empty() &&
	    selectInfo->columnParser.getDistinctFlag()) {
		string concat;
		for (size_t i = 0; i < textVector.size(); i++)
			concat += textVector[i];

		pair<set<string>::iterator, bool> result;
		result = ctx->concatOutputTextSet.insert(concat);
		if (!result.second) {
			// There has been the same string.
			return true;
		}
	}

	// add text ouput
	if (!textVector.empty())
		selectInfo->textRows.push_back(textVector);
	return true;
}

//
// Select status parsers
//
bool SQLProcessorSelect::parseSectionColumn(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_COLUMN;
	return true;
}

bool SQLProcessorSelect::parseSectionFrom(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_FROM;
	m_ctx->selectInfo->fromParser.setColumnIndexResolver
	  (&m_ctx->columnIndexResolver);
	m_ctx->selectInfo->fromParser.setSubQueryMode(m_ctx->subQueryMode);
	return true;
}

bool SQLProcessorSelect::parseSectionWhere(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_WHERE;
	return true;
}

bool SQLProcessorSelect::parseSectionOrder(void)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(&currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		m_ctx->selectInfo->statement.setParsingPosition(currPos);
		return false;
	}

	m_ctx->section = SELECT_PARSING_SECTION_ORDER_BY;
	return true;
}

bool SQLProcessorSelect::parseSectionGroup(void)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(&currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		m_ctx->selectInfo->statement.setParsingPosition(currPos);
		return false;
	}

	m_ctx->section = SELECT_PARSING_SECTION_GROUP_BY;
	return true;
}

bool SQLProcessorSelect::parseSectionLimit(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_LIMIT;
	return true;
}

//
// Select statment parsers
//
void SQLProcessorSelect::parseSelectedColumns(void)
{
	m_ctx->selectInfo->columnParser.add(m_ctx->currWord,
	                                    m_ctx->currWordLower);
}

void SQLProcessorSelect::parseGroupBy(void)
{
	m_ctx->groupByColumns.push_back(m_ctx->currWord);
}

void SQLProcessorSelect::parseFrom(void)
{
	m_ctx->selectInfo->fromParser.add(m_ctx->currWord,
	                                  m_ctx->currWordLower);
}

void SQLProcessorSelect::parseWhere(void)
{
	m_ctx->selectInfo->whereParser.add(m_ctx->currWord,
	                                   m_ctx->currWordLower);
}

void SQLProcessorSelect::parseOrderBy(void)
{
	m_ctx->selectInfo->orderedColumns.push_back(m_ctx->currWord);
}

void SQLProcessorSelect::parseLimit(void)
{
	MLPL_BUG("Not implemented: %s: %s\n",
	         m_ctx->currWord.c_str(), __func__);
}

//
// General sub routines
//
void SQLProcessorSelect::setup(void)
{
	// Other elements are set in parseSelectStatement().
	SeparatorChecker *sep = &m_ctx->separatorSpaceComma;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_GROUP_BY] = sep;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_ORDER_BY] = sep;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_LIMIT] = sep;
}

string SQLProcessorSelect::readNextWord(ParsingPosition *position)
{
	SeparatorChecker *separator = m_ctx->selectSeprators[m_ctx->section];
	if (position)
		*position = m_ctx->selectInfo->statement.getParsingPosition();
	return m_ctx->selectInfo->statement.readWord(*separator);
}

void SQLProcessorSelect::parseColumnName(const string &name,
                                         string &baseName, string &tableVar)
{
	size_t dotPos = name.find('.');
	if (dotPos == 0) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Column name begins from dot. : %s", name.c_str());
	}
	if (dotPos == (name.size() - 1)) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Column name ends with dot. : %s", name.c_str());
	}

	if (dotPos != string::npos) {
		tableVar = string(name, 0, dotPos);
		baseName = string(name, dotPos + 1);
	} else {
		baseName = name;
	}
}

SQLTableInfo *
SQLProcessorSelect::getTableInfoFromColumnInfo(SQLColumnInfo *columnInfo) const
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLTableInfo *tableInfo = NULL;
	if (selectInfo->tables.size() == 1) {
		SQLTableInfo *_tableInfo = *selectInfo->tables.begin();
		if (columnInfo->tableVar.empty() ||
		    columnInfo->tableVar == _tableInfo->varName ||
		    columnInfo->tableVar == _tableInfo->name) {
			tableInfo = _tableInfo;
		}
	} else if (!columnInfo->tableVar.empty()) {
		tableInfo = getTableInfoFromVarName(columnInfo->tableVar);
	} else {
		tableInfo = getTableInfoWithScanTables(columnInfo);
	}

	// try to search from the parent if it exists
	const SQLProcessorSelect *parent = m_ctx->parentProcessor;
	if (!tableInfo && parent)
		tableInfo = parent->getTableInfoFromColumnInfo(columnInfo);

	if (!tableInfo) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found SQLTableInfo for the column: %s",
		  columnInfo->tableVar.c_str());
	}

	// set the pointer where the current row has
	if (!columnInfo->currTargetItemGroupAddr) {
		columnInfo->currTargetItemGroupAddr =
		  &m_ctx->evalTargetItemGroup;
	}

	return tableInfo;
}

SQLTableInfo *
SQLProcessorSelect::getTableInfoWithScanTables(SQLColumnInfo *columnInfo) const
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	const string &columnName = columnInfo->name;
	SQLTableInfoListIterator it = selectInfo->tables.begin();
	for (; it != selectInfo->tables.end(); ++it) {
		SQLTableInfo *tableInfo = *it;
		const SQLTableStaticInfo *staticInfo = tableInfo->staticInfo;
		ColumnNameAccessInfoMapConstIterator jt =
		  staticInfo->columnAccessInfoMap.find(columnName);
		if (jt != staticInfo->columnAccessInfoMap.end())
			return tableInfo; // found
	}
	return NULL;
}

SQLTableInfo *
SQLProcessorSelect::getTableInfoFromVarName(const string &tableVar) const
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLTableVarNameInfoMapIterator it
	  = selectInfo->tableVarInfoMap.find(tableVar);
	if (it == selectInfo->tableVarInfoMap.end())
		return NULL;
	return it->second;
}

FormulaVariableDataGetter *
SQLProcessorSelect::formulaColumnDataGetterFactory(const string &name,
                                                   void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);
	SQLSelectInfo *selectInfo = ctx->selectInfo;
	return new SQLFormulaColumnDataGetter(name, selectInfo,
	                                      ctx->useEvalTargetItemGroup);
}

void SQLProcessorSelect::makeGroupByTables(void)
{
	ItemTablePtr &selectedTable = m_ctx->selectInfo->selectedTable;

	// make target column index vector
	for (size_t i = 0; i < m_ctx->groupByColumns.size(); i++) {
		string &columnName = m_ctx->groupByColumns[i];
		size_t columnIndex = getColumnIndexInJoinedTable(columnName);
		m_ctx->groupByColumnIndexes.push_back(columnIndex);
	}

	// divide the joined table into the grouped
	selectedTable->foreach<SQLProcessorSelect *>(makeGroupedTable, this);

	// copy the table pointers to groupedTables.
	ItemGroupTableMapIterator it = m_ctx->groupedTableMap.begin();
	for (; it != m_ctx->groupedTableMap.end(); ++it) {
		ItemTablePtr tablePtr(it->second);
		m_ctx->selectInfo->groupedTables.push_back(tablePtr);
	}
}

bool SQLProcessorSelect::checkSectionParserChange(void)
{
	map<string, SelectSectionParser>::iterator it;
	it = m_ctx->selectSectionParserMap.find(m_ctx->currWordLower);
	if (it == m_ctx->selectSectionParserMap.end())
		return false;
	// When the function returns 'true', it means
	// the current word is section keyword and
	SelectSectionParser sectionParser = it->second;
	return (this->*sectionParser)();
}

size_t SQLProcessorSelect::getColumnIndexInJoinedTable(const string &columnName)
{
	// TODO: use more efficient algorithm
	// (However, typical number of tables is two or three. So this
	//  may not take a lot of time.)
	size_t baseIndex = 0;
	const SQLTableStaticInfo *tableStaticInfo = NULL;
	string baseName, tableVar;
	parseColumnName(columnName, baseName, tableVar);
	SQLTableInfoList &tables = m_ctx->selectInfo->tables;
	if (tables.size() > 1) {
		SQLTableInfoListIterator it = tables.begin();
		for (; it != tables.end(); ++it) {
			SQLTableInfo *tableInfo = *it;
			tableStaticInfo = tableInfo->staticInfo;
			if (tableVar == tableInfo->varName)
				break;
			baseIndex += tableStaticInfo->columnDefList.size();
		}
		if (it == tables.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s (%s)",
			  tableVar.c_str(), columnName.c_str());
		}
	} else {
		tableStaticInfo = (*tables.begin())->staticInfo;
	}
	size_t index = baseIndex +
	               SQLUtils::getColumnIndex(baseName, tableStaticInfo);
	return index;
}
