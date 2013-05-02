#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "StringUtils.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"
#include "Helpers.h"

namespace testDBAgentSQLite3 {

static string dbPath = DBAgentSQLite3::getDBPath(DEFAULT_DB_DOMAIN_ID);

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

#define DEFINE_DBAGENT_WITH_INIT(DB_NAME, OBJ_NAME) \
string _path = getFixturesDir() + DB_NAME; \
DBAgentSQLite3::defineDBPath(DEFAULT_DB_DOMAIN_ID, _path); \
DBAgentSQLite3 OBJ_NAME; \

void _assertCreate(void)
{
	DBAgentSQLite3 dbAgent;

	DBAgentTableCreationArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	dbAgent.createTable(arg);

	// check if the table has been created successfully
	cut_assert_exist_path(dbPath.c_str());
	string cmd = StringUtils::sprintf("sqlite3 %s \".table\"",
	                                  dbPath.c_str());
	string output = executeCommand(cmd);
	assertExist(TABLE_NAME_TEST, output);
}
#define assertCreate() cut_trace(_assertCreate())

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

void _assertInsert(uint64_t id, int age, const char *name, double height)
{
	DBAgentSQLite3 dbAgent;

	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	InProcessItemGroupPtr row;
	row->ADD_NEW_ITEM(Uint64, id);
	row->ADD_NEW_ITEM(Int, age);
	row->ADD_NEW_ITEM(String, name);
	row->ADD_NEW_ITEM(Double, height);
	arg.row = row;
	dbAgent.insert(arg);

	assertExistRecord(id, age, name,height);
}
#define assertInsert(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertInsert(ID,AGE,NAME,HEIGHT));

void _assertUpdate(uint64_t id, int age, const char *name, double height)
{
	DBAgentSQLite3 dbAgent;

	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_TEST;
	for (size_t i = IDX_TEST_TABLE_ID; i < NUM_COLUMNS_TEST; i++)
		arg.columnIndexes.push_back(i);
	arg.columnDefs = COLUMN_DEF_TEST;
	InProcessItemGroupPtr row;
	row->ADD_NEW_ITEM(Uint64, id);
	row->ADD_NEW_ITEM(Int, age);
	row->ADD_NEW_ITEM(String, name);
	row->ADD_NEW_ITEM(Double, height);
	arg.row = row;
	dbAgent.update(arg);

	assertExistRecord(id, age, name,height);
}
#define assertUpdate(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertUpdate(ID,AGE,NAME,HEIGHT));

static void makeTestDB(void)
{
	// make table
	assertCreate();

	// insert data
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		assertInsert(ID[i], AGE[i], NAME[i], HEIGHT[i]);
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		g_testDataIdIndexMap[ID[i]] = i;
	cppcut_assert_equal(NUM_TEST_DATA, g_testDataIdIndexMap.size());
}

static void _assertSelectHeightOrder(size_t limit = 0, size_t offset = 0,
                                     size_t forceExpectedRows = (size_t)-1)
{
	makeTestDB();
	DBAgentSQLite3 dbAgent;
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TEST;
	const ColumnDef &columnDef = COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT];
	arg.statements.push_back(columnDef.columnName);
	arg.columnTypes.push_back(columnDef.type);
	arg.orderBy = StringUtils::sprintf("%s DESC", columnDef.columnName);
	arg.limit = limit;
	arg.offset = offset;
	dbAgent.select(arg);

	// check the result
	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	size_t numExpectedRows;
	if (forceExpectedRows == (size_t)-1)
		numExpectedRows = limit == 0 ? NUM_TEST_DATA : arg.limit;
	else
		numExpectedRows = forceExpectedRows;
	cppcut_assert_equal(numExpectedRows, itemList.size());
	if (numExpectedRows == 0)
		return;

	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	set<double> expectedSet;
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		expectedSet.insert(HEIGHT[i]);

