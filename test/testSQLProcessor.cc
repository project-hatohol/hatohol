#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <typeinfo>
#include <cutter.h>
#include <cppcutter.h>
#include "SQLProcessor.h"
#include "FormulaFunction.h"
#include "Asura.h"
#include "Utils.h"
#include "FormulaTestUtils.h"
#include "AssertJoin.h"
#include "AsuraException.h"
#include "Helpers.h"

namespace testSQLProcessor {

// joinedTable, selectedTable, and the first element of groupedTables
// dereference the same ItemTable when there's no where section and no grouping.
// (Note: This strongly depends on the current implementation)
// So the number of the referece count should be 3.
static const int expectedRefCountOfResultTableInTheSimplestCase = 3;

//
// Test Table Definition
//
static const int TABLE0_ID  = 1;
static const int TABLE1_ID  = 2;
static const int TABLE2_ID  = 3;
static const int TABLE_Z_ID = 1000000;
static const int TABLE_N_ID = 1000001;

static const char *TABLE0_NAME = "TestTable0";
static const char *TABLE1_NAME = "TestTable1";
static const char *TABLE2_NAME = "TestTable2";
static const char *TABLE_Z_NAME = "TestTableZ"; // Empty (Zero) Table
static const char *TABLE_N_NAME = "TestTableN"; // Null Table

//
// Test Column Definition
//
static const char *COLUMN_NAME_NUMBER = "number";
static const char *COLUMN_NAME_NAME = "name";

static const char *COLUMN_NAME_AGE    = "age";
static const char *COLUMN_NAME_ANIMAL = "animal";
static const char *COLUMN_NAME_FOOD   = "food";

static const char *COLUMN_NAME_MENTAL_AGE = "mental_age";
static const char *COLUMN_NAME_FAVORITE   = "favorite";

static const char *COLUMN_NAME_Z_NUM  = "z_num";
static const char *COLUMN_NAME_Z_STR  = "z_str";

enum {
	ITEM_ID_NUMBER,
	ITEM_ID_NAME,

	ITEM_ID_AGE,
	ITEM_ID_ANIMAL,
	ITEM_ID_FOOD,

	ITEM_ID_MENTAL_AGE,
	ITEM_ID_FAVORITE,

