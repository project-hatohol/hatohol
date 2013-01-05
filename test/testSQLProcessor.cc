#include <string>
#include <vector>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <cutter.h>
#include "SQLProcessor.h"

namespace testSQLProcessor {

class TestSQLProcessor : public SQLProcessor {
public:
	// Implementation of abstruct method
	bool select(SQLSelectResult &result, SQLSelectInfo   &selectInfo) {
		return false;
	}

	void callParseSelectStatement(SQLSelectInfo &selInfo);
};

void TestSQLProcessor::callParseSelectStatement(SQLSelectInfo &selectInfo)
{
	vector<string> words;
	StringUtils::split(words, statement, ' ');
	parseSelectStatement(selectInfo);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_selectOneItem(void)
{
	TestSQLProcessor proc;
	const char *selectedItem = "row";
	const char *tableName = "table";
	string statement = StringUtils::sprintf("select %s from %s",
	                                        selectedItem, tableName);
	SQLSelectInfo selectInfo(statement);
	proc.callParseSelectStatement(selectInfo);

	cut_assert_equal_int(1, selectInfo.columns.size());
	cut_assert_equal_string(selectedItem,
	                        selectInfo.columns[0].c_str());

	cut_assert_equal_string(tableName, selectInfo.table.c_str());
}

void test_selectTableVar(void)
{
	SQLSelectStruct selStruct;
	TestSQLProcessor proc;
	const char *tableVarName = "tvar";
	string statement =
	  StringUtils::sprintf("select rowName from tableName %s",
	                       tableVarName);
	proc.callParseSelectStatement(selStruct, statement);
	cut_assert_equal_string(tableVarName, selStruct.tableVar.c_str());
}

void test_selectOrderBy(void)
{
	SQLSelectStruct selStruct;
	TestSQLProcessor proc;
	const char *orderName = "orderRow";
	string statement =
	  StringUtils::sprintf("select rowName from tableName order by %s",
	                       orderName);
	proc.callParseSelectStatement(selStruct, statement);

	cut_assert_equal_int(1, selStruct.orderedColumns.size());
	cut_assert_equal_string(orderName,
	                        selStruct.orderedColumns[0].c_str());
}

} // namespace testSQLProcessor
