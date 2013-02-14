#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <stdexcept>
using namespace std;

#include "SQLProcessorInsert.h"
#include "Utils.h"

struct SQLProcessorInsert::PrivateContext {
	string             currWord;
	string             currWordLower;
	InsertParseSection section;

	// constructor
	PrivateContext(void)
	: section(INSERT_PARSING_SECTION_TABLE)
	{
	}
};

const SQLProcessorInsert::InsertSubParser
SQLProcessorInsert::m_insertSubParsers[] = {
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
	while (!insertInfo.statement.finished()) {
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
