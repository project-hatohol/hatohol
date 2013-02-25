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

#include <stdexcept>
using namespace std;

#include "SQLProcessorInsert.h"
#include "Utils.h"
#include "SQLProcessorException.h"
#include "ItemGroupPtr.h"
#include "ItemTablePtr.h"
#include "SQLUtils.h"

enum ExpectedParenthesisType {
	EXPECTED_PARENTHESIS_NONE,
	EXPECTED_PARENTHESIS_OPEN_COLUMN,
	EXPECTED_PARENTHESIS_CLOSE_COLUMN,
	EXPECTED_PARENTHESIS_OPEN_VALUE,
	EXPECTED_PARENTHESIS_CLOSE_VALUE,
};

typedef map<ItemId, string *>    ItemIdValueMap;
typedef ItemIdValueMap::iterator ItemIdValueMapIterator;

struct SQLProcessorInsert::PrivateContext {
	SQLInsertInfo     *insertInfo;
	string             currWord;
	string             currWordLower;
	InsertParseSection section;
	bool               errorFlag;
	ExpectedParenthesisType expectedParenthesis;
	string             pendingWord;
	bool               openQuot;
	const SQLTableStaticInfo *tableStaticInfo;
	ItemIdValueMap     itemIdValueMap;

	// constructor
	PrivateContext(void)
	: insertInfo(NULL),
	  section(INSERT_PARSING_SECTION_INSERT),
	  errorFlag(false),
	  expectedParenthesis(EXPECTED_PARENTHESIS_NONE),
	  openQuot(false),
	  tableStaticInfo(NULL)
	{
	}
};

const SQLProcessorInsert::InsertSubParser
SQLProcessorInsert::m_insertSubParsers[] = {
	&SQLProcessorInsert::parseInsert,
	&SQLProcessorInsert::parseInto,
	&SQLProcessorInsert::parseTable,
	&SQLProcessorInsert::parseColumn,
	&SQLProcessorInsert::parseValuesKeyword,
	&SQLProcessorInsert::parseValue,
	&SQLProcessorInsert::parseEnd,
};

// ---------------------------------------------------------------------------
// Public methods (SQLInsertInfo)
// ---------------------------------------------------------------------------
SQLInsertInfo::SQLInsertInfo(ParsableString &_statement)
: SQLProcessorInfo(_statement)
{
}

SQLInsertInfo::~SQLInsertInfo()
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorInsert::init(void)
{
	// check the size of m_insertSubParsers
	size_t size = sizeof(SQLProcessorInsert::m_insertSubParsers) / 
	                sizeof(InsertSubParser);
	if (size != NUM_INSERT_PARSING_SECTION) {
		string msg;
		TRMSG(msg, "sizeof(m_insertSubParsers) is invalid: "
		           "(expcect/actual: %d/%d).",
		      NUM_INSERT_PARSING_SECTION, size);
		throw logic_error(msg);
	}
}

