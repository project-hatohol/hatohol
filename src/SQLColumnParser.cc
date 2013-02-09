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
	string                       pendingWord;
	string                       pendingWordLower;
	string                       currFormulaString;

	// constructor and methods
	PrivateContext(void)
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
	SQLFormulaParser::copyFunctionParserMap(m_functionParserMap);
	m_functionParserMap["max"] =
	  static_cast<FunctionParser>(&SQLColumnParser::funcParserMax);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLColumnParser::SQLColumnParser(void)
{
	setFunctionParserMap(&m_functionParserMap);
	m_ctx = new PrivateContext();
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
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
	appendFormulaString(word);
	return SQLFormulaParser::add(word, wordLower);
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
	formulaInfo->expression = m_ctx->currFormulaString;
	m_ctx->currFormulaString.clear();
}

bool SQLColumnParser::closeCurrentFormula(void)
{
	SQLFormulaInfo *formulaInfo = new SQLFormulaInfo();
	m_formulaInfoVector.push_back(formulaInfo);
	formulaInfo->hasStatisticalFunc = hasStatisticalFunc();
	formulaInfo->formula = takeFormula();
	return true;
}

//
// SeparatorChecker callbacks
//
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
// functino parsers
//
bool SQLColumnParser::funcParserMax(void)
{
	FormulaFuncMax *funcMax = new FormulaFuncMax();
	return insertElement(funcMax);
}

