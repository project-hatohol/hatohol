#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <stdexcept>
#include <cstring>
#include "SQLProcessor.h"
#include "Utils.h"

static const int UNKNOWN_TABLE_ID = -1;

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
};

map<string, SQLProcessor::SelectSubParser>
  SQLProcessor::m_selectSectionParserMap;

struct SQLProcessor::SelectParserContext {
	SQLProcessor       *sqlProcessor;

	string              currWord;
	SelectParseSection  section;
	size_t              indexInTheStatus;
	SQLSelectInfo      &selectInfo;

	// used in where section
	bool                quotOpen;

	// methods
	SelectParserContext(SQLProcessor *sqlProc,
	                    SelectParseSection _section,
	                    SQLSelectInfo &_selectInfo)
	: sqlProcessor(sqlProc),
	  section(_section),
	  indexInTheStatus(0),
	  selectInfo(_selectInfo),
	  quotOpen(false)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods (SQLColumnDefinitino)
// ---------------------------------------------------------------------------
SQLColumnDefinition::SQLColumnDefinition(void)
: columnBaseDef(NULL),
  tableInfo(NULL)
{
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
SQLColumnInfo::SQLColumnInfo(void)
: tableInfo(NULL),
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
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL;
	else
		columnType = SQLColumnInfo::COLUMN_TYPE_NORMAL;
}

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(ParsableString &_query)
: query(_query),
  useIndex(false)
{
	rootWhereElem = new SQLWhereElement();
	currWhereElem = rootWhereElem;
}

SQLSelectInfo::~SQLSelectInfo()
{
	SQLColumnInfoVectorIterator columnIt = columns.begin();
	for (; columnIt != columns.end(); ++columnIt)
		delete *columnIt;

	SQLTableInfoVectorIterator tableIt = tables.begin();
	for (; tableIt != tables.end(); ++tableIt)
		delete *tableIt;

	if (rootWhereElem)
		delete rootWhereElem;;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessor::init(void)
{
	m_selectSectionParserMap["from"]  = &SQLProcessor::parseSectionFrom;
	m_selectSectionParserMap["where"] = &SQLProcessor::parseSectionWhere;
	m_selectSectionParserMap["order"] = &SQLProcessor::parseSectionOrder;
	m_selectSectionParserMap["group"] = &SQLProcessor::parseSectionGroup;
	m_selectSectionParserMap["limit"] = &SQLProcessor::parseSectionLimit;
}

bool SQLProcessor::select(SQLSelectInfo &selectInfo)
{
	// disassemble the query statement
	if (!parseSelectStatement(selectInfo))
		return false;
	if (!checkParsedResult(selectInfo))
		return false;

	// associate each column with the table
	if (!associateColumnWithTable(selectInfo))
		return false;

	// associate each table with static table information
	if (!associateTableWithStaticInfo(selectInfo))
		return false;

	// make ItemTable objects for all specified tables
	if (!setColumnTypeAndBaseDefInColumnInfo(selectInfo))
		return false;
	if (!makeColumnDefs(selectInfo))
		return false;
	if (!enumerateNeededItemIds(selectInfo))
		return false;
	if (!makeItemTables(selectInfo))
		return false;

	// join tables
	if (!doJoin(selectInfo))
		return false;

	// pickup matching rows
	if (!selectInfo.joinedTable->foreach<SQLSelectInfo&>
	                                    (pickupMatchingRows, selectInfo))
		return false;

	// convert data to string
	if (!selectInfo.selectedTable->foreach<SQLSelectInfo&>(makeTextRows,
	                                                       selectInfo))
		return false;

	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessor::SQLProcessor(TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_separatorSpaceComma(" ,"),
  m_separatorCountSpaceComma(", "),
  m_separatorCBForWhere(" ='"),
  m_tableNameStaticInfoMap(tableNameStaticInfoMap)
{
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_SELECT] =
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_GROUP_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_FROM] =
	  &m_separatorCountSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_WHERE] = 
	  &m_separatorCBForWhere;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_ORDER_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_LIMIT] = 
	  &m_separatorSpaceComma;
}

