/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>
#include "StringUtils.h"
#include "DBAgentSQLite3.h"
#include "HatoholException.h"
#include "Helpers.h"
#include "ConfigManager.h"
#include "DBAgentTest.h"
using namespace std;
using namespace mlpl;

namespace testDBAgentSQLite3 {

class DBAgentCheckerSQLite3 : public DBAgentChecker {
public:
	virtual void assertTable(
	  DBAgent &dbAgent, const DBAgent::TableProfile &tableProfile) override
	{
		const string dbPath = cast(dbAgent).getDBPath();

		// check if the table has been created successfully
		cut_assert_exist_path(dbPath.c_str());
		string cmd = StringUtils::sprintf("sqlite3 %s \".table\"",
		                                  dbPath.c_str());
		string output = executeCommand(cmd);
		assertExist(tableProfile.name, output);

		//
		// check table definition
		//
		cmd = StringUtils::sprintf(
		  "sqlite3 %s \"select * from sqlite_master "
		  "where type='table' and tbl_name='%s'\"",
		  dbPath.c_str(), tableProfile.name);
		output = executeCommand(cmd);
		StringVector outVec;
		StringUtils::split(outVec, output, '|');
		const size_t expectedNumOut = 5;
		cppcut_assert_equal(expectedNumOut, outVec.size());

		// fixed 'table'
		size_t idx = 0;
		string expected = "table";
		cppcut_assert_equal(expected, outVec[idx++]);

		// name and table name
		cppcut_assert_equal(string(tableProfile.name), outVec[idx++]);
		cppcut_assert_equal(string(tableProfile.name), outVec[idx++]);

		// rootpage (we ignore it)
		idx++;

		// table schema
		if (StringUtils::hasPrefix(outVec[idx], "CREATE TABLE \"")) {
			// need double quote around table name for this case
			expected = StringUtils::sprintf("CREATE TABLE \"%s\"(",
							tableProfile.name);
		} else {
			expected = StringUtils::sprintf("CREATE TABLE %s(",
							tableProfile.name);
		}
		for (size_t i = 0; i < tableProfile.numColumns; i++) {
			const ColumnDef &columnDef = tableProfile.columnDefs[i];

			// name
			expected += columnDef.columnName;
			expected += " ";

			// type 
			switch (columnDef.type) {
			case SQL_COLUMN_TYPE_INT:
			case SQL_COLUMN_TYPE_BIGUINT:
				expected += "INTEGER ";
				break;             
			case SQL_COLUMN_TYPE_VARCHAR:
			case SQL_COLUMN_TYPE_CHAR:
			case SQL_COLUMN_TYPE_TEXT:
				expected += "TEXT ";
				break;
			case SQL_COLUMN_TYPE_DOUBLE:
				expected += "REAL ";
				break;
			case SQL_COLUMN_TYPE_DATETIME:
				expected += " ";
				break;
			case NUM_SQL_COLUMN_TYPES:
			default:
				cut_fail("Unknwon type: %d\n", columnDef.type);
			}

			// key 
			if (columnDef.keyType == SQL_KEY_PRI) {
				expected += "PRIMARY KEY";
				if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC)
					expected += " AUTOINCREMENT";
			}

			if (i < tableProfile.numColumns - 1)
				expected += ",";
		}
		expected += ")\n";
		cppcut_assert_equal(expected, outVec[idx++]);
	}

	virtual void assertMakeCreateIndexStatement(
	  const std::string sql, const DBAgent::TableProfile &tableProfile,
	  const DBAgent::IndexDef &indexDef) override
	{
		const string expect = makeExpectedCreateIndexStatement(
		  tableProfile, indexDef);
		cppcut_assert_equal(expect, sql);
	}

	virtual void assertMakeDropIndexStatement(
	  const std::string sql,
	  const std::string &name, const std::string &tableName) override
	{
		string expect = "DROP INDEX ";
		expect += name;
		cppcut_assert_equal(expect, sql);
	}

