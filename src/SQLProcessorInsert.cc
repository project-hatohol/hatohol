#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <stdexcept>
using namespace std;

#include "SQLProcessorInsert.h"
#include "Utils.h"

enum ExpectedParenthesisType {
	EXPECTED_PARENTHESIS_NONE,
	EXPECTED_PARENTHESIS_OPEN_COLUMN,
	EXPECTED_PARENTHESIS_CLOSE_COLUMN,
	EXPECTED_PARENTHESIS_OPEN_VALUE,
	EXPECTED_PARENTHESIS_CLOSE_VALUE,
};

struct SQLProcessorInsert::PrivateContext {
	SQLInsertInfo     *insertInfo;
	string             currWord;
	string             currWordLower;
	InsertParseSection section;
	bool               errorFlag;
	ExpectedParenthesisType expectedParenthesis;
	string             pendingWord;

	// constructor
	PrivateContext(void)
	: insertInfo(NULL),
	  section(INSERT_PARSING_SECTION_INSERT),
	  errorFlag(false),
	  expectedParenthesis(EXPECTED_PARENTHESIS_NONE)
	{
	}
};

const SQLProcessorInsert::InsertSubParser
SQLProcessorInsert::m_insertSubParsers[] = {
	&SQLProcessorInsert::parseInsert,
	&SQLProcessorInsert::parseInto,
	&SQLProcessorInsert::parseTable,
	&SQLProcessorInsert::parseColumn,
	&SQLProcessorInsert::parseValuesKeyword,
	&SQLProcessorInsert::parseValue,
	&SQLProcessorInsert::parseEnd,
};

// ---------------------------------------------------------------------------
// Public methods (SQLInsertInfo)
// ---------------------------------------------------------------------------
SQLInsertInfo::SQLInsertInfo(ParsableString &_statement)
: statement(_statement)
{
}

SQLInsertInfo::~SQLInsertInfo()
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorInsert::init(void)
{
	// check the size of m_insertSubParsers
	size_t size = sizeof(SQLProcessorInsert::m_insertSubParsers) / 
	                sizeof(InsertSubParser);
	if (size != NUM_INSERT_PARSING_SECTION) {
		string msg;
		TRMSG(msg, "sizeof(m_insertSubParsers) is invalid: "
		           "(expcect/actual: %d/%d).",
		      NUM_INSERT_PARSING_SECTION, size);
		throw logic_error(msg);
	}
}

