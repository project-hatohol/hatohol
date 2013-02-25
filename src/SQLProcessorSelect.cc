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

#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <stdexcept>
#include <cstring>
#include "SQLProcessorSelect.h"
#include "SQLProcessorException.h"
#include "AsuraException.h"
#include "ItemEnum.h"
#include "SQLUtils.h"
#include "Utils.h"

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
	  m_tableVarInfoMap(NULL)
	{
	}

	virtual int getIndex(const string &tableName,
	                     const string &columnName) const
	{
		const SQLTableStaticInfo *staticInfo =
		  getTableStaticInfo(tableName, columnName);
		return SQLUtils::getColumnIndex(columnName, staticInfo);
	}

	void setTableVarInfoMap(SQLTableVarNameInfoMap *tableVarInfoMap)
	{
		m_tableVarInfoMap = tableVarInfoMap;
	}

protected:
	const SQLTableStaticInfo *
	getTableStaticInfo(const string &tableName,
	                   const string &columnName) const
	{
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
		if (it == m_tableNameStaticInfoMap.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s\n", tableName.c_str());
		}
		return it->second;
	}

private:
	TableNameStaticInfoMap &m_tableNameStaticInfoMap;
	SQLTableVarNameInfoMap *m_tableVarInfoMap;
};

struct SQLProcessorSelect::PrivateContext {
	static const SelectSubParser            selectSubParsers[];
	static map<string, SelectSectionParser> selectSectionParserMap;

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

	// currently processed Item Group used in selectMatchingRows()
	ItemGroupPtr        evalTargetItemGroup;

	// The number of times to be masked for generating output lines.
	size_t              makeTextRowsWriteMaskCount;

	// Members for processing GROUP BY
	StringVector        groupByColumns;
	ItemDataTableMap    groupedTableMap;
	size_t              groupTargetColumnIndex;

	// methods
	PrivateContext(SQLProcessorSelect *procSelect, const string &_dbName,
	               TableNameStaticInfoMap &nameInfoMap)
	: selectInfo(NULL),
	  dbName(_dbName),
	  tableNameStaticInfoMap(nameInfoMap),
	  separatorSpaceComma(" ,"),
	  section(SELECT_PARSING_SECTION_COLUMN),
	  columnIndexResolver(nameInfoMap),
	  evalTargetItemGroup(NULL),
	  makeTextRowsWriteMaskCount(0)
	{
	}

	void clear(void)
	{
		columnIndexResolver.setTableVarInfoMap(NULL);
		section = SELECT_PARSING_SECTION_COLUMN;
		currWord.clear();
		currWordLower.clear();
		evalTargetItemGroup = NULL;
		makeTextRowsWriteMaskCount = 0;
		groupByColumns.clear();
		groupedTableMap.clear();
	}

	void setSelectInfo(SQLSelectInfo *_selectInfo) {
		selectInfo = _selectInfo;
		columnIndexResolver.setTableVarInfoMap
		  (&selectInfo->tableVarInfoMap);
	};
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
	SQLFormulaColumnDataGetter(string &name,
	                           SQLSelectInfo *selectInfo,
	                           ItemGroupPtr &evalTargetItemGroup)
	: m_evalTargetItemGroup(evalTargetItemGroup),
	  m_columnInfo(NULL)
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
		if (m_columnInfo->columnType !=
		    SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			string msg;
			TRMSG(msg,
			      "m_columnInfo->columnType(%d) != TYPE_NORMAL.",
			      m_columnInfo->columnType);
			throw logic_error(msg);
		}
		ItemId itemId = m_columnInfo->columnBaseDef->itemId;
		return ItemDataPtr(m_evalTargetItemGroup->getItem(itemId));
	}

	SQLColumnInfo *getColumnInfo(void) const
	{
		return m_columnInfo;
	}

private:
	ItemGroupPtr  &m_evalTargetItemGroup;
	SQLColumnInfo *m_columnInfo;
};

// ---------------------------------------------------------------------------
// Public methods (SQLColumnDefinitino)
// ---------------------------------------------------------------------------
SQLOutputColumn::SQLOutputColumn(SQLFormulaInfo *_formulaInfo)
: formulaInfo(_formulaInfo),
  columnInfo(NULL),
  columnBaseDef(NULL),
  columnBaseDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::SQLOutputColumn(const SQLColumnInfo *_columnInfo)
: formulaInfo(NULL),
  columnInfo(_columnInfo),
  columnBaseDef(NULL),
  columnBaseDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::~SQLOutputColumn()
{
	if (columnBaseDefDeleteFlag)
		delete columnBaseDef;
}

