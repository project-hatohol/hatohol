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

#include "Logger.h"
using namespace mlpl;

#include "SQLFormulaParser.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::SQLFormulaParser(void)
: m_separator(" ()")
{
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('(', separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  (')', separatorCbParenthesisClose, this);
}

SQLFormulaParser::~SQLFormulaParser()
{
}

void SQLFormulaParser::setColumnDataGetterFactory
       (FormulaVariableDataGetterFactory columnDataGetterFactory,
        void *columnDataGetterFactoryPriv)
{
       m_columnDataGetterFactory = columnDataGetterFactory;
       m_columnDataGetterFactoryPriv = columnDataGetterFactoryPriv;
}

bool SQLFormulaParser::add(string& word, string &wordLower)
{
	MLPL_BUG("Not implemented: %s: %s\n", word.c_str(), __func__);
	return false;
}

bool SQLFormulaParser::flush(void)
{
	MLPL_BUG("Not implemented: %s\n", __func__);
	return false;
}

SeparatorCheckerWithCallback *SQLFormulaParser::getSeparatorChecker(void)
{
	return &m_separator;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// SeparatorChecker callbacks
//
void SQLFormulaParser::separatorCbParenthesisOpen
  (const char separator, SQLFormulaParser *formulaParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

void SQLFormulaParser::separatorCbParenthesisClose
  (const char separator, SQLFormulaParser *formulaParser)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
}

