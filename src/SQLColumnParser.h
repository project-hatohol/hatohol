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

#ifndef SQLColumnParser_h
#define SQLColumnParser_h

#include <string>
#include <list>
#include <deque>
using namespace std;

#include "ParsableString.h"
using namespace mlpl;

#include "SQLFormulaParser.h"
#include "FormulaElement.h"
#include "FormulaFunction.h"

struct SQLFormulaInfo {
	FormulaElement *formula;
	string          expression;
	bool            hasStatisticalFunc;

	// construct and destructor
	SQLFormulaInfo(void);
	~SQLFormulaInfo();
};

typedef vector<SQLFormulaInfo *>       SQLFormulaInfoVector;
typedef SQLFormulaInfoVector::iterator SQLFormulaInfoVectorIterator;

class SQLColumnParser : public SQLFormulaParser
{
public:
	static void init(void);

	SQLColumnParser(void);
	virtual ~SQLColumnParser();
	virtual bool add(string& word, string &wordLower);
	bool close(void);
	const SQLFormulaInfoVector &getFormulaInfoVector(void) const;

protected:
	//
	// general sub routines
	//
	void appendFormulaString(const char character);
	void appendFormulaString(string &str);
	bool closeCurrFormulaInfo(void);
	void closeCurrentFormulaString(void);
	bool closeCurrentFormula(void);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbComma
	  (const char separator, SQLColumnParser *columnParser);
	virtual void separatorCbParenthesisOpen(const char separator);
	virtual void separatorCbParenthesisClose(const char separator);

	//
	// functino parsers
	//
	bool funcParserMax(void);

private:
	// Type definition
	struct PrivateContext;

	// static function
	static FunctionParserMap          m_functionParserMap;

	// General variables
	SQLFormulaInfoVector              m_formulaInfoVector;
	PrivateContext                   *m_ctx;
};

#endif // SQLColumnParser_h