ItemDataPtr SQLOutputColumn::getItem(const ItemGroup *itemGroup) const
{
	if (formulaInfo)
		return formulaInfo->formula->evaluate();
	if (columnInfo)
		return itemGroup->getItem(columnBaseDef->itemId);
	MLPL_BUG("formulaInfo and columnInfo are both NULL.");
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// Public methods (SQLTableInfo)
// ---------------------------------------------------------------------------
SQLTableInfo::SQLTableInfo(void)
: staticInfo(NULL)
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLColumnInfo)
// ---------------------------------------------------------------------------
SQLColumnInfo::SQLColumnInfo(string &_name)
: name(_name),
  tableInfo(NULL),
  columnBaseDef(NULL),
  columnType(COLUMN_TYPE_UNKNOWN)
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
SQLSelectInfo::SQLSelectInfo(ParsableString &_statement)
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
		string msg;
		TRMSG(msg, "sizeof(m_selectSubParsers) is invalid: "
		           "(expcect/actual: %d/%d).",
		      NUM_SELECT_PARSING_SECTION, size);
		throw logic_error(msg);
	}
}

bool SQLProcessorSelect::select(SQLSelectInfo &selectInfo)
{
	m_ctx->clear();
	setSelectInfoToPrivateContext(selectInfo);
	try {
		// disassemble the query statement
		parseSelectStatement();
		makeTableInfo();
		checkParsedResult();

		// set members in SQLFormulaColumnDataGetter
		fixupColumnNameMap();

		// associate each column with the table
		associateColumnWithTable();

		// associate each table with static table information
		associateTableWithStaticInfo();

		// make ItemTable objects for all specified tables
		setColumnTypeAndBaseDefInColumnInfo();
		makeColumnDefs();
		makeItemTables();

		// join tables
		doJoin();

		// pickup matching rows
		selectMatchingRows();

		// grouping if needed
		makeGroups();

		// convert data to string
		makeTextOutputForAllGroups();

	} catch (const SQLProcessorException &e) {
		const char *message = e.what();
		selectInfo.errorMessage = message;
		MLPL_DBG("Got SQLProcessorException: <%s:%d> %s\n",
		         e.getSourceFileName().c_str(),
		         e.getLineNumber(), message);
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessorSelect::SQLProcessorSelect
  (const string &dbName, TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(this, dbName, tableNameStaticInfoMap);

	// Other elements are set in parseSelectStatement().
	SeparatorChecker *sep = &m_ctx->separatorSpaceComma;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_GROUP_BY] = sep;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_ORDER_BY] = sep;
	m_ctx->selectSeprators[SELECT_PARSING_SECTION_LIMIT] = sep;
}

SQLProcessorSelect::~SQLProcessorSelect()
{
	if (m_ctx)
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

void SQLProcessorSelect::setSelectInfoToPrivateContext(SQLSelectInfo &selectInfo)
{
	m_ctx->setSelectInfo(&selectInfo);
}

void SQLProcessorSelect::parseSelectStatement(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	MLPL_DBG("<%s> %s\n", __func__, selectInfo->statement.getString());
	map<string, SelectSectionParser>::iterator it;
	SelectSubParser subParser = NULL;

	// set ColumnDataGetterFactory
	selectInfo->columnParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);
	selectInfo->whereParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);

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

		// check if this is a keyword.
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		it = m_ctx->selectSectionParserMap.find(m_ctx->currWordLower);
		if (it != m_ctx->selectSectionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is section keyword and
			SelectSectionParser sectionParser = it->second;
			if ((this->*sectionParser)())
				continue;
		}

		// parse each component
		if (m_ctx->section >= NUM_SELECT_PARSING_SECTION) {
			THROW_ASURA_EXCEPTION(
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
		const SQLTableElement *tableElem = *it;
		SQLTableInfo *tableInfo = new SQLTableInfo();
		tableInfo->name    = tableElem->getName();
		tableInfo->varName = tableElem->getVarName();
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

		// set SQLColumnInfo::tableInfo and SQLTableInfo::columnList.
		if (selectInfo->tables.size() == 1) {
			SQLTableInfo *tableInfo = *selectInfo->tables.begin();
			if (columnInfo->tableVar.empty())
				columnInfo->associate(tableInfo);
			else if (columnInfo->tableVar == tableInfo->varName)
				columnInfo->associate(tableInfo);
			else {
				THROW_SQL_PROCESSOR_EXCEPTION(
				  "columnInfo.tableVar (%s) != "
				  "tableInfo.varName (%s)\n",
				  columnInfo->tableVar.c_str(),
				  tableInfo->varName.c_str());
			}
			continue;
		}

		const SQLTableInfo *tableInfo
		  = getTableInfoFromVarName(*selectInfo, columnInfo->tableVar);
		columnInfo->associate(const_cast<SQLTableInfo *>(tableInfo));
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
			  "Not found table: %s\n", tableInfo->name.c_str());
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
}