SQLProcessor::~SQLProcessor()
{
}

bool
SQLProcessor::checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
                                      const SQLColumnInfo &columnInfo) const
{
	if (columnInfo.name == "*")
		return true;
	if (!columnInfo.tableInfo)
		return false;
	if (columnInfo.baseName == "*")
		return true;
	return false;
}

bool SQLProcessor::parseSelectStatement(SQLSelectInfo &selectInfo)
{
	MLPL_DBG("<%s> %s\n", __func__, selectInfo.query.getString());
	map<string, SelectSubParser>::iterator it;
	SelectSubParser subParser = NULL;
	SelectParserContext ctx(this, SELECT_PARSING_SECTION_SELECT,
	                        selectInfo);
	m_separatorCBForWhere.setCallbackTempl<SelectParserContext>
	                                      ('=', whereCbEq, &ctx);
	m_separatorCBForWhere.setCallbackTempl<SelectParserContext>
	                                      ('\'', whereCbQuot, &ctx);

	while (!selectInfo.query.finished()) {
		ctx.currWord = readNextWord(ctx);
		if (ctx.currWord.empty())
			continue;

		// check if this is a keyword.
		string lowerWord = ctx.currWord;
		transform(lowerWord.begin(), lowerWord.end(),
		          lowerWord.begin(), ::tolower);
		
		it = m_selectSectionParserMap.find(lowerWord);
		if (it != m_selectSectionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is section keyword and
			subParser = it->second;
			if ((this->*subParser)(ctx)) {
				ctx.indexInTheStatus = 0;
				continue;
			}
		}

		// parse each component
		if (ctx.section >= NUM_SELECT_PARSING_SECTION) {
			MLPL_BUG("section(%d) >= NUM_SELECT_PARSING_SECTION\n",
			         ctx.section);
			return false;
		}
		subParser = m_selectSubParsers[ctx.section];
		if (!(this->*subParser)(ctx))
			return false;

		ctx.indexInTheStatus++;
	}

	return true;
}

bool SQLProcessor::checkParsedResult(const SQLSelectInfo &selectInfo) const
{
	if (selectInfo.columns.empty()) {
		MLPL_DBG("Not found: columns.\n");
		return false;
	}

	if (selectInfo.tables.empty()) {
		MLPL_DBG("Not found: tables.\n");
		return false;
	}

	return true;
}

bool SQLProcessor::associateColumnWithTable(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;

		// set SQLColumnInfo::tableInfo and SQLTableInfo::columnList.
		if (selectInfo.tables.size() == 1) {
			SQLTableInfo *tableInfo = *selectInfo.tables.begin();
			if (columnInfo->tableVar.empty())
				columnInfo->associate(tableInfo);
			else if (columnInfo->tableVar == tableInfo->varName)
				columnInfo->associate(tableInfo);
			else {
				MLPL_DBG("columnInfo.tableVar (%s) != "
				         "tableInfo.varName (%s)\n",
				         columnInfo->tableVar.c_str(),
				         tableInfo->varName.c_str());
				return false;
			}
			continue;
		}

		map<string, const SQLTableInfo *>::iterator it;
		it = selectInfo.tableVarInfoMap.find(columnInfo->tableVar);
		if (it == selectInfo.tableVarInfoMap.end()) {
			MLPL_DBG("Failed to find: %s (%s)\n",
			         columnInfo->tableVar.c_str(),
			         columnInfo->name.c_str());
			return false;
		}
		columnInfo->associate(const_cast<SQLTableInfo *>(it->second));

		// set SQLColumnInfo::baseDef.
	}
	return true;
}

bool SQLProcessor::associateTableWithStaticInfo(SQLSelectInfo &selectInfo)
{
	SQLTableInfoVectorIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt) {
		SQLTableInfo *tableInfo = *tblInfoIt;
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableInfo->name);
		if (it == m_tableNameStaticInfoMap.end()) {
			MLPL_DBG("Not found table: %s\n",
			         tableInfo->name.c_str());
			return false;
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
	return true;
}