SQLProcessorInsert::SQLProcessorInsert
  (TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_tableNameStaticInfoMap(tableNameStaticInfoMap),
  m_ctx(NULL),
  m_separator(" (),\'")
{
	m_ctx = new PrivateContext();

	m_separator.setCallbackTempl<SQLProcessorInsert>
	  ('(', _separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  (')', _separatorCbParenthesisClose, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  (',', _separatorCbComma, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  ('\'', _separatorCbQuot, this);
}

SQLProcessorInsert::~SQLProcessorInsert()
{
	if (m_ctx)
		delete m_ctx;
}

bool SQLProcessorInsert::insert(SQLInsertInfo &insertInfo)
{
	try {
		if (!parseInsertStatement(insertInfo))
			return false;
		checkTableAndColumns(insertInfo);
		makeColumnDefValueMap(insertInfo);
		doInsetToTable(insertInfo);
	} catch (const SQLProcessorException &e) {
		const char *message = e.what();
		insertInfo.errorMessage = message;
		MLPL_DBG("Got SQLProcessorException: %s\n", message);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool SQLProcessorInsert::parseInsertStatement(SQLInsertInfo &insertInfo)
{
	m_ctx->insertInfo = &insertInfo;
	while (!insertInfo.statement.finished() || m_ctx->errorFlag) {
		m_ctx->currWord = insertInfo.statement.readWord(m_separator);
		if (m_ctx->errorFlag)
			return false;
		if (m_ctx->currWord.empty())
			continue;
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		
		// parse each component
		if (m_ctx->section >= NUM_INSERT_PARSING_SECTION) {
			MLPL_BUG("section(%d) >= NUM_INSERT_PARSING_SECTION\n",
			         m_ctx->section);
			return false;
		}
		InsertSubParser subParser = m_insertSubParsers[m_ctx->section];
		if (!(this->*subParser)())
			return false;
	}
	return true;
}

void SQLProcessorInsert::checkTableAndColumns(SQLInsertInfo &insertInfo)
{
	TableNameStaticInfoMapIterator it =
	  m_tableNameStaticInfoMap.find(insertInfo.table);
	if (it == m_tableNameStaticInfoMap.end()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found: table in m_tableNameStaticInfoMap: %s",
		  insertInfo.table.c_str());
	}
	m_ctx->tableStaticInfo = it->second;

	if (insertInfo.columnVector.size() != insertInfo.valueVector.size()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "The number of column and value is diffrent: %zd, %zd",
		  insertInfo.columnVector.size(),
		  insertInfo.valueVector.size());
	}
}

void SQLProcessorInsert::makeColumnDefValueMap(SQLInsertInfo &insertInfo)
{
	const SQLTableStaticInfo *tableStaticInfo = m_ctx->tableStaticInfo;
	for (size_t i = 0; i < insertInfo.columnVector.size(); i++) {
		string &name = insertInfo.columnVector[i];
		const ColumnDef *columnDef =
		  SQLUtils::getColumnDef(name, tableStaticInfo);
		pair<ItemIdValueMapIterator, bool> ret = 
		  m_ctx->itemIdValueMap.insert(
		    pair<ItemId, string *>
		      (columnDef->itemId, &insertInfo.valueVector[i]));
		if (!ret.second) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to insert '%s' in table: '%s'",
			  name.c_str(), tableStaticInfo->tableName);
		}
	}
}

void SQLProcessorInsert::doInsetToTable(SQLInsertInfo &insertInfo)
{
	ItemDataPtr dataPtr;
	ItemGroupPtr grpPtr(new ItemGroup(), false);
	ItemIdValueMapIterator colValIt;
	ColumnDefListConstIterator it;
	const SQLTableStaticInfo *tableStaticInfo = m_ctx->tableStaticInfo;

	// Make one row
	it = tableStaticInfo->columnDefList.begin();
	for (; it != tableStaticInfo->columnDefList.end(); ++it) {
		const ColumnDef &columnDef = *it;
		colValIt = m_ctx->itemIdValueMap.find(columnDef.itemId);
		if (colValIt == m_ctx->itemIdValueMap.end()) {
			dataPtr = SQLUtils::createDefaultItemData(&columnDef);
		} else {
			dataPtr = SQLUtils::createItemData(&columnDef,
			                                   *colValIt->second);
			m_ctx->itemIdValueMap.erase(colValIt);
		}
		if (!dataPtr.hasData()) {
			const char *valueString = NULL;
			if (colValIt != m_ctx->itemIdValueMap.end())
				valueString = colValIt->second->c_str();
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to create ItemData for value '%s': "
			  "table: %s, column: %s",
			  valueString, columnDef.tableName,
			  columnDef.columnName);
		}
		grpPtr->add(dataPtr);
	}
	grpPtr->freeze();

	// Insert row
	ItemTablePtr tablePtr = (*tableStaticInfo->tableGetFunc)();
	tablePtr->add(grpPtr);
}

//
// Sub parsers
//
bool SQLProcessorInsert::parseInsert(void)
{
	return checkCurrWord("insert", INSERT_PARSING_SECTION_INTO);
}

bool SQLProcessorInsert::parseInto(void)
{
	return checkCurrWord("into", INSERT_PARSING_SECTION_TABLE);
}

bool SQLProcessorInsert::parseTable(void)
{
	m_ctx->insertInfo->table = m_ctx->currWord;
	m_ctx->section = INSERT_PARSING_SECTION_COLUMN;
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_OPEN_COLUMN;
	return true;
}

bool SQLProcessorInsert::parseColumn(void)
{
	if (!m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is not empty: %s.\n",
		          m_ctx->pendingWord.c_str());
		return false;
	}
	m_ctx->pendingWord = m_ctx->currWord;
	return true;
}

bool SQLProcessorInsert::parseValuesKeyword(void)
{
	if (!checkCurrWord("values", INSERT_PARSING_SECTION_VALUE))
		return false;
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_OPEN_VALUE;
	return true;
}

bool SQLProcessorInsert::parseValue(void)
{
	if (!m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is not empty: %s.\n",
		          m_ctx->pendingWord.c_str());
		return false;
	}
	m_ctx->pendingWord = m_ctx->currWord;
	return true;
}

