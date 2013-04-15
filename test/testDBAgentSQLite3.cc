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

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_AGE,
	IDX_TEST_TABLE_NAME,
	IDX_TEST_TABLE_HEIGHT,
};

static const size_t NUM_TEST_DATA = 3;
static const uint64_t ID[NUM_TEST_DATA]   = {1, 2, 0xfedcba9876543210};
static const int AGE[NUM_TEST_DATA]       = {14, 17, 180};
static const char *NAME[NUM_TEST_DATA]    = {"rei", "aoi", "giraffe"};
static const double HEIGHT[NUM_TEST_DATA] = {158.2, 203.9, -23593.2};
static map<uint64_t, size_t> g_testDataIdIndexMap;

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

void _assertExistRecord(uint64_t id, int age, const char *name, double height)
{
	// check if the columns is inserted

	// INFO: We use the trick that unsigned interger is stored as
	// signed interger. So large integers (MSB bit is one) are recognized
	// as negative intergers. So we use PRId64 in the following statement.
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from %s where id=%"PRId64 "\"",
	               dbPath.c_str(), TABLE_NAME_TEST, id);
	string output = executeCommand(cmd);
	
	const ColumnDef &columnDefHeight =
	   COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT];
	
	// Here we also use PRId64 (not PRIu64) with the same
	// reason of the above comment.
	string fmt = StringUtils::sprintf("%%"PRId64"|%%d|%%s|%%%d.%dlf\n",
	                                  columnDefHeight.columnLength,
	                                  columnDefHeight.decFracLength);
	string expectedOut = StringUtils::sprintf(fmt.c_str(),
	                                          id, age, name, height);
	cppcut_assert_equal(expectedOut, output);
}
#define assertExistRecord(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertExistRecord(ID,AGE,NAME,HEIGHT))

void _assertInsertStatic(uint64_t id, int age, const char *name, double height)
{
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	arg.row->add(new ItemUint64(id), false);
	arg.row->add(new ItemInt(age), false);
	arg.row->add(new ItemString(name), false);
	arg.row->add(new ItemDouble(height), false);
	DBAgentSQLite3::insert(dbPath, arg);

	assertExistRecord(id, age, name,height);
}
#define assertInsertStatic(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertInsertStatic(ID,AGE,NAME,HEIGHT));

void _assertUpdateStatic(uint64_t id, int age, const char *name, double height)
{
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_TEST;
	for (size_t i = IDX_TEST_TABLE_ID; i < NUM_COLUMNS_TEST; i++)
		arg.columnIndexes.push_back(i);
	arg.columnDefs = COLUMN_DEF_TEST;
	arg.row->add(new ItemUint64(id), false);
	arg.row->add(new ItemInt(age), false);
	arg.row->add(new ItemString(name), false);
	arg.row->add(new ItemDouble(height), false);
	DBAgentSQLite3::update(dbPath, arg);

	assertExistRecord(id, age, name,height);
}
#define assertUpdateStatic(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertUpdateStatic(ID,AGE,NAME,HEIGHT));

static void makeTestDB(void)
{
	// make table
	assertCreateStatic();

	// insert data
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		assertInsertStatic(ID[i], AGE[i], NAME[i], HEIGHT[i]);
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		g_testDataIdIndexMap[ID[i]] = i;
	cppcut_assert_equal(NUM_TEST_DATA, g_testDataIdIndexMap.size());
}

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

static void _assertSelectHeightOrder(size_t limit = 0, size_t offset = 0)
{
	makeTestDB();
	DBAgentSelectWithStatementArg arg;
	arg.tableName = TABLE_NAME_TEST;
	const ColumnDef &columnDef = COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT];
	arg.statements.push_back(columnDef.columnName);
	arg.columnTypes.push_back(columnDef.type);
	arg.orderBy = StringUtils::sprintf("%s DESC", columnDef.columnName);
	arg.limit = limit;
	arg.offset = offset;
	DBAgentSQLite3::select(dbPath, arg);

	// check the result
	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	size_t numExpectedRows = limit == 0 ? NUM_TEST_DATA : arg.limit;
	cppcut_assert_equal(numExpectedRows, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	set<double> expectedSet;
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		expectedSet.insert(HEIGHT[i]);

	ItemGroupListConstIterator grpListIt = itemList.begin();
	set<double>::reverse_iterator heightIt = expectedSet.rbegin();
	size_t count = 0;
	for (size_t i = 0; i < NUM_TEST_DATA && count < arg.limit;
	     i++, ++grpListIt, ++heightIt, count++) {
		int idx = 0;
		double expected = *heightIt;
		if (i < arg.offset)
			continue;
		assertItemData(double, *grpListIt, expected, idx);
	}
}
#define assertSelectHeightOrder(...) \
cut_trace(_assertSelectHeightOrder(__VA_ARGS__))

void setup(void)
{
	g_testDataIdIndexMap.clear();
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
	// create table
	assertCreateStatic();

	// insert a row
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsertStatic(ID, AGE, NAME, HEIGHT);
}

void test_insertStaticUint64_0x7fffffffffffffff(void)
{
	// create table
	assertCreateStatic();

	// insert a row
	const uint64_t ID = 0x7fffffffffffffff;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsertStatic(ID, AGE, NAME, HEIGHT);
}

