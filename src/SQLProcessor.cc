#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <cstring>
#include "SQLProcessor.h"

enum SelectParseRegion {
	SELECT_PARSING_REGION_SELECT,
	SELECT_PARSING_REGION_GROUP_BY,
	SELECT_PARSING_REGION_FROM,
	SELECT_PARSING_REGION_WHERE,
	SELECT_PARSING_REGION_ORDER_BY,
	NUM_SELECT_PARSING_REGION,
};

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
};

SeparatorChecker SQLProcessor::m_separatorSpaceComma(" ,");
SeparatorChecker *SQLProcessor::m_selectSeprators[] = {
	&SQLProcessor::m_separatorSpaceComma, // select
	&SQLProcessor::m_separatorSpaceComma, // group by
	&SQLProcessor::m_separatorSpaceComma, // from
	&SQLProcessor::m_separatorSpaceComma, // where
	&SQLProcessor::m_separatorSpaceComma, // order by
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
	  selectInfo(_selectInfo)
	{
	}

};

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(ParsableString &_query)
: query(_query),
  whereElem(NULL)
{
}


SQLSelectInfo::~SQLSelectInfo()
{
	if (whereElem)
		delete whereElem;
}

// ---------------------------------------------------------------------------
// Public methods (SQLColumnResult)
// ---------------------------------------------------------------------------
SQLSelectResult::SQLSelectResult(void)
: useIndex(false)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
bool SQLProcessor::select(SQLSelectResult &result,
                          SQLSelectInfo   &selectInfo)
{
	// disassemble the query.
	if (!parseSelectStatement(selectInfo))
		return false;

	// gather data
	for (size_t i = 0; i < selectInfo.columnInfo.size(); i++) {
		const SQLColumnInfo &columnInfo = selectInfo.columnInfo[i];
		map<string, TableProcFunc>::iterator it;
		it = m_tableProcFuncMap.find(columnInfo.table);
		if (it == m_tableProcFuncMap.end())
			return false;
		TableProcFunc func = it->second;
		if (!(this->*func)(result, selectInfo, columnInfo))
			return false;
	}

	// make big one table

	// pickup matching rows

	// convert data to string

	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessor::SQLProcessor(
  TableProcFuncMap            &tableProcFuncMap,
  TableIdColumnBaseDefListMap &tableColumnBaseDefListMap,
  TableIdNameMap              &tableIdNameMap)
: m_tableProcFuncMap(tableProcFuncMap),
  m_tableColumnBaseDefListMap(tableColumnBaseDefListMap),
  m_tableIdNameMap(tableIdNameMap)
{
	m_selectRegionParserMap["from"]  = &SQLProcessor::parseRegionFrom;
	m_selectRegionParserMap["where"] = &SQLProcessor::parseRegionWhere;
	m_selectRegionParserMap["order"] = &SQLProcessor::parseRegionOrder;
	m_selectRegionParserMap["group"] = &SQLProcessor::parseRegionGroup;
}

SQLProcessor::~SQLProcessor()
{
}

bool SQLProcessor::selectedAllColumns(const SQLColumnInfo &columnInfo)
{
	if (columnInfo.names.size() != 1)
		return false;

	const string &columnName = columnInfo.names[0];
	if (columnName == "*")
		return true;

	if (columnInfo.tableVar.empty())
		return false;

	size_t len = columnInfo.tableVar.size();

	static const char DOT_ASTERISK[] = ".*";
	static const size_t LEN_DOT_ASTERISK = sizeof(DOT_ASTERISK) - 1;
	if (columnName.size() != len + LEN_DOT_ASTERISK)
		return false;
	const char *columnNameCStr = columnName.c_str();
	if (strncmp(columnNameCStr, columnInfo.tableVar.c_str(), len) != 0)
		return false;
	if (strncmp(&columnNameCStr[len], DOT_ASTERISK, LEN_DOT_ASTERISK) == 0)
		return true;
	return false;
}

bool SQLProcessor::parseSelectStatement(SQLSelectInfo &selectInfo)
{
	MLPL_DBG("**** %s\n", __func__);
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

	// check the results
	if (selectInfo.columnInfo.empty()) {
		return false;
	}

	return true;
}

void SQLProcessor::addColumnDefs(SQLSelectResult &result,
                                 const ColumnBaseDefinition &columnBaseDef,
                                 const SQLColumnInfo &columnInfo)
{
	result.columnDefs.push_back(SQLColumnDefinition());
	SQLColumnDefinition &colDef = result.columnDefs.back();
	colDef.baseDef      = &columnBaseDef;
	colDef.schema       = getDBName();
	colDef.table        = columnInfo.table;
	colDef.tableVar     = columnInfo.tableVar;
	colDef.column       = columnBaseDef.columnName;
	colDef.columnVar    = columnBaseDef.columnName;
}

void SQLProcessor::addAllColumnDefs(SQLSelectResult &result, int tableId,
                                    const SQLColumnInfo &columnInfo)
{
	TableIdColumnBaseDefListMap::iterator it;
	it = m_tableColumnBaseDefListMap.find(tableId);
	if (it == m_tableColumnBaseDefListMap.end()) {
		MLPL_BUG("Not found table: %d\n", tableId);
		return;
	}

	ColumnBaseDefList &baseDefList = it->second;;
	ColumnBaseDefListIterator baseDef = baseDefList.begin();
	for (; baseDef != baseDefList.end(); ++baseDef)
		addColumnDefs(result, *baseDef, columnInfo);
}

bool SQLProcessor::setSelectResult(const ItemGroup *itemGroup,
                                   SQLSelectResult &result)
{
	for (size_t i = 0; i < result.columnDefs.size(); i++) {
		const SQLColumnDefinition &colDef = result.columnDefs[i];
		const ItemData *item =
		  itemGroup->getItem(colDef.baseDef->itemId);
		if (!item) {
			MLPL_BUG("Failed to get ItemData: %"PRIu_ITEM
			         " from ItemGroup: %"PRIu_ITEM_GROUP"\n",
			         colDef.baseDef->itemId,
			         itemGroup->getItemGroupId());
			return false;
		}
		result.textRows.push_back(item->getString());
	}
	return true;
}

bool
SQLProcessor::generalSelect(SQLSelectResult &result, SQLSelectInfo &selectInfo,
                            const SQLColumnInfo &columnInfo,
                            const ItemTable *itemTable, int tableId)
{
	if (columnInfo.names.empty())
		return false;

	// Set column definition
	if (selectedAllColumns(columnInfo)) {
		addAllColumnDefs(result, tableId, columnInfo);
	} else {
		MLPL_BUG("Not implemented: indivisual columns\n");
		return false;
	}

	return itemTable->foreach<SQLSelectResult&>(setSelectResult, result);
}

//bool SQLProcessor::makeTextRowsForeach(const ItemGroup *SQLSelectResult &result)

/*
bool SQLProcessor::makeTextRows(SQLSelectResult &result)
{
	return result->selectedTable->foreach(bool (*func)(const ItemGroup *, T arg), T arg) const
}*/

//
// Select status parsers
//
bool SQLProcessor::parseRegionFrom(SelectParserContext &ctx)
{
	ctx.region = SELECT_PARSING_REGION_FROM;
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
	ctx.selectInfo.columns.push_back(ctx.currWord);
	return true;
}

bool SQLProcessor::parseGroupBy(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return false;
}

bool SQLProcessor::parseFrom(SelectParserContext &ctx)
{
	if (ctx.indexInTheStatus == 0)
		ctx.selectInfo.table = ctx.currWord;
	else if (ctx.indexInTheStatus == 1)
		ctx.selectInfo.tableVar = ctx.currWord;
	else {
		// TODO: return error?
	}
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
	SeparatorChecker &separator = *(m_selectSeprators[ctx.region]);
	if (position)
		*position = ctx.selectInfo.query.getParsingPosition();
	return ctx.selectInfo.query.readWord(separator);
}
