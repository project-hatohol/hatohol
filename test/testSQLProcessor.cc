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

class TestSQLProcessor : public SQLProcessor {
public:
	TestSQLProcessor(void)
	: SQLProcessor(m_tableNameStaticInfoMap)
	{
	}

	// Implementation of abstruct method
	bool select(SQLSelectInfo &selectInfo) {
		return false;
	}

	const char *getDBName(void) {
		return "TestTable";
	}

	void callParseSelectStatement(SQLSelectInfo &selectInfo) {
		parseSelectStatement(selectInfo);
	}

private:
	TableNameStaticInfoMap m_tableNameStaticInfoMap;
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneItem(void)
{
	TestSQLProcessor proc;
	const char *selectedItem = "columnArg";
	const char *tableName = "table";
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
	  StringUtils::sprintf("columnArg from tableName %s", tableVarName));
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
	  StringUtils::sprintf("column from tableName order by %s", orderName));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(1, selectInfo.orderedColumns.size());
	cut_assert_equal_string(orderName,
	                        selectInfo.orderedColumns[0].c_str());
}

void test_selectTwoTable(void)
{
	TestSQLProcessor proc;
	const char *table1 = "Tbl1";
	const char *table2 = "Tbl2";
	ParsableString parsable(
	  StringUtils::sprintf("* from %s %s", table1, table2));
	SQLSelectInfo selectInfo(parsable);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(2, selectInfo.tables.size());
	cut_assert_equal_string(table1, selectInfo.tables[0].name.c_str());
	cut_assert_equal_string(table2, selectInfo.tables[1].name.c_str());
}

} // namespace testSQLProcessor
