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

enum KeywordParsingStep {
	KEYWORD_STEP_NULL,

	BETWEEN_STEP_EXPECT_V0,
	BETWEEN_STEP_EXPECT_AND,
	BETWEEN_STEP_EXPECT_V1,

	IN_STEP_EXPECT_PARENTHESIS_OPEN,
	IN_STEP_EXPECT_VALUE,
	IN_STEP_GOT_VALUE,

	EXISTS_STEP_EXPECT_PARENTHESIS_OPEN,
	EXISTS_STEP_EXPECT_SELECT,
	EXISTS_STEP_FIND_CLOSE,

	IS_STEP_EXPECT_NULL_OR_NOT,
	IS_STEP_GOT_NOT,
};

struct SQLWhereParser::PrivateContext {
	KeywordParsingStep kwParsingStep;

	ItemDataPtr betweenV0;
	ItemDataPtr betweenV1;

	ItemGroupPtr inValues;
	bool         openQuot;
	int          nestCountForExists;
	const char  *existsStatementBegin;

	// constructor
	PrivateContext(void)
	: kwParsingStep(KEYWORD_STEP_NULL),
	  openQuot(false),
	  nestCountForExists(0),
	  existsStatementBegin(NULL)
	{
	}

	void clear(void)
	{
		kwParsingStep = KEYWORD_STEP_NULL;

		betweenV0 = NULL;
		betweenV1 = NULL;

		inValues = ItemGroupPtr();
		openQuot = false;

		nestCountForExists = 0;
		existsStatementBegin = NULL;
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
	m_keywordHandlerMap["exists"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerExists);
	m_keywordHandlerMap["not"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerNot);
	m_keywordHandlerMap["is"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerIs);
	m_keywordHandlerMap["="] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerEqual);
	m_keywordHandlerMap["<"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerLessThan);
	m_keywordHandlerMap["<="] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerLessEqual);
	m_keywordHandlerMap[">"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerGreaterThan);
	m_keywordHandlerMap[">="] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerGreaterEqual);
	m_keywordHandlerMap["<>"] =
	  static_cast<KeywordHandler>(&SQLWhereParser::kwHandlerNotEqual);
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

	separator->addSeparator(",");
	separator->setCallbackTempl<SQLWhereParser>
	  (',', _separatorCbComma, this);
}

SQLWhereParser::~SQLWhereParser()
{
	if (m_ctx)
		delete m_ctx;
}

void SQLWhereParser::add(string& word, string &wordLower)
{
	if (m_ctx->kwParsingStep == KEYWORD_STEP_NULL)
		SQLFormulaParser::add(word, wordLower);
	else if (m_ctx->kwParsingStep <  IN_STEP_EXPECT_PARENTHESIS_OPEN)
		addForBetween(word, wordLower);
	else if (m_ctx->kwParsingStep < EXISTS_STEP_EXPECT_PARENTHESIS_OPEN)
		addForIn(word, wordLower);
	else if (m_ctx->kwParsingStep < IS_STEP_EXPECT_NULL_OR_NOT)
		addForExists(word, wordLower);
	else
		addForIs(word, wordLower);
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
	if (m_ctx->kwParsingStep == BETWEEN_STEP_EXPECT_V0) {
		m_ctx->betweenV0 = ItemDataUtils::createAsNumber(word);
		if (!m_ctx->betweenV0.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to parse as a number: %s", word.c_str());
		} else {
			m_ctx->kwParsingStep = BETWEEN_STEP_EXPECT_AND;
		}
	} else if (m_ctx->kwParsingStep == BETWEEN_STEP_EXPECT_AND) {
		if (wordLower != "and") {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Expected 'and', bug got: %s", word.c_str());
		} else {
			m_ctx->kwParsingStep = BETWEEN_STEP_EXPECT_V1;
		}
	} else if (m_ctx->kwParsingStep == BETWEEN_STEP_EXPECT_V1) {
		m_ctx->betweenV1 = ItemDataUtils::createAsNumber(word);
		if (!m_ctx->betweenV1.hasData()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to parse as a number: %s", word.c_str());
		} else  {
			createBetweenElement();
		}
	} else {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Illegal state: %d", m_ctx->kwParsingStep);
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
	if (m_ctx->kwParsingStep != IN_STEP_EXPECT_VALUE) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Illegal state: %d", m_ctx->kwParsingStep);
	}

	ItemDataPtr dataPtr = ItemDataUtils::createAsNumberOrString(word);
	if (!dataPtr.hasData()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Failed to parse: %s", word.c_str());
	}
	m_ctx->inValues->add(dataPtr);
	m_ctx->kwParsingStep = IN_STEP_GOT_VALUE;
}

