#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <cstring>
#include "SQLProcessor.h"

enum SelectParseRegion {
	SELECT_PARSE_REGION_SELECT,
	SELECT_PARSE_REGION_GROUP_BY,
	SELECT_PARSE_REGION_FROM,
	SELECT_PARSE_REGION_WHERE,
	SELECT_PARSE_REGION_ORDER_BY,
	NUM_SELECT_PARSE_REGION,
};

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
};

struct SQLProcessor::SelectParserContext {
	size_t             idx;
	vector<string>    &words;
	size_t             numWords;
	const char        *currWord;
	SelectParseRegion  region;
	size_t             indexInTheStatus;
	SQLSelectStruct   &selectStruct;
};

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
bool SQLProcessor::selectedAllColumns(SQLSelectStruct &selectStruct)
{
	if (selectStruct.columns.size() != 1)
		return false;

	string &column = selectStruct.columns[0];
	if (column == "*")
		return true;

	if (selectStruct.tableVar.empty())
		return false;

	size_t len = selectStruct.tableVar.size();

	static const char DOT_ASTERISK[] = ".*";
	static const size_t LEN_DOT_ASTERISK = sizeof(DOT_ASTERISK) - 1;
	if (column.size() != len + LEN_DOT_ASTERISK)
		return false;
	if (strncmp(column.c_str(), selectStruct.tableVar.c_str(), len) != 0)
		return false;
	if (strncmp(&column.c_str()[len], DOT_ASTERISK, LEN_DOT_ASTERISK) == 0)
		return true;
	return false;
}

bool SQLProcessor::parseSelectStatement(SQLSelectStruct &selectStruct,
                                        vector<string> &words)
{
	MLPL_DBG("**** %s\n", __func__);
	size_t numWords = words.size();
	if (numWords < 2)
		return false;
	if (!StringUtils::casecmp(words[0], "select")) {
		MLPL_BUG("First word is NOT 'select': %s\n", words[0].c_str());
		return false;
	}

	map<string, SelectSubParser>::iterator it;
	SelectSubParser subParser = NULL;
	SelectParserContext ctx =
	  {0, words, numWords, NULL, // currWord
	   SELECT_PARSE_REGION_SELECT, 0, selectStruct};

	for (ctx.idx = 1; ctx.idx < numWords; ctx.idx++) {
		string &currWord = words[ctx.idx];
		ctx.currWord = currWord.c_str();

		// check if this is a keyword.
		string lowerWord = currWord;
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
		if (ctx.region >= NUM_SELECT_PARSE_REGION) {
			MLPL_BUG("region(%d) >= NUM_SELECT_PARSE_REGION\n",
			         ctx.region);
			return false;
		}
		subParser = m_selectSubParsers[ctx.region];
		if (!(this->*subParser)(ctx))
			return false;

		ctx.indexInTheStatus++;
	}

	// check the results
	if (selectStruct.columns.empty()) {
		return false;
	}

	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseRegionFrom(SelectParserContext &ctx)
{
	ctx.region = SELECT_PARSE_REGION_FROM;
	return true;
}

bool SQLProcessor::parseRegionWhere(SelectParserContext &ctx)
{
	ctx.region = SELECT_PARSE_REGION_WHERE;
	return true;
}

bool SQLProcessor::parseRegionOrder(SelectParserContext &ctx)
{
	if (ctx.idx == ctx.numWords - 1)
		return false;

	if (!StringUtils::casecmp(ctx.words[ctx.idx+1], "by"))
		return false;

	ctx.region = SELECT_PARSE_REGION_ORDER_BY;
	ctx.idx++;
	return true;
}

bool SQLProcessor::parseRegionGroup(SelectParserContext &ctx)
{
	if (ctx.idx == ctx.numWords - 1)
		return false;

	if (!StringUtils::casecmp(ctx.words[ctx.idx+1], "by"))
		return false;

	ctx.region = SELECT_PARSE_REGION_GROUP_BY;
	ctx.idx++;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	ctx.selectStruct.columns.push_back(ctx.currWord);
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
		ctx.selectStruct.table = ctx.currWord;
	else if (ctx.indexInTheStatus == 1)
		ctx.selectStruct.tableVar = ctx.currWord;
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
	ctx.selectStruct.orderedColumns.push_back(ctx.currWord);
	return true;
}
