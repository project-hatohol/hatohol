#include "DBAgentTest.h"
#include "Helpers.h"

const char *TABLE_NAME_TEST = "test_table";
const ColumnDef COLUMN_DEF_TEST[] = {
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
	15,                                // columnLength
	1,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
const size_t NUM_COLUMNS_TEST = sizeof(COLUMN_DEF_TEST) / sizeof(ColumnDef);

const size_t NUM_TEST_DATA = 3;
const uint64_t ID[NUM_TEST_DATA]   = {1, 2, 0xfedcba9876543210};
const int AGE[NUM_TEST_DATA]       = {14, 17, 180};
const char *NAME[NUM_TEST_DATA]    = {"rei", "aoi", "giraffe"};
const double HEIGHT[NUM_TEST_DATA] = {158.2, 203.9, -23593.2};

void _checkInsert(DBAgent &dbAgent, DBAgentChecker &checker,
                   uint64_t id, int age, const char *name, double height)
{
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Uint64, id);
	row->ADD_NEW_ITEM(Int, age);
	row->ADD_NEW_ITEM(String, name);
	row->ADD_NEW_ITEM(Double, height);
	arg.row = row;
	dbAgent.insert(arg);

	checker.assertInsert(arg, id, age, name, height);
}

void dbAgentTestCreateTable(DBAgent &dbAgent, DBAgentChecker &checker)
{
	DBAgentTableCreationArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	dbAgent.createTable(arg);

	checker.assertTable(arg);
}

void dbAgentTestInsert(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;

	checkInsert(dbAgent, checker, ID, AGE, NAME, HEIGHT);
}

void dbAgentTestInsertUint64
  (DBAgent &dbAgent, DBAgentChecker &checker, uint64_t id)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;

	checkInsert(dbAgent, checker, id, AGE, NAME, HEIGHT);
}

void dbAgentTestUpdate(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table and insert a row
	dbAgentTestInsert(dbAgent, checker);

	// insert a row
	const uint64_t ID = 9;
	const int AGE = 20;
	const char *NAME = "yui";
	const double HEIGHT = 158.0;
	checker.assertUpdate(ID, AGE, NAME, HEIGHT);
}

void dbAgentTestUpdateCondition(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	static const size_t NUM_DATA = 3;
	const uint64_t ID[NUM_DATA]   = {1, 3, 9};
	const int      AGE[NUM_DATA]  = {20, 18, 17};
	const char    *NAME[NUM_DATA] = {"yui", "aoi", "Q-taro"};
	const double HEIGHT[NUM_DATA] = {158.0, 161.3, 70.0};

	// insert the first and the second rows
	for (size_t  i = 0; i < NUM_DATA - 1; i++) {
		checkInsert(dbAgent, checker,
		            ID[i], AGE[i], NAME[i], HEIGHT[i]);
	}

	// update the second row
	size_t targetIdx = NUM_DATA - 2;
	string condition =
	   StringUtils::sprintf("age=%d and name='%s'",
	                        AGE[targetIdx], NAME[targetIdx]);
	size_t idx = NUM_DATA - 1;
	checker.assertUpdate(ID[idx], AGE[idx], NAME[idx], HEIGHT[idx],
	                     condition);
}

void dbAgentTestSelect(DBAgent &dbAgent)
{
	map<uint64_t, size_t> testDataIdIndexMap;
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent, testDataIdIndexMap);

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
	cppcut_assert_equal(NUM_TEST_DATA, groupList.size());
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
		itrId = testDataIdIndexMap.find(id);
		cppcut_assert_equal(false, itrId == testDataIdIndexMap.end(),
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
		testDataIdIndexMap.erase(itrId);
	}
}

void dbAgentTestSelectEx(DBAgent &dbAgent)
{
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

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

void dbAgentTestSelectExWithCond(DBAgent &dbAgent)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

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

void dbAgentTestSelectExWithCondAllColumns(DBAgent &dbAgent)
{
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	size_t targetRow = 1;

	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

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

void dbAgentTestSelectHeightOrder
  (DBAgent &dbAgent, size_t limit, size_t offset, size_t forceExpectedRows)
{
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

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

// --------------------------------------------------------------------------
// DBAgentChecker
// --------------------------------------------------------------------------
void DBAgentChecker::createTable(DBAgent &dbAgent)
{
	DBAgentTableCreationArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	dbAgent.createTable(arg);
}

void DBAgentChecker::insert
  (DBAgent &dbAgent, uint64_t id, int age, const char *name, double height)
{
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Uint64, id);
	row->ADD_NEW_ITEM(Int, age);
	row->ADD_NEW_ITEM(String, name);
	row->ADD_NEW_ITEM(Double, height);
	arg.row = row;
	dbAgent.insert(arg);
}

void DBAgentChecker::makeTestData(DBAgent &dbAgent)
{
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		insert(dbAgent, ID[i], AGE[i], NAME[i], HEIGHT[i]);
}

void DBAgentChecker::makeTestData
  (DBAgent &dbAgent, map<uint64_t, size_t> &testDataIdIndexMap)
{
	makeTestData(dbAgent);
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		testDataIdIndexMap[ID[i]] = i;
	cppcut_assert_equal(NUM_TEST_DATA, testDataIdIndexMap.size());
}

void dbAgentTestDelete(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert rows
	const size_t NUM_TEST = 3;
	const uint64_t ID[NUM_TEST]   = {1,2,3};
	const int AGE[NUM_TEST]       = {14, 17, 16};
	const char *NAME[NUM_TEST]    = {"rei", "mio", "azusa"};
	const double HEIGHT[NUM_TEST] = {158.2, 165.3, 155.2};
	for (size_t i = 0; i < NUM_TEST; i++) {
		checkInsert(dbAgent, checker,
		            ID[i], AGE[i], NAME[i], HEIGHT[i]);
	}

	// delete
	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_TEST;
	const int thresAge = 15;
	const ColumnDef &columnDefAge = COLUMN_DEF_TEST[IDX_TEST_TABLE_AGE];
	arg.condition = StringUtils::sprintf
	                  ("%s<%d", columnDefAge.columnName, thresAge);
	dbAgent.deleteRows(arg);

	// check
	vector<string> actualIds;
	const ColumnDef &columnDefId = COLUMN_DEF_TEST[IDX_TEST_TABLE_ID];
	checker.getIDStringVector(columnDefId, actualIds);
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