	virtual void assertFixupIndexes(
	  DBAgent &dbAgent, const DBAgent::TableProfile &tableProfile) override
	{
		const string dbPath = cast(dbAgent).getDBPath();
		vector<string> expectLines;

		// from ColumnDef
		for (size_t i = 0; i < tableProfile.numColumns; i++) {
			const ColumnDef &columnDef = tableProfile.columnDefs[i];
			bool isUnique = false;
			switch (columnDef.keyType) {
			case SQL_KEY_UNI:
				isUnique = true;
			case SQL_KEY_IDX:
				break;
			default:
				continue;
			}

			const int columnIndexes[] = {
			  (int)i, DBAgent::IndexDef::END};
			const DBAgent::IndexDef indexDef = {
			  columnDef.columnName, columnIndexes, isUnique};
			string line = makeExpectedCreateIndexStatement(
			                tableProfile, indexDef);
			expectLines.push_back(line);
		}

		// from IndexDefArray
		const DBAgent::IndexDef *indexDef = tableProfile.indexDefArray;
		for (; indexDef->name != NULL; indexDef++) {
			string line = makeExpectedCreateIndexStatement(
			                tableProfile, *indexDef);
			expectLines.push_back(line);
		}

		// check the index definitions
		cut_assert_exist_path(dbPath.c_str());
		string cmd = StringUtils::sprintf(
		  "sqlite3 %s \"select sql from "
		  "sqlite_master where type='index' and tbl_name='%s'\"",
		  dbPath.c_str(), tableProfile.name);
		string output = executeCommand(cmd);
		StringVector outLines;
		StringUtils::split(outLines, output, '\n');

		cppcut_assert_equal(expectLines.size(), outLines.size());
		LinesComparator comp;
		for (size_t i = 0; i < expectLines.size(); i++)
			comp.add(expectLines[i], outLines[i]);
		comp.assert(false);
	}

	virtual void assertExistingRecord(DBAgent &dbAgent,
	                                  uint64_t id, int age,
	                                  const char *name, double height,
	                                  int datetime,
	                                  size_t numColumns,
	                                  const ColumnDef *columnDefs,
	                                  const set<size_t> *nullIndexes) override
	{
		const string dbPath = cast(dbAgent).getDBPath();

		// INFO: We use the trick that unsigned interger is stored as
		// signed interger. So large integers (MSB bit is one) are
		// recognized as negative intergers. So we use PRId64
		// in the following statement.
		string cmd =
		  StringUtils::sprintf(
		    "sqlite3 %s \"select * from %s where id=%" PRId64 "\"",
	            dbPath.c_str(), TABLE_NAME_TEST, id);
		string result = executeCommand(cmd);

		// check the number of obtained lines
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		size_t numExpectedLines = 1;
		cppcut_assert_equal(numExpectedLines, lines.size());
		assertExistingRecordEachWord
		  (id, age, name, height, datetime,
		   numColumns, columnDefs, lines[0], '|', nullIndexes, "",
		   "%" PRId64);
	}

	virtual void getIDStringVector(
	  DBAgent &dbAgent,
	  const DBAgent::TableProfile &tableProfile,
	  const size_t &columnIdIdx, vector<string> &actualIds) override
	{
		const string dbPath = cast(dbAgent).getDBPath();
		const ColumnDef &columnDefId =
			tableProfile.columnDefs[columnIdIdx];
		cut_assert_exist_path(dbPath.c_str());
		string cmd =
		  StringUtils::sprintf(
		    "sqlite3 %s \"SELECT %s FROM %s ORDER BY %s ASC\"",
		    dbPath.c_str(), columnDefId.columnName,
		    TABLE_NAME_TEST, columnDefId.columnName);
		string output = executeCommand(cmd);
		StringUtils::split(actualIds, output, '\n');
	}

protected:
	string makeExpectedCreateIndexStatement(
	  const DBAgent::TableProfile &tableProfile,
	  const DBAgent::IndexDef &indexDef)
	{
		string indexName = StringUtils::sprintf("i_%s_%s",
		                     tableProfile.name, indexDef.name);
		string expect = "CREATE ";
		if (indexDef.isUnique)
			expect += "UNIQUE ";
		expect += "INDEX ";
		expect += indexName;
		expect += " ON ";
		expect += tableProfile.name;
		expect += "(";
		for (size_t i = 0; true; i++) {
			const int columnIdx = indexDef.columnIndexes[i];
			if (columnIdx == DBAgent::IndexDef::END)
				break;
			if (i >= 1)
				expect += ",";
			expect += tableProfile.columnDefs[columnIdx].columnName;
		}
		expect += ")";
		return expect;
	}

