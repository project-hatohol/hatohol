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
	virtual bool insert(SQLInsertInfo &insertInfo);
};

#endif // SQLProcessorInsert_h

