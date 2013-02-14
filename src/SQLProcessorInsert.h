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

	//
	// Sub parsers
	//
	bool parseInsert(void);
	bool parseTable(void);
	bool parseColumn(void);
	bool parseValue(void);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbParenthesisOpen(const char separator,
	                                        SQLProcessorInsert *obj);
	void separatorCbParenthesisOpen(const char separator);

	static void _separatorCbParenthesisClose(const char separator,
	                                         SQLProcessorInsert *obj);
	void separatorCbParenthesisClose(const char separator);

	static void _separatorCbComma(const char separator,
	                              SQLProcessorInsert *obj);
	void separatorCbComma(const char separator);

	static void _separatorCbQuot(const char separator,
	                             SQLProcessorInsert *obj);
	void separatorCbQuot(const char separator);

private:
	static const InsertSubParser m_insertSubParsers[];
	enum InsertParseSection {
		INSERT_PARSING_SECTION_INSERT,
		INSERT_PARSING_SECTION_TABLE,
		INSERT_PARSING_SECTION_COLUMN,
		INSERT_PARSING_SECTION_VALUE,
		NUM_INSERT_PARSING_SECTION,
	};

	PrivateContext              *m_ctx;
	SeparatorCheckerWithCallback m_separator;
};

#endif // SQLProcessorInsert_h

