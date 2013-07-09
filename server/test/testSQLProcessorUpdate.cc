/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "Hatohol.h"
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
	} catch (const exception &e) {
		cut_fail("Got exception: %s", e.what());
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
	hatoholInit();
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

void test_parseMultiColumns(void)
{
	const char *tableName  = "tableName";
	struct ColumnValuePair {
		const char *column;
		const char *value;
	};
	ColumnValuePair columnValuePair[] = {
	  {"columnName1", "value"},
	  {"columnName2", "Foo Goo"},
	  {"columnName3", "-5"},
	};
	const size_t numColumnValuePair =
	  sizeof(columnValuePair) / sizeof(ColumnValuePair);
	const char *whereColumn = "a";
	int         whereValue  = 5;
	string statement =
	  StringUtils::sprintf(
	    "update %s SET %s='%s', %s='%s', %s=%s where %s=%d",
	   tableName,
	   columnValuePair[0].column, columnValuePair[0].value,
	   columnValuePair[1].column, columnValuePair[1].value,
	   columnValuePair[2].column, columnValuePair[2].value,
	   whereColumn, whereValue);
	DEFINE_UPDATEINFO_AND_ASSERT_SELECT(updateInfo, statement);

	StringVector expectedColumns;
	StringVector expectedValues;
	for (size_t i = 0; i < numColumnValuePair; i++) {
		ColumnValuePair *cvPair = &columnValuePair[i];
		expectedColumns.push_back(cvPair->column);
		expectedValues.push_back(cvPair->value);
	}

	cppcut_assert_equal(string(tableName), updateInfo.table);
	assertStringVector(expectedColumns, updateInfo.columnVector);
	assertStringVector(expectedValues, updateInfo.valueVector);
	assertBinomialFormula(FormulaComparatorEqual,
	                      updateInfo.whereParser.getFormula(),
	                      FormulaVariable, const char *, whereColumn,
	                      FormulaValue, int, whereValue);
}

} // namespace testSQLProcessorUpdate