void test_insertStaticUint64_0x8000000000000000(void)
{
	// create table
	assertCreateStatic();

	// insert a row
	const uint64_t ID = 0x8000000000000000;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsertStatic(ID, AGE, NAME, HEIGHT);
}

void test_insertStaticUint64_0xffffffffffffffff(void)
{
	// create table
	assertCreateStatic();

	// insert a row
	const uint64_t ID = 0xffffffffffffffff;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsertStatic(ID, AGE, NAME, HEIGHT);
}

void test_updateStatic(void)
{
	// create table and insert a row
	test_insertStatic();

	// insert a row
	const uint64_t ID = 9;
	const int AGE = 20;
	const char *NAME = "yui";
	const double HEIGHT = 158.0;
	assertUpdateStatic(ID, AGE, NAME, HEIGHT);
}

void test_selectStatic(void)
{
	makeTestDB();

	// get records
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	arg.columnIndexes.push_back(IDX_TEST_TABLE_ID);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_AGE);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_NAME);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_HEIGHT);
	DBAgentSQLite3::select(dbPath, arg);

	// check the result
	const ItemGroupList &groupList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal(groupList.size(), arg.dataTable->getNumberOfRows());
	ItemGroupListConstIterator it = groupList.begin();
	size_t srcDataIdx = 0;
	map<uint64_t, size_t>::iterator itrId;
	for (; it != groupList.end(); ++it, srcDataIdx++) {
		ItemData *itemData;
		size_t columnIdx = 0;
		const ItemGroup *itemGroup = *it;
		cppcut_assert_equal(itemGroup->getNumberOfItems(),
		                    NUM_COLUMNS_TEST);

		// id
		uint64_t id;
		itemData = itemGroup->getItemAt(columnIdx++);
		itemData->get(&id);
		itrId = g_testDataIdIndexMap.find(id);
		cppcut_assert_equal(false, itrId == g_testDataIdIndexMap.end(),
		                    cut_message("id: 0x%"PRIx64, id));
		srcDataIdx = itrId->second;

		// age
		int valInt;
		itemData = itemGroup->getItemAt(columnIdx++);
		itemData->get(&valInt);
		cppcut_assert_equal(AGE[srcDataIdx], valInt);

		// name
		string valStr;
		itemData = itemGroup->getItemAt(columnIdx++);
		itemData->get(&valStr);
		cppcut_assert_equal(NAME[srcDataIdx], valStr.c_str());

		// height
		double valDouble;
		itemData = itemGroup->getItemAt(columnIdx++);
		itemData->get(&valDouble);
		cppcut_assert_equal(HEIGHT[srcDataIdx], valDouble);

		// delete the element from idSet
		g_testDataIdIndexMap.erase(itrId);
	}
}

void test_selectStatementStatic(void)
{
	makeTestDB();
	DBAgentSelectWithStatementArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.statements.push_back("count(*)");
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_TEXT);
	DBAgentSQLite3::select(dbPath, arg);

	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal((size_t)1, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	string expectedCount = StringUtils::sprintf("%zd", NUM_TEST_DATA);
	cppcut_assert_equal(expectedCount,
	                    itemGroup->getItemAt(0)->getString());
}

void test_selectStatementStaticWithCond(void)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	makeTestDB();
	DBAgentSelectWithStatementArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.statements.push_back(columnDefId.columnName);
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_BIGUINT);

	arg.condition = StringUtils::sprintf
	                  ("%s=%zd", columnDefId.columnName, ID[targetRow]);
	DBAgentSQLite3::select(dbPath, arg);

	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal((size_t)1, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	ItemUint64 *item = dynamic_cast<ItemUint64 *>(itemGroup->getItemAt(0));
	cppcut_assert_not_null(item);
	cppcut_assert_equal(ID[targetRow], item->get());
}

void test_selectStatementStaticWithCondAllColumns(void)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	makeTestDB();
	DBAgentSelectWithStatementArg arg;
	arg.tableName = TABLE_NAME_TEST;
	for (size_t i = 0; i < NUM_COLUMNS_TEST; i++) {
		const ColumnDef &columnDef = COLUMN_DEF_TEST[i];
		arg.statements.push_back(columnDef.columnName);
		arg.columnTypes.push_back(columnDef.type);
	}

	arg.condition = StringUtils::sprintf
	                  ("%s=%zd", columnDefId.columnName, ID[targetRow]);
	DBAgentSQLite3::select(dbPath, arg);

	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal((size_t)1, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal(NUM_COLUMNS_TEST, itemGroup->getNumberOfItems());

	// check the results
	int idx = 0;
	assertItemData(uint64_t, itemGroup, ID[targetRow], idx);
	assertItemData(int,      itemGroup, AGE[targetRow], idx);
	assertItemData(string,   itemGroup, NAME[targetRow], idx);
	assertItemData(double,   itemGroup, HEIGHT[targetRow], idx);
}

void test_selectStatementStaticWithOrderBy(void)
{
	assertSelectHeightOrder();
}

void test_selectStatementStaticWithOrderByLimit(void)
{
	assertSelectHeightOrder(1);
}

} // testDBAgentSQLite3

