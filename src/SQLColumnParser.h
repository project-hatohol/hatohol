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

class SQLColumnParser
{
public:
	static void init(void);

	SQLColumnParser(void);
	virtual ~SQLColumnParser();
	void setColumnDataGetterFactory
	       (FormulaVariableDataGetterFactory columnDataGetterFactory,
	        void *columnDataGetterFactoryPriv);
	bool add(string& word, string &wordLower);
	bool flush(void);
	const SQLFormulaInfoVector &getFormulaInfoVector(void) const;
	SeparatorCheckerWithCallback *getSeparatorChecker(void);

protected:
	// Type definition
	typedef bool (SQLColumnParser::*FunctionParser)(void);
	typedef map<string, FunctionParser> FunctionParserMap;
	typedef FunctionParserMap::iterator FunctionParserMapIterator;;

	//
	// general sub routines
	//
	void appendFormulaString(const char character);
	void appendFormulaString(string &str);
	FormulaVariable *makeFormulaVariable(string &name);
	bool closeCurrFormulaInfo(void);
	void closeCurrentFormulaString(void);
	bool closeCurrentFormula(void);
	bool createdNewElement(FormulaElement *formulaElement);
	FormulaFunction *getFormulaFunctionFromStack(void);
	bool makeFunctionParserIfPendingWordIsFunction(void);
	bool passFunctionArgIfOpen(string &arg);
	bool closeFunctionIfOpen(void);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbComma
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisOpen
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbParenthesisClose
	  (const char separator, SQLColumnParser *columnParser);

	//
	// functino parsers
	//
	bool funcParserMax(void);

private:
	// Type definition
	struct ParsingContext;

	// Static variables
	static FunctionParserMap m_functionParserMap;

	// General variables
	FormulaVariableDataGetterFactory  m_columnDataGetterFactory;
	void                             *m_columnDataGetterFactoryPriv;
	SQLFormulaInfoVector              m_formulaInfoVector;
	SeparatorCheckerWithCallback      m_separator;
	ParsingContext                   *m_ctx;
};

#endif // SQLColumnParser_h