	ITEM_ID_Z_NUM,
	ITEM_ID_Z_STR,
};

static const size_t NUM_COLUMN0_DEFS = 2;
static ColumnDef COLUMN0_DEFS[NUM_COLUMN0_DEFS] = {
  {ITEM_ID_NUMBER, TABLE0_NAME, COLUMN_NAME_NUMBER, SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_NAME,   TABLE0_NAME, COLUMN_NAME_NAME,   SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const size_t NUM_COLUMN1_DEFS = 3;
static ColumnDef COLUMN1_DEFS[NUM_COLUMN1_DEFS] = {
  {ITEM_ID_AGE,    TABLE1_NAME, COLUMN_NAME_AGE,    SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_ANIMAL, TABLE1_NAME, COLUMN_NAME_ANIMAL, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
  {ITEM_ID_FOOD,   TABLE1_NAME, COLUMN_NAME_FOOD,   SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const size_t NUM_COLUMN2_DEFS = 2;
static ColumnDef COLUMN2_DEFS[NUM_COLUMN2_DEFS] = {
  {ITEM_ID_MENTAL_AGE, TABLE2_NAME, COLUMN_NAME_MENTAL_AGE,
   SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_FAVORITE,   TABLE2_NAME, COLUMN_NAME_FAVORITE,
   SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const size_t NUM_COLUMN_Z_DEFS = 2;
static ColumnDef COLUMN_Z_DEFS[NUM_COLUMN_Z_DEFS] = {
  {ITEM_ID_Z_NUM, TABLE_Z_NAME, COLUMN_NAME_Z_NUM, SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_Z_STR, TABLE_Z_NAME, COLUMN_NAME_Z_STR, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

//
// Test Row Definition
//
struct TestData0 {
	int         number;
	const char *name;
};

static TestData0 testData0[] = {
  {15,  "ant"},
  {100, "Dog"},
  {-5,  "Clothes make the man."},
  {-5,  "Angel"},
  {-5,  "cat"},
  {5,   "catfish"},
  {100, "feel like a bike ride"},
};
static size_t numTestData0 = sizeof(testData0) / sizeof(TestData0);

struct TestData1 {
	int         age;
	const char *animal;
	const char *food;
};

static TestData1 testData1[] = {
  {20, "bird", "meet"},
  {-5, "cat", "parfait"},
  {100,"Dragon", "orange"},
  {52, "fellow", "pizza"},
};
static size_t numTestData1 = sizeof(testData1) / sizeof(TestData1);

struct TestData2 {
	int         mentalAge;
	const char *favorite;
};

static TestData2 testData2[] = {
  {20,  "Dragon"},
  {100, "Dog"},
};
static size_t numTestData2 = sizeof(testData2) / sizeof(TestData2);

struct TestDataZ {
};

static TestDataZ testDataZ[] = {
};
static size_t numTestDataZ = sizeof(testDataZ) / sizeof(TestDataZ);

struct TestDataPointerSet {
	TestData0 *data0;
	TestData1 *data1;
	TestData2 *data2;
};

//
// Test Data structure
//
struct TableData {
	int tableId;
	const char *tableName;
	int numColumnDefs;
	ColumnDef *columnDefs;
};

static const TableData tableData[] = {
  {TABLE0_ID, TABLE0_NAME, NUM_COLUMN0_DEFS, COLUMN0_DEFS},
  {TABLE1_ID, TABLE1_NAME, NUM_COLUMN1_DEFS, COLUMN1_DEFS},
  {TABLE2_ID, TABLE2_NAME, NUM_COLUMN2_DEFS, COLUMN2_DEFS},
  {TABLE_Z_ID, TABLE_Z_NAME, NUM_COLUMN_Z_DEFS, COLUMN_Z_DEFS},
  {TABLE_N_ID, TABLE_N_NAME, NUM_COLUMN_Z_DEFS, COLUMN_Z_DEFS},
};
static const size_t NUM_TABLE_DATA = sizeof(tableData) / sizeof(TableData);

//
// Test helper class: TestSQLProcessor
//
#define TBL_FNC(X) static_cast<SQLTableMakeFunc>(X)

class TestSQLProcessor : public SQLProcessor {
public:
	TestSQLProcessor(void)
	: SQLProcessor(getDBName(), m_tableNameStaticInfoMap)
	{
		// To use the same table for tests
		m_table0Ptr = makeTable0();
		m_table1Ptr = makeTable1();
		m_table2Ptr = makeTable2();

		initStaticInfo();
	}

	// Implementation of abstruct method
	const char *getDBName(void) {
		return "TestTable";
	}

private:
	static ItemTablePtr    m_table0Ptr;
	static ItemTablePtr    m_table1Ptr;
	static ItemTablePtr    m_table2Ptr;
	TableNameStaticInfoMap m_tableNameStaticInfoMap;
	SQLTableStaticInfo     m_staticInfo[NUM_TABLE_DATA];

	void initStaticInfoEach(SQLTableStaticInfo *staticInfo,
	                        const TableData &tableData,
	                        SQLTableGetFunc tableGetFunc)
	{
		m_tableNameStaticInfoMap[tableData.tableName] = staticInfo;
		staticInfo->tableId = tableData.tableId;
		staticInfo->tableName = tableData.tableName;
		staticInfo->tableGetFunc  = tableGetFunc;

		ColumnDefList &list =
		  const_cast<ColumnDefList &>
		  (staticInfo->columnDefList);

		ColumnNameAccessInfoMap &map = staticInfo->columnAccessInfoMap;
		for (int i = 0; i < tableData.numColumnDefs; i++) {
			ColumnDef *columnDefs = tableData.columnDefs;
			list.push_back(columnDefs[i]);
			ColumnAccessInfo accessInfo = {i, &columnDefs[i]};
			map[columnDefs[i].columnName] = accessInfo;
		}
	}

	static ItemTablePtr table0GetFunc(void) {
		return m_table0Ptr;
	}

	static ItemTablePtr table1GetFunc(void) {
		return m_table1Ptr;
	}

	static ItemTablePtr table2GetFunc(void) {
		return m_table2Ptr;
	}

	static ItemTablePtr tableZGetFunc(void) {
		return ItemTablePtr();
	}

	static ItemTablePtr tableNGetFunc(void) {
		return ItemTablePtr(NULL);
	}

	void initStaticInfo(void) {
		SQLTableGetFunc tableGetFuncArray[NUM_TABLE_DATA] = {
			table0GetFunc,
			table1GetFunc,
			table2GetFunc,
			tableZGetFunc,
			tableNGetFunc,
		};

		for (size_t i = 0; i < NUM_TABLE_DATA; i++) {
			initStaticInfoEach(&m_staticInfo[i], tableData[i],
			                   tableGetFuncArray[i]);
		}
	}

	ItemTablePtr makeTable0(void) {
		ItemTablePtr tablePtr;
		for (size_t i = 0; i < numTestData0; i++) {
			ItemGroup *grp = tablePtr->addNewGroup();
			grp->add(new ItemInt(ITEM_ID_NUMBER,
			                     testData0[i].number), false);
			grp->add(new ItemString(ITEM_ID_NAME,
			                        testData0[i].name), false);
		}
		return tablePtr;
	}

	ItemTablePtr makeTable1(void) {
		ItemTablePtr tablePtr;
		for (size_t i = 0; i < numTestData1; i++) {
			ItemGroup *grp = tablePtr->addNewGroup();
			grp->add(new ItemInt(ITEM_ID_AGE,
			                     testData1[i].age), false);
			grp->add(new ItemString(ITEM_ID_ANIMAL,
			                        testData1[i].animal), false);
			grp->add(new ItemString(ITEM_ID_FOOD,
			                        testData1[i].food), false);
		}
		return tablePtr;
	}

	ItemTablePtr makeTable2(void) {
		ItemTablePtr tablePtr;
		for (size_t i = 0; i < numTestData2; i++) {
			ItemGroup *grp = tablePtr->addNewGroup();
			grp->add(new ItemInt(ITEM_ID_MENTAL_AGE,
			                     testData2[i].mentalAge), false);
			grp->add(new ItemString(ITEM_ID_FAVORITE,
			                        testData2[i].favorite), false);
		}
		return tablePtr;
	}

};

ItemTablePtr TestSQLProcessor::m_table0Ptr;
ItemTablePtr TestSQLProcessor::m_table1Ptr;
ItemTablePtr TestSQLProcessor::m_table2Ptr;

static const int getMaxDataInTestData(void)
{
	int max = testData0[0].number;
	for (size_t i = 1; i < numTestData0; i++) {
		if (testData0[i].number > max)
			max = testData0[i].number;
	}
	return max;
}

static void getDistinctValueInTestData0(set<int> &valueSet)
{
	int count = 0;
	for (size_t i = 0; i < numTestData0; i++)
		valueSet.insert(testData0[i].number);
}

static void getValueData0NumberEqData1Age(multiset<int> &valueSet)
{
	for (size_t i = 0; i < numTestData0; i++) {
		for (size_t j = 0; j < numTestData1; j++) {
			if (testData0[i].number != testData1[j].age)
				continue;
			valueSet.insert(testData0[i].number);
			break;
		}
	}
}


static void
getIndexesOfDataWithValueInTestData0(vector<size_t> &indexes, int value)
{
	for (size_t i = 0; i < numTestData0; i++){
		if (testData0[i].number == value)
			indexes.push_back(i);
	}
}

static int sumNumberOfTestData0(void)
{
	int sum = 0;
	for (size_t i = 0; i < numTestData0; i++)
		sum += testData0[i].number;
	return sum;
}

static const size_t NO_CHECK = (size_t)-1;
static const size_t EXPECTED_NUM_ROWS_EQUAL_SELECTED = (size_t)-1;

static void _assertSQLSelectInfoBasic
  (SQLSelectInfo &selectInfo,
   size_t expectedNumColumns, size_t expectedNumSelectedRows,
   size_t expectedNumRows = EXPECTED_NUM_ROWS_EQUAL_SELECTED)
{
	if (expectedNumRows == EXPECTED_NUM_ROWS_EQUAL_SELECTED)
		expectedNumRows = expectedNumSelectedRows;

	// SQLOutputColumn
	cppcut_assert_equal(expectedNumColumns,
	                    selectInfo.outputColumnVector.size());
	for (size_t i = 0; i < expectedNumColumns; i++) {
		SQLOutputColumn &outCol = selectInfo.outputColumnVector[i];
		cppcut_assert_not_null(outCol.columnDef);
	}

	// Selected Rows
	if (expectedNumSelectedRows == NO_CHECK)
		return;
	cppcut_assert_equal(expectedNumSelectedRows,
	                    selectInfo.selectedTable->getNumberOfRows());

	// Text Rows
	cppcut_assert_equal(expectedNumRows, selectInfo.textRows.size());
	for (size_t i = 0; i < expectedNumRows; i++) {
		StringVector &textRows = selectInfo.textRows[i];
		cppcut_assert_equal(expectedNumColumns, textRows.size());
	}
}
#define assertSQLSelectInfoBasic(SI, ENC, ENS, ...) \
cut_trace(_assertSQLSelectInfoBasic(SI, ENC, ENS, ##__VA_ARGS__))

static void _asssertExecSelect
  (SQLSelectInfo &selectInfo,
   size_t expectedNumColumns, size_t expectedNumSelectedRows = NO_CHECK,
   size_t expectedNumRows = EXPECTED_NUM_ROWS_EQUAL_SELECTED,
   bool expectedResult = true)
{
	try {
		TestSQLProcessor proc;
		cppcut_assert_equal(expectedResult, proc.select(selectInfo));
		assertSQLSelectInfoBasic(selectInfo, expectedNumColumns,
		                         expectedNumSelectedRows,
		                         expectedNumRows);
	} catch (const AsuraException &e) {
		cut_fail("Got exception: %s", e.getFancyMessage().c_str());
	}
}
#define asssertExecSelect(S, ENC, ...) \
cut_trace(_asssertExecSelect(S, ENC, ##__VA_ARGS__))

#define DEFINE_SELECTINFO_AND_ASSERT_SELECT(SEL_VAR, STATEMENT, ENC, ...) \
ParsableString _parsable(STATEMENT); \
SQLSelectInfo SEL_VAR(_parsable); \
asssertExecSelect(SEL_VAR, ENC, ##__VA_ARGS__);

typedef string (*TestDataGetter)(int row, int column);

static string testData0Getter(int row, int column)
{
	cppcut_assert_equal(true, row < numTestData0);
	cppcut_assert_equal(true, column < NUM_COLUMN0_DEFS);
	if (column == 0)
		return StringUtils::toString(testData0[row].number);
	if (column == 1)
		return testData0[row].name;
	return "";
}

static string testData1Getter(int row, int column)
{
	cppcut_assert_equal(true, row < numTestData1);
	cppcut_assert_equal(true, column < NUM_COLUMN1_DEFS);
	if (column == 0)
		return StringUtils::toString(testData1[row].age);
	if (column == 1)
		return testData1[row].animal;
	if (column == 2)
		return testData1[row].food;
	return "";
}

void _assertSelectAll(string tableName, TestDataGetter testDataGetter,
                      size_t expectedNumColumns, size_t expectedNumRows,
                      const char *varName = NULL)
{
	string statement;
	if (!varName) {
		statement = StringUtils::sprintf("* from %s",
		                                 tableName.c_str());
	} else {
		statement = StringUtils::sprintf("%s.* from %s %s",
		                                 varName, tableName.c_str(),
		                                 varName);
	}
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, (string)statement, expectedNumColumns, expectedNumRows);

	// text output
	for (size_t i = 0; i < selectInfo.textRows.size(); i++) {
		for (size_t j = 0; j < selectInfo.textRows[i].size(); j++) {
			string expect = (*testDataGetter)(i,j);
			cppcut_assert_equal(expect, selectInfo.textRows[i][j]);
		}
	}
}
#define assertSelectAll(S, G, C, R, ...) \
cut_trace(_assertSelectAll(S, G, C, R, ##__VA_ARGS__))

struct InnerJoinedRowsCheckerNumberAge {
	bool operator()(const TestData0 &row0, const TestData1 &row1) const {
		return row0.number == row1.age;
	}
};

static void assertJoinRunner(const ItemGroup *itemGroup,
                             TestData0 *refData0, TestData1 *refData1,
                             size_t data0Index, size_t data1Index)
{
	int idx = 0;
	assertItemData(int,    itemGroup, refData0->number, idx);
	assertItemData(string, itemGroup, refData0->name,   idx);
	assertItemData(int,    itemGroup, refData1->age,    idx);
	assertItemData(string, itemGroup, refData1->animal, idx);
	assertItemData(string, itemGroup, refData1->food,   idx);
}

void _assertInerJoinHelper(const string &statement)
{
	// check the result
	const size_t expectedNumColumns = NUM_COLUMN0_DEFS + NUM_COLUMN1_DEFS;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, expectedNumColumns);

	cppcut_assert_equal(expectedRefCountOfResultTableInTheSimplestCase,
	                    selectInfo.joinedTable->getUsedCount());

	AssertInnerJoin<TestData0, TestData1, InnerJoinedRowsCheckerNumberAge>
	  assertJoin((ItemTable *)selectInfo.selectedTable,
	             testData0, testData1, numTestData0, numTestData1);
	assertJoin.run(assertJoinRunner);
}
#define assertInerJoinHelper(X) cut_trace(_assertInerJoinHelper(X))

// The first argument is not a reference and is copied. This is to avoid
// the original data from damaging.
void _assertEqualUnawareOfOrder(multiset<string> expectedSet,
                                vector<StringVector> &textRows,
                                size_t targetIndex)
{
	cppcut_assert_equal(expectedSet.size(), textRows.size());
	multiset<string>::iterator it;
	for (size_t i = 0; i < textRows.size(); i++) {
		it = expectedSet.find(textRows[i][targetIndex]);
		cppcut_assert_equal(true, it != expectedSet.end());
		expectedSet.erase(it);
	}
	cppcut_assert_equal(true, expectedSet.empty());
}
#define assertEqualUnawareOfOrder(E,T,I) \
cut_trace(_assertEqualUnawareOfOrder(E,T,I))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneColumn(void)
{
	string statement =
	  StringUtils::sprintf("%s from %s", COLUMN_NAME_NUMBER, TABLE0_NAME);
	const size_t numColumns = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0);

	// column
	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());

	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(COLUMN_NAME_NUMBER),
	                    formulaInfo->expression);
	assertFormulaVariable(formulaInfo->formula, COLUMN_NAME_NUMBER);

	// table
	cppcut_assert_equal((size_t)1, selectInfo.tables.size());
	SQLTableInfoListIterator table = selectInfo.tables.begin();
	cppcut_assert_equal(string(TABLE0_NAME), (*table)->name);
}

void test_selectMultiColumn(void)
{
	TestSQLProcessor proc;
	const char *columns[] = {"c1", "a2", "b3", "z4", "x5", "y6"};
	const size_t numColumns = sizeof(columns) / sizeof(const char *);
	const char *tableName = TABLE0_NAME;
	ParsableString parsable(
	  StringUtils::sprintf("%s,%s,%s, %s, %s, %s from %s",
	                       columns[0], columns[1], columns[2],
	                       columns[3], columns[4], columns[5],
	                       tableName));
	SQLSelectInfo selectInfo(parsable);
	proc.select(selectInfo);

	// columns
	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal(numColumns, formulaInfoVector.size());

	for (int idx = 0; idx < numColumns; idx++) {
		SQLFormulaInfo *formulaInfo = formulaInfoVector[idx];
		cppcut_assert_equal(string(columns[idx]),
		                    formulaInfo->expression);
		assertFormulaVariable(formulaInfo->formula, columns[idx]);
	}

	// table
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoListIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableName, (*table)->name.c_str());
}

void test_selectTableVar(void)
{
	TestSQLProcessor proc;
	const char *tableVarName = "tvar";
	ParsableString parsable(
	  StringUtils::sprintf("columnArg from %s %s",
	                       TABLE0_NAME, tableVarName));
	SQLSelectInfo selectInfo(parsable);
	proc.select(selectInfo);
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoListIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableVarName, (*table)->varName.c_str());
}

void test_selectOrderBy(void)
{
	TestSQLProcessor proc;
	const char *orderName = "orderArg";
	ParsableString parsable(
	  StringUtils::sprintf("column from %s order by %s", TABLE0_NAME,
	                       orderName));
	SQLSelectInfo selectInfo(parsable);
	proc.select(selectInfo);

	cut_assert_equal_int(1, selectInfo.orderedColumns.size());
	cut_assert_equal_string(orderName,
	                        selectInfo.orderedColumns[0].c_str());
}

void test_selectTwoTable(void)
{
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("* from %s,%s", TABLE0_NAME, TABLE1_NAME));
	SQLSelectInfo selectInfo(parsable);
	proc.select(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoListIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(TABLE0_NAME, (*table)->name.c_str());
	++table;
	cut_assert_equal_string(TABLE1_NAME, (*table)->name.c_str());
}

void test_selectTwoTableWithNames(void)
{
	const char var1[] = "var1";
	const char var2[] = "var2";
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("* from %s %s,%s %s",
	                       TABLE0_NAME, var1, TABLE1_NAME, var2));
	SQLSelectInfo selectInfo(parsable);
	proc.select(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoListIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(TABLE0_NAME, (*table)->name.c_str());
	cut_assert_equal_string(var1, (*table)->varName.c_str());
	++table;
	cut_assert_equal_string(TABLE1_NAME, (*table)->name.c_str());
	cut_assert_equal_string(var2, (*table)->varName.c_str());
}

void test_selectTwoTableWithoutVar(void)
{
	string statement =
	  StringUtils::sprintf("select %s from %s, %s",
	                       COLUMN_NAME_ANIMAL, TABLE0_NAME, TABLE1_NAME);
	const size_t numColumns = 1;
	const size_t numExpectedRows = numTestData0 * numTestData1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numExpectedRows);
}

void test_selectTestData(void)
{
	string statement = 
	  StringUtils::sprintf("%s,%s from %s", COLUMN_NAME_NUMBER,
	                       COLUMN_NAME_NAME, TABLE0_NAME);
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, NUM_COLUMN0_DEFS, numTestData0);

	// assertion
	for (size_t i = 0; i < NUM_COLUMN0_DEFS; i++) {
		SQLOutputColumn &outCol = selectInfo.outputColumnVector[i];
		cppcut_assert_equal(string(COLUMN0_DEFS[i].columnName),
		                    outCol.column);
	}

	for (size_t i = 0; i < numTestData0; i++) {
		cppcut_assert_equal(NUM_COLUMN0_DEFS,
		                    selectInfo.textRows[i].size());
		string &number = selectInfo.textRows[i][0];
		string &name   = selectInfo.textRows[i][1];
		string expected_number
		  = StringUtils::sprintf("%d", testData0[i].number);
		string expected_name = testData0[i].name;
		cppcut_assert_equal(expected_number, number);
		cppcut_assert_equal(expected_name, name);
	}
}

void test_whereAndLimit(void)
{
	string statement = 
	  StringUtils::sprintf("%s from %s % where %s=%d limit 1",
	                       COLUMN_NAME_NAME, TABLE0_NAME,
	                       COLUMN_NAME_NUMBER, testData0[0].number);
	cut_notify("test of 'limit 1' is not performed. "
	           "We should added assertions to test it "
	           "when 'limit' is supported.");
	const int numExpectedColumns = 1;
	const int numExpectedRows = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numExpectedColumns, numExpectedRows);
	cppcut_assert_equal(string(testData0[0].name),
	                    selectInfo.textRows[0][0]);
}

void test_selectMax(void)
{
	string testFormula =
	  StringUtils::sprintf("max(%s)", COLUMN_NAME_NUMBER);
	string statement =
	  StringUtils::sprintf("%s from %s", testFormula.c_str(), TABLE0_NAME);
	const size_t numColumns = 1;
	const size_t numExpectedRows = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0, numExpectedRows);

	// assertion
	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(testFormula, outCol.column);
	cppcut_assert_equal(StringUtils::toString(getMaxDataInTestData()),
	                    selectInfo.textRows[0][0]);
}

void test_selectCount(void)
{
	string testFormula =
	  StringUtils::sprintf("count(%s)", COLUMN_NAME_NUMBER);
	string statement =
	  StringUtils::sprintf("%s from %s", testFormula.c_str(), TABLE0_NAME);
	const size_t numColumns = 1;
	const size_t numExpectedRows = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0, numExpectedRows);

	// assertion
	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(testFormula, outCol.column);
	cppcut_assert_equal(StringUtils::toString(numTestData0),
	                    selectInfo.textRows[0][0]);
}

void test_selectCountDistinct(void)
{
	string testFormula =
	  StringUtils::sprintf("count(distinct %s)", COLUMN_NAME_NUMBER);
	string statement =
	  StringUtils::sprintf("%s from %s", testFormula.c_str(), TABLE0_NAME);
	const size_t numColumns = 1;
	const size_t numExpectedRows = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0, numExpectedRows);

	// assertion
	set<int> distinctValues;
	getDistinctValueInTestData0(distinctValues);
	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(testFormula, outCol.column);
	cppcut_assert_equal(StringUtils::toString(distinctValues.size()),
	                    selectInfo.textRows[0][0]);
}

void test_selectSum(void)
{
	string testFormula =
	  StringUtils::sprintf("sum(%s)", COLUMN_NAME_NUMBER);
	string statement =
	  StringUtils::sprintf("%s from %s", testFormula.c_str(), TABLE0_NAME);
	const size_t numColumns = 1;
	const size_t numExpectedRows = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0, numExpectedRows);

	// assertion
	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(testFormula, outCol.column);
	int expected = sumNumberOfTestData0();
	cppcut_assert_equal(StringUtils::toString(expected),
	                    selectInfo.textRows[0][0]);
}

void test_selectAlias(void)
{
	const char *alias = "foo";
	string statement =
	  StringUtils::sprintf("%s as %s from %s", COLUMN_NAME_NUMBER, alias,
	                       TABLE0_NAME);
	const size_t numColumns = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0);

	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(string(COLUMN_NAME_NUMBER), outCol.column);
	cppcut_assert_equal(string(alias), outCol.columnVar);

	// text output
	for (size_t i = 0; i < selectInfo.textRows.size(); i++) {
		cppcut_assert_equal(StringUtils::toString(testData0[i].number),
		                    selectInfo.textRows[i][0]);
	}
}

void test_selectAllTable0(void)
{
	assertSelectAll(TABLE0_NAME, testData0Getter,
	                NUM_COLUMN0_DEFS, numTestData0);
}

void test_selectAllTable1(void)
{
	assertSelectAll(TABLE1_NAME, testData1Getter,
	                NUM_COLUMN1_DEFS, numTestData1);
}

void test_selectAllTable0VarName(void)
{
	assertSelectAll(TABLE0_NAME, testData0Getter,
	                NUM_COLUMN0_DEFS, numTestData0, "foo");
}

void test_selectAllTable1VarName(void)
{
	assertSelectAll(TABLE1_NAME, testData1Getter,
	                NUM_COLUMN1_DEFS, numTestData1, "foo");
}

void test_join(void)
{
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	   selectInfo,
	   StringUtils::sprintf("* from %s,%s", TABLE0_NAME, TABLE1_NAME),
	   NUM_COLUMN0_DEFS + NUM_COLUMN1_DEFS, numTestData0 * numTestData1);

	// text output
	int row = 0;
	for (size_t tbl0idx = 0; tbl0idx < numTestData0; tbl0idx++) {
		for (size_t tbl1idx = 0; tbl1idx < numTestData1;
		     tbl1idx++, row++) {
			// number
			cppcut_assert_equal(
			  StringUtils::toString(testData0[tbl0idx].number),
		          selectInfo.textRows[row][0]);
			// name
			cppcut_assert_equal(
			  string(testData0[tbl0idx].name),
		          selectInfo.textRows[row][1]);
			// age
			cppcut_assert_equal(
			  StringUtils::toString(testData1[tbl1idx].age),
		          selectInfo.textRows[row][2]);
			// animal
			cppcut_assert_equal(
			  string(testData1[tbl1idx].animal),
		          selectInfo.textRows[row][3]);
			// food
			cppcut_assert_equal(
			  string(testData1[tbl1idx].food),
		          selectInfo.textRows[row][4]);
		}
	}
}

void test_emptyTable(void)
{
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	   selectInfo,
	   StringUtils::sprintf("* from %s", TABLE_Z_NAME),
	   NUM_COLUMN_Z_DEFS, numTestDataZ);
}

