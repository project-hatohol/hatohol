#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "Asura.h"
#include "SQLProcessorUpdate.h"
#include "SQLProcessorException.h"
#include "FormulaTestUtils.h"
#include "Helpers.h"

namespace testSQLProcessorUpdate {

static TableNameStaticInfoMap dummyMap;

static const char *dummyTableName = "tableName";

class TestSQLProcessorUpdate : public SQLProcessorUpdate {
public:
	TestSQLProcessorUpdate(void)
	: SQLProcessorUpdate(dummyMap)
	{
	}

	void callParseUpdateStatement(SQLUpdateInfo &updateInfo)
	{
		parseUpdateStatement(updateInfo);
	}
};

static void _asssertExecUpdate(SQLUpdateInfo &updateInfo)
{
	try {
		TestSQLProcessorUpdate proc;
		proc.callParseUpdateStatement(updateInfo);
	} catch (exception *e) {
		cut_fail("Got exception: %s", e->what());
		delete e;
	}
}
#define asssertExecUpdate(I) cut_trace(_asssertExecUpdate(I))

#define DEFINE_UPDATEINFO_AND_ASSERT_SELECT(INS_VAR, STATEMENT) \
ParsableString _parsable(STATEMENT); \
SQLUpdateInfo INS_VAR(_parsable); \
asssertExecUpdate(INS_VAR)

void setupDummyMap(void)
{
	if (!dummyMap.empty())
		return;
	dummyMap[dummyTableName] = NULL;
}

void setup(void)
{
	asuraInit();
	setupDummyMap();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_parseOneColumnInt(void)
{
	const char *tableName  = "tableName";
	const char *columnName = "columnName";
	const char *valueStr   = "10";
	const char *whereColumn = "a";
	int         whereValue  = 5;
	string statement =
	  StringUtils::sprintf("update %s SET %s=%s where %s=%d",
	                       tableName, columnName, valueStr,
	                       whereColumn, whereValue);
	DEFINE_UPDATEINFO_AND_ASSERT_SELECT(updateInfo, statement);

	StringVector expectedColumns;
	expectedColumns.push_back(columnName);
	StringVector expectedValues;
	expectedValues.push_back(valueStr);

	cppcut_assert_equal(string(tableName), updateInfo.table);
	assertStringVector(expectedColumns, updateInfo.columnVector);
	assertStringVector(expectedValues, updateInfo.valueVector);
	assertBinomialFormula(FormulaComparatorEqual,
	                      updateInfo.whereParser.getFormula(),
	                      FormulaVariable, const char *, whereColumn,
	                      FormulaValue, int, whereValue);
}

void test_parseOneColumnString(void)
{
	const char *tableName  = "tableName";
	const char *columnName = "columnName";
	const char *valueStr   = "Foo Goo";
	const char *whereColumn = "a";
	int         whereValue  = 5;
	string statement =
	  StringUtils::sprintf("update %s SET %s='%s' where %s=%d",
	                       tableName, columnName, valueStr,
	                       whereColumn, whereValue);
	DEFINE_UPDATEINFO_AND_ASSERT_SELECT(updateInfo, statement);

	StringVector expectedColumns;
	expectedColumns.push_back(columnName);
	StringVector expectedValues;
	expectedValues.push_back(valueStr);

	cppcut_assert_equal(string(tableName), updateInfo.table);
	assertStringVector(expectedColumns, updateInfo.columnVector);
	assertStringVector(expectedValues, updateInfo.valueVector);
	assertBinomialFormula(FormulaComparatorEqual,
	                      updateInfo.whereParser.getFormula(),
	                      FormulaVariable, const char *, whereColumn,
	                      FormulaValue, int, whereValue);
}

} // namespace testSQLProcessorUpdate
