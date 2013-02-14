#ifndef SQLProcessorInsert_h
#define SQLProcessorInsert_h

#include "ParsableString.h"
using namespace mlpl;


struct SQLInsertInfo {
	// input statement
	ParsableString   statement;
};

class SQLProcessorInsert
{
public:
	static void init(void);
	virtual bool insert(SQLInsertInfo &insertInfo);
};

#endif // SQLProcessorInsert_h