void test_nullTable(void)
{
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	   selectInfo,
	   StringUtils::sprintf("* from %s", TABLE_N_NAME),
	   NUM_COLUMN_Z_DEFS, numTestDataZ, numTestDataZ, false);
}

void test_insert(void)
{
	int number = 10;
	const char *name = "foo";
	ParsableString _parsable(
	  StringUtils::sprintf("insert into %s (%s,%s) values (%d,%s)",
	                       TABLE0_NAME,
	                       COLUMN_NAME_NUMBER, COLUMN_NAME_NAME,
	                       number, name));
	SQLInsertInfo insertInfo(_parsable);
	TestSQLProcessor proc;
	cppcut_assert_equal(true, proc.insert(insertInfo));
}

void test_update(void)
{
	const int idxData = 0;
	int newNumber = testData0[idxData].number + 1;
	ParsableString _parsable(
	  StringUtils::sprintf("update %s set %s=%d where %s=%d",
	                       TABLE0_NAME,
	                       COLUMN_NAME_NUMBER, newNumber,
	                       COLUMN_NAME_NUMBER, testData0[idxData].number));
	SQLUpdateInfo updateInfo(_parsable);
	TestSQLProcessor proc;
	cppcut_assert_equal(true, proc.update(updateInfo));

	ParsableString selectStatement(
	  StringUtils::sprintf("select %s from %s where %s='%s'",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_NAME, testData0[idxData].name));
	SQLSelectInfo selectInfo(selectStatement);
	proc.select(selectInfo);
	assertSQLSelectInfoBasic(selectInfo, 1, 1);
	StringVector expectedRows;
	expectedRows.push_back(StringUtils::sprintf("%d", newNumber));
	assertStringVector(expectedRows, selectInfo.textRows[0]);
}

