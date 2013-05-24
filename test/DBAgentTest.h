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

template<class AGENT, class AGENT_CHECKER>
void createTable(AGENT &dbAgent)
{
	DBAgentTableCreationArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	dbAgent.createTable(arg);

	AGENT_CHECKER checker;
	checker.assertTable(arg);
};

template<class AGENT, class AGENT_CHECKER>
void testInsert(AGENT &dbAgent)
{
	// create table
	createTable<AGENT, AGENT_CHECKER>(dbAgent);

	// insert a row
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;

	AGENT_CHECKER checker;
	checkInsert(dbAgent, checker, ID, AGE, NAME, HEIGHT);
}

template<class AGENT, class AGENT_CHECKER>
void testInsertUint64(AGENT &dbAgent, uint64_t ID)
{
	// create table
	createTable<AGENT, AGENT_CHECKER>(dbAgent);

	// insert a row
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;

	AGENT_CHECKER checker;
	checkInsert(dbAgent, checker, ID, AGE, NAME, HEIGHT);
}

void dbAgentTestSelect(DBAgent &dbAgent);
void dbAgentTestSelectEx(DBAgent &dbAgent);
void dbAgentTestSelectExWithCond(DBAgent &dbAgent);
void dbAgentTestSelectExWithCondAllColumns(DBAgent &dbAgent);
void dbAgentTestSelectHeightOrder
 (DBAgent &dbAgent, size_t limit = 0, size_t offset = 0,
  size_t forceExpectedRows = (size_t)-1);

#endif // DBAgentTestCommon_h
