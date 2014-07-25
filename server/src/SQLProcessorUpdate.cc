/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Logger.h>
#include <StringUtils.h>
#include "SQLProcessorUpdate.h"
#include "Utils.h"
#include "HatoholException.h"
#include "SQLProcessorException.h"
#include "SQLUtils.h"
using namespace std;
using namespace mlpl;

struct SQLProcessorUpdate::PrivateContext {
	TableNameStaticInfoMap      &tableNameStaticInfoMap;
	SeparatorCheckerWithCallback separator;

	SQLUpdateInfo     *updateInfo;
	string             currWord;
	string             currWordLower;
	UpdateParseSection section;
	string             pendingWord;
	bool               openQuot;
	SeparatorCheckerWithCallback *whereParserSeparatorChecker;

	// group to be being processed
	ItemGroupPtr       evalTargetItemGroup;

	// constructor
	PrivateContext(TableNameStaticInfoMap &_tableNameStaticInfoMap)
	: tableNameStaticInfoMap(_tableNameStaticInfoMap),
	  separator(" ,\'="),
	  updateInfo(NULL),
	  section(UPDATE_PARSING_SECTION_UPDATE),
	  openQuot(false),
	  whereParserSeparatorChecker(NULL),
	  evalTargetItemGroup(NULL)
	{
	}

	void clear(void)
	{
		separator.unsetAlternative();
		section = UPDATE_PARSING_SECTION_UPDATE;
		openQuot = false;
		whereParserSeparatorChecker = NULL;
		evalTargetItemGroup = NULL;
	}
};

const SQLProcessorUpdate::UpdateSubParser
SQLProcessorUpdate::m_updateSubParsers[] = {
	&SQLProcessorUpdate::parseUpdate,
	&SQLProcessorUpdate::parseTable,
	&SQLProcessorUpdate::parseSetKeyword,
	&SQLProcessorUpdate::parseColumn,
	&SQLProcessorUpdate::parseEqual,
	&SQLProcessorUpdate::parseValue,
	&SQLProcessorUpdate::parsePostOneSet,
	&SQLProcessorUpdate::parseWhere,
};

class SQLFormulaUpdateColumnDataGetter : public FormulaVariableDataGetter {
public:
	SQLFormulaUpdateColumnDataGetter(const string &name,
	                                 SQLUpdateInfo *updateInfo,
	                                 ItemGroupPtr &evalTargetItemGroup)
	: m_name(name),
	  m_updateInfo(updateInfo),
	  m_evalTargetItemGroup(evalTargetItemGroup)
	{
	}

	virtual ItemDataPtr getData(void)
	{
		ItemDataPtr dataPtr = 
		  SQLUtils::getItemDataFromItemGroupWithColumnName
		    (m_name, m_updateInfo->tableStaticInfo,
		     m_evalTargetItemGroup);
		return dataPtr;
	}

private:
	string         m_name;
	SQLUpdateInfo *m_updateInfo;
	ItemGroupPtr  &m_evalTargetItemGroup;
};

// ---------------------------------------------------------------------------
// Public methods (SQLUpdateInfo)
// ---------------------------------------------------------------------------
SQLUpdateInfo::SQLUpdateInfo(const ParsableString &_statement)
: SQLProcessorInfo(_statement),
  itemFalsePtr(new ItemBool(false), false)
{
}

SQLUpdateInfo::~SQLUpdateInfo()
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorUpdate::init(void)
{
	// check the size of m_updateSubParsers
	size_t size = sizeof(SQLProcessorUpdate::m_updateSubParsers) /
	                sizeof(UpdateSubParser);
	if (size != NUM_UPDATE_PARSING_SECTION) {
		THROW_HATOHOL_EXCEPTION(
		  "sizeof(m_updateSubParsers) is invalid: "
		  "(expcect/actual: %d/%zd).",
		  NUM_UPDATE_PARSING_SECTION, size);
	}
}

SQLProcessorUpdate::SQLProcessorUpdate
  (TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(tableNameStaticInfoMap);

	m_ctx->separator.setCallbackTempl<SQLProcessorUpdate>
	  (',', _separatorCbComma, this);
	m_ctx->separator.setCallbackTempl<SQLProcessorUpdate>
	  ('\'', _separatorCbQuot, this);
	m_ctx->separator.setCallbackTempl<SQLProcessorUpdate>
	  ('=', _separatorCbEqual, this);
}

SQLProcessorUpdate::~SQLProcessorUpdate()
{
	delete m_ctx;
}