void SQLProcessorSelect::setColumnTypeAndBaseDefInColumnInfo(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;

		// baseDef
		if (columnInfo->columnType != SQLColumnInfo::COLUMN_TYPE_NORMAL)
			continue;

		if (!columnInfo->tableInfo) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "columnInfo->stableInfo is NULL\n");
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo)
			THROW_SQL_PROCESSOR_EXCEPTION("staticInfo is NULL\n");

		columnInfo->columnBaseDef =
		  SQLUtils::getColumnBaseDefinition(columnInfo->baseName,
		                                    staticInfo);
	}
}

void SQLProcessorSelect::addOutputColumn
  (SQLSelectInfo *selectInfo, const SQLColumnInfo *columnInfo,
   const ColumnBaseDefinition *columnBaseDef, const SQLFormulaInfo *formulaInfo)
{
	const SQLTableInfo *tableInfo = columnInfo->tableInfo;
	selectInfo->outputColumnVector.push_back(SQLOutputColumn(columnInfo));
	SQLOutputColumn &outCol = selectInfo->outputColumnVector.back();
	outCol.columnBaseDef = columnBaseDef;
	outCol.tableInfo     = tableInfo;
	outCol.schema        = m_ctx->dbName;
	outCol.table         = tableInfo->name;
	outCol.tableVar      = tableInfo->varName;
	outCol.column        = columnBaseDef->columnName;
	if (!formulaInfo) // when 'select * or n.*'
		outCol.columnVar = columnBaseDef->columnName;
	else if (formulaInfo->alias.empty())
		outCol.columnVar = columnBaseDef->columnName;
	else
		outCol.columnVar = formulaInfo->alias;
}

void SQLProcessorSelect::addOutputColumn(SQLSelectInfo *selectInfo,
                                         SQLFormulaInfo *formulaInfo)
{
	selectInfo->outputColumnVector.push_back(SQLOutputColumn(formulaInfo));
	SQLOutputColumn &outCol = selectInfo->outputColumnVector.back();
	outCol.columnBaseDef = makeColumnBaseDefForFormula(formulaInfo);
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
		  "tableInfo->staticInfo is NULL\n");
	}

	ColumnBaseDefListConstIterator it;
	it = tableInfo->staticInfo->columnBaseDefList.begin();
	for (; it != tableInfo->staticInfo->columnBaseDefList.end(); ++it) {
		const ColumnBaseDefinition *columnBaseDef = &(*it);
		addOutputColumn(selectInfo, columnInfo, columnBaseDef);
	}
}

const ColumnBaseDefinition *
SQLProcessorSelect::makeColumnBaseDefForFormula(SQLFormulaInfo *formulaInfo)
{
	ColumnBaseDefinition *columnBaseDef = new ColumnBaseDefinition();
	columnBaseDef->itemId = SYSTEM_ITEM_ID_ANONYMOUS;
	columnBaseDef->tableName = "";
	columnBaseDef->columnName = "";
	columnBaseDef->type = SQL_COLUMN_TYPE_BIGUINT;
	columnBaseDef->columnLength = 20;
	MLPL_BUG("Tentative implementation: type and columnLength must be "
	         "calculated by analyzing used columns.\n");
	columnBaseDef->flags = 0; // TODO: substitute the appropriate value.
	return columnBaseDef;
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
			  "columnInfo->tableInfo is NULL\n");
		}

		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL_OF_TABLE) {
			addOutputColumnsOfOneTable(selectInfo, columnInfo);
		} else if (columnType == SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			if (!columnInfo->columnBaseDef) {
				THROW_SQL_PROCESSOR_EXCEPTION(
				  "columnInfo.columnBaseDef is NULL\n");
			}
			addOutputColumn(selectInfo, columnInfo,
			                columnInfo->columnBaseDef,
			                formulaInfo);
		} else {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Invalid columnType: %d\n", columnType);
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
		THROW_ASURA_EXCEPTION(
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
			  "ItemTable: table has no data. name: %s, var: %s\n",
			  (*tblInfoIt)->name.c_str(),
			  (*tblInfoIt)->varName.c_str());
		}
		(*tblElemIt)->setItemTable(tablePtr);
	}
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
	selectInfo->joinedTable->foreach<SQLProcessorSelect *>
	  (pickupMatchingRows, this);
}