	ItemGroupListConstIterator grpListIt = itemList.begin();
	set<double>::reverse_iterator heightIt = expectedSet.rbegin();
	size_t count = 0;
	for (size_t i = 0; i < NUM_TEST_DATA && count < arg.limit;
	     i++, ++heightIt, count++) {
		int idx = 0;
		double expected = *heightIt;
		if (i < arg.offset)
			continue;
		assertItemData(double, *grpListIt, expected, idx);
		grpListIt++;
	}
}
#define assertSelectHeightOrder(...) \
cut_trace(_assertSelectHeightOrder(__VA_ARGS__))

void setup(void)
{
	g_testDataIdIndexMap.clear();
	deleteDB();
	DBAgentSQLite3::defineDBPath(DEFAULT_DB_DOMAIN_ID, dbPath);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
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

void test_create(void)
{
	assertCreate();
}

void test_insert(void)
{
	// create table
	assertCreate();

	// insert a row
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsert(ID, AGE, NAME, HEIGHT);
}

void test_insertUint64_0x7fffffffffffffff(void)
{
	// create table
	assertCreate();

	// insert a row
	const uint64_t ID = 0x7fffffffffffffff;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsert(ID, AGE, NAME, HEIGHT);
}

void test_insertUint64_0x8000000000000000(void)
{
	// create table
	assertCreate();

	// insert a row
	const uint64_t ID = 0x8000000000000000;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsert(ID, AGE, NAME, HEIGHT);
}

void test_insertUint64_0xffffffffffffffff(void)
{
	// create table
	assertCreate();

	// insert a row
	const uint64_t ID = 0xffffffffffffffff;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsert(ID, AGE, NAME, HEIGHT);
}

void test_update(void)
{
	// create table and insert a row
	test_insert();

	// insert a row
	const uint64_t ID = 9;
	const int AGE = 20;
	const char *NAME = "yui";
	const double HEIGHT = 158.0;
	assertUpdate(ID, AGE, NAME, HEIGHT);
}

void test_select(void)
{
	makeTestDB();

	DBAgentSQLite3 dbAgent;

	// get records
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	arg.columnIndexes.push_back(IDX_TEST_TABLE_ID);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_AGE);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_NAME);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_HEIGHT);
	dbAgent.select(arg);

	// check the result
	const ItemGroupList &groupList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal(groupList.size(), arg.dataTable->getNumberOfRows());
	ItemGroupListConstIterator it = groupList.begin();
	size_t srcDataIdx = 0;
	map<uint64_t, size_t>::iterator itrId;
	for (; it != groupList.end(); ++it, srcDataIdx++) {
		const ItemData *itemData;
		size_t columnIdx = 0;
		const ItemGroup *itemGroup = *it;
		cppcut_assert_equal(itemGroup->getNumberOfItems(),
		                    NUM_COLUMNS_TEST);

		// id
		itemData = itemGroup->getItemAt(columnIdx++);
		uint64_t id = ItemDataUtils::getUint64(itemData);
		itrId = g_testDataIdIndexMap.find(id);
		cppcut_assert_equal(false, itrId == g_testDataIdIndexMap.end(),
		                    cut_message("id: 0x%"PRIx64, id));
		srcDataIdx = itrId->second;

		// age
		itemData = itemGroup->getItemAt(columnIdx++);
		int valInt = ItemDataUtils::getInt(itemData);
		cppcut_assert_equal(AGE[srcDataIdx], valInt);

		// name
		itemData = itemGroup->getItemAt(columnIdx++);
		string valStr = ItemDataUtils::getString(itemData);
		cppcut_assert_equal(NAME[srcDataIdx], valStr.c_str());

		// height
		itemData = itemGroup->getItemAt(columnIdx++);
		double valDouble = ItemDataUtils::getDouble(itemData);
		cppcut_assert_equal(HEIGHT[srcDataIdx], valDouble);

		// delete the element from idSet
		g_testDataIdIndexMap.erase(itrId);
	}
}

void test_selectExStatic(void)
{
	makeTestDB();
	DBAgentSQLite3 dbAgent;
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.statements.push_back("count(*)");
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_TEXT);
	dbAgent.select(arg);

	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal((size_t)1, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	string expectedCount = StringUtils::sprintf("%zd", NUM_TEST_DATA);
	cppcut_assert_equal(expectedCount,
	                    itemGroup->getItemAt(0)->getString());
}

