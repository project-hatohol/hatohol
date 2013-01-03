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

	int indexInTheStatus = 0;
	SelectParserStatus status = SELECT_PARSER_STATUS_SELECT;
	for (size_t i = 1; i < numWords; i++, indexInTheStatus++) {
		string &word = words[i];
		MLPL_DBG("[%zd] %s\n", i, word.c_str());

		// check if the word is keyword.
		if (StringUtils::casecmp(words[i], "from")) {
			status = SELECT_PARSER_STATUS_FROM;
			indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(words[i], "where")) {
			status = SELECT_PARSER_STATUS_WHERE;
			indexInTheStatus = 0;
			continue;
		} else if (StringUtils::casecmp(words[i], "order")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					status = SELECT_PARSER_STATUS_ORDER_BY;
					indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}
		else if (StringUtils::casecmp(words[i], "group")) {
			if (i != numWords - 1) {
				if (StringUtils::casecmp(words[i+1], "by")) {
					status = SELECT_PARSER_STATUS_GROUP_BY;
					indexInTheStatus = 0;
					i++;
					continue;
				}
			}
		}

		if (status == SELECT_PARSER_STATUS_SELECT) {
			// TODO: support ','comma separation
			selectStruct.selectRows.push_back(word);
		} else if (status == SELECT_PARSER_STATUS_FROM) {
			if (indexInTheStatus == 0)
				selectStruct.table = word;
			else if (indexInTheStatus == 1)
				selectStruct.tableVar = word;
			else {
				// TODO: return error?
			}
		} else if (status == SELECT_PARSER_STATUS_WHERE) {
		} else if (status == SELECT_PARSER_STATUS_ORDER_BY) {
		} else if (status == SELECT_PARSER_STATUS_GROUP_BY) {
		}
	}

	// check the results
	if (selectStruct.selectRows.empty()) {
		return false;
	}

	return false;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
