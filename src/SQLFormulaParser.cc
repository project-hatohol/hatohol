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
	bool                    quotOpen;
	string                  pendingWord;
	string                  pendingWordLower;
	deque<FormulaElement *> formulaElementStack;

	// methods
	PrivateContext(void)
	: errorFlag(false),
	  quotOpen(false)
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
// Static member and static public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::KeywordHandlerMap
SQLFormulaParser::m_defaultKeywordHandlerMap;

void SQLFormulaParser::init(void)
{
	m_defaultKeywordHandlerMap["and"] = &SQLFormulaParser::kwHandlerAnd;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::SQLFormulaParser(void)
: m_separator(" ()'"),
  m_formula(NULL),
  m_keywordHandlerMap(&m_defaultKeywordHandlerMap)
{
	m_ctx = new PrivateContext();
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('(', separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  (')', separatorCbParenthesisClose, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('\'', _separatorCbQuot, this);
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

	if (m_ctx->quotOpen)
		return addStringValue(word);

	if (passFunctionArgIfOpen(word))
		return true;

	KeywordHandlerMapIterator it;
	it = m_keywordHandlerMap->find(wordLower);
	if (it != m_keywordHandlerMap->end()) {
		KeywordHandler func = it->second;
		return (this->*func)();
	}

	m_ctx->pushPendingWords(word, wordLower);
	return true;
}

bool SQLFormulaParser::flush(void)
{
	if (m_ctx->errorFlag)
		return false;

	if (!makeFormulaElementFromPendingWord())
		return false;
	return true;
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

bool SQLFormulaParser::createdLHSElement(FormulaElement *formulaElement)
{
	if (m_ctx->formulaElementStack.empty())
		m_formula = formulaElement;
	else {
		FormulaElement *lhsElem;
		lhsElem = m_ctx->formulaElementStack.back();
		formulaElement->setLeftHand(lhsElem);
		m_ctx->formulaElementStack.pop_back();
		if (m_ctx->formulaElementStack.empty())
			m_formula = formulaElement;
	}
	m_ctx->formulaElementStack.push_back(formulaElement);
	return true;
}

void SQLFormulaParser::setErrorFlag(void)
{
	m_ctx->errorFlag = true;
}

FormulaElement *SQLFormulaParser::getCurrentElement(void) const
{
	if (m_ctx->formulaElementStack.empty())
		return NULL;
	return m_ctx->formulaElementStack.back();
}

bool SQLFormulaParser::makeFormulaElementFromPendingWord(void)
{
	if (!m_ctx->hasPendingWord())
		return true;

	bool shouldLHS = false;
	FormulaElement *currElement = getCurrentElement();
	if (!currElement)
		shouldLHS = true;
	else {
		if (!currElement->getLeftHand())
			shouldLHS = true;
		else if (!currElement->getRightHand())
			shouldLHS = false;
		else {
			string msg;
			TRMSG(msg, "Both hands are not null.");
			throw logic_error(msg);
		}
	}

	FormulaElement *formulaElement;
	bool isFloat;
	if (StringUtils::isNumber(m_ctx->pendingWord, &isFloat)) {
		if (shouldLHS) {
			MLPL_DBG("shouldLHS is true. But number is given.");
			return false;
		}
		if (!isFloat) {
			int number = atoi(m_ctx->pendingWord.c_str());
			formulaElement = new FormulaValue(number);
		} else {
			double number = atof(m_ctx->pendingWord.c_str());
			formulaElement = new FormulaValue(number);
		}
	} else
		formulaElement = makeFormulaVariable(m_ctx->pendingWord);
	m_ctx->clearPendingWords();

	bool ret = true;
	if (shouldLHS)
		ret = createdLHSElement(formulaElement);
	else
		currElement->setRightHand(formulaElement);
	return ret;
}

bool SQLFormulaParser::addStringValue(string &word)
{
	string msg;
	FormulaElement *currElement = getCurrentElement();
	if (!currElement) {
		MLPL_DBG("Current element: NULL.\n");
		return false;
	}
	
	if (!currElement->getLeftHand()) {
		TRMSG(msg, "Left hand element: NULL.\n");
		throw logic_error(msg);
	}

	if (currElement->getRightHand()) {
		TRMSG(msg, "Righthand element: NOT NULL.\n");
		throw logic_error(msg);
	}
	
	FormulaElement *formulaValue = new FormulaValue(word);
	currElement->setRightHand(formulaValue);
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

void SQLFormulaParser::_separatorCbQuot
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbQuot(separator);
}

void SQLFormulaParser::separatorCbQuot(const char separator)
{
	if (m_ctx->quotOpen) {
		m_ctx->quotOpen = false;
		m_separator.unsetAlternative();
		return;
	}
	m_ctx->quotOpen = true;
	m_separator.setAlternative(&ParsableString::SEPARATOR_QUOT);
}

//
// Keyword handlers
//
bool SQLFormulaParser::kwHandlerAnd(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__); 
	return false;
}
