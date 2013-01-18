#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include "SQLProcessor.h"

namespace testSQLProcessor {

static const int TABLE_ID = 1;
static const int TABLE_ID_A = 2;

static const char *TABLE_NAME = "TestTable";
static const char *TABLE_NAME_A = "TestTableA";

static const char *COLUMN_NAME1 = "column1";
static const char *COLUMN_NAME2 = "column2";

static const char *COLUMN_NAME_A1 = "columnA1";
static const char *COLUMN_NAME_A2 = "columnA2";
static const char *COLUMN_NAME_A3 = "columnA3";

static const int NUM_COLUMN_DEFS = 2;
static ColumnBaseDefinition COLUMN_DEFS[NUM_COLUMN_DEFS] = {
  {0, TABLE_NAME, COLUMN_NAME1, SQL_COLUMN_TYPE_INT, 11, 0},
  {0, TABLE_NAME, COLUMN_NAME2, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

static const int NUM_COLUMN_DEFS_A = 3;
static ColumnBaseDefinition COLUMN_DEFS_A[NUM_COLUMN_DEFS_A] = {
  {0, TABLE_NAME_A, COLUMN_NAME_A1, SQL_COLUMN_TYPE_INT, 11, 0},
  {0, TABLE_NAME_A, COLUMN_NAME_A2, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
  {0, TABLE_NAME_A, COLUMN_NAME_A3, SQL_COLUMN_TYPE_VARCHAR, 20, 0},
};

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
	              const SQLTableInfo &tableInfo,
	              const ItemIdVector &itemIdVector)
	{
		const ItemTablePtr tablePtr;
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
		m_tableNameStaticInfoMap[TABLE_NAME] = staticInfo;
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
		initStaticInfoEach(&m_staticInfo[0], TABLE_ID, TABLE_NAME,
		                   NUM_COLUMN_DEFS, COLUMN_DEFS);
		initStaticInfoEach(&m_staticInfo[1], TABLE_ID_A, TABLE_NAME_A,
		                   NUM_COLUMN_DEFS_A, COLUMN_DEFS_A);
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneItem(void)
{
	TestSQLProcessor proc;
	const char *selectedItem = "columnArg";
	const char *tableName = TABLE_NAME;
	ParsableString parsable(
	  StringUtils::sprintf("%s from %s", selectedItem, tableName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(1, selectInfo.columns.size());
	cut_assert_equal_string(selectedItem, selectInfo.columns[0].name.c_str());

	cut_assert_equal_int(1, selectInfo.tables.size());
	cut_assert_equal_string(tableName, selectInfo.tables[0].name.c_str());
}

void test_selectTableVar(void)
{
	TestSQLProcessor proc;
	const char *tableVarName = "tvar";
	ParsableString parsable(
	  StringUtils::sprintf("columnArg from %s %s",
	                       TABLE_NAME, tableVarName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);
	cut_assert_equal_int(1, selectInfo.tables.size());
	cut_assert_equal_string(tableVarName, selectInfo.tables[0].varName.c_str());
}

void test_selectOrderBy(void)
{
	TestSQLProcessor proc;
	const char *orderName = "orderArg";
	ParsableString parsable(
	  StringUtils::sprintf("column from %s  order by %s", TABLE_NAME,
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
	  StringUtils::sprintf("* from %s,%s", TABLE_NAME, TABLE_NAME_A));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	cut_assert_equal_string(TABLE_NAME, selectInfo.tables[0].name.c_str());
	cut_assert_equal_string(TABLE_NAME_A, selectInfo.tables[1].name.c_str());
}

void test_selectTwoTableWithNames(void)
{
	const char var1[] = "var1";
	const char var2[] = "var2";
	TestSQLProcessor proc;
	ParsableString parsable(
	  StringUtils::sprintf("* from %s %s,%s %s",
	                       TABLE_NAME, var1, TABLE_NAME_A, var2));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	cut_assert_equal_string(TABLE_NAME, selectInfo.tables[0].name.c_str());
	cut_assert_equal_string(var1, selectInfo.tables[0].varName.c_str());
	cut_assert_equal_string(TABLE_NAME_A, selectInfo.tables[1].name.c_str());
	cut_assert_equal_string(var2, selectInfo.tables[1].varName.c_str());
}


} // namespace testSQLProcessor
