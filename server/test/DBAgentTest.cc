/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <gcutter.h>
#include "DBAgentTest.h"
#include "SQLUtils.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

class TestDBAgent : public DBAgent {
public:
	static string callMakeDatetimeString(int datetime)
	{
		return makeDatetimeString(datetime);
	}

	virtual string
	makeCreateIndexStatement(const TableProfile &tableProfile,
	                         const IndexDef &indexDef) override
	{
		return DBAgent::makeCreateIndexStatement(tableProfile,
		                                         indexDef);
	}

	virtual string
	makeDropIndexStatement(const string &name,
	                       const string &tableName) // orvierride
	{
		return DBAgent::makeDropIndexStatement(name, tableName);
	}
};

const char *TABLE_NAME_TEST = "test_table";
const ColumnDef COLUMN_DEF_TEST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"age",                             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"height",                          // columnName
	SQL_COLUMN_TYPE_DOUBLE,            // type
	15,                                // columnLength
	1,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"time",                            // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
// TODO: remove this and use the tableProfile
const size_t NUM_COLUMNS_TEST = ARRAY_SIZE(COLUMN_DEF_TEST);

const DBAgent::TableProfile tableProfileTest(
  TABLE_NAME_TEST, COLUMN_DEF_TEST,
  NUM_IDX_TEST_TABLE
);

const size_t NUM_TEST_DATA = 3;
const uint64_t ID[NUM_TEST_DATA]   = {1, 2, 0xfedcba9876543210};
const int AGE[NUM_TEST_DATA]       = {14, 17, 180};
const char *NAME[NUM_TEST_DATA]    = {"rei", "aoi", "giraffe"};
const double HEIGHT[NUM_TEST_DATA] = {158.2, 203.9, -23593.2};
const int TIME[NUM_TEST_DATA]   = {1376462763, CURR_DATETIME, 0};

// table for auto increment test
const char *TABLE_NAME_TEST_AUTO_INC = "test_table_auto_inc";
static const ColumnDef COLUMN_DEF_TEST_AUTO_INC[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
},{
	"val",                             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_TEST_TABLE_AUTO_INC_ID,
	IDX_TEST_TABLE_AUTO_INC_VAL,
	IDX_TEST_TABLE_AUTO_INC_NAME,
	NUM_IDX_TEST_TABLE_AUTO_INC,
};

const DBAgent::TableProfile tableProfileTestAutoInc(
  TABLE_NAME_TEST_AUTO_INC, COLUMN_DEF_TEST_AUTO_INC,
  NUM_IDX_TEST_TABLE_AUTO_INC
);

static ItemDataNullFlagType calcNullFlag(set<size_t> *nullIndexes, size_t idx)
{
	if (!nullIndexes)
		return ITEM_DATA_NOT_NULL;
	if (nullIndexes->count(idx) == 0)
		return ITEM_DATA_NOT_NULL;
	return ITEM_DATA_NULL;
}

struct TestValues {
	uint64_t     id;
	int          age;
	const char  *name;
	double       height;

	TestValues(void)
	: id(-1),
	  age(-1),
	  name(NULL),
	  height(0)
	{
	}
};

struct TestAutoIncValues {
	int          id;
	int          val;
	const char  *name;

	TestAutoIncValues(void)
	: id(-1),
	  val(-1),
	  name(NULL)
	{
	}
};

struct CheckInsertParamBase {
	virtual void fillRows(DBAgent::InsertArg &arg) = 0;
	virtual void check(DBAgent &dbAgent, DBAgentChecker &checker) = 0;
	virtual const DBAgent::TableProfile &getTableProfile(void) = 0;
	virtual bool getUpsertOnDuplicate(void) = 0;
};

template<typename DATA_TYPE>
struct CheckInsertParamTempl : public CheckInsertParamBase {
	DATA_TYPE    val;
	bool         useExpectValues;
	DATA_TYPE    expectValues;
	set<size_t> *nullIndexes;
	bool         upsertOnDuplicate;

	CheckInsertParamTempl(void)
	: useExpectValues(false),
	  nullIndexes(NULL),
	  upsertOnDuplicate(false)
	{
	}

	const DATA_TYPE *getExpectData(void)
	{
		const TestValues *expect = &val;
		if (useExpectValues)
			expect = &expectValues;
		return expect;
	}

