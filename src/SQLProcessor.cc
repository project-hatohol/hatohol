#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <cstring>
#include "SQLProcessor.h"

static const int UNKNOWN_TABLE_ID = -1;

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
};

struct SQLProcessor::SelectParserContext {
	string             currWord;
	SelectParseRegion  region;
	size_t             indexInTheStatus;
	SQLSelectInfo     &selectInfo;

	// methods
	SelectParserContext(SelectParseRegion _region,
	                    SQLSelectInfo &_selectInfo)
	: region(_region),
	  indexInTheStatus(0),
	  selectInfo(_selectInfo)
	{
	}
};

struct SQLProcessor::AddItemGroupArg {
	SQLSelectInfo &selectInfo;
	SQLTableInfo &tableInfo;
	const ItemTablePtr &itemTablePtr;
};

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
: tableInfo(NULL)
{
}

void SQLColumnInfo::associate(SQLTableInfo *_tableInfo)
{
	tableInfo = _tableInfo;
	_tableInfo->columnList.push_back(this);
}

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(ParsableString &_query)
: query(_query),
  whereElem(NULL),
  useIndex(false)
{
}


SQLSelectInfo::~SQLSelectInfo()
{
	if (whereElem)
		delete whereElem;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
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
	if (!makeItemTables(selectInfo))
		return false;

	// join tables
	ItemTablePtrListConstIterator it = selectInfo.itemTablePtrList.begin();
	for (; it != selectInfo.itemTablePtrList.end(); ++it) {
		const ItemTablePtr &table = *it;
		selectInfo.joinedTable = selectInfo.joinedTable->join(table);
	}

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
  m_tableNameStaticInfoMap(tableNameStaticInfoMap)
{
	m_selectSeprators[SQLProcessor::SELECT_PARSING_REGION_SELECT] =
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_REGION_GROUP_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_REGION_FROM] =
	  &m_separatorCountSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_REGION_WHERE] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_REGION_ORDER_BY] = 
	  &m_separatorSpaceComma;

	m_selectRegionParserMap["from"]  = &SQLProcessor::parseRegionFrom;
	m_selectRegionParserMap["where"] = &SQLProcessor::parseRegionWhere;
	m_selectRegionParserMap["order"] = &SQLProcessor::parseRegionOrder;
	m_selectRegionParserMap["group"] = &SQLProcessor::parseRegionGroup;
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
	SelectParserContext ctx(SELECT_PARSING_REGION_SELECT, selectInfo);

	while (!selectInfo.query.finished()) {
		ctx.currWord = readNextWord(ctx);
		if (ctx.currWord.empty())
			continue;

		// check if this is a keyword.
		string lowerWord = ctx.currWord;
		transform(lowerWord.begin(), lowerWord.end(),
		          lowerWord.begin(), ::tolower);
		
		it = m_selectRegionParserMap.find(lowerWord);
		if (it != m_selectRegionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is region keyword and
			subParser = it->second;
			if ((this->*subParser)(ctx)) {
				ctx.indexInTheStatus = 0;
				continue;
			}
		}

		// parse each component
		if (ctx.region >= NUM_SELECT_PARSING_REGION) {
			MLPL_BUG("region(%d) >= NUM_SELECT_PARSING_REGION\n",
			         ctx.region);
			return false;
		}
		subParser = m_selectSubParsers[ctx.region];
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
	for (size_t i = 0; i < selectInfo.columns.size(); i++) {
		SQLColumnInfo &columnInfo = selectInfo.columns[i];

		if (selectInfo.tables.size() == 1) {
			SQLTableInfo *tableInfo = &selectInfo.tables[0];
			if (columnInfo.tableVar.empty())
				columnInfo.associate(tableInfo);
			else if (columnInfo.tableVar == tableInfo->varName)
				columnInfo.associate(tableInfo);
			else {
				MLPL_DBG("columnInfo.tableVar (%s) != "
				         "tableInfo.varName (%s)\n",
				         columnInfo.tableVar.c_str(),
				         tableInfo->varName.c_str());
				return false;
			}
			continue;
		}

		map<string, const SQLTableInfo *>::iterator it;
		it = selectInfo.tableMap.find(columnInfo.tableVar);
		if (it == selectInfo.tableMap.end()) {
			MLPL_DBG("Failed to find: %s (%s)\n",
			         columnInfo.tableVar.c_str(),
			         columnInfo.name.c_str());
			return false;
		}
		columnInfo.associate(const_cast<SQLTableInfo *>(it->second));
	}
	return true;
}

