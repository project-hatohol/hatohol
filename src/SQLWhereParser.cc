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

#include "SQLWhereParser.h"
#include "FormulaOperator.h"
#include "ItemDataUtils.h"
#include "SQLProcessorException.h"
#include "FormulaOperator.h"

enum BetweenStep {
	BETWEEN_STEP_NULL,
	BETWEEN_STEP_EXPECT_V0,
	BETWEEN_STEP_EXPECT_AND,
	BETWEEN_STEP_EXPECT_V1,
};

enum InStep {
	IN_STEP_NULL,
	IN_STEP_EXPECT_PARENTHESIS_OPEN,
	IN_STEP_EXPECT_VALUE,
	IN_STEP_GOT_VALUE,
};


struct SQLWhereParser::PrivateContext {
	BetweenStep betweenStep;
	ItemDataPtr betweenV0;
	ItemDataPtr betweenV1;

	InStep       inStep;
	ItemGroupPtr inValues;
	bool         openQuot;

	// constructor
	PrivateContext(void)
	: betweenStep(BETWEEN_STEP_NULL),
	  inStep(IN_STEP_NULL),
	  openQuot(false)
	{
	}

	void clear(void)
	{
		betweenStep = BETWEEN_STEP_NULL;
		betweenV0 = NULL;
		betweenV1 = NULL;

		inStep = IN_STEP_NULL;
		inValues = NULL;
		openQuot = false;
	}
};

// ---------------------------------------------------------------------------
// Static public methods
// ---------------------------------------------------------------------------
SQLFormulaParser::KeywordHandlerMap SQLWhereParser::m_keywordHandlerMap;


void SQLWhereParser::init(void)
{
	SQLFormulaParser::copyKeywordHandlerMap(m_keywordHandlerMap);
	m_keywordHandlerMap["between"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerBetween);
	m_keywordHandlerMap["in"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerIn);
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SQLWhereParser::SQLWhereParser(void)
: m_ctx(NULL)
{
	setKeywordHandlerMap(&m_keywordHandlerMap);
	m_ctx = new PrivateContext();
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->addSeparator("=");
	separator->setCallbackTempl<SQLWhereParser>
	  ('=', _separatorCbEqual, this);
	separator->addSeparator(">");
	separator->setCallbackTempl<SQLWhereParser>
	  ('>', _separatorCbGreaterThan, this);
}

SQLWhereParser::~SQLWhereParser()
{
	if (m_ctx)
		delete m_ctx;
}

void SQLWhereParser::add(string& word, string &wordLower)
{
	if (m_ctx->betweenStep != BETWEEN_STEP_NULL)
		addForBetween(word, wordLower);
	else if (m_ctx->inStep != IN_STEP_NULL)
		addForIn(word, wordLower);
	else
		SQLFormulaParser::add(word, wordLower);
}

void SQLWhereParser::clear(void)
{
	m_ctx->clear();
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->unsetAlternative();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

//
// general sub routines
//
void SQLWhereParser::createBetweenElement(void)
{
	if (*m_ctx->betweenV0 >= *m_ctx->betweenV1) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Invalid between argument order: (%s) >= v1 (%s)",
		  m_ctx->betweenV0->getString().c_str(),
		  m_ctx->betweenV1->getString().c_str());
	}
	FormulaElement *elem = new FormulaBetween(m_ctx->betweenV0,
	                                          m_ctx->betweenV1);
	insertElement(elem);
	clear();
}

void SQLWhereParser::addForBetween(const string& word, const string &wordLower)
{
	if (m_ctx->betweenStep == BETWEEN_STEP_EXPECT_V0) {
		m_ctx->betweenV0 = ItemDataUtils::createAsNumber(word);
		if (!m_ctx->betweenV0.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to parse as a number: %s", word.c_str());
		} else {
			m_ctx->betweenStep = BETWEEN_STEP_EXPECT_AND;
		}
	} else if (m_ctx->betweenStep == BETWEEN_STEP_EXPECT_AND) {
		if (wordLower != "and") {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Expected 'and', bug got: %s", word.c_str());
		} else {
			m_ctx->betweenStep = BETWEEN_STEP_EXPECT_V1;
		}
	} else if (m_ctx->betweenStep == BETWEEN_STEP_EXPECT_V1) {
		m_ctx->betweenV1 = ItemDataUtils::createAsNumber(word);
		if (!m_ctx->betweenV1.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to parse as a number: %s", word.c_str());
		} else  {
			createBetweenElement();
		}
	} else {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Illegal state: %d", m_ctx->betweenStep);
	}
}