	bool getUpsertOnDuplicate(void)
	{
		return upsertOnDuplicate;
	}
};

struct CheckInsertParam : public CheckInsertParamTempl<TestValues> {

	void fillRows(DBAgent::InsertArg &arg) override
	{
		size_t idx = 0;
		arg.row->addNewItem(val.id,   calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(val.age,  calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(val.name, calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(val.height,
		                    calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(CURR_DATETIME,
		                    calcNullFlag(nullIndexes, idx++));
	}

	void check(DBAgent &dbAgent, DBAgentChecker &checker) override
	{
		const TestValues *expect = getExpectData();
		checker.assertExistingRecord(
		  dbAgent,
		  expect->id, expect->age, expect->name, expect->height,
		  CURR_DATETIME,
		  NUM_COLUMNS_TEST, COLUMN_DEF_TEST, nullIndexes);
	}

	const DBAgent::TableProfile &getTableProfile(void) override
	{
		return tableProfileTest;
	}
};

struct CheckInsertParamAutoInc :
   public CheckInsertParamTempl<TestAutoIncValues> {

	void fillRows(DBAgent::InsertArg &arg) override
	{
		size_t idx = 0;
		arg.row->addNewItem(val.id,   calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(val.val,  calcNullFlag(nullIndexes, idx++));
		arg.row->addNewItem(val.name, calcNullFlag(nullIndexes, idx++));
	}

	const DBAgent::TableProfile &getTableProfile(void) override
	{
		return tableProfileTestAutoInc;
	}
};

static void checkInsert(DBAgent &dbAgent, DBAgentChecker &checker,
                        CheckInsertParamBase &param)
{
	DBAgent::InsertArg arg(param.getTableProfile());
	param.fillRows(arg);
	arg.upsertOnDuplicate = param.getUpsertOnDuplicate();
	dbAgent.insert(arg);
	param.check(dbAgent, checker);
}

static void checkUpdate(DBAgent &dbAgent, DBAgentChecker &checker,
                        uint64_t id, int age, const char *name, double height,
                        const string &condition = "")
{
	DBAgent::UpdateArg arg(tableProfileTest);
	int idx = IDX_TEST_TABLE_ID;
	arg.add(idx++, id);
	arg.add(idx++, age);
	arg.add(idx++, name);
	arg.add(idx++, height);
	arg.add(idx++, CURR_DATETIME);
	arg.condition = condition;
	dbAgent.update(arg);

	checker.assertExistingRecord(
	  dbAgent, id, age, name, height, CURR_DATETIME,
	  NUM_COLUMNS_TEST, COLUMN_DEF_TEST);
}

static void createTestTableAutoInc(DBAgent &dbAgent, DBAgentChecker &checker)
{
	dbAgent.createTable(tableProfileTestAutoInc);
	checker.assertTable(dbAgent, tableProfileTestAutoInc);
	dbAgent.fixupIndexes(tableProfileTestAutoInc);
}

void dbAgentTestExecSql(DBAgent &dbAgent, DBAgentChecker &checker)
{
	dbAgent.execSql("BEGIN");
	dbAgent.execSql("ROLLBACK");
}

void dbAgentTestCreateTable(DBAgent &dbAgent, DBAgentChecker &checker)
{
	dbAgent.createTable(tableProfileTest);
	checker.assertTable(dbAgent, tableProfileTest);
	dbAgent.fixupIndexes(tableProfileTest);
}

void dbAgentDataMakeCreateIndexStatement(void)
{
	gcut_add_datum("Not unique", "isUnique", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("Unique",     "isUnique", G_TYPE_BOOLEAN, FALSE, NULL);
}

void dbAgentTestMakeCreateIndexStatement(
  DBAgent &dbAgent, DBAgentChecker &checker, gconstpointer data)
{
	const bool isUnique = gcut_data_get_boolean(data, "isUnique");
	const int columnIndexes[] = {
	  IDX_TEST_TABLE_AGE, IDX_TEST_TABLE_NAME, IDX_TEST_TABLE_HEIGHT,
	  DBAgent::IndexDef::END
	};
	DBAgent::IndexDef indexDef = { "testIndex", columnIndexes, isUnique };
	TestDBAgent *testAgent = static_cast<TestDBAgent *>(&dbAgent);

	string sql = testAgent->makeCreateIndexStatement(tableProfileTest,
	                                                 indexDef);
	checker.assertMakeCreateIndexStatement(sql, tableProfileTest, indexDef);
}

void dbAgentTestMakeDropIndexStatement(
  DBAgent &dbAgent, DBAgentChecker &checker)
{
	const string name = "testIndex";
	const string tableName = "tableeeeeNameeeee";
	TestDBAgent *testAgent = static_cast<TestDBAgent *>(&dbAgent);
	string sql = testAgent->makeDropIndexStatement(name, tableName);
	checker.assertMakeDropIndexStatement(sql, name, tableName);
}

void dbAgentTestFixupIndexes(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// We make a copy to update a pointer of the indexDefArray
	DBAgent::TableProfile tableProfile = tableProfileTest;

	const int columnIndexes0[] = {
	  IDX_TEST_TABLE_AGE, IDX_TEST_TABLE_NAME, IDX_TEST_TABLE_HEIGHT,
	  DBAgent::IndexDef::END
	};

	const int columnIndexes1[] = {
	  IDX_TEST_TABLE_NAME, IDX_TEST_TABLE_TIME, DBAgent::IndexDef::END
	};

	const int columnIndexes2[] = {
	  IDX_TEST_TABLE_HEIGHT, DBAgent::IndexDef::END
	};

	const DBAgent::IndexDef indexDefArray[] = {
	  {"testIndex",        columnIndexes0, false},
	  {"testUniqIndex",    columnIndexes1, true},
	  {"testSingleColumn", columnIndexes2, false},
	  {NULL, NULL, false},
	};

	tableProfile.indexDefArray = indexDefArray;
	dbAgent.createTable(tableProfile);
	dbAgent.fixupIndexes(tableProfile);
	checker.assertFixupIndexes(dbAgent, tableProfile);

	// check the drop
	const DBAgent::IndexDef indexDefArrayForDrop[] = {
	  {"testUniqIndex",    columnIndexes1, true},
	  {NULL, NULL, false},
	};
	tableProfile.indexDefArray = indexDefArrayForDrop;
	dbAgent.fixupIndexes(tableProfile);
	checker.assertFixupIndexes(dbAgent, tableProfile);
}

void dbAgentTestInsert(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	CheckInsertParam param;
	param.val.id     = 1;
	param.val.age    = 14;
	param.val.name   = "rei";
	param.val.height = 158.2;
	checkInsert(dbAgent, checker, param);
}

void dbAgentTestInsertUint64
  (DBAgent &dbAgent, DBAgentChecker &checker, uint64_t id)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	CheckInsertParam param;
	param.val.id     = id;
	param.val.age    = 14;
	param.val.name   = "rei";
	param.val.height = 158.2;
	checkInsert(dbAgent, checker, param);
}

void dbAgentTestInsertNull(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	CheckInsertParam param;
	param.val.id     = 999;
	param.val.age    = 0; // we set NULL for this column later
	param.val.name   = "rei";
	param.val.height = 0; // we set NULL for this column later

	set<size_t> nullIndexes;
	nullIndexes.insert(IDX_TEST_TABLE_AGE);
	nullIndexes.insert(IDX_TEST_TABLE_HEIGHT);
	param.nullIndexes = &nullIndexes;

	checkInsert(dbAgent, checker, param);
}

void dbAgentTestUpsert(DBAgent &dbAgent, DBAgentChecker &checker)
{
	// create table
	dbAgentTestCreateTable(dbAgent, checker);

	// insert a row
	CheckInsertParam param;
	param.val.id     = 1;
	param.val.age    = 14;
	param.val.name   = "rei";
	param.val.height = 158.2;
	checkInsert(dbAgent, checker, param);

	param.val.age    = 33;
	param.val.height = 172.5;
	param.upsertOnDuplicate = true;
	checkInsert(dbAgent, checker, param);
}

void dbAgentTestUpsertWithPrimaryKeyAutoInc(
  DBAgent &dbAgent, DBAgentChecker &checker)
{
	struct : public CheckInsertParamAutoInc {
		void check(DBAgent &dbAgent, DBAgentChecker &checker) override
		{
			const string statement = StringUtils::sprintf(
			  "SELECT * from  %s", getTableProfile().name);
			const string expect = StringUtils::sprintf(
			  "1|%d|%s", val.val, val.name);
			assertDBContent(&dbAgent, statement, expect);
		}
	} param;

	// create table
	createTestTableAutoInc(dbAgent, checker);

	// insert a row
	param.val.id     = AUTO_INCREMENT_VALUE; // primary key
	param.val.val    = -3;
	param.val.name   = "rei";
	checkInsert(dbAgent, checker, param);

	// name (unique key) is not changed. So the index will be duplicated.
	param.val.id     = AUTO_INCREMENT_VALUE; // primary key
	param.val.val    = 223;

	param.upsertOnDuplicate = true;
	checkInsert(dbAgent, checker, param);
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
	checkUpdate(dbAgent, checker, ID, AGE, NAME, HEIGHT);
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
		CheckInsertParam param;
		param.val.id     = ID[i];
		param.val.age    = AGE[i];
		param.val.name   = NAME[i];
		param.val.height = HEIGHT[i];
		checkInsert(dbAgent, checker, param);
	}

	// update the second row
	size_t targetIdx = NUM_DATA - 2;
	string condition =
	   StringUtils::sprintf("age=%d and name='%s'",
	                        AGE[targetIdx], NAME[targetIdx]);
	size_t idx = NUM_DATA - 1;
	checkUpdate(dbAgent, checker,
	            ID[idx], AGE[idx], NAME[idx], HEIGHT[idx], condition);
}

void dbAgentTestSelect(DBAgent &dbAgent)
{
	map<uint64_t, size_t> testDataIdIndexMap;
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent, testDataIdIndexMap);

	// get records
	DBAgent::SelectArg arg(tableProfileTest);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_ID);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_AGE);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_NAME);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_HEIGHT);
	arg.columnIndexes.push_back(IDX_TEST_TABLE_TIME);
	dbAgent.select(arg);

	// check the result
	const ItemGroupList &groupList = arg.dataTable->getItemGroupList();
	cppcut_assert_equal(groupList.size(), arg.dataTable->getNumberOfRows());
	cppcut_assert_equal(NUM_TEST_DATA, groupList.size());
	size_t srcDataIdx = 0;
	map<uint64_t, size_t>::iterator itrId;
	ItemGroupListConstIterator itemGrpItr = groupList.begin();
	for (; itemGrpItr != groupList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		cppcut_assert_equal((*itemGrpItr)->getNumberOfItems(),
		                    NUM_COLUMNS_TEST);

		// id
		uint64_t id;
		itemGroupStream >> id;
		itrId = testDataIdIndexMap.find(id);
		cppcut_assert_equal(false, itrId == testDataIdIndexMap.end(),
		                    cut_message("id: 0x%" PRIx64, id));
		srcDataIdx = itrId->second;

		// age
		int valInt;
		itemGroupStream >> valInt;
		cppcut_assert_equal(AGE[srcDataIdx], valInt);

		// name
		string valStr;
		itemGroupStream >> valStr;
		cppcut_assert_equal(NAME[srcDataIdx], valStr.c_str());

		// height
		double valDouble;
		itemGroupStream >> valDouble;
		cppcut_assert_equal(HEIGHT[srcDataIdx], valDouble);

		// delete the element from idSet
		testDataIdIndexMap.erase(itrId);
	}
}

