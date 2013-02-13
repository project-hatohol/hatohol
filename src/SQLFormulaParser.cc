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
#include "FormulaOperator.h"

struct SQLFormulaParser::PrivateContext {
	bool                    errorFlag;
	bool                    quotOpen;
	string                  pendingWord;
	string                  pendingWordLower;
	FormulaElement         *currElement;

	// methods
	PrivateContext(void)
	: errorFlag(false),
	  quotOpen(false),
	  currElement(NULL)
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

SQLFormulaParser::FunctionParserMap
SQLFormulaParser::m_defaultFunctionParserMap;

void SQLFormulaParser::init(void)
{
	m_defaultKeywordHandlerMap["and"] = &SQLFormulaParser::kwHandlerAnd;
	m_defaultKeywordHandlerMap["or"]  = &SQLFormulaParser::kwHandlerOr;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::SQLFormulaParser(void)
: m_columnDataGetterFactory(NULL),
  m_separator(" ()'"),
  m_formula(NULL),
  m_hasStatisticalFunc(false),
  m_keywordHandlerMap(&m_defaultKeywordHandlerMap),
  m_functionParserMap(&m_defaultFunctionParserMap)
{
	m_ctx = new PrivateContext();
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('(', _separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  (')', _separatorCbParenthesisClose, this);
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
	if (word.empty())
		return true;

	if (m_ctx->errorFlag)
		return false;

	KeywordHandlerMapIterator it;
	it = m_keywordHandlerMap->find(wordLower);
	if (it != m_keywordHandlerMap->end()) {
		if (!flush())
			return false;
		KeywordHandler func = it->second;
		return (this->*func)();
	}

	if (m_ctx->hasPendingWord()) {
		MLPL_DBG("already has pending word: %s, curr: %s.\n",
		         m_ctx->pendingWord.c_str(), word.c_str());
		return false;
	}

	if (m_ctx->quotOpen)
		return addStringValue(word);

	if (passFunctionArgIfOpen(word))
		return true;

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

bool SQLFormulaParser::close(void)
{
	return flush();
}

SeparatorCheckerWithCallback *SQLFormulaParser::getSeparatorChecker(void)
{
	return &m_separator;
}

FormulaElement *SQLFormulaParser::getFormula(void) const
{
	return m_formula;
}

bool SQLFormulaParser::hasStatisticalFunc(void) const
{
	return m_hasStatisticalFunc;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// general sub routines
//
void SQLFormulaParser::copyKeywordHandlerMap(KeywordHandlerMap &kwHandlerMap)
{
	KeywordHandlerMapIterator it = m_defaultKeywordHandlerMap.begin();
	for (; it != m_defaultKeywordHandlerMap.end(); ++it)
		kwHandlerMap[it->first] = it->second;
}

void SQLFormulaParser::copyFunctionParserMap(FunctionParserMap &fncParserMap)
{
	FunctionParserMapIterator it = m_defaultFunctionParserMap.begin();
	for (; it != m_defaultFunctionParserMap.end(); ++it)
		fncParserMap[it->first] = it->second;
}

void SQLFormulaParser::setKeywordHandlerMap(KeywordHandlerMap *kwHandlerMap)
{
	m_keywordHandlerMap = kwHandlerMap;
}

void SQLFormulaParser::setFunctionParserMap(FunctionParserMap *fncParserMap)
{
	m_functionParserMap = fncParserMap;
}

FormulaVariable *SQLFormulaParser::makeFormulaVariable(string &name)
{
	if (!m_columnDataGetterFactory) {
		string msg;
		TRMSG(msg, "m_columnDataGetterFactory: NULL.");
		throw logic_error(msg);
	}
	if (!m_columnDataGetterFactory) {
		MLPL_BUG("m_columnDataGetterFactory: NULL\n");
		return NULL;
	}
	FormulaVariableDataGetter *dataGetter =
	  (*m_columnDataGetterFactory)(name, m_columnDataGetterFactoryPriv);
	FormulaVariable *formulaVariable =
	  new FormulaVariable(name, dataGetter);
	return formulaVariable;
}

bool SQLFormulaParser::passFunctionArgIfOpen(string &word)
{
	if (!m_ctx->currElement)
		return false;
	FormulaFunction *formulaFunc = 
	  dynamic_cast<FormulaFunction *>(m_ctx->currElement);
	if (!formulaFunc)
		return false;
	FormulaVariable *formulaVariable = makeFormulaVariable(word);
	if (!formulaVariable)
		return false;
	return formulaFunc->addArgument(formulaVariable);
}

bool SQLFormulaParser::insertElement(FormulaElement *formulaElement)
{
	// check statistical function
	if (!m_hasStatisticalFunc) {
		FormulaStatisticalFunc *statFunc =
		  dynamic_cast<FormulaStatisticalFunc *>(formulaElement);
		if (statFunc)
			m_hasStatisticalFunc = true;
	}

	if (!m_ctx->currElement) {
		m_formula = formulaElement;
		m_ctx->currElement = formulaElement;
		return true;
	}

	if (formulaElement->isTerminalElement())
		return insertAsRightHand(formulaElement);

	FormulaElement *targetElem =
	  m_ctx->currElement->findInsertPoint(formulaElement);

	// If priority of 'formulaElement' is higher than the current element,
	// 'formulaElement' just becomes a right hand of the current element.
	if (!targetElem)
		return insertAsRightHand(formulaElement);

	// If priority of 'formulaElement' is equal to 'targetElem',
	// the target element is changed to the right hand of 'targetElem'.
	if (formulaElement->priorityEqual(targetElem))
		targetElem = targetElem->getRightHand();

	FormulaElement *targetParent = targetElem->getParent();
	formulaElement->setLeftHand(targetElem);
	if (targetParent) {
		if (targetParent->getRightHand() != targetElem) {
			string msg;
			string tree;
			m_formula->getTreeInfo(tree);
			TRMSG(msg, "targetParent->getRightHand(): [%p] != "
			           "targetElement [%p]\n%s",
			      targetParent->getRightHand(), targetElem,
			      tree.c_str());
			throw logic_error(msg);
		}
		targetParent->setRightHand(formulaElement);
	} else {
		m_formula = formulaElement;
	}
	m_ctx->currElement = formulaElement;
	return true;
}

void SQLFormulaParser::setErrorFlag(void)
{
	m_ctx->errorFlag = true;
}

FormulaElement *SQLFormulaParser::getCurrentElement(void) const
{
	return m_ctx->currElement;
}

bool SQLFormulaParser::insertAsRightHand(FormulaElement *formulaElement)
{
	string msg;
	FormulaElement *currElement = m_ctx->currElement;
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

	currElement->setRightHand(formulaElement);
	m_ctx->currElement = formulaElement;
	return true;
}

bool SQLFormulaParser::makeFormulaElementFromPendingWord(void)
{
	if (!m_ctx->hasPendingWord())
		return true;

	FormulaElement *formulaElement;
	bool isFloat;
	if (StringUtils::isNumber(m_ctx->pendingWord, &isFloat)) {
		if (!isFloat) {
			int number = atoi(m_ctx->pendingWord.c_str());
			formulaElement = new FormulaValue(number);
		} else {
			double number = atof(m_ctx->pendingWord.c_str());
			formulaElement = new FormulaValue(number);
		}
		m_ctx->clearPendingWords();
		return insertAsRightHand(formulaElement);
	}

	formulaElement = makeFormulaVariable(m_ctx->pendingWord);
	if (!formulaElement)
		return false;
	m_ctx->clearPendingWords();
	return insertElement(formulaElement);
}

bool SQLFormulaParser::addStringValue(string &word)
{
	return insertAsRightHand(new FormulaValue(word));
}

FormulaElement *SQLFormulaParser::takeFormula(void)
{
	FormulaElement *ret = m_formula;
	m_formula = NULL;
	m_hasStatisticalFunc = false;
	m_ctx->currElement = NULL;
	return ret;
}

bool SQLFormulaParser::makeFunctionParserIfPendingWordIsFunction(void)
{
	if (!m_ctx->hasPendingWord())
		return false;

	FunctionParserMapIterator it;
	it = m_functionParserMap->find(m_ctx->pendingWordLower);
	if (it == m_functionParserMap->end())
		return false;

	FunctionParser func = it->second;
	if (!(this->*func)())
		setErrorFlag();

	m_ctx->clearPendingWords();
	return true;
}

bool SQLFormulaParser::closeFunctionIfOpen(void)
{
	return true;
}

//
// SeparatorChecker callbacks
//
void SQLFormulaParser::_separatorCbParenthesisOpen
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbParenthesisOpen(separator);
}

void SQLFormulaParser::separatorCbParenthesisOpen(const char separator)
{

	bool isFunc = makeFunctionParserIfPendingWordIsFunction();
	if (isFunc)
		return;

	FormulaElement *formulaParenthesis = new FormulaParenthesis();
	if (!insertElement(formulaParenthesis))
		setErrorFlag();
}

void SQLFormulaParser::_separatorCbParenthesisClose
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbParenthesisClose(separator);
}

void SQLFormulaParser::separatorCbParenthesisClose(const char separator)
{
	if (closeFunctionIfOpen())
		return;

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
	FormulaOperatorAnd *opAnd = new FormulaOperatorAnd();
	return insertElement(opAnd);
}

bool SQLFormulaParser::kwHandlerOr(void)
{
	FormulaOperatorOr *opOr = new FormulaOperatorOr();
	return insertElement(opOr);
}
