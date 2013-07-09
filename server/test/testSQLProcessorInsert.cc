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
#include "SQLProcessorInsert.h"
#include "SQLProcessorException.h"
#include "Helpers.h"

namespace testSQLProcessorInsert {

static TableNameStaticInfoMap dummyMap;

static const char *dummyTableName = "tableName";

class TestSQLProcessorInsert : public SQLProcessorInsert {
public:
	TestSQLProcessorInsert(void)
	: SQLProcessorInsert(dummyMap)
	{
	}

	bool callParseInsertStatement(SQLInsertInfo &insertInfo)
	{
		return parseInsertStatement(insertInfo);
	}
};

static void _asssertExecInsert(SQLInsertInfo &insertInfo)
{
	try {
		TestSQLProcessorInsert proc;
		cppcut_assert_equal(true,
		                    proc.callParseInsertStatement(insertInfo));
	} catch (const SQLProcessorException &e) {
		cut_fail("Got exception: %s", e.what());
	}
}
#define asssertExecInsert(I) cut_trace(_asssertExecInsert(I))

#define DEFINE_INSERTINFO_AND_ASSERT_SELECT(INS_VAR, STATEMENT) \
ParsableString _parsable(STATEMENT); \
SQLInsertInfo INS_VAR(_parsable); \
asssertExecInsert(INS_VAR)

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