void dbAgentTestSelectEx(DBAgent &dbAgent)
{
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

	DBAgent::SelectExArg arg(tableProfileTest);
	arg.add("count(*)", SQL_COLUMN_TYPE_TEXT);
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

	DBAgent::SelectExArg arg(tableProfileTest);
	arg.add(IDX_TEST_TABLE_ID);

	arg.condition = StringUtils::sprintf(
	  "%s=%" PRIu64, columnDefId.columnName, ID[targetRow]);
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

	DBAgent::SelectExArg arg(tableProfileTest);
	for (size_t i = 0; i < NUM_COLUMNS_TEST; i++)
		arg.add(i);

	arg.condition = StringUtils::sprintf(
	  "%s=%" PRIu64, columnDefId.columnName, ID[targetRow]);
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

	DBAgent::SelectExArg arg(tableProfileTest);
	const ColumnDef &columnDef = COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT];
	arg.add(IDX_TEST_TABLE_HEIGHT);
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
		CheckInsertParam param;
		param.val.id     = ID[i];
		param.val.age    = AGE[i];
		param.val.name   = NAME[i];
		param.val.height = HEIGHT[i];
		checkInsert(dbAgent, checker, param);
	}

	// delete
	DBAgent::DeleteArg arg(tableProfileTest);
	const int thresAge = 15;
	const ColumnDef &columnDefAge = COLUMN_DEF_TEST[IDX_TEST_TABLE_AGE];
	arg.condition = StringUtils::sprintf
	                  ("%s<%d", columnDefAge.columnName, thresAge);
	dbAgent.deleteRows(arg);

	// check
	vector<string> actualIds;
	checker.getIDStringVector(dbAgent, tableProfileTest,
				  IDX_TEST_TABLE_ID,
				  actualIds);
	size_t matchCount = 0;
	for (size_t i = 0; i < NUM_TEST; i++) {
		if (AGE[i] < thresAge)
			continue;
		cppcut_assert_equal(true, actualIds.size() > matchCount);
		string &actualIdStr = actualIds[matchCount];
		string expectedIdStr = StringUtils::sprintf("%" PRIu64, ID[i]);
		cppcut_assert_equal(expectedIdStr, actualIdStr);
		matchCount++;
	}
	cppcut_assert_equal(matchCount, actualIds.size());
}