void test_crossJoin(void) {
	string statement =
	  StringUtils::sprintf("select * from %s,%s", TABLE0_NAME, TABLE1_NAME);
	const size_t expectedNumColumns = NUM_COLUMN0_DEFS + NUM_COLUMN1_DEFS;
	const size_t expectedNumRows = numTestData0 * numTestData1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, expectedNumColumns, expectedNumRows);

	cppcut_assert_equal(expectedRefCountOfResultTableInTheSimplestCase,
	                    selectInfo.joinedTable->getUsedCount());

	AssertCrossJoin<TestData0, TestData1>
	  assertJoin((ItemTable *)selectInfo.selectedTable,
	             testData0, testData1, numTestData0, numTestData1);
	assertJoin.run(assertJoinRunner);
}

void test_innerJoin(void) {
	string statement =
	  StringUtils::sprintf("select * from %s inner join %s on %s.%s=%s.%s",
	                       TABLE0_NAME, TABLE1_NAME,
	                       TABLE0_NAME, COLUMN_NAME_NUMBER,
	                       TABLE1_NAME, COLUMN_NAME_AGE);
	assertInerJoinHelper(statement);
}

void test_innerJoinWithTableNames(void) {
	const char *tableVarName0 = "tvar0";
	const char *tableVarName1 = "tvar1";
	string statement =
	  StringUtils::sprintf(
	    "select * from %s %s inner join %s %s on %s.%s=%s.%s",
	    TABLE0_NAME, tableVarName0, TABLE1_NAME, tableVarName1,
	    tableVarName0, COLUMN_NAME_NUMBER,
	    tableVarName1, COLUMN_NAME_AGE);
	assertInerJoinHelper(statement);
}

