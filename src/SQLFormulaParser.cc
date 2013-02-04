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

#include <stdexcept>

#include "Logger.h"
using namespace mlpl;

#include "SQLFormulaParser.h"

struct SQLFormulaParser::PrivateContext {
	bool                    errorFlag;
	string                  pendingWord;
	string                  pendingWordLower;
	deque<FormulaElement *> formulaElementStack;

	// methods
	PrivateContext(void)
	: errorFlag(false)
	{
	}

	void pushPendingWords(string &raw, string &lower) {
		pendingWord = raw;
		pendingWordLower = lower;;
	}

	bool hasPendingWord(void) {
		return !pendingWord.empty();
	}

	void clearPendingWords(void) {
		pendingWord.clear();
		pendingWordLower.clear();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::SQLFormulaParser(void)
: m_separator(" ()"),
  m_formula(NULL)
{
	m_ctx = new PrivateContext();
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('(', separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  (')', separatorCbParenthesisClose, this);
}

SQLFormulaParser::~SQLFormulaParser()
{
	if (m_ctx)
		delete m_ctx;
	if (m_formula)
		delete m_formula;
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
	if (m_ctx->errorFlag)
		return false;

	if (m_ctx->hasPendingWord()) {
		MLPL_DBG("hasPendingWord(): true.\n");
		return false;
	}

	if (passFunctionArgIfOpen(word))
		return true;

	m_ctx->pushPendingWords(word, wordLower);
	return true;
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

FormulaElement *SQLFormulaParser::getFormula(void) const
{
	return m_formula;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// general sub routines
//
FormulaVariable *SQLFormulaParser::makeFormulaVariable(string &name)
{
	if (!m_columnDataGetterFactory) {
		string msg;
		TRMSG(msg, "m_columnDataGetterFactory: NULL.");
		throw logic_error(msg);
	}
	FormulaVariableDataGetter *dataGetter =
	  (*m_columnDataGetterFactory)(name, m_columnDataGetterFactoryPriv);
	FormulaVariable *formulaVariable =
	  new FormulaVariable(name, dataGetter);
	return formulaVariable;
}

FormulaFunction *
SQLFormulaParser::SQLFormulaParser::getFormulaFunctionFromStack(void)
{
	if (m_ctx->formulaElementStack.empty())
		return NULL;
	FormulaElement *elem = m_ctx->formulaElementStack.back();
	return dynamic_cast<FormulaFunction *>(elem);
}

bool SQLFormulaParser::passFunctionArgIfOpen(string &word)
{
	FormulaFunction *formulaFunc = getFormulaFunctionFromStack();
	if (!formulaFunc)
		return false;
	FormulaVariable *formulaVariable = makeFormulaVariable(word);
	if (!formulaFunc->addArgument(formulaVariable))
		return false;
	return true;
}

bool SQLFormulaParser::createdNewElement(FormulaElement *formulaElement)
{
	if (m_ctx->formulaElementStack.empty())
		m_formula = formulaElement;
	m_ctx->formulaElementStack.push_back(formulaElement);
	return true;
}

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

