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

#include "SQLProcessorTypes.h"
#include "SQLProcessorException.h"

// ---------------------------------------------------------------------------
// SQLProcessorInfo
// ---------------------------------------------------------------------------
SQLProcessorInfo::SQLProcessorInfo(const ParsableString &_statement)
: statement(_statement)
{
}

SQLProcessorInfo::~SQLProcessorInfo()
{
}

// ---------------------------------------------------------------------------
// SQLProcessorSelectShareInfo
// ---------------------------------------------------------------------------
SQLProcessorSelectShareInfo::SQLProcessorSelectShareInfo
  (SQLProcessorSelectFactory &selectFactory)
: processorSelectFactory(selectFactory),
  statement(NULL),
  allowSectionParserChange(true)
{
}

void SQLProcessorSelectShareInfo::clear(void)
{
	statement = NULL;
	allowSectionParserChange = true;
}