void SQLWhereParser::closeInParenthesis(void)
{
	if (m_ctx->inValues->getNumberOfItems() == 0)
		THROW_SQL_PROCESSOR_EXCEPTION("No parameters in IN operator.");
	FormulaIn *formulaIn = new FormulaIn(m_ctx->inValues);
	insertElement(formulaIn);
}

void SQLWhereParser::addForIn(const string& word, const string &wordLower)
{
	if (m_ctx->inStep != IN_STEP_EXPECT_VALUE) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Illegal state: %d", m_ctx->betweenStep);
	}

	ItemDataPtr dataPtr = ItemDataUtils::createAsNumberOrString(word);
	if (!dataPtr.hasData()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Failed to parse: %s", word.c_str());
	}
	m_ctx->inValues->add(dataPtr);
	m_ctx->inStep = IN_STEP_GOT_VALUE;
}

//
// SeparatorChecker callbacks
//
void SQLWhereParser::_separatorCbEqual(const char separator,
                                       SQLWhereParser *whereParser)
{
	whereParser->separatorCbEqual(separator);
}

void SQLWhereParser::separatorCbEqual(const char separator)
{
	flush();

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement) {
		THROW_SQL_PROCESSOR_EXCEPTION("No left hand side of '='.");
		return;
	}

	// create ComparatorEqual
	FormulaComparatorEqual *formulaComparatorEqual
	  = new FormulaComparatorEqual();
	insertElement(formulaComparatorEqual);
}

void SQLWhereParser::_separatorCbGreaterThan(const char separator,
                                       SQLWhereParser *whereParser)
{
	whereParser->separatorCbGreaterThan(separator);
}

void SQLWhereParser::separatorCbGreaterThan(const char separator)
{
	flush();

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION("No left hand side of '>'.");

	FormulaGreaterThan *formulaGreaterThan = new FormulaGreaterThan();
	insertElement(formulaGreaterThan);
}

void SQLWhereParser::separatorCbParenthesisOpen(const char separator)
{
	if (m_ctx->inStep == IN_STEP_NULL)
		SQLFormulaParser::separatorCbParenthesisOpen(separator);
	else if (m_ctx->inStep == IN_STEP_EXPECT_PARENTHESIS_OPEN)
		m_ctx->inStep = IN_STEP_EXPECT_VALUE;
	else 
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: '('");
}

void SQLWhereParser::separatorCbParenthesisClose(const char separator)
{
	if (m_ctx->inStep == IN_STEP_NULL) {
		SQLFormulaParser::separatorCbParenthesisClose(separator);
	} else if (m_ctx->inStep == IN_STEP_GOT_VALUE) {
		closeInParenthesis();
		m_ctx->inStep = IN_STEP_NULL;
	} else
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: ')'");
}

void SQLWhereParser::separatorCbQuot(const char separator)
{
	if (m_ctx->inStep == IN_STEP_NULL) {
		SQLFormulaParser::separatorCbQuot(separator);
	} else if (m_ctx->inStep == IN_STEP_EXPECT_VALUE && !m_ctx->openQuot) {
		m_ctx->openQuot = true;
		SeparatorCheckerWithCallback *separator = getSeparatorChecker();
		separator->setAlternative(&ParsableString::SEPARATOR_QUOT);
	} else if (m_ctx->inStep == IN_STEP_GOT_VALUE && m_ctx->openQuot) {
		m_ctx->openQuot = false;
		SeparatorCheckerWithCallback *separator = getSeparatorChecker();
		separator->unsetAlternative();
	} else {
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: Quotation.");
	}
}

//
// Keyword handlers
//
void SQLWhereParser::kwHandlerBetween(void)
{
	FormulaElement *currElem = getCurrentElement();
	if (!currElem) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Got 'between', but no column name just before it.");
	}
	FormulaVariable *formulaVariable
	   = dynamic_cast<FormulaVariable *>(currElem);
	if (!formulaVariable) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not a valid column name before 'between'.");
	}
	// Note: 'formulaVariable' checked above will be the left child
	// by insertElement() in createBetweenElement().

	m_ctx->betweenStep = BETWEEN_STEP_EXPECT_V0;
	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->setAlternative(&ParsableString::SEPARATOR_SPACE);
}

void SQLWhereParser::kwHandlerIn(void)
{
	FormulaElement *currElem = getCurrentElement();
	if (!currElem) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Got 'in', but no column name just before it.");
	}
	FormulaVariable *formulaVariable
	   = dynamic_cast<FormulaVariable *>(currElem);
	if (!formulaVariable) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not a valid column name before 'in'.");
	}
	// Note: 'formulaVariable' checked above will be the left child
	// by insertElement() in createBetweenElement().

	m_ctx->inStep = IN_STEP_EXPECT_PARENTHESIS_OPEN;
}