void dbAgentTestAddColumns(DBAgent &dbAgent, DBAgentChecker &checker)
{
	static const size_t numColumnsOfTable0 = 2;
	static const size_t numColumnsOfTable1 =
	  NUM_COLUMNS_TEST - numColumnsOfTable0;
	static const DBAgent::TableProfile tableProfile0(
	  "test_table", COLUMN_DEF_TEST,
	  numColumnsOfTable0);
	static const DBAgent::TableProfile tableProfile1(
	  "test_table", &COLUMN_DEF_TEST[numColumnsOfTable0],
	  numColumnsOfTable1);

	// A table with the first two columns of'tableProfileTest' is created.
	dbAgent.createTable(tableProfile0);

	// Add remaining columns
	DBAgent::AddColumnsArg arg(tableProfile1);
	for (size_t idx = 0; idx < numColumnsOfTable1; idx++)
		arg.columnIndexes.push_back(idx);
	dbAgent.addColumns(arg);

	checker.assertTable(dbAgent, tableProfileTest);
}

void dbAgentTestRenameTable(DBAgent &dbAgent, DBAgentChecker &checker)
{
	static const DBAgent::TableProfile tableProfileSrc(
	  "test_table_src", COLUMN_DEF_TEST,
	  NUM_COLUMNS_TEST);
	static const DBAgent::TableProfile tableProfileDest(
	  "test_table_dest", COLUMN_DEF_TEST,
	  NUM_COLUMNS_TEST);

	dbAgent.createTable(tableProfileSrc);
	checker.assertTable(dbAgent, tableProfileSrc);

	dbAgent.renameTable(tableProfileSrc.name, tableProfileDest.name);
	checker.assertTable(dbAgent, tableProfileDest);
}

