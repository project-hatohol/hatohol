#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
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

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_parseOneColumn(void)
{
	const char *tableName  = "tableName";
	const char *columnName = "columnName";
	const char *valueStr   = "10";
	string statement =
	  StringUtils::sprintf("%s (%s) values (%s)",
	                       tableName, columnName, valueStr);
	DEFINE_INSERTINFO_AND_ASSERT_SELECT(insertInfo, statement);
}

} // namespace testSQLProcessorInsert
