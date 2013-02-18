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

#include "SQLProcessorUpdate.h"
#include "Utils.h"
#include "AsuraException.h"

struct SQLProcessorUpdate::PrivateContext {
	SQLUpdateInfo     *updateInfo;
	string             currWord;
	string             currWordLower;
	UpdateParseSection section;
	string             pendingWord;
	bool               openQuot;

	// constructor
	PrivateContext(void)
	: updateInfo(NULL),
	  section(UPDATE_PARSING_SECTION_UPDATE),
	  openQuot(false)
	{
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
	&SQLProcessorUpdate::parseWhereKeyword,
	&SQLProcessorUpdate::parseWhere,
	&SQLProcessorUpdate::parseEnd,
};

// ---------------------------------------------------------------------------
// Public methods (SQLUpdateInfo)
// ---------------------------------------------------------------------------
SQLUpdateInfo::SQLUpdateInfo(ParsableString &_statement)
: statement(_statement)
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
		THROW_ASURA_EXCEPTION_WITH_LOG(BUG,
		  "sizeof(m_updateSubParsers) is invalid: "
		  "(expcect/actual: %d/%d).",
		  NUM_UPDATE_PARSING_SECTION, size);
	}
}

SQLProcessorUpdate::SQLProcessorUpdate
  (TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_tableNameStaticInfoMap(tableNameStaticInfoMap),
  m_ctx(NULL),
  m_separator(" ,\'=")
{
	m_ctx = new PrivateContext();

	m_separator.setCallbackTempl<SQLProcessorUpdate>
	  (',', _separatorCbComma, this);
	m_separator.setCallbackTempl<SQLProcessorUpdate>
	  ('\'', _separatorCbQuot, this);
	m_separator.setCallbackTempl<SQLProcessorUpdate>
	  ('=', _separatorCbEqual, this);
}

SQLProcessorUpdate::~SQLProcessorUpdate()
{
	if (m_ctx)
		delete m_ctx;
}

bool SQLProcessorUpdate::update(SQLUpdateInfo &updateInfo)
{
	parseUpdateStatement(updateInfo);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SQLProcessorUpdate::parseUpdateStatement(SQLUpdateInfo &updateInfo)
{
	m_ctx->updateInfo = &updateInfo;
	while (!updateInfo.statement.finished()) {
		m_ctx->currWord = updateInfo.statement.readWord(m_separator);
		if (m_ctx->currWord.empty())
			continue;
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		
		// parse each component
		if (m_ctx->section >= NUM_UPDATE_PARSING_SECTION) {
			THROW_ASURA_EXCEPTION_WITH_LOG(BUG,
			  "section(%d) >= NUM_UPDATE_PARSING_SECTION\n",
			  m_ctx->section);
		}
		UpdateSubParser subParser = m_updateSubParsers[m_ctx->section];
		(this->*subParser)();
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
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::parseEqual(void)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::parseValue(void)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::parseWhereKeyword(void)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::parseWhere(void)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::parseEnd(void)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
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
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::_separatorCbQuot(const char separator,
                                          SQLProcessorUpdate *obj)
{
	obj->separatorCbQuot(separator);
}

void SQLProcessorUpdate::separatorCbQuot(const char separator)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

void SQLProcessorUpdate::_separatorCbEqual(const char separator,
                                           SQLProcessorUpdate *obj)
{
	obj->separatorCbEqual(separator);
}

void SQLProcessorUpdate::separatorCbEqual(const char separator)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
}

//
// General sub routines
//
bool SQLProcessorUpdate::checkCurrWord(string expected,
                                       UpdateParseSection nextSection)
{
	if (m_ctx->currWordLower != expected) {
		MLPL_DBG("currWordLower is not '%s': %s\n",
		         expected.c_str(), m_ctx->currWordLower.c_str());
		return false;
	}
	m_ctx->section = nextSection;
	return true;
}

