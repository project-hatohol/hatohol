#include "DBAgentTest.h"

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

void DBAgentChecker::makeTestData
  (DBAgent &dbAgent, map<uint64_t, size_t> &testDataIdIndexMap)
{
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		insert(dbAgent, ID[i], AGE[i], NAME[i], HEIGHT[i]);
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		testDataIdIndexMap[ID[i]] = i;
	cppcut_assert_equal(NUM_TEST_DATA, testDataIdIndexMap.size());
}