bool SQLProcessorUpdate::update(SQLUpdateInfo &updateInfo)
{
	m_ctx->clear();
	try {
		parseUpdateStatement(updateInfo);
		getStaticTableInfo(updateInfo);
		getTable(updateInfo);
		doUpdate(updateInfo);
	} catch (const SQLProcessorException &e) {
		MLPL_DBG("Got SQLProcessorException: %s",
		         e.getFancyMessage().c_str());
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SQLProcessorUpdate::parseUpdateStatement(SQLUpdateInfo &updateInfo)
{
	m_ctx->updateInfo = &updateInfo;
	while (!updateInfo.statement.finished()) {
		m_ctx->currWord = readCurrWord();
		if (m_ctx->currWord.empty())
			continue;
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		
		// parse each component
		if (m_ctx->section >= NUM_UPDATE_PARSING_SECTION) {
			THROW_HATOHOL_EXCEPTION(
			  "section(%d) >= NUM_UPDATE_PARSING_SECTION",
			  m_ctx->section);
		}
		UpdateSubParser subParser = m_updateSubParsers[m_ctx->section];
		(this->*subParser)();
	}
	if (m_ctx->section == UPDATE_PARSING_SECTION_WHERE)
		updateInfo.whereParser.close();
}

void SQLProcessorUpdate::getStaticTableInfo(SQLUpdateInfo &updateInfo)
{
	TableNameStaticInfoMapIterator it =
	  m_ctx->tableNameStaticInfoMap.find(updateInfo.table);
	if (it == m_ctx->tableNameStaticInfoMap.end()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found: table: %s", updateInfo.table.c_str());
	}
	updateInfo.tableStaticInfo = it->second;
}

void SQLProcessorUpdate::getTable(SQLUpdateInfo &updateInfo)
{
	ItemTablePtr tablePtr = updateInfo.tableStaticInfo->tableGetFunc();
	if (!tablePtr.hasData()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "tablePtr has no data (%s).", updateInfo.table.c_str());
	}
	updateInfo.tablePtr = tablePtr;
}

void SQLProcessorUpdate::doUpdate(SQLUpdateInfo &updateInfo)
{
	bool successed =
	  updateInfo.tablePtr->foreach<PrivateContext *>
	                                (updateMatchingRows, m_ctx);
	if (!successed) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Failed to search matched rows.");
	}
}

//
// Sub parsers
//
void SQLProcessorUpdate::parseUpdate(void)
{
	checkCurrWord("update", UPDATE_PARSING_SECTION_TABLE);
}

void SQLProcessorUpdate::parseTable(void)
{
	m_ctx->updateInfo->table = m_ctx->currWord;
	m_ctx->section = UPDATE_PARSING_SECTION_SET_KEYWORD;
}

void SQLProcessorUpdate::parseSetKeyword(void)
{
	checkCurrWord("set", UPDATE_PARSING_SECTION_COLUMN);
}

void SQLProcessorUpdate::parseColumn(void)
{
	m_ctx->updateInfo->columnVector.push_back(m_ctx->currWord);
	m_ctx->section = UPDATE_PARSING_SECTION_EQUAL;
}

void SQLProcessorUpdate::parseEqual(void)
{
	THROW_SQL_PROCESSOR_EXCEPTION(
	  "The parser expected '='. But got: %s", m_ctx->currWord.c_str());
}

void SQLProcessorUpdate::parseValue(void)
{
	m_ctx->updateInfo->valueVector.push_back(m_ctx->currWord);
	m_ctx->section = UPDATE_PARSING_SECTION_POST_ONE_SET;
}

void SQLProcessorUpdate::parsePostOneSet(void)
{
	checkCurrWord("where", UPDATE_PARSING_SECTION_WHERE);
	SQLWhereParser &whereParser = m_ctx->updateInfo->whereParser;
	m_ctx->whereParserSeparatorChecker = whereParser.getSeparatorChecker();
	whereParser.setColumnDataGetterFactory(formulaColumnDataGetterFactory,
	                                       m_ctx);
}

void SQLProcessorUpdate::parseWhere(void)
{
	m_ctx->updateInfo->whereParser.add(m_ctx->currWord,
	                                   m_ctx->currWordLower);
}

//
// SeparatorChecker callbacks
//
void SQLProcessorUpdate::_separatorCbComma(const char separator,
                                           SQLProcessorUpdate *obj)
{
	obj->separatorCbComma(separator);
}

