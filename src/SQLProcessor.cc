#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include "SQLProcessor.h"

enum SelectParserStatus {
	SELECT_PARSER_STATUS_SELECT,
	SELECT_PARSER_STATUS_GROUP_BY,
	SELECT_PARSER_STATUS_FROM,
	SELECT_PARSER_STATUS_WHERE,
	SELECT_PARSER_STATUS_ORDER_BY,
	NUM_SELECT_PARSER_STATUS,
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

	size_t indexInTheStatus = 0;
	SelectParserStatus status = SELECT_PARSER_STATUS_SELECT;
	for (size_t i = 1; i < numWords; i++) {
		string &word = words[i];

		// check the keywords.
		if (StringUtils::casecmp(word, "from")) {
			status = SELECT_PARSER_STATUS_FROM;
			indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(word, "where")) {
			status = SELECT_PARSER_STATUS_WHERE;
			indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(word, "order")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					status = SELECT_PARSER_STATUS_ORDER_BY;
					indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}
		else if (StringUtils::casecmp(word, "group")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					status = SELECT_PARSER_STATUS_GROUP_BY;
					indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}

		// paser each component
		if (status >= NUM_SELECT_PARSER_STATUS) {
			MLPL_BUG("status(%d) >= NUM_SELECT_PARSER_STATUS\n",
			         status);
			return false;
		}
		const SelectSubParser subParser = m_selectSubParsers[status];
		i = (this->*subParser)(i, indexInTheStatus, selectStruct, words);

		indexInTheStatus++;
	}

	// check the results
	if (selectStruct.selectedColumns.empty()) {
		return false;
	}

	return false;
}

//
// Select statment parsers
//
size_t
SQLProcessor::parseSelectedColumns(size_t idx, size_t indexInTheStatus,
                                   SQLSelectStruct &selectStruct,
                                   vector<string> &words)
{
	selectStruct.selectedColumns.push_back(words[idx]);
	return idx;
}

size_t SQLProcessor::parseGroupBy(size_t idx, size_t indexInTheStatus,
                                  SQLSelectStruct &selectStruct,
                                  vector<string> &words)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return idx;
}

size_t SQLProcessor::parseFrom(size_t idx, size_t indexInTheStatus,
                               SQLSelectStruct &selectStruct,
                               vector<string> &words)
{
	string &word = words[idx];
	if (indexInTheStatus == 0)
		selectStruct.table = word;
	else if (indexInTheStatus == 1)
		selectStruct.tableVar = word;
	else {
		// TODO: return error?
	}
	return idx;
}

size_t SQLProcessor::parseWhere(size_t idx, size_t indexInTheStatus,
                                SQLSelectStruct &selectStruct,
                                vector<string> &words)
{
	MLPL_BUG("Not implemented: WHERE\n");
	return idx;
}

size_t SQLProcessor::parseOrderBy(size_t idx, size_t indexInTheStatus,
                                  SQLSelectStruct &selectStruct,
                                  vector<string> &words)
{
	selectStruct.orderedColumns.push_back(words[idx]);
	return idx;
}
