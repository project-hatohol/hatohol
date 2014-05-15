/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include "Hatohol.h"
#include "DBAgentTest.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testDBAgentMySQL {

static const char *TEST_DB_NAME = "test_db_agent_mysql";

DBAgentMySQL *g_dbAgent = NULL;

static string getEngine(const string &dbName, const string &tableName)
{
	string sql =
	  "SELECT ENGINE FROM information_schema.TABLES where TABLE_NAME='";
	sql += tableName;
	sql += "';";
	string output = execMySQL(dbName, sql);
	StringVector words;
	StringUtils::split(words, output, '\n');
	cppcut_assert_equal((size_t)1, words.size());
	return words[0];
}

class DBAgentCheckerMySQL : public DBAgentChecker {
public:
	// overriden virtual methods
	virtual void assertTable(const DBAgent::TableProfile &tableProfile)
	{
		// get the table information with mysql command.
		string sql = "desc ";
		sql += tableProfile.name;
		string result = execMySQL(TEST_DB_NAME, sql, true);

		// check the number of obtained lines
		size_t linesIdx = 0;
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		cppcut_assert_equal(tableProfile.numColumns+1, lines.size());

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
		for (size_t  i = 0; i < tableProfile.numColumns; i++) {
			size_t idx = 0;
			StringVector words;
			const bool doMerge = false;
			StringUtils::split(words, lines[linesIdx++], '\t',
			                   doMerge);
			cppcut_assert_equal(numHeaders, words.size());
			const ColumnDef &columnDef = tableProfile.columnDefs[i];

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
			case SQL_KEY_IDX:
				// These index is not crated until
				// fixupIndexes() is called.
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
	                                  const ColumnDef *columnDefs,
	                                  const set<size_t> *nullIndexes)
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
		                             lines[linesIdx++], '\t',
		                             nullIndexes, "NULL");
	}

	virtual void assertMakeCreateIndexStatement(
	  const std::string sql, const DBAgent::IndexDef &indexDef) // override
	{
		const string expect = makeExpectedCreateIndexStatement(
		  *indexDef.tableProfile, indexDef);
		cppcut_assert_equal(expect, sql);
	}

	virtual void assertMakeDropIndexStatement(
	  const std::string sql,
	  const std::string &name, const std::string &tableName) // override
	{
		string expect = "DROP INDEX ";
		expect += name;
		expect += " ON ";
		expect += tableName;
		cppcut_assert_equal(expect, sql);
	}

	virtual void assertFixupIndexes(
	  const DBAgent::TableProfile &tableProfile,
	  const DBAgent::IndexDef *indexDefArray) // override
	{
		const bool isMemoryEngine =
		  (getEngine(TEST_DB_NAME, tableProfile.name) == "MEMORY");
		const string expectedHeader =
		  "Table\tNon_unique\tKey_name\tSeq_in_index\t"
		  "Column_name\tCollation\tCardinality\tSub_part\tPacked\t"
		  "Null\tIndex_type\tComment\tIndex_comment";
		vector<string> expectLines;
		expectLines.push_back(expectedHeader);

		// from ColumnDef
		for (size_t i = 0; i < tableProfile.numColumns; i++) {
			const ColumnDef &columnDef = tableProfile.columnDefs[i];
			bool isUnique = false;
			string keyName = columnDef.columnName;
			switch (columnDef.keyType) {
			case SQL_KEY_PRI:
				keyName = "PRIMARY";
			case SQL_KEY_UNI:
				isUnique = true;
			case SQL_KEY_IDX:
				break;
			default:
				continue;
			}

			string line = makeExpectedIndexStruct(
			  tableProfile.name, isUnique, keyName,
			  columnDef.columnName, columnDef.canBeNull,
			  isMemoryEngine);
			expectLines.push_back(line);
		}

		// from IndexDefArray
		const DBAgent::IndexDef *indexDef = indexDefArray;
		for (; indexDef->name != NULL; indexDef++) {
			const int *columnIdxPtr = indexDef->columnIndexes;
			int seq = 1;
			while (*columnIdxPtr != DBAgent::IndexDef::END) {
				const ColumnDef &columnDef =
				  tableProfile.columnDefs[*columnIdxPtr++];

				string line = makeExpectedIndexStruct(
				  tableProfile.name, indexDef->isUnique,
				  indexDef->name, columnDef.columnName,
				  columnDef.canBeNull, isMemoryEngine, seq,
				  *columnIdxPtr == DBAgent::IndexDef::END);
				expectLines.push_back(line);

				seq++;
			}
		}

		// check the index definitions
		string sql = "SHOW INDEX FROM ";
		sql += tableProfile.name;
		string output = execMySQL(TEST_DB_NAME, sql, true);
		StringVector outLines;
		StringUtils::split(outLines, output, '\n');

		cppcut_assert_equal(expectLines.size(), outLines.size());
		LinesComparator comp;
		for (size_t i = 0; i < expectLines.size(); i++)
			comp.add(expectLines[i], outLines[i]);
		comp.assert(false);
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

protected:
	string makeExpectedCreateIndexStatement(
	  const DBAgent::TableProfile &tableProfile,
	  const DBAgent::IndexDef &indexDef)
	{
		string expect = "CREATE ";
		if (indexDef.isUnique)
			expect += "UNIQUE ";
		expect += "INDEX ";
		expect += indexDef.name;
		expect += " ON ";
		expect += tableProfile.name;
		expect += " (";
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

	string makeExpectedIndexStruct(
	  const string &tableName, const bool &isUnique, const string &keyName,
	  const string &columnName, const bool &canBeNull,
	  const bool &isMemoryEngine, const size_t &seqInIndex = 1,
	  const bool &isLastColumn = false)
	{
		string s;

		// Table
		s += tableName;
		s += "\t";

		// nonUnique
		if (isUnique)
			s += "0\t";
		else
			s += "1\t";

		// Key_name
		s += keyName;
		s += "\t";

		// Seq_in_index
		s += StringUtils::sprintf("%zd\t", seqInIndex);

		// Column_name
		s += columnName;
		s += "\t";

		s += isMemoryEngine ? "NULL\t" : "A\t"; // Collation

		// Cardinality
		if (!isMemoryEngine || keyName == "PRIMARY" ||
		    columnName == keyName || isLastColumn)
			s += "0\t";
		else
			s += "NULL\t";

		s += "NULL\t"; // Sub_part
		s += "NULL\t"; // Packed
		s += canBeNull ? "YES\t" : "\t"; // Null
		// Index_type
		s += isMemoryEngine ? "HASH\t" : "BTREE\t";
		s += "\t";      // Comment
		// The following component is degenerated
		// s += "\t";      // Index_comment

		return s;
	}

};

static DBAgentCheckerMySQL dbAgentChecker;

class TestDBAgentMySQL : public DBAgentMySQL {
public:
	TestDBAgentMySQL(void)
	: DBAgentMySQL(TEST_DB_NAME)
	{
	}

	void callExecSql(const string &statement)
	{
		return execSql(statement);
	}
};

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

void cut_setup(void)
{
	bool recreate = true;
	hatoholInit();
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
}

void cut_teardown(void)
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

void test_getIndexes(void)
{
	const string tableName = "footable";
	TestDBAgentMySQL dbAgent;
	string stmt = "CREATE TABLE ";
	stmt += tableName;
	stmt += " (id INT (11) PRIMARY KEY, a INT (11) UNIQUE, "
	        "b INT (11), c INT (11))";
	dbAgent.callExecSql(stmt);
	vector<DBAgentMySQL::IndexStruct> indexStructVect;
	dbAgent.getIndexes(indexStructVect, tableName);

	cppcut_assert_equal((size_t)2, indexStructVect.size());
	
	// id
	DBAgentMySQL::IndexStruct *idxStruct = &indexStructVect[0];
	cppcut_assert_equal(tableName,         idxStruct->table);
	cppcut_assert_equal(false,             idxStruct->nonUnique);
	cppcut_assert_equal(string("PRIMARY"), idxStruct->keyName);
	cppcut_assert_equal((size_t)1,         idxStruct->seqInIndex);
	cppcut_assert_equal(string("id"),      idxStruct->columnName);

	// a
	idxStruct = &indexStructVect[1];
	cppcut_assert_equal(tableName,         idxStruct->table);
	cppcut_assert_equal(false,             idxStruct->nonUnique);
	cppcut_assert_equal(string("a"),       idxStruct->keyName);
	cppcut_assert_equal((size_t)1,         idxStruct->seqInIndex);
	cppcut_assert_equal(string("a"),       idxStruct->columnName);
}

//
// The following tests are using DBAgentTest functions.
//
void test_execSql(void)
{
	createGlobalDBAgent();
	dbAgentTestExecSql(*g_dbAgent, dbAgentChecker);
}

void test_createTable(void)
{
	createGlobalDBAgent();
	dbAgentTestCreateTable(*g_dbAgent, dbAgentChecker);
}

void data_makeCreateIndexStatement(void)
{
	dbAgentDataMakeCreateIndexStatement();
}

void test_makeCreateIndexStatement(gconstpointer data)
{
	createGlobalDBAgent();
	dbAgentTestMakeCreateIndexStatement(*g_dbAgent, dbAgentChecker, data);
}

void test_makeDropIndexStatement(void)
{
	createGlobalDBAgent();
	dbAgentTestMakeDropIndexStatement(*g_dbAgent, dbAgentChecker);
}

void test_fixupIndexes(void)
{
	createGlobalDBAgent();
	dbAgentTestFixupIndexes(*g_dbAgent, dbAgentChecker);
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

void test_insertNull(void)
{
	createGlobalDBAgent();
	dbAgentTestInsertNull(*g_dbAgent, dbAgentChecker);
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

void test_selectExWithOrderBy(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectHeightOrder(*g_dbAgent);
}

void test_selectExWithOrderByLimit(void)
{
	createGlobalDBAgent();
	dbAgentTestSelectHeightOrder(*g_dbAgent, 1);
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

void test_addColumns(void)
{
	createGlobalDBAgent();
	dbAgentTestAddColumns(*g_dbAgent, dbAgentChecker);
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

void test_updateIfExistElseInsert(void)
{
	createGlobalDBAgent();
	dbAgentUpdateIfExistEleseInsert(*g_dbAgent, dbAgentChecker);
}

void test_getLastInsertId(void)
{
	createGlobalDBAgent();
	dbAgentGetLastInsertId(*g_dbAgent, dbAgentChecker);
}

void test_getNumberOfAffectedRows(void)
{
	createGlobalDBAgent();
	dbAgentGetNumberOfAffectedRows(*g_dbAgent, dbAgentChecker);
}

} // testDBAgentMySQL

