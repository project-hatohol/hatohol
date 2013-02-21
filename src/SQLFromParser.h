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

#ifndef SQLFromParser_h
#define SQLFromParser_h

#include <string>
using namespace std;

#include <ParsableString.h>
using namespace mlpl;

#include "SQLTableFormula.h"

class SQLFromParser
{
public:
	SQLFromParser(void);
	virtual ~SQLFromParser();
	SQLTableFormula *getTableFormula(void) const;
	SeparatorCheckerWithCallback *getSeparatorChecker(void);

	virtual bool add(string& word, string &wordLower);
	virtual bool close(void);

protected:
	//
	// general sub routines
	//

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbEqual(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbEqual(const char separator);

	static void _separatorCbComma(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbComma(const char separator);

private:
	struct PrivateContext;
	PrivateContext              *m_ctx;
	SeparatorCheckerWithCallback m_separator;
};

#endif // SQLFromParser_h


