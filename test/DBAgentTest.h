#ifndef DBAgentTestCommon_h
#define DBAgentTestCommon_h

#include "SQLProcessorTypes.h"
#include "DBAgent.h"

extern const char *TABLE_NAME_TEST;
extern const ColumnDef COLUMN_DEF_TEST[];
extern const size_t NUM_COLUMNS_TEST;

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_AGE,
	IDX_TEST_TABLE_NAME,
	IDX_TEST_TABLE_HEIGHT,
};

extern const size_t NUM_TEST_DATA;
extern const uint64_t ID[];
extern const int AGE[];
extern const char *NAME[];
extern const double HEIGHT[];

class DBAgentChecker {
public:
	virtual void assertTable(const DBAgentTableCreationArg &arg) = 0;
};

#endif // DBAgentTestCommon_h
