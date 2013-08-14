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

#include <cppcutter.h>
#include "DBAgentMySQL.h"
#include "SQLUtils.h"
#include "DBAgentTest.h"
#include "Helpers.h"

namespace testDBAgentMySQL {

static const char *TEST_DB_NAME = "test_db_agent_mysql";

DBAgentMySQL *g_dbAgent = NULL;

class DBAgentCheckerMySQL : public DBAgentChecker {
public:
	// overriden virtual methods
	virtual void assertTable(const DBAgentTableCreationArg &arg)
	{
		// get the table information with mysql command.
		string sql = "desc ";
		sql += arg.tableName;
		string result = execMySQL(TEST_DB_NAME, sql, true);

		// check the number of obtained lines
		size_t linesIdx = 0;
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		cppcut_assert_equal(arg.numColumns+1, lines.size());

		// assert header output
		const char *headers[] = {
		  "Field", "Type", "Null", "Key", "Default", "Extra"};
		const size_t numHeaders =
			sizeof(headers) / sizeof(const char *);
		StringVector actualHeaders;
		StringUtils::split(actualHeaders, lines[linesIdx++], '\t');
		cppcut_assert_equal(numHeaders, actualHeaders.size());
		for (size_t i = 0; i < numHeaders; i++) {
			cppcut_assert_equal(string(headers[i]),
			                    actualHeaders[i]);
		}

		// assert tables
		string expected;
		for (size_t  i = 0; i < arg.numColumns; i++) {
			size_t idx = 0;
			StringVector words;
			const bool doMerge = false;
			StringUtils::split(words, lines[linesIdx++], '\t',
			                   doMerge);
			cppcut_assert_equal(numHeaders, words.size());
			const ColumnDef &columnDef = arg.columnDefs[i];

			// column name
			expected = columnDef.columnName;
			cppcut_assert_equal(expected, words[idx++]);

			// type
			switch (columnDef.type) {
			case SQL_COLUMN_TYPE_INT:
				expected = StringUtils::sprintf("int(%zd)",
				             columnDef.columnLength);
				break;             
			case SQL_COLUMN_TYPE_BIGUINT:
				expected = StringUtils::sprintf(
				             "bigint(%zd) unsigned",
				             columnDef.columnLength);
				break;             
			case SQL_COLUMN_TYPE_VARCHAR:
				expected = StringUtils::sprintf("varchar(%zd)",
				             columnDef.columnLength);
				break;
			case SQL_COLUMN_TYPE_CHAR:
				expected = StringUtils::sprintf("char(%zd)",
				             columnDef.columnLength);
				break;
			case SQL_COLUMN_TYPE_TEXT:
				expected = "text";
				break;
			case SQL_COLUMN_TYPE_DOUBLE:
				expected = StringUtils::sprintf(
				             "double(%zd,%zd)",
				             columnDef.columnLength,
				             columnDef.decFracLength);
				break;
			case SQL_COLUMN_TYPE_DATETIME:
				expected = "datetime";
				break;
			case NUM_SQL_COLUMN_TYPES:
			default:
				cut_fail("Unknwon type: %d\n", columnDef.type);
			}
			cppcut_assert_equal(expected, words[idx++]);

			// nullable
			expected = columnDef.canBeNull ? "YES" : "NO";
			cppcut_assert_equal(expected, words[idx++]);

			// key
			switch (columnDef.keyType) {
			case SQL_KEY_PRI:
				expected = "PRI";
				break;
			case SQL_KEY_UNI:
				expected = "UNI";
				break;
			case SQL_KEY_MUL:
				expected = "MUL";
				break;
			case SQL_KEY_NONE:
				expected = "";
				break;
			default:
				cut_fail("Unknwon key type: %d\n",
				         columnDef.keyType);
			}
			cppcut_assert_equal(expected, words[idx++]);

			// default
			expected = columnDef.defaultValue ?: "NULL";
			cppcut_assert_equal(expected, words[idx++]);

			// extra
			if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC)
				expected = "auto_increment";
			else
				expected = "";
			cppcut_assert_equal(expected, words[idx++]);
		}
	}

	virtual void assertExistingRecord(uint64_t id, int age,
	                                  const char *name, double height,
	                                  int datetime,
	                                  size_t numColumns,
	                                  const ColumnDef *columnDefs)
	{
		// get the table information with mysql command.
		string sql =
		   StringUtils::sprintf("select * from %s where id=%"PRIu64,
		                        TABLE_NAME_TEST, id);
		string result = execMySQL(TEST_DB_NAME, sql, true);

		// check the number of obtained lines
		size_t linesIdx = 0;
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		size_t numExpectedLines = 2;
		cppcut_assert_equal(numExpectedLines, lines.size());

		// assert header output
		StringVector actualHeaders;
		StringUtils::split(actualHeaders, lines[linesIdx++], '\t');
		cppcut_assert_equal(numColumns, actualHeaders.size());
		for (size_t i = 0; i < numColumns; i++) {
			const ColumnDef &columnDef = columnDefs[i];
			cppcut_assert_equal(string(columnDef.columnName),
			                    actualHeaders[i]);
		}

		assertExistingRecordEachWord(id, age, name, height, datetime,
		                             numColumns, columnDefs,
		                             lines[linesIdx++], '\t');
	}