void test_innerJoinThreeTables(void) {
	string statement =
	  StringUtils::sprintf("select * from %s inner join %s on %s.%s=%s.%s "
	                       "inner join %s on %s.%s=%s.%s",
	                       TABLE0_NAME, TABLE1_NAME,
	                       TABLE0_NAME, COLUMN_NAME_NUMBER,
	                       TABLE1_NAME, COLUMN_NAME_AGE,
	                       TABLE2_NAME,
	                       TABLE1_NAME, COLUMN_NAME_ANIMAL,
	                       TABLE2_NAME, COLUMN_NAME_FAVORITE);
	// check the result
	const size_t expectedNumColumns = NUM_COLUMN0_DEFS + NUM_COLUMN1_DEFS
	                                  + NUM_COLUMN2_DEFS;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, expectedNumColumns);

	vector<TestDataPointerSet> expectDataPtrVector;
	for (size_t i = 0; i < numTestData0; i++) {
		TestData0 *data0 = &testData0[i];
		for (size_t j = 0;  j < numTestData1; j++) {
			TestData1 *data1 = &testData1[j];
			if (data0->number != data1->age)
				continue;
			for (size_t k = 0;  k < numTestData2; k++) {
				TestData2 *data2 = &testData2[k];
				if (strcmp(data1->animal, data2->favorite))
					continue;
				TestDataPointerSet dataSet;
				dataSet.data0 = data0;
				dataSet.data1 = data1;
				dataSet.data2 = data2;
				expectDataPtrVector.push_back(dataSet);
			}
		}
	}
	cppcut_assert_equal(expectDataPtrVector.size(), 
	                    selectInfo.textRows.size());
	for (size_t i = 0; i < expectDataPtrVector.size(); i++) {
		TestData0 *data0 = expectDataPtrVector[i].data0;
		TestData1 *data1 = expectDataPtrVector[i].data1;
		TestData2 *data2 = expectDataPtrVector[i].data2;
		StringVector &textVect = selectInfo.textRows[i];
		int tvecIdx = 0;

		// testData0
		cppcut_assert_equal(StringUtils::toString(data0->number),
		                    textVect[tvecIdx++]);
		cppcut_assert_equal(string(data0->name), textVect[tvecIdx++]);

		// testData1
		cppcut_assert_equal(StringUtils::toString(data1->age),
		                    textVect[tvecIdx++]);
		cppcut_assert_equal(string(data1->animal), textVect[tvecIdx++]);
		cppcut_assert_equal(string(data1->food), textVect[tvecIdx++]);

		// testData2
		cppcut_assert_equal(StringUtils::toString(data2->mentalAge),
		                    textVect[tvecIdx++]);
		cppcut_assert_equal(string(data2->favorite),
		                    textVect[tvecIdx++]);
	}
}