bool SQLProcessorInsert::parseEnd(void)
{
	MLPL_DBG("Parsing had already been done: %s\n",
	         m_ctx->currWord.c_str());
	return false;
}

//
// SeparatorChecker callbacks
//
void SQLProcessorInsert::_separatorCbParenthesisOpen(const char separator,
                                                     SQLProcessorInsert *obj)
{
	obj->separatorCbParenthesisOpen(separator);
}

void SQLProcessorInsert::separatorCbParenthesisOpen(const char separator)
{
	ExpectedParenthesisType expectedType = m_ctx->expectedParenthesis;
	if (expectedType == EXPECTED_PARENTHESIS_OPEN_COLUMN) {
		m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_CLOSE_COLUMN;
	} else  if (expectedType == EXPECTED_PARENTHESIS_OPEN_VALUE) {
		m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_CLOSE_VALUE;
	} else {
		MLPL_DBG("Illegal state: m_ctx->expectedParenthesis: %d\n",
		         expectedType);
		m_ctx->errorFlag = true;
	}
}

void SQLProcessorInsert::_separatorCbParenthesisClose(const char separator,
                                         SQLProcessorInsert *obj)
{
	obj->separatorCbParenthesisClose(separator);
}

void SQLProcessorInsert::separatorCbParenthesisClose(const char separator)
{
	ExpectedParenthesisType expectedType = m_ctx->expectedParenthesis;
	if (expectedType == EXPECTED_PARENTHESIS_CLOSE_COLUMN) {
		if (!pushColumn()) {
			m_ctx->errorFlag = true;
			return;
		}
		m_ctx->section = INSERT_PARSING_SECTION_VALUES_KEYWORD;
	} else if (expectedType == EXPECTED_PARENTHESIS_CLOSE_VALUE) {
		if (!pushValue()) {
			m_ctx->errorFlag = true;
			return;
		}
		m_ctx->section = INSERT_PARSING_SECTION_END;
	} else {
		MLPL_DBG("Illegal state: m_ctx->expectedParenthesis: %d\n",
		         m_ctx->expectedParenthesis);
		m_ctx->errorFlag = true;
	}
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_NONE;
}

void SQLProcessorInsert::_separatorCbComma(const char separator,
                                           SQLProcessorInsert *obj)
{
	obj->separatorCbComma(separator);
}

void SQLProcessorInsert::separatorCbComma(const char separator)
{
	if (m_ctx->section == INSERT_PARSING_SECTION_COLUMN) {
		if (!pushColumn()) {
			m_ctx->errorFlag = true;
			return;
		}
	} else if (m_ctx->section == INSERT_PARSING_SECTION_VALUE) {
		if (!pushValue()) {
			m_ctx->errorFlag = true;
			return;
		}
	} else {
		MLPL_DBG("Illegal section: m_ctx->section: %d\n",
		         m_ctx->section);
		m_ctx->errorFlag = true;
	}
}

void SQLProcessorInsert::_separatorCbQuot(const char separator,
                                          SQLProcessorInsert *obj)
{
	obj->separatorCbQuot(separator);
}

void SQLProcessorInsert::separatorCbQuot(const char separator)
{
	if (!m_ctx->openQuot) {
		m_separator.setAlternative(&ParsableString::SEPARATOR_QUOT);
		m_ctx->openQuot = true;
	} else {
		if (m_ctx->section != INSERT_PARSING_SECTION_VALUE) {
			MLPL_DBG("Quotation is used in section: %d\n",
			         m_ctx->section);
			m_ctx->errorFlag = true;
			return;
		}
		m_separator.unsetAlternative();
		m_ctx->openQuot = false;
	}
}

//
// General sub routines
//
bool SQLProcessorInsert::checkCurrWord(string expected,
                                       InsertParseSection nextSection)
{
	if (m_ctx->currWordLower != expected) {
		MLPL_DBG("currWordLower is not '%s': %s\n",
		         expected.c_str(), m_ctx->currWordLower.c_str());
		return false;
	}
	m_ctx->section = nextSection;
	return true;
}

bool SQLProcessorInsert::pushColumn(void)
{
	if (m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is empty.\n");
		return false;
	}
	m_ctx->insertInfo->columnVector.push_back(m_ctx->pendingWord);
	m_ctx->pendingWord.clear();
	return true;
}

bool SQLProcessorInsert::pushValue(void)
{
	if (m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is empty.\n");
		return false;
	}
	m_ctx->insertInfo->valueVector.push_back(m_ctx->pendingWord);
	m_ctx->pendingWord.clear();
	return true;
}
