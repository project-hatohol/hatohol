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

#ifndef SQLFormulaParser_h
#define SQLFormualaParser_h

#include <string>
#include <map>
using namespace std;

#include <ParsableString.h>
using namespace mlpl;

#include "FormulaElement.h"
#include "FormulaFunction.h"

class SQLFormulaParser
{
public:
	static void init(void);
	SQLFormulaParser(void);
	virtual ~SQLFormulaParser();
	void setColumnDataGetterFactory
	       (FormulaVariableDataGetterFactory columnDataGetterFactory,
	        void *columnDataGetterFactoryPriv);
	virtual bool add(string& word, string &wordLower);
	virtual bool flush(void);
	SeparatorCheckerWithCallback *getSeparatorChecker(void);
	FormulaElement *getFormula(void) const;

protected:
	//
	// type definition
	//
	typedef bool (SQLFormulaParser::*KeywordHandler)(void);
	typedef map<string, KeywordHandler> KeywordHandlerMap;
	typedef KeywordHandlerMap::iterator KeywordHandlerMapIterator;

	//
	// general sub routines
	//
	FormulaVariable *makeFormulaVariable(string &name);
	FormulaFunction *getFormulaFunctionFromStack(void);
	bool passFunctionArgIfOpen(string &word);
	bool createdLHSElement(FormulaElement *formulaElement);
	void setErrorFlag(void);
	FormulaElement *getCurrentElement(void) const;
	bool makeFormulaElementFromPendingWord(void);
	bool addStringValue(string &word);
	bool checkKeyword(string &wordLower);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbParenthesisOpen
	  (const char separator, SQLFormulaParser *formulaParser);
	static void separatorCbParenthesisClose
	  (const char separator, SQLFormulaParser *formulaParser);
	static void _separatorCbQuot
	  (const char separator, SQLFormulaParser *formulaParser);
	void separatorCbQuot(const char separator);

	//
	// Keyword handlers
	//
	bool kwHandlerAnd(void);

private:
	static KeywordHandlerMap          m_defaultKeywordHandlerMap;

	struct PrivateContext;
	PrivateContext                   *m_ctx;
	FormulaVariableDataGetterFactory  m_columnDataGetterFactory;
	void                             *m_columnDataGetterFactoryPriv;
	SeparatorCheckerWithCallback      m_separator;
	FormulaElement                   *m_formula;
	KeywordHandlerMap                *m_keywordHandlerMap;
};

#endif // SQLFormualaParser_h