	DBAgentSQLite3 &cast(DBAgent &dbAgent)
	{
		DBAgentSQLite3 &casted =
		  dynamic_cast<DBAgentSQLite3 &>(dbAgent);
		cppcut_assert_not_null(&casted);
		return casted;
	}
};

static DBAgentCheckerSQLite3 dbAgentChecker;

class TestDBAgentSQLite3 : public DBAgentSQLite3 {
public:
	void callExecSql(const string &statement)
	{
		execSql("%s", statement.c_str());
	}
};

static void deleteDB(void)
{
	DBAgentSQLite3 dbAgent;
	unlink(dbAgent.getDBPath().c_str());
	cut_assert_not_exist_path(dbAgent.getDBPath().c_str());
}

#define DEFINE_DBAGENT_WITH_INIT(DB_NAME, OBJ_NAME) \
  DBAgentSQLite3 OBJ_NAME(DB_NAME, getFixturesDir())

void cut_setup(void)
{
	deleteDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbPathByEnv(void)
{
	const char *dbDir = getenv(ConfigManager::HATOHOL_DB_DIR_ENV_VAR_NAME);
	if (!dbDir) {
		cut_omit("Not set: %s", 
	                 ConfigManager::HATOHOL_DB_DIR_ENV_VAR_NAME);
	}
	DBAgentSQLite3 dbAgent;
	cut_assert_equal_substring(dbDir, dbAgent.getDBPath().c_str(),
	                           strlen(dbDir));
}

void test_createdWithSpecifiedDbPath(void)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	const string &dbDirectory = configMgr->getDatabaseDirectory();
	string dbName = "test-database-name.01234567";
	string expectPath = StringUtils::sprintf(
	  "%s/%s.db", dbDirectory.c_str(), dbName.c_str());
	unlink(expectPath.c_str());
	cut_assert_not_exist_path(expectPath.c_str());

	DBAgentSQLite3 sqlite3(dbName.c_str());
	cut_assert_exist_path(expectPath.c_str());
}

void test_getIndexes(void)
{
	const string tableName = "footable";
	TestDBAgentSQLite3 dbAgent;
	string stmt = "CREATE TABLE ";
	stmt += tableName;
	stmt += " (id INTEGER PRIMARY KEY, a INTEGER, b INTEGER, c INTEGER)";
	dbAgent.callExecSql(stmt);

	string stmtA = StringUtils::sprintf("CREATE INDEX i_a ON %s (a)",
	                                    tableName.c_str());
	dbAgent.callExecSql(stmtA);

	string stmtBC = StringUtils::sprintf("CREATE INDEX i_bc ON %s (b,c)",
	                                     tableName.c_str());
	dbAgent.callExecSql(stmtBC);

	vector<DBAgentSQLite3::IndexStruct> indexStructVect;
	dbAgent.getIndexes(indexStructVect, tableName);

	cppcut_assert_equal((size_t)2, indexStructVect.size());

	// i_a
	DBAgentSQLite3::IndexStruct *idxStruct = &indexStructVect[0];
	cppcut_assert_equal(string("i_a"), idxStruct->name);
	cppcut_assert_equal(tableName,     idxStruct->tableName);
	cppcut_assert_equal(stmtA,         idxStruct->sql);

	// i_bc
	idxStruct = &indexStructVect[1];
	cppcut_assert_equal(string("i_bc"), idxStruct->name);
	cppcut_assert_equal(tableName,      idxStruct->tableName);
	cppcut_assert_equal(stmtBC,         idxStruct->sql);
}

