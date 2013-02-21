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

#include "SQLFromParser.h"
#include "FormulaOperator.h"
#include "ItemDataUtils.h"
#include "SQLProcessorException.h"

struct SQLFromParser::PrivateContext {

	ParsingState     state;
	SQLTableFormula *tableFormula;
	string           pendingWord;
	string           pendingWordLower;

	// constructor
	PrivateContext(void)
	: state(PARSING_STAT_EXPECT_FROM),
	  tableFormula(NULL)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFromParser::SQLFromParser(void)
: m_ctx(NULL),
  m_separator(" =,")
{
	m_ctx = new PrivateContext();
	m_separator.setCallbackTempl<SQLFromParser>
	  ('=', _separatorCbEqual, this);
	m_separator.setCallbackTempl<SQLFromParser>
	  (',', _separatorCbComma, this);
}

SQLFromParser::~SQLFromParser()
{
	if (m_ctx)
		delete m_ctx;
}

SQLTableFormula *SQLFromParser::getTableFormula(void) const
{
	return m_ctx->tableFormula;
}

SeparatorCheckerWithCallback *SQLFromParser::getSeparatorChecker(void)
{
	return &m_separator;
}

void SQLFromParser::add(const string &word, const string &wordLower)
{
	if (m_ctx->state == PARSING_STAT_EXPECT_FROM) {
		goNextStateIfWordIsExpected("from", wordLower,
		                            PARSING_STAT_EXPECT_TABLE_NAME);
		return;
	}

	if (!m_ctx->pendingWord.empty()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Invalid consecutive words: %s, %s\n",
		  m_ctx->pendingWord.c_str(), word.c_str());
	}

	m_ctx->pendingWord = word;
	m_ctx->pendingWordLower = wordLower;
}

void SQLFromParser::flush(void)
{
	if (m_ctx->pendingWord.empty())
		return;

	if (m_ctx->state == PARSING_STAT_EXPECT_TABLE_NAME) {
		string &tableName = m_ctx->pendingWord;
		SQLTableFormula *tableElem = new SQLTableElement(tableName);
		insertTableFormula(tableElem);
	}
}

void SQLFromParser::close(void)
{
	flush();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// general sub routines
//
void SQLFromParser::goNextStateIfWordIsExpected(const string &expectedWord,
                                                const string &actualWord,
                                                ParsingState nextState)
{
	if (actualWord != expectedWord) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Expected: %s, but got: %s\n",
		  expectedWord.c_str(), actualWord.c_str());
	}
	m_ctx->state = nextState;
}

void SQLFromParser::insertTableFormula(SQLTableFormula *tableFormula)
{
	if (!m_ctx->tableFormula) {
		m_ctx->tableFormula = tableFormula;
		return;
	}

	THROW_SQL_PROCESSOR_EXCEPTION(
	  "Not implemented: %s\n", __PRETTY_FUNCTION__);
}

//
// SeparatorChecker callbacks
//
void SQLFromParser::_separatorCbEqual(const char separator,
                                       SQLFromParser *fromParsesr)
{
	fromParsesr->separatorCbEqual(separator);
}

void SQLFromParser::separatorCbEqual(const char separator)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void SQLFromParser::_separatorCbComma(const char separator,
                                      SQLFromParser *fromParsesr)
{
	fromParsesr->separatorCbComma(separator);
}

void SQLFromParser::separatorCbComma(const char separator)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}