void test_groupBy(void) {
	string statement =
	  StringUtils::sprintf("select %s from %s group by %s",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_NUMBER);
	// check the result
	set<int> distinctValues;
	getDistinctValueInTestData0(distinctValues);
	const size_t expectedNumColumns = 1;
	const size_t expectedNumRows = distinctValues.size();
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, expectedNumColumns, numTestData0,
	  expectedNumRows);

	// make the expected output string set and check it
	multiset<string> expectedOutputSet;
	for (set<int>::iterator it = distinctValues.begin();
	     it != distinctValues.end(); ++it) {
		string text = StringUtils::sprintf("%d", *it);
		expectedOutputSet.insert(text);
	}
	size_t targetIndex = 0;
	assertEqualUnawareOfOrder(expectedOutputSet,
	                          selectInfo.textRows, targetIndex);
}

void test_whereIn(void) {
	int selectValue0 = -5;
	int selectValue1 = 15;
	string statement =
	  StringUtils::sprintf("select %s from %s where %s in ('%d', '%d')",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_NUMBER, selectValue0, selectValue1);
	// check the result
	const size_t expectedNumColumns = 1;
	vector<size_t> targetRowsIndexes;
	getIndexesOfDataWithValueInTestData0(targetRowsIndexes, selectValue0);
	getIndexesOfDataWithValueInTestData0(targetRowsIndexes, selectValue1);
	const size_t expectedNumRows = targetRowsIndexes.size();
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, expectedNumColumns, expectedNumRows);

	// make the expected output string set
	multiset<string> expectedOutputSet;
	for (size_t i = 0; i < targetRowsIndexes.size(); i++) {
		size_t index = targetRowsIndexes[i];
		string text = StringUtils::sprintf("%d", testData0[index]);
		expectedOutputSet.insert(text);
	}
	
	// check the textRows
	size_t targetIndex = 0;
	assertEqualUnawareOfOrder(expectedOutputSet,
	                          selectInfo.textRows, targetIndex);
}

