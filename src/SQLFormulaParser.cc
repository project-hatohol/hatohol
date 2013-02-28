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
#include "SQLProcessorException.h"

struct SQLFormulaParser::PrivateContext {
	bool                    quotOpen;
	string                  pendingWord;
	string                  pendingWordLower;
	string                  pendingOperator;
	FormulaElement         *currElement;
	deque<FormulaElement *> parenthesisStack; 

	// methods
	PrivateContext(void)
	: quotOpen(false),
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
  m_separator(" ()'+/<>"),
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
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('+', _separatorCbPlus, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('/', _separatorCbDiv, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('<', _separatorCbLessThan, this);
	m_separator.setCallbackTempl<SQLFormulaParser>
	  ('>', _separatorCbGreaterThan, this);
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

void SQLFormulaParser::add(string& word, string &wordLower)
{
	if (word.empty())
		return;

	// check for pending operators
	KeywordHandlerMapIterator it;
	if (!m_ctx->pendingOperator.empty()) {
		it = m_keywordHandlerMap->find(m_ctx->pendingOperator);
		if (it == m_keywordHandlerMap->end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Unknown operator: %s",
			  m_ctx->pendingOperator.c_str());
		}
		KeywordHandler func = it->second;
		(this->*func)();
		m_ctx->pendingOperator.clear();
	}

	// check for pending keywords
	it = m_keywordHandlerMap->find(wordLower);
	if (it != m_keywordHandlerMap->end()) {
		flush();
		KeywordHandler func = it->second;
		(this->*func)();
		return;
	}

	if (m_ctx->hasPendingWord()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Invalid consecutive words: %s, %s.",
		         m_ctx->pendingWord.c_str(), word.c_str());
	}

	if (m_ctx->quotOpen) {
		addStringValue(word);
		return;
	}

	if (passFunctionArgIfOpen(word))
		return;

	m_ctx->pushPendingWords(word, wordLower);
}

void SQLFormulaParser::flush(void)
{
	makeFormulaElementFromPendingWord();
}

void SQLFormulaParser::close(void)
{
	flush();
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
	if (!m_columnDataGetterFactory)
		THROW_ASURA_EXCEPTION("m_columnDataGetterFactory: NULL.");
	FormulaVariableDataGetter *dataGetter =
	  (*m_columnDataGetterFactory)(name, m_columnDataGetterFactoryPriv);
	FormulaVariable *formulaVariable =
	  new FormulaVariable(name, dataGetter);
	return formulaVariable;
}

FormulaElement *SQLFormulaParser::makeFormulaVariableOrValue(string &word)
{
	bool isFloat;
	FormulaElement *formulaElement;
	if (StringUtils::isNumber(word, &isFloat)) {
		MLPL_WARN("Strict number generation algorithm "
		          "has to be implemented.\n");
		if (!isFloat) {
			int number = atoi(word.c_str());
			formulaElement = new FormulaValue(number);
		} else {
			double number = atof(word.c_str());
			formulaElement = new FormulaValue(number);
		}
	} else {
		formulaElement = makeFormulaVariable(word);
	}
	return formulaElement;

}

bool SQLFormulaParser::passFunctionArgIfOpen(string &word)
{
	if (!m_ctx->currElement)
		return false;
	FormulaFunction *formulaFunc = 
	  dynamic_cast<FormulaFunction *>(m_ctx->currElement);
	if (!formulaFunc)
		return false;
	FormulaElement *formulaElement = makeFormulaVariableOrValue(word);
	if (!formulaElement)
		return false;
	m_ctx->currElement = formulaElement;
	return formulaFunc->addArgument(formulaElement);
}

void SQLFormulaParser::insertElement(FormulaElement *formulaElement)
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
		return;
	}

	if (formulaElement->isTerminalElement()) {
		insertAsHand(formulaElement);
		return;
	}

	FormulaElement *upperLimit = NULL;
	if (!m_ctx->parenthesisStack.empty())
		upperLimit = m_ctx->parenthesisStack.back();
	FormulaElement *targetElem =
	  m_ctx->currElement->findInsertPoint(formulaElement, upperLimit);

	// If priority of 'formulaElement' is higher than the current element,
	// 'formulaElement' just becomes a right hand of the current element.
	if (!targetElem) {
		insertAsHand(formulaElement);
		return;
	}

	// If priority of 'formulaElement' is equal to 'targetElem',
	// the target element is changed to the right hand of 'targetElem'.
	if (formulaElement->priorityEqual(targetElem))
		targetElem = targetElem->getRightHand();

	FormulaElement *targetParent = targetElem->getParent();
	formulaElement->setLeftHand(targetElem);
	if (targetParent) {
		if (targetParent->isUnary()) {
			targetParent->setLeftHand(formulaElement);
		} else if (targetParent->getRightHand() != targetElem) {
			string tree;
			m_formula->getTreeInfo(tree);
			THROW_ASURA_EXCEPTION(
			  "targetParent->getRightHand(): [%p] != "
			  "targetElement [%p]\n%s",
			  targetParent->getRightHand(), targetElem,
			  tree.c_str());
		} else {
			targetParent->setRightHand(formulaElement);
		}
	} else {
		m_formula = formulaElement;
	}
	m_ctx->currElement = formulaElement;
}

