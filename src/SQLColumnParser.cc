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
#include "SQLColumnParser.h"
#include "FormulaFunction.h"

struct SQLColumnParser::PrivateContext {
	string                       currFormulaString;
	bool                         dontAppendFormulaString;
	bool                         expectAlias;
	string                       alias;

	// constructor and methods
	PrivateContext(void)
	: dontAppendFormulaString(false),
	  expectAlias(false)
	{
	}

	void clear(void) {
		currFormulaString.clear();
		expectAlias = false;
		alias.clear();
	}
};

SQLFormulaParser::KeywordHandlerMap SQLColumnParser::m_keywordHandlerMap;
SQLFormulaParser::FunctionParserMap SQLColumnParser::m_functionParserMap;

// ---------------------------------------------------------------------------
// Public static methods (SQLFormulaInfo)
// ---------------------------------------------------------------------------
SQLFormulaInfo::SQLFormulaInfo(void)
: formula(NULL),
  hasStatisticalFunc(false)
{
}

SQLFormulaInfo::~SQLFormulaInfo()
{
	if (formula)
		delete formula;
}

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
void SQLColumnParser::init(void)
{
	SQLFormulaParser::copyKeywordHandlerMap(m_keywordHandlerMap);
	m_keywordHandlerMap["as"] =
	  static_cast<KeywordHandler>(&SQLColumnParser::kwHandlerAs);
	m_keywordHandlerMap["distinct"] =
	  static_cast<KeywordHandler>(&SQLColumnParser::kwHandlerDistinct);

	SQLFormulaParser::copyFunctionParserMap(m_functionParserMap);
	m_functionParserMap["max"] =
	  static_cast<FunctionParser>(&SQLColumnParser::funcParserMax);
	m_functionParserMap["count"] =
	  static_cast<FunctionParser>(&SQLColumnParser::funcParserCount);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLColumnParser::SQLColumnParser(void)
{
	setKeywordHandlerMap(&m_keywordHandlerMap);
	setFunctionParserMap(&m_functionParserMap);
	m_ctx = new PrivateContext();
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->setCallbackTempl<SQLColumnParser>
	  (' ', separatorCbSpace, this);
	separator->addSeparator(",");
	separator->setCallbackTempl<SQLColumnParser>
	  (',', separatorCbComma, this);
}

SQLColumnParser::~SQLColumnParser()
{
	for (size_t i = 0; i < m_formulaInfoVector.size(); i++)
		delete m_formulaInfoVector[i];
	if (m_ctx)
		delete m_ctx;
}

bool SQLColumnParser::add(string &word, string &wordLower)
{
	if (m_ctx->expectAlias) {
		m_ctx->alias = word;
		m_ctx->expectAlias = false;
		return true;
	}

	if (!SQLFormulaParser::add(word, wordLower))
		return false;
	if (!m_ctx->dontAppendFormulaString)
		appendFormulaString(word);
	else
		m_ctx->dontAppendFormulaString = false;
	return true;
}

bool SQLColumnParser::close(void)
{
	flush();
	return closeCurrFormulaInfo();
}

const SQLFormulaInfoVector &SQLColumnParser::getFormulaInfoVector(void) const
{
	return m_formulaInfoVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
//
// general sub routines
//
void SQLColumnParser::appendFormulaString(const char character)
{
	m_ctx->currFormulaString += character;
}

void SQLColumnParser::appendFormulaString(string &str)
{
	m_ctx->currFormulaString += str;
}

bool SQLColumnParser::closeCurrFormulaInfo(void)
{
	if (!closeCurrentFormula())
		return false;
	closeCurrentFormulaString();
	m_ctx->clear();
	return true;
}

void SQLColumnParser::closeCurrentFormulaString(void)
{
	if (m_ctx->currFormulaString.empty())
		return;
	if (m_formulaInfoVector.empty()) {
		string msg;
		TRMSG(msg, "m_formulaInfoVector.empty().");
		throw logic_error(msg);
	}

	SQLFormulaInfo *formulaInfo = m_formulaInfoVector.back();
	formulaInfo->expression =
	  StringUtils::stripBothEndsSpaces(m_ctx->currFormulaString);
	formulaInfo->alias = m_ctx->alias;
}

bool SQLColumnParser::closeCurrentFormula(void)
{
	SQLFormulaInfo *formulaInfo = new SQLFormulaInfo();
	m_formulaInfoVector.push_back(formulaInfo);
	formulaInfo->hasStatisticalFunc = hasStatisticalFunc();
	formulaInfo->formula = takeFormula();
	if (!formulaInfo->formula) {
		string msg;
		TRMSG(msg, "formulaInfo->formula is NULL.\n");
		throw logic_error(msg);
	}
	return true;
}

//
// SeparatorChecker callbacks
//
void SQLColumnParser::separatorCbSpace(const char separator,
                                       SQLColumnParser *columnParser)
{
	columnParser->appendFormulaString(separator);
}

void SQLColumnParser::separatorCbComma(const char separator,
                                       SQLColumnParser *columnParser)
{
	columnParser->close();
}

void SQLColumnParser::separatorCbParenthesisOpen(const char separator)
{
	appendFormulaString(separator);
	SQLFormulaParser::separatorCbParenthesisOpen(separator);
}

void SQLColumnParser::separatorCbParenthesisClose(const char separator)
{
	appendFormulaString(separator);
	SQLFormulaParser::separatorCbParenthesisClose(separator);
}

//
// Keyword handlers
//
bool SQLColumnParser::kwHandlerAs(void)
{
	m_ctx->expectAlias = true;
	m_ctx->dontAppendFormulaString = true;
	return true;
}

bool SQLColumnParser::kwHandlerDistinct(void)
{
	FormulaElement *formulaElement = getTopOnParenthesisStack();
	FormulaFuncCount *formulaFuncCount =
	  dynamic_cast<FormulaFuncCount *>(formulaElement);
	if (formulaFuncCount) {
		formulaFuncCount->setDistinct();
		return true;
	}
	
	MLPL_BUG("Not implemented: DISTINCT.\n");
	return false;
}

//
// functino parsers
//
bool SQLColumnParser::funcParserMax(void)
{
	FormulaFuncMax *funcMax = new FormulaFuncMax();
	insertElement(funcMax);
	return true;
}

bool SQLColumnParser::funcParserCount(void)
{
	FormulaFuncCount *funcCount = new FormulaFuncCount();
	insertElement(funcCount);
	return true;
}