void test_distinct(void) {
	string statement =
	  StringUtils::sprintf("select distinct %s from %s",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME);
	// check the result
	set<int> distinctValues;
	getDistinctValueInTestData0(distinctValues);
	const size_t numColumns = 1;
	const size_t numExpectedRows = distinctValues.size();
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numTestData0, numExpectedRows);

	// make the expected output string set
	multiset<string> expectedOutputSet;
	for (set<int>::iterator it = distinctValues.begin();
	     it != distinctValues.end(); ++it) {
		string text = StringUtils::sprintf("%d", *it);
		expectedOutputSet.insert(text);
	}
	size_t targetIndex = 0;
	assertEqualUnawareOfOrder(expectedOutputSet,
	                          selectInfo.textRows, targetIndex);
}

void test_exists(void) {
	string statement =
	  StringUtils::sprintf("select %s from %s where "
	                       "exists (select %s from %s where %s.%s=%s.%s)",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_AGE, TABLE1_NAME, 
	                       TABLE0_NAME, COLUMN_NAME_NUMBER,
	                       TABLE1_NAME, COLUMN_NAME_AGE);

	// check the result
	multiset<int> numberSet;
	getValueData0NumberEqData1Age(numberSet);
	const size_t numColumns = 1;
	const size_t numExpectedRows = numberSet.size();
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(
	  selectInfo, statement, numColumns, numExpectedRows);

	// make the expected output string set
	multiset<string> expectedOutputSet;
	for (set<int>::iterator it = numberSet.begin();
	     it != numberSet.end(); ++it) {
		string text = StringUtils::sprintf("%d", *it);
		expectedOutputSet.insert(text);
	}
	size_t targetIndex = 0;
	assertEqualUnawareOfOrder(expectedOutputSet,
	                          selectInfo.textRows, targetIndex);
}

void test_wrapInStatement(void) {
	string statement =
	  StringUtils::sprintf("select %s\n from %s \nwhere %s>1\n",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_NUMBER);

	// check the result
	const size_t numColumns = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(selectInfo, statement, numColumns);
}

void test_wrapInStatementWithoutSpace(void) {
	string statement =
	  StringUtils::sprintf("select %s\nfrom %s\nwhere %s>1\n",
	                       COLUMN_NAME_NUMBER, TABLE0_NAME,
	                       COLUMN_NAME_NUMBER);

	// check the result
	const size_t numColumns = 1;
	DEFINE_SELECTINFO_AND_ASSERT_SELECT(selectInfo, statement, numColumns);
}

} // namespace testSQLProcessor
