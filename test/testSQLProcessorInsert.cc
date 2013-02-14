#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "Asura.h"
#include "SQLProcessorInsert.h"

namespace testSQLProcessorInsert {

class TestSQLProcessorInsert : public SQLProcessorInsert {
};

static void _asssertExecInsert(SQLInsertInfo &insertInfo)
{
	TestSQLProcessorInsert proc;
	cppcut_assert_equal(true, proc.insert(insertInfo));
}
#define asssertExecInsert(I) cut_trace(_asssertExecInsert(I))

#define DEFINE_INSERTINFO_AND_ASSERT_SELECT(INS_VAR, STATEMENT) \
ParsableString _parsable(STATEMENT); \
SQLInsertInfo INS_VAR(_parsable); \
asssertExecInsert(INS_VAR)


static void _assertStringVector(StringVector &expected, StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_parseOneColumn(void)
{
	const char *tableName  = "tableName";
	const char *columnName = "columnName";
	const char *valueStr   = "10";
	string statement =
	  StringUtils::sprintf("insert into %s (%s) values (%s)",
	                       tableName, columnName, valueStr);
	DEFINE_INSERTINFO_AND_ASSERT_SELECT(insertInfo, statement);

	StringVector expectedColumns;
	expectedColumns.push_back(columnName);

	StringVector expectedValues;
	expectedValues.push_back(valueStr);

	cppcut_assert_equal(string(tableName), insertInfo.table);
	assertStringVector(expectedColumns, insertInfo.columnVector);
	assertStringVector(expectedValues, insertInfo.valueVector);
}

} // namespace testSQLProcessorInsert