bool
SQLProcessor::setColumnTypeAndBaseDefInColumnInfo(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;

		// columnType
		columnInfo->setColumnType();

		// baseDef
		if (columnInfo->columnType == SQLColumnInfo::COLUMN_TYPE_ALL)
			continue;

		if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->stableInfo is NULL\n");
			return false;
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo) {
			MLPL_BUG("staticInfo is NULL\n");
			return false;
		}

		ItemNameColumnBaseDefRefMapConstIterator it;
		it = staticInfo->columnBaseDefMap.find(columnInfo->baseName);
		if (it == staticInfo->columnBaseDefMap.end()) {
			MLPL_DBG("Not found column: %s\n",
			         columnInfo->name.c_str());
			return false;
		}
		columnInfo->columnBaseDef = it->second;
	}
	return true;
}

void SQLProcessor::addColumnDefs(SQLSelectInfo &selectInfo,
                                 const SQLTableInfo &tableInfo,
                                 const ColumnBaseDefinition &columnBaseDef)
{
	selectInfo.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = selectInfo.columnDefs.back();
	colDef.columnBaseDef = &columnBaseDef;
	colDef.tableInfo     = &tableInfo;
	colDef.schema        = getDBName();
	colDef.table         = tableInfo.name;
	colDef.tableVar      = tableInfo.varName;
	colDef.column        = columnBaseDef.columnName;
	colDef.columnVar     = columnBaseDef.columnName;
}

bool SQLProcessor::addAllColumnDefs(SQLSelectInfo &selectInfo,
                                    const SQLTableInfo &tableInfo)
{
	if (!tableInfo.staticInfo) {
		MLPL_BUG("tableInfo.staticInfo is NULL\n");
		return false;
	}

	ColumnBaseDefListConstIterator it;
	it = tableInfo.staticInfo->columnBaseDefList.begin();
	for (; it != tableInfo.staticInfo->columnBaseDefList.end(); ++it) {
		const ColumnBaseDefinition &columnBaseDef = *it;
		addColumnDefs(selectInfo, tableInfo, columnBaseDef);
	}
	return true;
}

bool SQLProcessor::makeColumnDefs(SQLSelectInfo &selectInfo)
{
	SQLColumnInfoVectorIterator it = selectInfo.columns.begin();
	for (; it != selectInfo.columns.end(); ++it) {
		SQLColumnInfo *columnInfo = *it;
		int columnType = columnInfo->columnType;
		if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->tableInfo is NULL\n");
			return false;
		}
		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL) {
			if (!addAllColumnDefs(selectInfo,
			                      *columnInfo->tableInfo)) {
				return false;
			}
		} else if (columnType == SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			if (!columnInfo->columnBaseDef) {
				MLPL_BUG("columnInfo.columnBaseDef is NULL\n");
				return false;
			}
			addColumnDefs(selectInfo,
			              *columnInfo->tableInfo,
			              *columnInfo->columnBaseDef);
		} else {
			MLPL_BUG("Invalid columnType: %d\n", columnType);
			return false;
		}
	}
	return true;
}

bool SQLProcessor::enumerateNeededItemIds(SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.columnDefs.size(); i++) {
		SQLColumnDefinition &columnDef = selectInfo.columnDefs[i];
		if (!columnDef.columnBaseDef) {
			MLPL_BUG("columDef.columnBaseDef is NULL\n");
			return false;
		}
		ItemId itemId = columnDef.columnBaseDef->itemId;

		const SQLTableInfo *tableInfo = columnDef.tableInfo;
		if (!tableInfo) {
			MLPL_BUG("tableInfo is NULL\n");
			return false;
		}

		SQLTableInfoItemIdVectorMapIterator it;
		it = selectInfo.neededItemIdVectorMap.find(tableInfo);
		if (it == selectInfo.neededItemIdVectorMap.end()) {
			pair<SQLTableInfoItemIdVectorMapIterator, bool> ret;
			ret = selectInfo.neededItemIdVectorMap.insert(
			        pair<const SQLTableInfo *, ItemIdVector>
			          (tableInfo, ItemIdVector()));
			it = ret.first;
		}
		ItemIdVector &itemIdVector = it->second;
		itemIdVector.push_back(itemId);
	}
	return true;
}