void test_selectExStaticWithCond(void)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	makeTestDB();
	DBAgentSQLite3 dbAgent;
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.statements.push_back(columnDefId.columnName);
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_BIGUINT);

	arg.condition = StringUtils::sprintf
	                  ("%s=%zd", columnDefId.columnName, ID[targetRow]);
	dbAgent.select(arg);

	const ItemGroupList &itemList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal((size_t)1, itemList.size());
	const ItemGroup *itemGroup = *itemList.begin();
	cppcut_assert_equal((size_t)1, itemGroup->getNumberOfItems());

	const ItemUint64 *item =
	   dynamic_cast<const ItemUint64 *>(itemGroup->getItemAt(0));
	cppcut_assert_not_null(item);
	cppcut_assert_equal(ID[targetRow], item->get());
}

void test_selectExStaticWithCondAllColumns(void)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	makeTestDB();
	DBAgentSQLite3 dbAgent;
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TEST;
	for (size_t i = 0; i < NUM_COLUMNS_TEST; i++) {
		const ColumnDef &columnDef = COLUMN_DEF_TEST[i];
		arg.statements.push_back(columnDef.columnName);
		arg.columnTypes.push_back(columnDef.type);
	}

	arg.condition = StringUtils::sprintf
	                  ("%s=%zd", columnDefId.columnName, ID[targetRow]);
	dbAgent.select(arg);

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

void test_selectExStaticWithOrderBy(void)
{
	assertSelectHeightOrder();
}

void test_selectExStaticWithOrderByLimit(void)
{
	assertSelectHeightOrder(1);
}

void test_selectExStaticWithOrderByLimitTwo(void)
{
	assertSelectHeightOrder(2);
}

void test_selectExStaticWithOrderByLimitOffset(void)
{
	assertSelectHeightOrder(2, 1);
}

void test_selectExStaticWithOrderByLimitOffsetOverData(void)
{
	assertSelectHeightOrder(1, NUM_TEST_DATA, 0);
}

void test_deleteStatic(void)
{
	// create table
	assertCreate();

	// insert rows
	const size_t NUM_TEST = 3;
	const uint64_t ID[NUM_TEST]   = {1,2,3};
	const int AGE[NUM_TEST]       = {14, 17, 16};
	const char *NAME[NUM_TEST]    = {"rei", "mio", "azusa"};
	const double HEIGHT[NUM_TEST] = {158.2, 165.3, 155.2};
	for (size_t i = 0; i < NUM_TEST; i++)
		assertInsert(ID[i], AGE[i], NAME[i], HEIGHT[i]);

	// delete
	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_TEST;
	const int thresAge = 15;
	const ColumnDef &columnDefAge = COLUMN_DEF_TEST[IDX_TEST_TABLE_AGE];
	arg.condition = StringUtils::sprintf
	                  ("%s<%d", columnDefAge.columnName, thresAge);
	DBAgentSQLite3 dbAgent;
	dbAgent.deleteRows(arg);

	// check
	cut_assert_exist_path(dbPath.c_str());
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"SELECT %s FROM %s ORDER BY %s ASC\"",
	               dbPath.c_str(), columnDefId.columnName,
	               TABLE_NAME_TEST, columnDefId.columnName);
	string output = executeCommand(cmd);
	vector<string> actualIds;
	StringUtils::split(actualIds, output, '\n');
	size_t matchCount = 0;
	for (size_t i = 0; i < NUM_TEST; i++) {
		if (AGE[i] < thresAge)
			continue;
		cppcut_assert_equal(true, actualIds.size() > matchCount);
		string &actualIdStr = actualIds[matchCount];
		string expectedIdStr = StringUtils::sprintf("%d", ID[i]);
		cppcut_assert_equal(expectedIdStr, actualIdStr);
		matchCount++;
	}
	cppcut_assert_equal(matchCount, actualIds.size());
}

} // testDBAgentSQLite3

