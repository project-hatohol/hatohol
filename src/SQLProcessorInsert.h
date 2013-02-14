#ifndef SQLProcessorInsert_h
#define SQLProcessorInsert_h

#include "ParsableString.h"
using namespace mlpl;


struct SQLInsertInfo {
	// input statement
	ParsableString   statement;

	// parsed matter
	string           table;
	StringVector     columnVector;
	StringVector     valueVector;

	//
	// constructor and destructor
	//
	SQLInsertInfo(ParsableString &_statment);
	virtual ~SQLInsertInfo();
};

class SQLProcessorInsert
{
public:
	static void init(void);
	SQLProcessorInsert(void);
	virtual ~SQLProcessorInsert();
	virtual bool insert(SQLInsertInfo &insertInfo);

protected:
	struct PrivateContext;
	typedef bool
	  (SQLProcessorInsert::*InsertSubParser)(void);

	bool parseInsertStatement(SQLInsertInfo &insertInfo);

private:
	static const InsertSubParser m_insertSubParsers[];
	enum InsertParseSection {
		INSERT_PARSING_SECTION_TABLE,
		INSERT_PARSING_SECTION_COLUMN,
		INSERT_PARSING_SECTION_VALUE,
		NUM_INSERT_PARSING_SECTION,
	};

	PrivateContext              *m_ctx;
	SeparatorCheckerWithCallback m_separator;
};

#endif // SQLProcessorInsert_h

