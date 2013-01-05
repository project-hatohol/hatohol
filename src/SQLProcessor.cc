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
SQLProcessor::SQLProcessor(void)
{
	m_selectRegionParserMap["from"]  = &SQLProcessor::parseRegionFrom;
	m_selectRegionParserMap["where"] = &SQLProcessor::parseRegionWhere;
	m_selectRegionParserMap["order"] = &SQLProcessor::parseRegionOrder;
	m_selectRegionParserMap["group"] = &SQLProcessor::parseRegionGroup;
}

SQLProcessor::~SQLProcessor()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool SQLProcessor::selectedAllColumns(SQLSelectInfo &selectInfo)
{
	if (selectInfo.columns.size() != 1)
		return false;

	string &column = selectInfo.columns[0];
	if (column == "*")
		return true;

	if (selectInfo.tableVar.empty())
		return false;

	size_t len = selectInfo.tableVar.size();

	static const char DOT_ASTERISK[] = ".*";
	static const size_t LEN_DOT_ASTERISK = sizeof(DOT_ASTERISK) - 1;
	if (column.size() != len + LEN_DOT_ASTERISK)
		return false;
	if (strncmp(column.c_str(), selectInfo.tableVar.c_str(), len) != 0)
		return false;
	if (strncmp(&column.c_str()[len], DOT_ASTERISK, LEN_DOT_ASTERISK) == 0)
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
	if (selectInfo.columns.empty()) {
		return false;
	}

	return true;
}

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
