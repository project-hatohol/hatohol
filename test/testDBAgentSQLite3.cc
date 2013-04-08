#define __STDC_FORMAT_MACROS
#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "StringUtils.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"
#include "Helpers.h"
#include "DBAgentTest.h"

namespace testDBAgentSQLite3 {

static string dbPath = "/tmp/testDBAgentSQLite3.db";

static const char *TABLE_NAME_TEST = "test_table";
static const ColumnDef COLUMN_DEF_TEST[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TEST,                   // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TEST,                   // tableName
	"age",                             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TEST,                   // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TEST,                   // tableName
	"height",                          // columnName
	SQL_COLUMN_TYPE_DOUBLE,            // type
	3,                                 // columnLength
	1,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_TEST =
  sizeof(COLUMN_DEF_TEST) / sizeof(ColumnDef);

static void deleteDB(void)
{
	unlink(dbPath.c_str());
	cut_assert_not_exist_path(dbPath.c_str());
}

static int getDBVersion(void)
{
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select version from system\"",
	               dbPath.c_str());
	string stdoutStr = executeCommand(cmd);
	int version = atoi(stdoutStr.c_str());
	return version;
}

#define DEFINE_DBAGENT_WITH_INIT(DB_NAME, OBJ_NAME) \
string _path = getFixturesDir() + DB_NAME; \
DBAgentSQLite3::defineDBPath(DefaultDBDomainId, _path); \
DBAgentSQLite3 OBJ_NAME; \

template<typename T> void _assertAddToDB
  (T *elem, void (*func)(DBAgentSQLite3 &, T *))
{
	DBAgentSQLite3 dbAgent;
	bool gotException = false;
	try {
		(*func)(dbAgent, elem);
	} catch (const AsuraException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

static void addTargetServer
  (DBAgentSQLite3 &dbAgent, MonitoringServerInfo *serverInfo)
{
	dbAgent.addTargetServer(serverInfo);
}
#define assertAddServerToDB(X) \
cut_trace(_assertAddToDB<MonitoringServerInfo>(X, addTargetServer))

static void addTriggerInfo(DBAgentSQLite3 &dbAgent, TriggerInfo *triggerInfo)
{
	dbAgent.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

void _assertCreateStatic(void)
{
	DBAgentTableCreationArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	DBAgentSQLite3::createTable(dbPath, arg);

	// check if the table has been created successfully
	cut_assert_exist_path(dbPath.c_str());
	string cmd = StringUtils::sprintf("sqlite3 %s \".table\"",
	                                  dbPath.c_str());
	string output = executeCommand(cmd);
	assertExist(TABLE_NAME_TEST, output);
}
#define assertCreateStatic() cut_trace(_assertCreateStatic())

static string makeExpectedOutput(MonitoringServerInfo *serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s\n",
	                        serverInfo->id, serverInfo->type,
	                        serverInfo->hostName.c_str(),
	                        serverInfo->ipAddress.c_str(),
	                        serverInfo->nickname.c_str());
	return expectedOut;
}

static string makeExpectedOutput(TriggerInfo *triggerInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%d|%d|%d|%u|%s|%s|%s\n",
	                        triggerInfo->id,
	                        triggerInfo->status, triggerInfo->severity,
	                        triggerInfo->lastChangeTime.tv_sec,
	                        triggerInfo->lastChangeTime.tv_nsec,
	                        triggerInfo->serverId,
	                        triggerInfo->hostId.c_str(),
	                        triggerInfo->hostName.c_str(),
	                        triggerInfo->brief.c_str());
	return expectedOut;
}

void setup(void)
{
	deleteDB();
	DBAgentSQLite3::init();
	DBAgentSQLite3::defineDBPath(DefaultDBDomainId, dbPath);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createSystemTable(void)
{
	DBAgentSQLite3 dbAgent;
	cut_assert_exist_path(dbPath.c_str());
	cppcut_assert_equal(DBAgentSQLite3::DB_VERSION, getDBVersion());
}

void test_testIsTableExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(true, dbAgent.isTableExisting("foo"));
}

void test_testIsTableExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(false, dbAgent.isTableExisting("NotExistTable"));
}

void test_testIsRecordExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectTrueCondition = "id=1";
	cppcut_assert_equal
	  (true, dbAgent.isRecordExisting("foo", expectTrueCondition));
}

void test_testIsRecordExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectFalseCondition = "id=100";
	cppcut_assert_equal
	  (false, dbAgent.isRecordExisting("foo", expectFalseCondition));
}

void test_testAddTargetServer(void)
{
	// added a record
	MonitoringServerInfo *testInfo = serverInfo;
	assertAddServerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from servers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_testGetTargetServers(void)
{
	for (size_t i = 0; i < NumServerInfo; i++)
		assertAddServerToDB(&serverInfo[i]);

	DBAgentSQLite3 dbAgent;
	MonitoringServerInfoList monitoringServers;
	dbAgent.getTargetServers(monitoringServers);
	cppcut_assert_equal(NumServerInfo, monitoringServers.size());

	string expectedText;
	string actualText;
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (size_t i = 0; i < NumServerInfo; i++, ++it) {
		expectedText += makeExpectedOutput(&serverInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}

void test_testAddTriggerInfo(void)
{
	// added a record
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from triggers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_testGetTriggerInfoList(void)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		assertAddTriggerToDB(&testTriggerInfo[i]);

	DBAgentSQLite3 dbAgent;
	TriggerInfoList triggerInfoList;
	dbAgent.getTriggerInfoList(triggerInfoList);
	cppcut_assert_equal(NumTestTriggerInfo, triggerInfoList.size());

	string expectedText;
	string actualText;
	TriggerInfoListIterator it = triggerInfoList.begin();
	for (size_t i = 0; i < NumTestTriggerInfo; i++, ++it) {
		expectedText += makeExpectedOutput(&testTriggerInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}

void test_createStatic(void)
{
	assertCreateStatic();
}

void test_insertStatic(void)
{
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;

	DBAgentValue val;
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;

	val.vUint64 = ID;
	arg.row.push_back(val);

	val.vInt = AGE;
	arg.row.push_back(val);

	val.vString = NAME;
	arg.row.push_back(val);

	// height
	val.vDouble = HEIGHT;
	arg.row.push_back(val);

	test_createStatic();
	DBAgentTableCreationArg creatArg;
	creatArg.tableName = TABLE_NAME_TEST;
	creatArg.numColumns = NUM_COLUMNS_TEST;
	creatArg.columnDefs = COLUMN_DEF_TEST;
	DBAgentSQLite3::insert(dbPath, arg);

	// check if the columns is inserted
	string cmd = StringUtils::sprintf("sqlite3 %s \"select * from %s\"",
	                                  dbPath.c_str(), TABLE_NAME_TEST);
	string output = executeCommand(cmd);
	
	const int indexDefHeight = 3;
	const ColumnDef &columnDefHeight = COLUMN_DEF_TEST[indexDefHeight];
	
	string fmt = StringUtils::sprintf("%%"PRIu64"|%%d|%%s|%%%d.%dlf\n",
	                                  columnDefHeight.columnLength,
	                                  columnDefHeight.decFracLength);
	string expectedOut = StringUtils::sprintf(fmt.c_str(),
	                                          ID, AGE, NAME, HEIGHT);
	cppcut_assert_equal(expectedOut, output);
}

} // testDBAgentSQLite3