bool SQLProcessor::associateTableWithStaticInfo(SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.tables.size(); i++) {
		SQLTableInfo &tableInfo = selectInfo.tables[i];
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableInfo.name);
		if (it == m_tableNameStaticInfoMap.end()) {
			MLPL_DBG("Not found table: %s\n",
			         tableInfo.name.c_str());
			return false;
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo.staticInfo = staticInfo;
	}
	return true;
}

bool SQLProcessor::makeColumnDefs(SQLSelectInfo &selectInfo)
{
	/*
	for (size_t i = 0; i < selectInfo.columns.size(); i++) {
		SQLColumnInfo &columnInfo = selectInfo.columns[i];
		if (checkSelectedAllColumns(selectInfo, columnInfo))
			addAllColumnDefs(selectInfo, tableInfo, tableId);
		else {
			MLPL_BUG("Not implemented: indivisual columns\n");
			return false;
		}
	}*/
	return false;
}

void SQLProcessor::addColumnDefs(SQLSelectInfo &selectInfo,
                                 const ColumnBaseDefinition &columnBaseDef,
                                 const SQLColumnInfo &columnInfo)
{
	/*
	selectInfo.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = selectInfo.columnDefs.back();
	colDef.baseDef      = &columnBaseDef;
	colDef.schema       = getDBName();
	colDef.table        = columnInfo.table;
	colDef.tableVar     = columnInfo.tableVar;
	colDef.column       = columnBaseDef.columnName;
	colDef.columnVar    = columnBaseDef.columnName;
	*/
}

void SQLProcessor::addAllColumnDefs(SQLSelectInfo &selectInfo,
                                    const SQLTableInfo &tableInfo, int tableId)
{
	/*
	TableIdColumnBaseDefListMap::iterator it;
	it = m_tableColumnBaseDefListMap.find(tableId);
	if (it == m_tableColumnBaseDefListMap.end()) {
		MLPL_BUG("Not found table: %d\n", tableId);
		return;
	}

	ColumnBaseDefList &baseDefList = it->second;;
	ColumnBaseDefListIterator baseDef = baseDefList.begin();
	for (; baseDef != baseDefList.end(); ++baseDef)
		addColumnDefs(selectInfo, *baseDef, columnInfo);
		*/
}

bool SQLProcessor::makeItemTables(SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.tables.size(); i++) {
		SQLTableInfo &tableInfo = selectInfo.tables[i];
		SQLTableMakeFunc func = tableInfo.staticInfo->tableMakeFunc;
		if (!(this->*func)(selectInfo, tableInfo))
			return false;
	}
	return true;
}

bool SQLProcessor::addItemGroup(const ItemGroup *itemGroup,
                                AddItemGroupArg &arg)
{
	list<const SQLColumnInfo *>::iterator it = 
	  arg.tableInfo.columnList.begin();

	for (; it != arg.tableInfo.columnList.end(); ++it) {
	}

	for (size_t i = 0; i < arg.tableInfo.columnList.size(); i++) {
	/*
		const SQLColumnDefinition &colDef = selectInfo.columnDefs[i];
		const ItemData *item =
		  itemGroup->getItem(colDef.baseDef->itemId);
		if (!item) {
			MLPL_BUG("Failed to get ItemData: %"PRIu_ITEM
			         " from ItemGroup: %"PRIu_ITEM_GROUP"\n",
			         colDef.baseDef->itemId,
			         itemGroup->getItemGroupId());
			return false;
		}
		selectInfo.textRows.push_back(item->getString());
		*/
	}
	return true;
}