void test_testIsTableExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable", dbAgent);
	cppcut_assert_equal(true, dbAgent.isTableExisting("foo"));
}

void test_testIsTableExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable", dbAgent);
	cppcut_assert_equal(false, dbAgent.isTableExisting("NotExistTable"));
}

void test_testIsRecordExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable", dbAgent);
	string expectTrueCondition = "id=1";
	cppcut_assert_equal
	  (true, dbAgent.isRecordExisting("foo", expectTrueCondition));
}

void test_testIsRecordExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable", dbAgent);
	string expectFalseCondition = "id=100";
	cppcut_assert_equal
	  (false, dbAgent.isRecordExisting("foo", expectFalseCondition));
}

//
// The following tests are using DBAgentTest functions.
//
void test_execSql(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestExecSql(dbAgent, dbAgentChecker);
}

void test_createTable(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestCreateTable(dbAgent, dbAgentChecker);
}

void data_makeCreateIndexStatement(void)
{
	dbAgentDataMakeCreateIndexStatement();
}

void test_makeCreateIndexStatement(gconstpointer data)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestMakeCreateIndexStatement(dbAgent, dbAgentChecker, data);
}

void test_makeDropIndexStatement(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestMakeDropIndexStatement(dbAgent, dbAgentChecker);
}

void test_fixupIndexes(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestFixupIndexes(dbAgent, dbAgentChecker);
}

void test_insert(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestInsert(dbAgent, dbAgentChecker);
}

void test_insertUint64_0x7fffffffffffffff(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestInsertUint64(dbAgent, dbAgentChecker, 0x7fffffffffffffff);
}

void test_insertUint64_0x8000000000000000(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestInsertUint64(dbAgent, dbAgentChecker, 0x8000000000000000);
}

void test_insertUint64_0xffffffffffffffff(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestInsertUint64(dbAgent, dbAgentChecker, 0xffffffffffffffff);
}

void test_insertNull(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestInsertNull(dbAgent, dbAgentChecker);
}

void test_upsert(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestUpsert(dbAgent, dbAgentChecker);
}

void test_upsertWithPrimaryKeyAutoInc(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestUpsertWithPrimaryKeyAutoInc(dbAgent, dbAgentChecker);
}

void test_update(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestUpdate(dbAgent, dbAgentChecker);
}

void test_updateCondition(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestUpdateCondition(dbAgent, dbAgentChecker);
}

void test_select(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelect(dbAgent);
}

void test_selectEx(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectEx(dbAgent);
}

void test_selectExWithCond(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectExWithCond(dbAgent);
}

void test_selectExWithCondAllColumns(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectExWithCondAllColumns(dbAgent);
}

void test_selectExWithOrderBy(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectHeightOrder(dbAgent);
}

void test_selectExWithOrderByLimit(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectHeightOrder(dbAgent, 1);
}

void test_selectExWithOrderByLimitTwo(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectHeightOrder(dbAgent, 2);
}

void test_selectExWithOrderByLimitOffset(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectHeightOrder(dbAgent, 2, 1);
}

void test_selectExWithOrderByLimitOffsetOverData(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestSelectHeightOrder(dbAgent, 1, NUM_TEST_DATA, 0);
}

void test_delete(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestDelete(dbAgent, dbAgentChecker);
}

void test_renameTable(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestRenameTable(dbAgent, dbAgentChecker);
}

void test_isTableExisting(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestIsTableExisting(dbAgent, dbAgentChecker);
}

void test_autoIncrement(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestAutoIncrement(dbAgent, dbAgentChecker);
}

void test_autoIncrementWithDel(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestAutoIncrementWithDel(dbAgent, dbAgentChecker);
}

void test_updateIfExistElseInsert(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentUpdateIfExistEleseInsert(dbAgent, dbAgentChecker);
}

void test_getLastInsertId(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentGetLastInsertId(dbAgent, dbAgentChecker);
}

void test_getNumberOfAffectedRows(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentGetNumberOfAffectedRows(dbAgent, dbAgentChecker);
}

} // testDBAgentSQLite3