bool SQLProcessor::makeItemTables(SQLSelectInfo &selectInfo)
{
	SQLTableInfoVectorIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt) {
		const SQLTableInfo *tableInfo = *tblInfoIt;
		SQLTableInfoItemIdVectorMapConstIterator it;
		it = selectInfo.neededItemIdVectorMap.find(tableInfo);
		if (it == selectInfo.neededItemIdVectorMap.end())
			continue;
		const ItemIdVector &itemIdVector = it->second;
		SQLTableMakeFunc func = tableInfo->staticInfo->tableMakeFunc;
		ItemTablePtr tablePtr;
		tablePtr = (this->*func)(selectInfo, *tableInfo, itemIdVector);
		if (!tablePtr.hasData())
			return false;
		selectInfo.itemTablePtrList.push_back(tablePtr);
	}
	return true;
}

bool SQLProcessor::doJoin(SQLSelectInfo &selectInfo)
{
	if (selectInfo.itemTablePtrList.size() == 1) {
		selectInfo.joinedTable = *selectInfo.itemTablePtrList.begin();
		return true;
	}

	ItemTablePtrListConstIterator it = selectInfo.itemTablePtrList.begin();
	for (; it != selectInfo.itemTablePtrList.end(); ++it) {
		const ItemTablePtr &tablePtr = *it;
		selectInfo.joinedTable = crossJoin(selectInfo.joinedTable,
		                                   tablePtr);
	}
	return true;
}

bool SQLProcessor::pickupMatchingRows(const ItemGroup *itemGroup,
                                      SQLSelectInfo &selectInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	selectInfo.selectedTable->add(const_cast<ItemGroup *>(itemGroup));
	return true;
}

bool SQLProcessor::makeTextRows(const ItemGroup *itemGroup,
                                SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.columnDefs.size(); i++) {
		const SQLColumnDefinition &colDef = selectInfo.columnDefs[i];
		const ItemData *item =
		  itemGroup->getItem(colDef.columnBaseDef->itemId);
		if (!item) {
			MLPL_BUG("Failed to get ItemData: %"PRIu_ITEM"\n",
			         colDef.columnBaseDef->itemId);
			return false;
		}
		selectInfo.textRows.push_back(item->getString());
	}
	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseSectionFrom(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_FROM;
	m_separatorCountSpaceComma.resetCounter();
	return true;
}

bool SQLProcessor::parseSectionWhere(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_WHERE;
	return true;
}

bool SQLProcessor::parseSectionOrder(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_ORDER_BY;
	return true;
}

bool SQLProcessor::parseSectionGroup(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_GROUP_BY;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	SQLColumnInfo *columnInfo = new SQLColumnInfo();
	ctx.selectInfo.columns.push_back(columnInfo);
	columnInfo->name = ctx.currWord;
	size_t dotPos = columnInfo->name.find('.');
	if (dotPos == 0) {
		MLPL_DBG("Column name begins from dot. : %s",
		         columnInfo->name.c_str());
		return false;
	}
	if (dotPos == (columnInfo->name.size() - 1)) {
		MLPL_DBG("Column name ends with dot. : %s",
		         columnInfo->name.c_str());
		return false;
	}

	if (dotPos != string::npos) {
		columnInfo->tableVar = string(columnInfo->name, 0, dotPos);
		columnInfo->baseName = string(columnInfo->name, dotPos + 1);
	} else {
		columnInfo->baseName = columnInfo->name;
	}
	return true;
}

bool SQLProcessor::parseGroupBy(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return false;
}