void dbAgentTestIsTableExisting(DBAgent &dbAgent, DBAgentChecker &checker)
{
	cppcut_assert_equal(false, dbAgent.isTableExisting(TABLE_NAME_TEST));
	dbAgentTestCreateTable(dbAgent, checker);
	cppcut_assert_equal(true, dbAgent.isTableExisting(TABLE_NAME_TEST));
}

static void insertRowToTestTableAutoInc(
  DBAgent &dbAgent, DBAgentChecker &checker, int val, const char *name)
{
	DBAgent::InsertArg arg(tableProfileTestAutoInc);
	arg.row->addNewItem(AUTO_INCREMENT_VALUE, ITEM_DATA_NULL);
	arg.row->addNewItem(val);
	arg.row->addNewItem(name);
	dbAgent.insert(arg);
}

static void deleteRowFromTestTableAutoInc(DBAgent &dbAgent,
                                          DBAgentChecker &checker, int id)
{
	DBAgent::DeleteArg arg(tableProfileTestAutoInc);
	const ColumnDef &columnDefId =
	   COLUMN_DEF_TEST_AUTO_INC[IDX_TEST_TABLE_AUTO_INC_ID];
	arg.condition = StringUtils::sprintf
	                  ("%s=%d", columnDefId.columnName, id);
	dbAgent.deleteRows(arg);
}