	virtual void getIDStringVector(const ColumnDef &columnDefId,
	                               vector<string> &actualIds)
	{
		string sql =
		  StringUtils::sprintf(
		    "SELECT %s FROM %s ORDER BY %s ASC",
		    columnDefId.columnName, columnDefId.tableName, 
		    columnDefId.columnName);
		string output = execMySQL(TEST_DB_NAME, sql, false);
		StringUtils::split(actualIds, output, '\n');
	}
};

static DBAgentCheckerMySQL dbAgentChecker;

static void _createGlobalDBAgent(void)
{
	try {
		g_dbAgent = new DBAgentMySQL(TEST_DB_NAME);
	} catch (const exception &e) {
		cut_fail("%s", e.what());
	}
}
#define createGlobalDBAgent() cut_trace(_createGlobalDBAgent())

void _assertIsRecordExisting(bool skipInsert)
{
	static const char *tableName = "foo";
	static const int  id = 1;
	string statement = StringUtils::sprintf(
	    "CREATE TABLE %s(id INT(11))", tableName);
	execMySQL(TEST_DB_NAME, statement);

	if (!skipInsert) {
		statement = StringUtils::sprintf(
		    "INSERT INTO %s VALUES (%d)", tableName, id);
		execMySQL(TEST_DB_NAME, statement);
	}

	string condition = StringUtils::sprintf("id=%d", id);
	createGlobalDBAgent();
	cppcut_assert_equal
	  (!skipInsert, g_dbAgent->isRecordExisting(tableName, condition));
}
#define assertIsRecordExisting(SI) cut_trace(_assertIsRecordExisting(SI))

void setup(void)
{
	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
}

void teardown(void)
{
	if (g_dbAgent) {
		delete g_dbAgent;
		g_dbAgent = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_create(void)
{
	createGlobalDBAgent();
}

void test_isRecordExisting(void)
{
	bool skipInsert = false;
	assertIsRecordExisting(skipInsert);
}

void test_isRecordExistingNotIncluded(void)
{
	bool skipInsert = true;
	assertIsRecordExisting(skipInsert);
}

void test_createTable(void)
{
	createGlobalDBAgent();
	dbAgentTestCreateTable(*g_dbAgent, dbAgentChecker);
}

void test_insert(void)
{
	createGlobalDBAgent();
	dbAgentTestInsert(*g_dbAgent, dbAgentChecker);
}

void test_insertUint64_0x7fffffffffffffff(void)
{
	createGlobalDBAgent();
	dbAgentTestInsertUint64(*g_dbAgent, dbAgentChecker, 0x7fffffffffffffff);
}

void test_insertUint64_0x8000000000000000(void)
{
	createGlobalDBAgent();
	dbAgentTestInsertUint64(*g_dbAgent, dbAgentChecker, 0x8000000000000000);
}

void test_insertUint64_0xffffffffffffffff(void)
{
	createGlobalDBAgent();
	dbAgentTestInsertUint64(*g_dbAgent, dbAgentChecker, 0xffffffffffffffff);
}

void test_update(void)
{
	createGlobalDBAgent();
	dbAgentTestUpdate(*g_dbAgent, dbAgentChecker);
}

void test_updateCondition(void)
{
	createGlobalDBAgent();
	dbAgentTestUpdateCondition(*g_dbAgent, dbAgentChecker);
}

void test_select(void)
{
	createGlobalDBAgent();
	dbAgentTestSelect(*g_dbAgent);
}

void test_selectEx(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectEx(*g_dbAgent);
}

void test_selectExWithCond(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectExWithCond(*g_dbAgent);
}

void test_selectExWithCondAllColumns(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectExWithCondAllColumns(*g_dbAgent);
}

void test_selectExWithOrderByLimitTwo(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectHeightOrder(*g_dbAgent, 2);
}

void test_selectExWithOrderByLimitOffset(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectHeightOrder(*g_dbAgent, 2, 1);
}

void test_selectExWithOrderByLimitOffsetOverData(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectHeightOrder(*g_dbAgent, 1, NUM_TEST_DATA, 0);
}

void test_delete(void)
{
	createGlobalDBAgent();
	dbAgentTestDelete(*g_dbAgent, dbAgentChecker);
}

void test_isTableExisting(void)
{
	createGlobalDBAgent();
	dbAgentTestIsTableExisting(*g_dbAgent, dbAgentChecker);
}

void test_autoIncrement(void)
{
	createGlobalDBAgent();
	dbAgentTestAutoIncrement(*g_dbAgent, dbAgentChecker);
}

void test_autoIncrementWithDel(void)
{
	createGlobalDBAgent();
	dbAgentTestAutoIncrementWithDel(*g_dbAgent, dbAgentChecker);
}

} // testDBAgentMySQL