// This function is typically called via makeItemTables().
// The dirct caller will be implemented in sub classes.
bool SQLProcessor::makeTable(SQLSelectInfo &selectInfo,
                             SQLTableInfo &tableInfo,
                             const ItemTablePtr itemTablePtr)
{
	/*
	tableInfo.tableId = tableId;

	selectInfo.itemTablePtrList.push_back(ItemTablePtr());
	ItemTablePtr &itemTablePtr = selectInfo.itemTablePtrList.back();

	AddItemGroupArg arg(selectInfo, tableInfo, itemTablePtr);
	return itemTable->foreach<AddItemTableArg&>(addItemGroup, arg);
	*/
	return false;
}

bool SQLProcessor::pickupMatchingRows(const ItemGroup *itemGroup,
                                      SQLSelectInfo &selectInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	selectInfo.selectedTable = selectInfo.joinedTable;
	return true;
}

bool SQLProcessor::makeTextRows(const ItemGroup *itemGroup,
                                SQLSelectInfo &selectInfo)
{
	for (size_t i = 0; i < selectInfo.columnDefs.size(); i++) {
		const SQLColumnDefinition &colDef = selectInfo.columnDefs[i];
		const ItemData *item =
		  itemGroup->getItem(colDef.baseDef->itemId);
		if (!item) {
			MLPL_BUG("Failed to get ItemData: %"PRIu_ITEM
			         " from ItemGroup: %"PRIu_ITEM_GROUP"\n",
			         colDef.baseDef->itemId,
			         itemGroup->getItemGroupId());
			return false;
		}
		selectInfo.textRows.push_back(item->getString());
	}
	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseRegionFrom(SelectParserContext &ctx)
{
	ctx.region = SELECT_PARSING_REGION_FROM;
	m_separatorCountSpaceComma.resetCounter();
	return true;
}

bool SQLProcessor::parseRegionWhere(SelectParserContext &ctx)
{
	ctx.region = SELECT_PARSING_REGION_WHERE;
	return true;
}

bool SQLProcessor::parseRegionOrder(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.region = SELECT_PARSING_REGION_ORDER_BY;
	return true;
}

bool SQLProcessor::parseRegionGroup(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.query.setParsingPosition(currPos);
		return false;
	}

	ctx.region = SELECT_PARSING_REGION_GROUP_BY;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	ctx.selectInfo.columns.push_back(SQLColumnInfo());
	SQLColumnInfo &columnInfo = ctx.selectInfo.columns.back();
	columnInfo.name = ctx.currWord;
	size_t dotPos = columnInfo.name.find('.');
	if (dotPos == 0) {
		MLPL_DBG("Column name begins from dot. : %s",
		         columnInfo.name.c_str());
		return false;
	}
	if (dotPos == (columnInfo.name.size() - 1)) {
		MLPL_DBG("Column name ends with dot. : %s",
		         columnInfo.name.c_str());
		return false;
	}

	if (dotPos != string::npos) {
		columnInfo.tableVar = string(columnInfo.name, 0, dotPos);
		columnInfo.baseName = string(columnInfo.name, dotPos + 1);
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
			MLPL_DBG("commaCount: %d\n", commaCount);
			return false;
		}
	}

	if (isTableName) {
		ctx.selectInfo.tables.push_back(SQLTableInfo());
		SQLTableInfo &tableInfo = ctx.selectInfo.tables.back();
		tableInfo.name = ctx.currWord;
		return true;
	}

	if (ctx.selectInfo.tables.empty()) {
		MLPL_DBG("selectInfo.tables is empty\n");
		return false;
	}

	SQLTableInfo &tableInfo = ctx.selectInfo.tables.back();
	tableInfo.varName = ctx.currWord;
	return true;
}

bool SQLProcessor::parseWhere(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: WHERE\n");
	//return false;
	return true;
}

bool SQLProcessor::parseOrderBy(SelectParserContext &ctx)
{
	ctx.selectInfo.orderedColumns.push_back(ctx.currWord);
	return true;
}

string SQLProcessor::readNextWord(SelectParserContext &ctx,
                                  ParsingPosition *position)
{
	SeparatorChecker *separator = m_selectSeprators[ctx.region];
	if (position)
		*position = ctx.selectInfo.query.getParsingPosition();
	return ctx.selectInfo.query.readWord(*separator);
}
