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

#include "SQLWhereParser.h"
#include "FormulaOperator.h"

// ---------------------------------------------------------------------------
// Static public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::KeywordHandlerMap SQLWhereParser::m_keywordHandlerMap;


void SQLWhereParser::init(void)
{
	SQLFormulaParser::copyKeywordHandlerMap(m_keywordHandlerMap);
	m_keywordHandlerMap["between"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerBetween);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLWhereParser::SQLWhereParser(void)
{
	setKeywordHandlerMap(&m_keywordHandlerMap);
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->addSeparator("=");
	separator->setCallbackTempl<SQLWhereParser>
	  ('=', _separatorCbEqual, this);
}

SQLWhereParser::~SQLWhereParser()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// SeparatorChecker callbacks
//
void SQLWhereParser::_separatorCbEqual(const char separator,
                                       SQLWhereParser *whereParser)
{
	whereParser->separatorCbEqual(separator);
}

void SQLWhereParser::separatorCbEqual(const char separator)
{
	if (!flush())
		return;

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement) {
		setErrorFlag();
		MLPL_DBG("lhsElement: NULL.");
		return;
	}

	// create ComparatorEqual
	FormulaComparatorEqual *formulaComparatorEqual
	  = new FormulaComparatorEqual();
	if (!insertElement(formulaComparatorEqual)) {
		setErrorFlag();
		return;
	}
}

//
// Keyword handlers
//
bool SQLWhereParser::kwHandlerBetween(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	return false;
}
