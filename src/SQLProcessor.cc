#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include "SQLProcessor.h"

enum SelectParserStatus {
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

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLProcessor::SQLProcessor(void)
{
}

SQLProcessor::~SQLProcessor()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool SQLProcessor::parseSelectStatement(SQLSelectStruct &selectStruct,
                                        vector<string> &words)
{
	MLPL_DBG("**** %s\n", __func__);
	size_t numWords = words.size();
	if (numWords < 2)
		return false;
	if (!StringUtils::casecmp(words[0], "select")) {
		MLPL_BUG("First word is NOT select: %s\n", words[0].c_str());
		return false;
	}

	SelectParserContext ctx = {0, words, numWords,
	                           NULL, // currWord
	                           SELECT_PARSE_REGION_SELECT,
	                           0, selectStruct};
	for (size_t i = 1; i < numWords; i++) {
		ctx.currWord = words[i].c_str();

		// check the keywords.
		if (StringUtils::casecmp(ctx.currWord, "from")) {
			ctx.region = SELECT_PARSE_REGION_FROM;
			ctx.indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(ctx.currWord, "where")) {
			ctx.region = SELECT_PARSE_REGION_WHERE;
			ctx.indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(ctx.currWord, "order")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					ctx.region = SELECT_PARSE_REGION_ORDER_BY;
					ctx.indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}
		else if (StringUtils::casecmp(ctx.currWord, "group")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					ctx.region = SELECT_PARSE_REGION_GROUP_BY;
					ctx.indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}

		// paser each component
		if (ctx.region >= NUM_SELECT_PARSE_REGION) {
			MLPL_BUG("region(%d) >= NUM_SELECT_PARSE_REGION\n",
			         ctx.region);
			return false;
		}
		const SelectSubParser subParser =
		  m_selectSubParsers[ctx.region];
		if (!(this->*subParser)(ctx))
			return false;

		ctx.indexInTheStatus++;
	}

	// check the results
	if (selectStruct.selectedColumns.empty()) {
		return false;
	}

	return false;
}

//
// Select status parsers
//

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	ctx.selectStruct.selectedColumns.push_back(ctx.currWord);
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
	return false;
}

bool SQLProcessor::parseOrderBy(SelectParserContext &ctx)
{
	ctx.selectStruct.orderedColumns.push_back(ctx.currWord);
	return true;
}