void SQLWhereParser::addForExists(const string &word, const string &wordLower)
{
	// This condition occurs when the inner select statment has
	// parenthesis.
	if (m_ctx->kwParsingStep == EXISTS_STEP_FIND_CLOSE)
		return;

	if (m_ctx->kwParsingStep != EXISTS_STEP_EXPECT_SELECT) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Illegal state: %d", m_ctx->kwParsingStep);
	}
	if (wordLower != "select") {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Expected 'select' but got: %s", wordLower.c_str());
	}

	SeparatorCheckerWithCallback *separator = getSeparatorChecker();
	separator->setAlternative(&ParsableString::SEPARATOR_PARENTHESIS);
	m_ctx->kwParsingStep = EXISTS_STEP_FIND_CLOSE;
}

void SQLWhereParser::addForIs(const string &word, const string &wordLower)
{
	if (wordLower == "not") {
		m_ctx->kwParsingStep = IS_STEP_GOT_NOT;
		return;
	}

	if (wordLower != "null") {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Expected 'null' but got: %s", wordLower.c_str());
	}

	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "No left hand side of 'IS (NOT) NULL'.");
	}

	FormulaElement *formulaElement = NULL;
	if (m_ctx->kwParsingStep == IS_STEP_GOT_NOT)
		formulaElement = new FormulaIsNotNull();
	else 
		formulaElement = new FormulaIsNull();
	insertElement(formulaElement);
	clear();
}

void SQLWhereParser::setupParsingExists(void)
{
	SQLProcessorSelectShareInfo *shareInfo = getShareInfo();
	if (shareInfo == NULL)
		THROW_ASURA_EXCEPTION("getShareInfo(): NULL\n");
	shareInfo->allowSectionParserChange = false;
	m_ctx->existsStatementBegin =
	  shareInfo->statement->getParsingPosition() + 1;
	m_ctx->kwParsingStep = EXISTS_STEP_EXPECT_SELECT;
}

void SQLWhereParser::makeFormulaExistsAndCleanup(void)
{
	SQLProcessorSelectShareInfo *shareInfo = getShareInfo();
	const ParsableString *statement = shareInfo->statement;
	const char *end = statement->getParsingPosition() - 1;
	long length = end - m_ctx->existsStatementBegin;
	string innerSelect = string(m_ctx->existsStatementBegin, length);
	SQLProcessorSelectFactory &selectFactory =
	  shareInfo->processorSelectFactory;
	FormulaExists *formulaExists =
	  new FormulaExists(innerSelect, selectFactory);
	insertElement(formulaExists);
	shareInfo->allowSectionParserChange = true;
	clear();
}

//
// SeparatorChecker callbacks
//
void SQLWhereParser::_separatorCbComma(const char separator,
                                       SQLWhereParser *whereParser)
{
	whereParser->separatorCbComma(separator);
}

void SQLWhereParser::separatorCbComma(const char separator)
{
	if (m_ctx->kwParsingStep == IN_STEP_GOT_VALUE)
		m_ctx->kwParsingStep = IN_STEP_EXPECT_VALUE;
	else 
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: ','");
}

