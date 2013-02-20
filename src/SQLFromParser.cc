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

struct SQLFromParser::PrivateContext {

	// constructor
	PrivateContext(void)
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

bool SQLFromParser::add(string& word, string &wordLower)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// general sub routines
//

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