void SQLProcessorSelect::makeGroups(void)
{
	ItemTablePtr &selectedTable = m_ctx->selectInfo->selectedTable;
	if (m_ctx->groupByColumns.empty()) {
		m_ctx->selectInfo->groupedTables.push_back(selectedTable);
		return;
	}
	for (size_t i = 0; i < m_ctx->groupByColumns.size(); i++) {
		string columnName = m_ctx->groupByColumns[i];
		makeGroupedTableForColumn(columnName);
	}
}

void SQLProcessorSelect::makeTextOutputForAllGroups(void)
{
	ItemTablePtrListIterator it = m_ctx->selectInfo->groupedTables.begin();
	for (; it != m_ctx->selectInfo->groupedTables.end(); ++it)
		makeTextOutput(*it);
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
	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	ctx->evalTargetItemGroup = nonConstItemGroup;
	FormulaElement *formula = ctx->selectInfo->whereParser.getFormula();
	ItemDataPtr result = formula->evaluate();
	if (!result.hasData())
		THROW_SQL_PROCESSOR_EXCEPTION("result has no data.\n");
	if (*result == *ctx->selectInfo->itemFalsePtr)
		return true;
	ctx->selectInfo->selectedTable->add(nonConstItemGroup);
	return true;
}

bool SQLProcessorSelect::makeGroupedTable(const ItemGroup *itemGroup,
                                          SQLProcessorSelect *sqlProcSelect)
{
	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	PrivateContext *ctx = sqlProcSelect->m_ctx;
	ItemDataTableMap &groupedTableMap = ctx->groupedTableMap;
	ItemDataPtr dataPtr = itemGroup->getItemAt(ctx->groupTargetColumnIndex);
	ItemDataTableMapIterator it = groupedTableMap.find(dataPtr);
	if (it == groupedTableMap.end()) {
		ItemTablePtr tablePtr;
		tablePtr->add(nonConstItemGroup);
		groupedTableMap[dataPtr] = tablePtr;
	} else {
		ItemTablePtr &tablePtr = it->second;
		tablePtr->add(nonConstItemGroup);
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

	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	ctx->evalTargetItemGroup = nonConstItemGroup;

	StringVector textVector;
	for (size_t i = 0; i < selectInfo->outputColumnVector.size(); i++) {
		const SQLOutputColumn &outputColumn =
		  selectInfo->outputColumnVector[i];
		const ItemDataPtr itemPtr = outputColumn.getItem(itemGroup);
		if (!itemPtr.hasData()) {
			THROW_ASURA_EXCEPTION(
			  "Failed to get item data: index: %d\n", i);
		}
		if (!doOutput)
			continue;
		textVector.push_back(itemPtr->getString());
	}
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
	if (dotPos == 0) { THROW_SQL_PROCESSOR_EXCEPTION(
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

const SQLTableInfo *
SQLProcessorSelect::getTableInfoFromVarName(const SQLSelectInfo &selectInfo,
                                            const string &tableVar)
{
	SQLTableVarNameInfoMapConstIterator it
	  = selectInfo.tableVarInfoMap.find(tableVar);
	if (it == selectInfo.tableVarInfoMap.end()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Failed to find: %s\n", tableVar.c_str());
	}
	return it->second;
}

FormulaVariableDataGetter *
SQLProcessorSelect::formulaColumnDataGetterFactory(string &name, void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);
	SQLSelectInfo *selectInfo = ctx->selectInfo;
	return new SQLFormulaColumnDataGetter(name, selectInfo,
	                                      ctx->evalTargetItemGroup);
}

void SQLProcessorSelect::makeGroupedTableForColumn(const string &columnName)
{
	ItemTablePtr &selectedTable = m_ctx->selectInfo->selectedTable;

	// search the column index

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
			baseIndex += tableStaticInfo->columnBaseDefList.size();
		}
		if (it == tables.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s (%s)",
			  tableVar.c_str(), columnName.c_str());
		}
	} else {
		tableStaticInfo = (*tables.begin())->staticInfo;
	}
	m_ctx->groupTargetColumnIndex =
	  baseIndex + SQLUtils::getColumnIndex(baseName, tableStaticInfo);

	// make map: key:column, value table
	selectedTable->foreach<SQLProcessorSelect *>(makeGroupedTable, this);

	// copy the table pointers to groupedTables.
	ItemDataTableMapIterator it = m_ctx->groupedTableMap.begin();
	for (; it != m_ctx->groupedTableMap.end(); ++it)
		m_ctx->selectInfo->groupedTables.push_back(it->second);
}