void SQLWhereParser::separatorCbParenthesisOpen(const char separator)
{
	if (m_ctx->kwParsingStep == KEYWORD_STEP_NULL)
		SQLFormulaParser::separatorCbParenthesisOpen(separator);
	else if (m_ctx->kwParsingStep == IN_STEP_EXPECT_PARENTHESIS_OPEN)
		m_ctx->kwParsingStep = IN_STEP_EXPECT_VALUE;
	else if (m_ctx->kwParsingStep == EXISTS_STEP_EXPECT_PARENTHESIS_OPEN)
		setupParsingExists();
	else if (m_ctx->kwParsingStep == EXISTS_STEP_FIND_CLOSE)
		m_ctx->nestCountForExists++;
	else 
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: '('");
}

void SQLWhereParser::separatorCbParenthesisClose(const char separator)
{
	if (m_ctx->kwParsingStep == KEYWORD_STEP_NULL) {
		SQLFormulaParser::separatorCbParenthesisClose(separator);
	} else if (m_ctx->kwParsingStep == IN_STEP_GOT_VALUE) {
		closeInParenthesis();
		m_ctx->kwParsingStep = KEYWORD_STEP_NULL;
	} else if (m_ctx->kwParsingStep == EXISTS_STEP_FIND_CLOSE) {
		if (m_ctx->nestCountForExists == 0) {
			makeFormulaExistsAndCleanup();
		} else
			m_ctx->nestCountForExists--;
	} else
		THROW_SQL_PROCESSOR_EXCEPTION("Unexpected: ')'");
}

void SQLWhereParser::separatorCbQuot(const char separator)
{
	if (m_ctx->kwParsingStep == KEYWORD_STEP_NULL) {
		SQLFormulaParser::separatorCbQuot(separator);
	} else if (m_ctx->kwParsingStep == IN_STEP_EXPECT_VALUE
	           && !m_ctx->openQuot) {
		m_ctx->openQuot = true;
		SeparatorCheckerWithCallback *separator = getSeparatorChecker();
		separator->setAlternative(&ParsableString::SEPARATOR_QUOT);
	} else if (m_ctx->kwParsingStep == IN_STEP_GOT_VALUE
	           && m_ctx->openQuot) {
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

	m_ctx->kwParsingStep = BETWEEN_STEP_EXPECT_V0;
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

	m_ctx->kwParsingStep = IN_STEP_EXPECT_PARENTHESIS_OPEN;
}

void SQLWhereParser::kwHandlerExists(void)
{
	m_ctx->kwParsingStep = EXISTS_STEP_EXPECT_PARENTHESIS_OPEN;
}

void SQLWhereParser::kwHandlerNot(void)
{
	FormulaOperatorNot *formulaOperatorNot = new FormulaOperatorNot();
	insertElement(formulaOperatorNot);
}

void SQLWhereParser::kwHandlerIs(void)
{
	m_ctx->kwParsingStep = IS_STEP_EXPECT_NULL_OR_NOT;
}

void SQLWhereParser::kwHandlerEqual(void)
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

void SQLWhereParser::kwHandlerLessThan(void)
{
	THROW_SQL_PROCESSOR_EXCEPTION("Not implemented: %s",
	                              __PRETTY_FUNCTION__);
}

void SQLWhereParser::kwHandlerLessEqual(void)
{
	THROW_SQL_PROCESSOR_EXCEPTION("Not implemented: %s",
	                              __PRETTY_FUNCTION__);
}

void SQLWhereParser::kwHandlerGreaterThan(void)
{
	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION("No left hand side of '>'.");

	FormulaGreaterThan *formulaGreaterThan = new FormulaGreaterThan();
	insertElement(formulaGreaterThan);
}

void SQLWhereParser::kwHandlerGreaterEqual(void)
{
	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION("No left hand side of '>='.");

	FormulaGreaterOrEqual *formulaGreaterOrEqual =
	  new FormulaGreaterOrEqual();
	insertElement(formulaGreaterOrEqual);
}

void SQLWhereParser::kwHandlerNotEqual(void)
{
	// Get Left-Hand
	FormulaElement *lhsElement = getCurrentElement();
	if (!lhsElement)
		THROW_SQL_PROCESSOR_EXCEPTION("No left hand side of '<>'.");

	FormulaComparatorNotEqual *formulaComparatorNotEqual =
	  new FormulaComparatorNotEqual();
	insertElement(formulaComparatorNotEqual);
}
