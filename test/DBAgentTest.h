#ifndef DBAgentTestCommon_h
#define DBAgentTestCommon_h

#include <cppcutter.h>
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
	virtual void assertInsert(const DBAgentInsertArg &arg,
	                          uint64_t id, int age, const char *name,
	                          double height) = 0;
	virtual void assertUpdate(uint64_t id, int age,
	                          const char *name, double height,
	                          const string &condition = "") = 0;
	virtual void getIDStringVector(const ColumnDef &columnDefId,
	                               vector<string> &actualIds) = 0;

	static void createTable(DBAgent &dbAgent);
	static void insert(DBAgent &dbAgent, uint64_t id, int age,
	                   const char *name, double height);
	static void makeTestData(DBAgent &dbAgent);
	static void makeTestData(DBAgent &dbAgent,
	                         map<uint64_t, size_t> &testDataIdIndexMap);
};

void _checkInsert(DBAgent &dbAgent, DBAgentChecker &checker,
                   uint64_t id, int age, const char *name, double height);
#define checkInsert(AGENT,CHECKER,ID,AGE,NAME,HEIGHT) \
cut_trace(_checkInsert(AGENT,CHECKER,ID,AGE,NAME,HEIGHT));

void dbAgentTestCreateTable(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestInsert(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestInsertUint64
  (DBAgent &dbAgent, DBAgentChecker &checker, uint64_t id);
void dbAgentTestUpdate(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdateCondition(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestSelect(DBAgent &dbAgent);
void dbAgentTestSelectEx(DBAgent &dbAgent);
void dbAgentTestSelectExWithCond(DBAgent &dbAgent);
void dbAgentTestSelectExWithCondAllColumns(DBAgent &dbAgent);
void dbAgentTestSelectHeightOrder
 (DBAgent &dbAgent, size_t limit = 0, size_t offset = 0,
  size_t forceExpectedRows = (size_t)-1);
void dbAgentTestDelete(DBAgent &dbAgent, DBAgentChecker &checker);

#endif // DBAgentTestCommon_h
