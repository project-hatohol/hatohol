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
	virtual void add(string& word, string &wordLower);
	virtual void flush(void);
	virtual void close(void);
	SeparatorCheckerWithCallback *getSeparatorChecker(void);
	FormulaElement *getFormula(void) const;
	bool hasStatisticalFunc(void) const;

protected:
	//
	// type definition
	//
	typedef void (SQLFormulaParser::*KeywordHandler)(void);
	typedef map<string, KeywordHandler> KeywordHandlerMap;
	typedef KeywordHandlerMap::iterator KeywordHandlerMapIterator;

	typedef void (SQLFormulaParser::*FunctionParser)(void);
	typedef map<string, FunctionParser> FunctionParserMap;
	typedef FunctionParserMap::iterator FunctionParserMapIterator;;


	//
	// general sub routines
	//
	static void copyKeywordHandlerMap(KeywordHandlerMap &kwHandlerMap);
	static void copyFunctionParserMap(FunctionParserMap &fncParserMap);

	void setKeywordHandlerMap(KeywordHandlerMap *kwHandlerMap);
	void setFunctionParserMap(FunctionParserMap *fncParserMap);
	FormulaVariable *makeFormulaVariable(string &name);
	FormulaElement *makeFormulaVariableOrValue(string &word);
	bool passFunctionArgIfOpen(string &word);
	void insertElement(FormulaElement *formulaElement);
	FormulaElement *getCurrentElement(void) const;
	void insertAsRightHand(FormulaElement *formulaElement);
	void insertAsHand(FormulaElement *formulaElement);
	void makeFormulaElementFromPendingWord(void);
	void addStringValue(string &word);
	FormulaElement *takeFormula(void);
	bool makeFunctionParserIfPendingWordIsFunction(void);

	/**
	 * Get a FormulaElement object at the top of ParenthesisStack, 
	 * which stacks instances of FormulaParenthesis, FormulaFunction, and
	 * its sub classes.
	 *
	 * @retrun A FormulaElement object at the top of ParenthesisStack
	 *         when there is at least one object in the stack.
	 *         If the stack is empty, NULL is returned.
	 */
	FormulaElement *getTopOnParenthesisStack(void) const;

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

	static void _separatorCbPlus
	  (const char separator, SQLFormulaParser *formulaParser);
	virtual void separatorCbPlus(const char separator);

	static void _separatorCbDiv
	  (const char separator, SQLFormulaParser *formulaParser);
	virtual void separatorCbDiv(const char separator);

	//
	// Keyword handlers
	//
	void kwHandlerAnd(void);
	void kwHandlerOr(void);

private:
	static KeywordHandlerMap          m_defaultKeywordHandlerMap;
	static FunctionParserMap          m_defaultFunctionParserMap;

	struct PrivateContext;
	PrivateContext                   *m_ctx;
	FormulaVariableDataGetterFactory  m_columnDataGetterFactory;
	void                             *m_columnDataGetterFactoryPriv;
	SeparatorCheckerWithCallback      m_separator;
	FormulaElement                   *m_formula;
	bool                              m_hasStatisticalFunc;
	KeywordHandlerMap                *m_keywordHandlerMap;
	FunctionParserMap                *m_functionParserMap;
};

#endif // SQLFormualaParser_h