void dbAgentTestAutoIncrement(DBAgent &dbAgent, DBAgentChecker &checker)
{
	createTestTableAutoInc(dbAgent, checker);
	insertRowToTestTableAutoInc(dbAgent, checker, 0, "taro");
	uint64_t expectedId = 1;
	cppcut_assert_equal(expectedId, dbAgent.getLastInsertId());
}

void dbAgentTestAutoIncrementWithDel(DBAgent &dbAgent, DBAgentChecker &checker)
{
	dbAgentTestAutoIncrement(dbAgent, checker);

	// add a new record
	insertRowToTestTableAutoInc(dbAgent, checker, 10, "jiro");
	uint64_t expectedId = 2;
	cppcut_assert_equal(expectedId, dbAgent.getLastInsertId());

	// delete the last record
	deleteRowFromTestTableAutoInc(dbAgent, checker, expectedId);

	// we expected the incremented ID 
	insertRowToTestTableAutoInc(dbAgent, checker, 20, "subro");
	expectedId++;
	cppcut_assert_equal(expectedId, dbAgent.getLastInsertId());

}

static bool dbAgentUpdateIfExistEleseInsertOneRecord(
  DBAgent &dbAgent, size_t i, string &expectedLine, size_t targetIndex,
  const string &fmt,
  uint64_t id, int age, const char *name, double height, int time)
{
	VariableItemGroupPtr row;
	row->addNewItem(id);
	row->addNewItem(age);
	row->addNewItem(name);
	row->addNewItem(height);
	row->addNewItem(time);
	bool updated = dbAgent.updateIfExistElseInsert(row, tableProfileTest,
	                                               targetIndex);
	string expectedTimeStr;
	if (time == CURR_DATETIME) {
		expectedTimeStr = DBCONTENT_MAGIC_CURR_DATETIME;
	} else {
		expectedTimeStr = StringUtils::eraseChars(
		  TestDBAgent::callMakeDatetimeString(time), "'");
	}
	expectedLine = StringUtils::sprintf(
	  fmt.c_str(), id, age, name, height, expectedTimeStr.c_str());
	return updated;
}

void dbAgentUpdateIfExistEleseInsert(DBAgent &dbAgent, DBAgentChecker &checker)
{
	dbAgentTestCreateTable(dbAgent, checker);
	string fmt = "%" PRIu64 "|%d|%s|";
	fmt += makeDoubleFloatFormat(COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT]);
	fmt += "|%s\n";
	string expectLines;
	string statement = StringUtils::sprintf(
	     "select * from %s order by %s asc",
	     TABLE_NAME_TEST, COLUMN_DEF_TEST[IDX_TEST_TABLE_ID].columnName);
	StringVector firstExpectedLines;
	string expectedLine;

	// First we expect an insertion
	const size_t targetIndex = IDX_TEST_TABLE_ID;
	for (size_t i = 0; i < NUM_TEST_DATA; i++) {
		bool updated = 
		  dbAgentUpdateIfExistEleseInsertOneRecord(
		    dbAgent, i, expectedLine, targetIndex, fmt,
		    i, AGE[i], NAME[i], HEIGHT[i], TIME[i]);
		cppcut_assert_equal(false, updated);
		firstExpectedLines.push_back(expectedLine);
		expectLines += expectedLine;
	}
	assertDBContent(&dbAgent, statement, expectLines);

	// Then we update the first and the third records
	set<size_t> targetRows;
	targetRows.insert(0);
	targetRows.insert(2);
	expectLines.clear();
	for (size_t i = 0; i < NUM_TEST_DATA; i++) {
		if (targetRows.count(i) == 0) {
			expectLines += firstExpectedLines[i];
			continue;
		}

		string name = StringUtils::sprintf("FOO-%zd", i);
		bool updated = 
		  dbAgentUpdateIfExistEleseInsertOneRecord(
		    dbAgent, i, expectedLine, targetIndex, fmt,
		    i, AGE[i]/2, name.c_str(), HEIGHT[i]/2, TIME[i]);
		cppcut_assert_equal(true, updated);
		expectLines += expectedLine;
	}
	assertDBContent(&dbAgent, statement, expectLines);
}

