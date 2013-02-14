#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "Asura.h"
#include "SQLProcessorInsert.h"

namespace testSQLProcessorInsert {

static TableNameStaticInfoMap dummyMap;

class TestSQLProcessorInsert : public SQLProcessorInsert {
public:
	TestSQLProcessorInsert(void)
	: SQLProcessorInsert(dummyMap)
	{
	}

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

static void _assertParseOneColumn(const char *tableName,
                                  const char *columnName,
                                  const char *valueString,
                                  const char *expectedValue = NULL)
{
	string statement =
	  StringUtils::sprintf("insert into %s (%s) values (%s)",
	                       tableName, columnName, valueString);
	DEFINE_INSERTINFO_AND_ASSERT_SELECT(insertInfo, statement);

	StringVector expectedColumns;
	expectedColumns.push_back(columnName);

	StringVector expectedValues;
	if (!expectedValue)
		expectedValues.push_back(valueString);
	else
		expectedValues.push_back(expectedValue);

	cppcut_assert_equal(string(tableName), insertInfo.table);
	assertStringVector(expectedColumns, insertInfo.columnVector);
	assertStringVector(expectedValues, insertInfo.valueVector);
}
#define assertParseOneColumn(T, C, V, ...) \
cut_trace(_assertParseOneColumn(T, C, V, ##__VA_ARGS__))

static string removeQuotation(string str)
{
	size_t length = str.size();
	if (!(str[0] == '\'' && str[length-1] == '\''))
		return str;
	return string(str, 1, length-2);
}

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

void test_parseOneColumnWithStringValue(void)
{
	const char *tableName  = "tableName";
	const char *columnName = "columnName";
	const char *valueExpected = "bite into a doughnut";
	string valueQuot = StringUtils::sprintf("'%s'", valueExpected);

	assertParseOneColumn(tableName, columnName, valueQuot.c_str(),
	                     valueExpected);
}

void test_parseMultiColumn(void)
{
	const char *tableName  = "tableName";
	struct Data {
		const char *columnName;
		const char *valueString;
	} data[] = {
	  {"c1", "10"}, {"c2", "'foo'"}, {"c3", "-5"},
	};
	const size_t numData = sizeof(data) / sizeof(Data);

	string statement =
	  StringUtils::sprintf("insert into %s (%s,%s,%s) values (%s,%s,%s)",
	                       tableName,
	                       data[0].columnName, data[1].columnName,
	                       data[2].columnName,
	                       data[0].valueString, data[1].valueString,
	                       data[2].valueString);

	DEFINE_INSERTINFO_AND_ASSERT_SELECT(insertInfo, statement);

	StringVector expectedColumns;
	StringVector expectedValues;
	for (size_t i = 0; i < numData; i++) {
		expectedColumns.push_back(data[i].columnName);
		expectedValues.push_back(removeQuotation(data[i].valueString));
	}

	cppcut_assert_equal(string(tableName), insertInfo.table);
	assertStringVector(expectedColumns, insertInfo.columnVector);
	assertStringVector(expectedValues, insertInfo.valueVector);
}


} // namespace testSQLProcessorInsert
