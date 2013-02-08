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
#define SQLFormulaParser_h

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
	static void copyKeywordHandlerMap(KeywordHandlerMap &kwHandlerMap);
	void setKeywordHandlerMap(KeywordHandlerMap *kwHandlerMap);
	FormulaVariable *makeFormulaVariable(string &name);
	bool passFunctionArgIfOpen(string &word);
	bool insertElement(FormulaElement *formulaElement);
	void setErrorFlag(void);
	FormulaElement *getCurrentElement(void) const;
	bool insertAsRightHand(FormulaElement *formulaElement);
	bool makeFormulaElementFromPendingWord(void);
	bool addStringValue(string &word);
	FormulaElement *takeFormula(void);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbParenthesisOpen
	  (const char separator, SQLFormulaParser *formulaParser);
	virtual void separatorCbParenthesisOpen(const char separator);

	static void _separatorCbParenthesisClose
	  (const char separator, SQLFormulaParser *formulaParser);
	virtual void separatorCbParenthesisClose(const char separator);

	static void _separatorCbQuot
	  (const char separator, SQLFormulaParser *formulaParser);
	virtual void separatorCbQuot(const char separator);

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