void dbAgentGetLastInsertId(DBAgent &dbAgent, DBAgentChecker &checker)
{
	using mlpl::StringUtils::sprintf;

	// create table
	createTestTableAutoInc(dbAgent, checker);
	static const size_t NUM_REPEAT = 3;
	for (uint64_t id = 1; id < NUM_REPEAT; id++) {
		int val = id * 3;
		insertRowToTestTableAutoInc(
		  dbAgent, checker, val,
		  sprintf("hanako.%" PRIu64, id).c_str());
		cppcut_assert_equal(id, dbAgent.getLastInsertId());
	}
}

void dbAgentGetNumberOfAffectedRows(DBAgent &dbAgent, DBAgentChecker &checker)
{
	DBAgentChecker::createTable(dbAgent);
	DBAgentChecker::makeTestData(dbAgent);

	DBAgent::DeleteArg arg(tableProfileTest);
	dbAgent.deleteRows(arg);
	cppcut_assert_equal(static_cast<uint64_t>(NUM_TEST_DATA),
			    dbAgent.getNumberOfAffectedRows());
}

// --------------------------------------------------------------------------
// DBAgentChecker
// --------------------------------------------------------------------------
void DBAgentChecker::createTable(DBAgent &dbAgent)
{
	dbAgent.createTable(tableProfileTest);
}

void DBAgentChecker::insert
  (DBAgent &dbAgent, uint64_t id, int age, const char *name, double height,
   int time)
{
	DBAgent::InsertArg arg(tableProfileTest);
	arg.row->addNewItem(id);
	arg.row->addNewItem(age);
	arg.row->addNewItem(name);
	arg.row->addNewItem(height);
	arg.row->addNewItem(time);
	dbAgent.insert(arg);
}

void DBAgentChecker::makeTestData(DBAgent &dbAgent)
{
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		insert(dbAgent, ID[i], AGE[i], NAME[i], HEIGHT[i], TIME[i]);
}

void DBAgentChecker::makeTestData
  (DBAgent &dbAgent, map<uint64_t, size_t> &testDataIdIndexMap)
{
	makeTestData(dbAgent);
	for (size_t i = 0; i < NUM_TEST_DATA; i++)
		testDataIdIndexMap[ID[i]] = i;
	cppcut_assert_equal(NUM_TEST_DATA, testDataIdIndexMap.size());
}

void DBAgentChecker::assertExistingRecordEachWord
  (uint64_t id, int age, const char *name, double height, int datetime,
   size_t numColumns, const ColumnDef *columnDefs, const string &line,
   const char splitChar, const set<size_t> *nullIndexes,
   const string &expectedNullNotation, const char *U64fmt)
{
	// value
	size_t idx = 0;
	string expected;
	StringVector words;
	StringUtils::split(words, line, splitChar, false);
	cppcut_assert_equal(numColumns, words.size(),
	                    cut_message("line: %s\n", line.c_str()));

	// id
	expected = (nullIndexes && nullIndexes->count(idx)) ?
	             expectedNullNotation : StringUtils::sprintf(U64fmt, id);
	cppcut_assert_equal(expected, words[idx++]);

	// age
	expected = (nullIndexes && nullIndexes->count(idx)) ?
	             expectedNullNotation : StringUtils::sprintf("%d", age);
	cppcut_assert_equal(expected, words[idx++]);

	// name
	expected = (nullIndexes && nullIndexes->count(idx)) ?
	             expectedNullNotation : name;
	cppcut_assert_equal(expected, words[idx++]);

	// height
	if (nullIndexes && nullIndexes->count(idx)) {
		expected = expectedNullNotation;
	} else {
		string fmt = makeDoubleFloatFormat(columnDefs[idx]);
		expected = StringUtils::sprintf(fmt.c_str(), height);
	}
	cppcut_assert_equal(expected, words[idx++]);

	// time
	if (nullIndexes && nullIndexes->count(idx)) {
		cppcut_assert_equal(string(expectedNullNotation), words[idx++]);
	} else if (datetime == CURR_DATETIME) {
		assertCurrDatetime(words[idx++]);
	} else {
		cut_fail("Not implemented");
		cppcut_assert_equal(expected, words[idx++]);
	}
}