void SQLProcessorUpdate::separatorCbComma(const char separator)
{
	if (m_ctx->section != UPDATE_PARSING_SECTION_POST_ONE_SET) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Quotation is used in the section: %d", m_ctx->section);
	}
	m_ctx->section = UPDATE_PARSING_SECTION_COLUMN;
}

void SQLProcessorUpdate::_separatorCbQuot(const char separator,
                                          SQLProcessorUpdate *obj)
{
	obj->separatorCbQuot(separator);
}

void SQLProcessorUpdate::separatorCbQuot(const char separator)
{
	if (m_ctx->section != UPDATE_PARSING_SECTION_VALUE
	    && m_ctx->section != UPDATE_PARSING_SECTION_POST_ONE_SET) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Quotation is used in the section: %d", m_ctx->section);
	}

	if (!m_ctx->openQuot) {
		m_ctx->separator.setAlternative
		  (&ParsableString::SEPARATOR_QUOT);
		m_ctx->openQuot = true;
	} else {
		m_ctx->separator.unsetAlternative();
		m_ctx->openQuot = false;
	}
}

void SQLProcessorUpdate::_separatorCbEqual(const char separator,
                                           SQLProcessorUpdate *obj)
{
	obj->separatorCbEqual(separator);
}

void SQLProcessorUpdate::separatorCbEqual(const char separator)
{
	if (m_ctx->section != UPDATE_PARSING_SECTION_EQUAL) {
		THROW_SQL_PROCESSOR_EXCEPTION("Detected unexpected '='.");
	}
	m_ctx->section = UPDATE_PARSING_SECTION_VALUE;
}

//
// General sub routines
//
string SQLProcessorUpdate::readCurrWord(void) 
{
	SeparatorCheckerWithCallback *separator;
	if (m_ctx->section != UPDATE_PARSING_SECTION_WHERE)
		separator = &m_ctx->separator;
	else
		separator = m_ctx->whereParserSeparatorChecker;
	return m_ctx->updateInfo->statement.readWord(*separator);
}

void SQLProcessorUpdate::checkCurrWord(const string &expected,
                                       UpdateParseSection nextSection)
{
	if (m_ctx->currWordLower != expected) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "currWordLower is not '%s': %s",
		  expected.c_str(), m_ctx->currWordLower.c_str());
	}
	m_ctx->section = nextSection;
}

FormulaVariableDataGetter *
SQLProcessorUpdate::formulaColumnDataGetterFactory(const string &name,
                                                   void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);
	SQLUpdateInfo *updateInfo = ctx->updateInfo;
	return new SQLFormulaUpdateColumnDataGetter(name, updateInfo,
	                                            ctx->evalTargetItemGroup);
}

bool SQLProcessorUpdate::updateMatchingCell
  (const ItemGroup *itemGroup, PrivateContext *ctx,
   const string &columnName, const string &value)
{
	SQLUpdateInfo *updateInfo = ctx->updateInfo;
	ItemDataPtr dataPtr = 
	  SQLUtils::getItemDataFromItemGroupWithColumnName
	    (columnName, updateInfo->tableStaticInfo,
	     ctx->evalTargetItemGroup);
	if (!dataPtr.hasData()) {
		MLPL_DBG("result has no data.\n");
		return false;
	}

	const ColumnDef *columnDef =
	  SQLUtils::getColumnDef(columnName, updateInfo->tableStaticInfo);
	ItemDataPtr srcDataPtr = SQLUtils::createItemData(columnDef, value);
	if (!srcDataPtr.hasData()) {
		MLPL_DBG("result has no data.\n");
		return false;
	}

	// TODO: check if this is safe
	*const_cast<ItemData *>(&*dataPtr) = *srcDataPtr;
	return true;
}

bool SQLProcessorUpdate::updateMatchingRows(const ItemGroup *itemGroup,
                                            PrivateContext *ctx)
{
	SQLUpdateInfo *updateInfo = ctx->updateInfo;
	ctx->evalTargetItemGroup = itemGroup;
	FormulaElement *formula = ctx->updateInfo->whereParser.getFormula();
	ItemDataPtr result = formula->evaluate();
	if (!result.hasData()) {
		MLPL_DBG("result has no data.\n");
		return false;
	}
	if (*result == *updateInfo->itemFalsePtr)
		return true;

	bool ret;
	for (size_t i = 0; i < updateInfo->columnVector.size(); i++) {
		ret = updateMatchingCell(itemGroup, ctx,
		                         updateInfo->columnVector[i],
		                         updateInfo->valueVector[i]);
		if (!ret)
			return false;
	}
	return true;
}