bool SQLProcessor::parseFrom(SelectParserContext &ctx)
{
	bool isTableName = true;
	if (ctx.indexInTheStatus > 0) {
		int commaCount = m_separatorCountSpaceComma.getCount(',');
		if (commaCount == 0)
			isTableName = false;
		else if (commaCount == 1)
			isTableName = true;
		else {
			string msg;
			TRMSG(msg, "commaCount: %d.", commaCount);
			throw logic_error(msg);
		}
		m_separatorCountSpaceComma.resetCounter();
	}

	if (isTableName) {
		SQLTableInfo *tableInfo = new SQLTableInfo();
		ctx.selectInfo.tables.push_back(tableInfo);
		tableInfo->name = ctx.currWord;
		return true;
	}

	if (ctx.selectInfo.tables.empty()) {
		MLPL_DBG("selectInfo.tables is empty\n");
		return false;
	}

	SQLTableInfo *tableInfo = ctx.selectInfo.tables.back();
	tableInfo->varName = ctx.currWord;

	pair<SQLTableVarNameInfoMapIterator, bool> ret =
	  ctx.selectInfo.tableVarInfoMap.insert
	    (pair<string, SQLTableInfo *>(tableInfo->varName, tableInfo));
	if (!ret.second) {
		string msg;
		TRMSG(msg, "Failed to insert: table name: %s, %s.",
		      tableInfo->varName.c_str(),
		      ctx.selectInfo.query.getString());
		throw msg;
	}
	return true;
}

bool SQLProcessor::parseWhere(SelectParserContext &ctx)
{
	MLPL_DBG("parseWhere: %s\n", ctx.currWord.c_str());
	bool doKeywordCheck = true;
	bool currWordString = false;
	if (ctx.quotOpen) {
		ctx.quotOpen = false;
		doKeywordCheck = false;
		currWordString = true;
	}

	// check if this is the keyword
	if (doKeywordCheck) {
		// TODO: implement
	}

	// parse word as the hand
	SQLWhereElement *whereElem = ctx.selectInfo.currWhereElem;
	bool shouldLeftHand = !(whereElem->getLeftHand());

	SQLWhereElement *handElem = NULL;
	if (currWordString)
		handElem = new SQLWhereString(ctx.currWord);
	else if (shouldLeftHand)
		handElem = new SQLWhereColumn(ctx.currWord);
	else if (StringUtils::isDecimal(ctx.currWord))
		handElem = new SQLWhereNumber(atof(ctx.currWord.c_str()));
	else
		handElem = new SQLWhereColumn(ctx.currWord);

	if (shouldLeftHand)
		whereElem->setLeftHand(handElem);
	else if (!whereElem->getRightHand())
		whereElem->setRightHand(handElem);
	else {
		string msg;
		TRMSG(msg, "Both hand of currWhereElem are not NULL.");
		throw logic_error(msg);
	}
	return true;
}

bool SQLProcessor::parseOrderBy(SelectParserContext &ctx)
{
	ctx.selectInfo.orderedColumns.push_back(ctx.currWord);
	return true;
}

bool SQLProcessor::parseSectionLimit(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: Limit\n");
	return true;
}

//
// Callbacks for parsing 'where' section
//
void SQLProcessor::whereCbEq(const char separator, SelectParserContext *ctx)
{
	SQLWhereOperator *opEq = new SQLWhereOperatorEqual();
	ctx->selectInfo.currWhereElem->setOperator(opEq);
}

void SQLProcessor::whereCbQuot(const char separator, SelectParserContext *ctx)
{
	SeparatorCheckerWithCallback &separatorCBForWhere
	  = ctx->sqlProcessor->m_separatorCBForWhere;
	if (ctx->quotOpen) {
		separatorCBForWhere.unsetAlternative();
		return;
	}
	ctx->quotOpen = true;
	separatorCBForWhere.setAlternative(&ParsableString::SEPARATOR_QUOT);
}

string SQLProcessor::readNextWord(SelectParserContext &ctx,
                                  ParsingPosition *position)
{
	SeparatorChecker *separator = m_selectSeprators[ctx.section];
	if (position)
		*position = ctx.selectInfo.query.getParsingPosition();
	return ctx.selectInfo.query.readWord(*separator);
}

