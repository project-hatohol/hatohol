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

namespace testSQLProcessor {

static const int TABLE_ID = 1;
static const int TABLE_ID_A = 2;

static const char *TABLE0_NAME = "TestTable0";
static const char *TABLE1_NAME = "TestTable1";

static const char *COLUMN_NAME_NUMBER = "number";
static const char *COLUMN_NAME_LINE = "name";

static const char *COLUMN_NAME_A1 = "columnA1";
static const char *COLUMN_NAME_A2 = "columnA2";
static const char *COLUMN_NAME_A3 = "columnA3";

enum {
	ITEM_ID_NUMBER,
	ITEM_ID_NAME,

	ITEM_ID_A1,
	ITEM_ID_A2,
	ITEM_ID_A3,
};

static const size_t NUM_COLUMN_DEFS = 2;
static ColumnBaseDefinition COLUMN_DEFS[NUM_COLUMN_DEFS] = {
  {ITEM_ID_NUMBER, TABLE0_NAME, COLUMN_NAME_NUMBER, SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_NAME, TABLE0_NAME, COLUMN_NAME_LINE, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const size_t NUM_COLUMN_DEFS_A = 3;
static ColumnBaseDefinition COLUMN_DEFS_A[NUM_COLUMN_DEFS_A] = {
  {ITEM_ID_A1, TABLE1_NAME, COLUMN_NAME_A1, SQL_COLUMN_TYPE_INT, 11, 0},
  {ITEM_ID_A2, TABLE1_NAME, COLUMN_NAME_A2, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
  {ITEM_ID_A3, TABLE1_NAME, COLUMN_NAME_A3, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

struct TestData {
	int         number;
	const char *name;
};

static TestData testData[] = {
  {15, "ant"},
  {100, "Dog"},
  {-5, "Clothes make the man."},
};
static size_t numTestData = sizeof(testData) / sizeof(TestData);

class TestSQLProcessor : public SQLProcessor {
public:
	TestSQLProcessor(void)
	: SQLProcessor(m_tableNameStaticInfoMap)
	{
		initStaticInfo();
	}

	// Implementation of abstruct method
	const char *getDBName(void) {
		return "TestTable";
	}

	void callParseSelectStatement(SQLSelectInfo &selectInfo) {
		parseSelectStatement(selectInfo);
	}

	const ItemTablePtr
	tableMakeFunc(SQLSelectInfo &selectInfo,
	              const SQLTableInfo &tableInfo)
	{
		const ItemTablePtr tablePtr;
		for (size_t i = 0; i < numTestData; i++) {
			ItemGroup *grp = tablePtr->addNewGroup();
			grp->add(new ItemInt(ITEM_ID_NUMBER,
			                     testData[i].number), false);
			grp->add(new ItemString(ITEM_ID_NAME, testData[i].name),
			         false);
		}
		return tablePtr;
	}

private:
	TableNameStaticInfoMap m_tableNameStaticInfoMap;
	SQLTableStaticInfo     m_staticInfo[2];


	void initStaticInfoEach(SQLTableStaticInfo *staticInfo,
	                        int tableId, const char *tableName,
	                        int numColumnDefs,
	                        ColumnBaseDefinition *columnDefs)
	{
		m_tableNameStaticInfoMap[tableName] = staticInfo;
		staticInfo->tableId = tableId;
		staticInfo->tableName = tableName;
		staticInfo->tableMakeFunc =
		  static_cast<SQLTableMakeFunc>
		  (&TestSQLProcessor::tableMakeFunc);

		ColumnBaseDefList &list =
		  const_cast<ColumnBaseDefList &>
		  (staticInfo->columnBaseDefList);

		ItemNameColumnBaseDefRefMap &map =
		  const_cast<ItemNameColumnBaseDefRefMap &>
		  (staticInfo->columnBaseDefMap);

		for (int i = 0; i < numColumnDefs; i++) {
			list.push_back(columnDefs[i]);
			map[columnDefs[i].columnName] = &columnDefs[i];
		}
	}

	void initStaticInfo(void) {
		initStaticInfoEach(&m_staticInfo[0], TABLE_ID, TABLE0_NAME,
		                   NUM_COLUMN_DEFS, COLUMN_DEFS);
		initStaticInfoEach(&m_staticInfo[1], TABLE_ID_A, TABLE1_NAME,
		                   NUM_COLUMN_DEFS_A, COLUMN_DEFS_A);
	}
};

static const int getMaxDataInTestData(void)
{
	int max = testData[0].number;
	for (size_t i = 1; i < numTestData; i++) {
		if (testData[i].number > max)
			max = testData[i].number;
	}
	return max;
}

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

	// Selected Rows
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

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneColumn(void)
{
	TestSQLProcessor proc;
	const char *columnName = "column";
	const char *tableName = TABLE0_NAME;
	ParsableString parsable(
	  StringUtils::sprintf("%s from %s", columnName, tableName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	// column
	const SQLFormulaInfoVector formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());

	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	assertFormulaVariable(formulaInfo->formula, columnName);

	// table
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableName, (*table)->name.c_str());
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
	proc.callParseSelectStatement(selectInfo);

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
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
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
	proc.callParseSelectStatement(selectInfo);
	cut_assert_equal_int(1, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(tableVarName, (*table)->varName.c_str());
}

void test_selectOrderBy(void)
{
	TestSQLProcessor proc;
	const char *orderName = "orderArg";
	ParsableString parsable(
	  StringUtils::sprintf("column from %s  order by %s", TABLE0_NAME,
	                       orderName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

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
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
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
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	SQLTableInfoVectorIterator table = selectInfo.tables.begin();
	cut_assert_equal_string(TABLE0_NAME, (*table)->name.c_str());
	cut_assert_equal_string(var1, (*table)->varName.c_str());
	++table;
	cut_assert_equal_string(TABLE1_NAME, (*table)->name.c_str());
	cut_assert_equal_string(var2, (*table)->varName.c_str());
}

void test_selectTestData(void)
{
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("%s,%s from %s",
	                       COLUMN_NAME_NUMBER, COLUMN_NAME_LINE,
	                       TABLE0_NAME));
	SQLSelectInfo selectInfo(parsable);
	cppcut_assert_equal(true, proc.select(selectInfo));

	// assertion
	assertSQLSelectInfoBasic(selectInfo, NUM_COLUMN_DEFS, numTestData);
	for (size_t i = 0; i < NUM_COLUMN_DEFS; i++) {
		SQLOutputColumn &outCol = selectInfo.outputColumnVector[i];
		cppcut_assert_equal(string(COLUMN_DEFS[i].columnName),
		                    outCol.column);
	}

	for (size_t i = 0; i < numTestData; i++) {
		cppcut_assert_equal(NUM_COLUMN_DEFS,
		                    selectInfo.textRows[i].size());
		string &number = selectInfo.textRows[i][0];
		string &name   = selectInfo.textRows[i][1];
		string expected_number
		  = StringUtils::sprintf("%d", testData[i].number);
		string expected_name = testData[i].name;
		cppcut_assert_equal(expected_number, number);
		cppcut_assert_equal(expected_name, name);
	}
}

void test_selectMax(void)
{
	TestSQLProcessor proc;
	string testFormula =
	  StringUtils::sprintf("max(%s)", COLUMN_NAME_NUMBER);
	ParsableString parsable(
	  StringUtils::sprintf("%s from %s", testFormula.c_str(), TABLE0_NAME));
	SQLSelectInfo selectInfo(parsable);
	cppcut_assert_equal(true, proc.select(selectInfo));
	const size_t numColumns = 1;
	const size_t numExpectedRows = 1;

	// assertion
	assertSQLSelectInfoBasic(selectInfo, numColumns, numTestData,
	                         numExpectedRows);
	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(testFormula, outCol.column);
	cppcut_assert_equal(StringUtils::toString(getMaxDataInTestData()),
	                    selectInfo.textRows[0][0]);
}

void test_selectAlias(void)
{
	TestSQLProcessor proc;
	const char *alias = "foo";
	ParsableString parsable(
	  StringUtils::sprintf("%s as %s from %s", COLUMN_NAME_NUMBER, alias,
	                       TABLE0_NAME));
	SQLSelectInfo selectInfo(parsable);
	cppcut_assert_equal(true, proc.select(selectInfo));
	const size_t numColumns = 1;

	// assertion
	assertSQLSelectInfoBasic(selectInfo, numColumns, numTestData);

	SQLOutputColumn &outCol = selectInfo.outputColumnVector[0];
	cppcut_assert_equal(string(COLUMN_NAME_NUMBER), outCol.column);

	// text output
	for (size_t i = 0; i < selectInfo.textRows.size(); i++) {
		cppcut_assert_equal(StringUtils::toString(testData[i].number),
		                    selectInfo.textRows[i][0]);
	}
}

} // namespace testSQLProcessor