FormulaElement *SQLFormulaParser::getCurrentElement(void) const
{
	return m_ctx->currElement;
}

void SQLFormulaParser::insertAsRightHand(FormulaElement *formulaElement)
{
	string treeInfo;
	FormulaElement *currElement = m_ctx->currElement;
	if (!currElement) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Tried to insert element, but no root element.");
	}

	if (!currElement->getLeftHand()) {
		currElement->getRootElement()->getTreeInfo(treeInfo);
		THROW_ASURA_EXCEPTION("Left hand element: NULL, %p.\n%s",
		                      formulaElement, treeInfo.c_str());
	}

	if (currElement->getRightHand()) {
		currElement->getRootElement()->getTreeInfo(treeInfo);
		THROW_ASURA_EXCEPTION("Righthand element: NOT NULL, %p.\n%s",
		                      formulaElement, treeInfo.c_str());
	}

	currElement->setRightHand(formulaElement);
	m_ctx->currElement = formulaElement;
}

void SQLFormulaParser::insertAsHand(FormulaElement *formulaElement)
{
	FormulaElement *currElement = m_ctx->currElement;
	if (!currElement)
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Tried to insert element, but no root element.");
	
	if (!currElement->getLeftHand()) {
		currElement->setLeftHand(formulaElement);
		m_ctx->currElement = formulaElement;
		return;
	}

	if (!currElement->getRightHand()) {
		currElement->setRightHand(formulaElement);
		m_ctx->currElement = formulaElement;
		return;
	}

	string treeInfo;
	formulaElement->getRootElement()->getTreeInfo(treeInfo);
	THROW_ASURA_EXCEPTION("Both hands are not NULL: %p.\n%s",
	                      formulaElement, treeInfo.c_str());
}

void SQLFormulaParser::makeFormulaElementFromPendingWord(void)
{
	if (!m_ctx->hasPendingWord())
		return;

	FormulaElement *formulaElement =
	  makeFormulaVariableOrValue(m_ctx->pendingWord);
	m_ctx->clearPendingWords();
	insertElement(formulaElement);
}

void SQLFormulaParser::addStringValue(string &word)
{
	insertAsRightHand(new FormulaValue(word));
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
	(this->*func)();
	m_ctx->parenthesisStack.push_back(m_ctx->currElement);

	m_ctx->clearPendingWords();
	return true;
}

FormulaElement *SQLFormulaParser::getTopOnParenthesisStack(void) const
{
	if (m_ctx->parenthesisStack.empty())
		return NULL;
	return m_ctx->parenthesisStack.back();
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
	insertElement(formulaParenthesis);
	m_ctx->parenthesisStack.push_back(formulaParenthesis);
}

void SQLFormulaParser::_separatorCbParenthesisClose
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbParenthesisClose(separator);
}

void SQLFormulaParser::separatorCbParenthesisClose(const char separator)
{
	flush();

	if (m_ctx->parenthesisStack.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Parenthesis is not open.");

	FormulaElement *parenthesisElem = m_ctx->parenthesisStack.back();
	m_ctx->parenthesisStack.pop_back();

	FormulaFunction *formulaFunction =
	  dynamic_cast<FormulaFunction *>(parenthesisElem);
	if (formulaFunction) {
		if (!formulaFunction->close()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to close function on ')'.");
		}
	}
	m_ctx->currElement = parenthesisElem;
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

void SQLFormulaParser::_separatorCbPlus
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbPlus(separator);
}

void SQLFormulaParser::separatorCbPlus(const char separator)
{
	flush();

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "No left hand side of '+' operator.");

	FormulaOperatorPlus *formulaOperatorPlus = new FormulaOperatorPlus();
	insertElement(formulaOperatorPlus);
}

void SQLFormulaParser::_separatorCbDiv
  (const char separator, SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbDiv(separator);
}

void SQLFormulaParser::separatorCbDiv(const char separator)
{
	flush();

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "No left hand side of '/' operator.");

	FormulaOperatorDiv *formulaOperatorDiv = new FormulaOperatorDiv();
	insertElement(formulaOperatorDiv);
}

void SQLFormulaParser::_separatorCbLessThan(const char separator,
                                            SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbLessThan(separator);
}

void SQLFormulaParser::separatorCbLessThan(const char separator)
{
	m_ctx->pendingOperator += separator;
}

void SQLFormulaParser::_separatorCbGreaterThan(const char separator,
                                               SQLFormulaParser *formulaParser)
{
	formulaParser->separatorCbGreaterThan(separator);
}

void SQLFormulaParser::separatorCbGreaterThan(const char separator)
{
	m_ctx->pendingOperator += separator;
}

//
// Keyword handlers
//
void SQLFormulaParser::kwHandlerAnd(void)
{
	FormulaOperatorAnd *opAnd = new FormulaOperatorAnd();
	insertElement(opAnd);
}

void SQLFormulaParser::kwHandlerOr(void)
{
	FormulaOperatorOr *opOr = new FormulaOperatorOr();
	insertElement(opOr);
}