SQLProcessorInsert::SQLProcessorInsert(void)
: m_ctx(NULL),
  m_separator(" (),\'")
{
	m_ctx = new PrivateContext();

	m_separator.setCallbackTempl<SQLProcessorInsert>
	  ('(', _separatorCbParenthesisOpen, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  (')', _separatorCbParenthesisClose, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  (',', _separatorCbComma, this);
	m_separator.setCallbackTempl<SQLProcessorInsert>
	  ('\'', _separatorCbQuot, this);
}

SQLProcessorInsert::~SQLProcessorInsert()
{
	if (m_ctx)
		delete m_ctx;
}

bool SQLProcessorInsert::insert(SQLInsertInfo &insertInfo)
{
	if (!parseInsertStatement(insertInfo))
		return false;
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool SQLProcessorInsert::parseInsertStatement(SQLInsertInfo &insertInfo)
{
	m_ctx->insertInfo = &insertInfo;
	while (!insertInfo.statement.finished()) {
		if (m_ctx->errorFlag)
			return false;

		m_ctx->currWord = insertInfo.statement.readWord(m_separator);
		if (m_ctx->currWord.empty())
			continue;
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		
		// parse each component
		if (m_ctx->section >= NUM_INSERT_PARSING_SECTION) {
			MLPL_BUG("section(%d) >= NUM_INSERT_PARSING_SECTION\n",
			         m_ctx->section);
			return false;
		}
		InsertSubParser subParser = m_insertSubParsers[m_ctx->section];
		if (!(this->*subParser)())
			return false;
	}
	return true;
}

//
// Sub parsers
//
bool SQLProcessorInsert::parseInsert(void)
{
	return checkCurrWord("insert", INSERT_PARSING_SECTION_INTO);
}

bool SQLProcessorInsert::parseInto(void)
{
	return checkCurrWord("into", INSERT_PARSING_SECTION_TABLE);
}

bool SQLProcessorInsert::parseTable(void)
{
	m_ctx->insertInfo->table = m_ctx->currWord;
	m_ctx->section = INSERT_PARSING_SECTION_COLUMN;
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_OPEN_COLUMN;
	return true;
}

bool SQLProcessorInsert::parseColumn(void)
{
	if (!m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is not empty: %s.\n",
		          m_ctx->pendingWord.c_str());
		return false;
	}
	m_ctx->pendingWord = m_ctx->currWord;
	return true;
}

bool SQLProcessorInsert::parseValuesKeyword(void)
{
	if (!checkCurrWord("values", INSERT_PARSING_SECTION_VALUE))
		return false;
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_OPEN_VALUE;
	return true;
}

bool SQLProcessorInsert::parseValue(void)
{
	if (!m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is not empty: %s.\n",
		          m_ctx->pendingWord.c_str());
		return false;
	}
	m_ctx->pendingWord = m_ctx->currWord;
	return true;
}

bool SQLProcessorInsert::parseEnd(void)
{
	MLPL_DBG("Parsing had already been done: %s\n",
	         m_ctx->currWord.c_str());
	return false;
}

//
// SeparatorChecker callbacks
//
void SQLProcessorInsert::_separatorCbParenthesisOpen(const char separator,
                                                     SQLProcessorInsert *obj)
{
	obj->separatorCbParenthesisOpen(separator);
}

void SQLProcessorInsert::separatorCbParenthesisOpen(const char separator)
{
	ExpectedParenthesisType expectedType = m_ctx->expectedParenthesis;
	if (expectedType == EXPECTED_PARENTHESIS_OPEN_COLUMN) {
		m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_CLOSE_COLUMN;
	} else  if (expectedType == EXPECTED_PARENTHESIS_OPEN_VALUE) {
		m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_CLOSE_VALUE;
	} else {
		MLPL_DBG("Illegal state: m_ctx->expectedParenthesis: %d\n",
		         expectedType);
		m_ctx->errorFlag = true;
	}
}

void SQLProcessorInsert::_separatorCbParenthesisClose(const char separator,
                                         SQLProcessorInsert *obj)
{
	obj->separatorCbParenthesisClose(separator);
}

void SQLProcessorInsert::separatorCbParenthesisClose(const char separator)
{
	ExpectedParenthesisType expectedType = m_ctx->expectedParenthesis;
	if (expectedType == EXPECTED_PARENTHESIS_CLOSE_COLUMN) {
		if (!pushColumn()) {
			m_ctx->errorFlag = true;
			return;
		}
		m_ctx->section = INSERT_PARSING_SECTION_VALUES_KEYWORD;
	} else if (expectedType == EXPECTED_PARENTHESIS_CLOSE_VALUE) {
		if (!pushValue()) {
			m_ctx->errorFlag = true;
			return;
		}
		m_ctx->section = INSERT_PARSING_SECTION_END;
	} else {
		MLPL_DBG("Illegal state: m_ctx->expectedParenthesis: %d\n",
		         m_ctx->expectedParenthesis);
		m_ctx->errorFlag = true;
	}
	m_ctx->expectedParenthesis = EXPECTED_PARENTHESIS_NONE;
}

void SQLProcessorInsert::_separatorCbComma(const char separator,
                                           SQLProcessorInsert *obj)
{
	obj->separatorCbComma(separator);
}

void SQLProcessorInsert::separatorCbComma(const char separator)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void SQLProcessorInsert::_separatorCbQuot(const char separator,
                                          SQLProcessorInsert *obj)
{
	obj->separatorCbQuot(separator);
}

void SQLProcessorInsert::separatorCbQuot(const char separator)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

//
// General sub routines
//
bool SQLProcessorInsert::checkCurrWord(string expected,
                                       InsertParseSection nextSection)
{
	if (m_ctx->currWordLower != expected) {
		MLPL_DBG("currWordLower is not '%s': %s\n",
		         expected.c_str(), m_ctx->currWordLower.c_str());
		return false;
	}
	m_ctx->section = nextSection;
	return true;
}

bool SQLProcessorInsert::pushColumn(void)
{
	if (m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is empty.\n");
		return false;
	}
	m_ctx->insertInfo->columnVector.push_back(m_ctx->pendingWord);
	m_ctx->pendingWord.clear();
	return true;
}

bool SQLProcessorInsert::pushValue(void)
{
	if (m_ctx->pendingWord.empty()) {
		MLPL_DBG("m_ctx->pendingWord is empty.\n");
		return false;
	}
	m_ctx->insertInfo->valueVector.push_back(m_ctx->pendingWord);
	m_ctx->pendingWord.clear();
	return true;
}
