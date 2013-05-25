#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "StringUtils.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"
#include "Helpers.h"
#include "ConfigManager.h"
#include "DBAgentTest.h"

namespace testDBAgentSQLite3 {

static string g_dbPath = DBAgentSQLite3::getDBPath(DEFAULT_DB_DOMAIN_ID);

void _assertExistRecord(uint64_t id, int age, const char *name, double height)
{
	// check if the columns is inserted

	// INFO: We use the trick that unsigned interger is stored as
	// signed interger. So large integers (MSB bit is one) are recognized
	// as negative intergers. So we use PRId64 in the following statement.
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from %s where id=%"PRId64 "\"",
	               g_dbPath.c_str(), TABLE_NAME_TEST, id);
	string output = executeCommand(cmd);
	
	const ColumnDef &columnDefHeight =
	   COLUMN_DEF_TEST[IDX_TEST_TABLE_HEIGHT];
	
	// Here we also use PRId64 (not PRIu64) with the same
	// reason of the above comment.
	string fmt = StringUtils::sprintf("%%"PRId64"|%%d|%%s|%%.%dlf\n",
	                                  columnDefHeight.decFracLength);
	string expectedOut = StringUtils::sprintf(fmt.c_str(),
	                                          id, age, name, height);
	cppcut_assert_equal(expectedOut, output);
}
#define assertExistRecord(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertExistRecord(ID,AGE,NAME,HEIGHT))

class DBAgentCheckerSQLite3 : public DBAgentChecker {
public:
	// overriden virtual methods
	virtual void assertTable(const DBAgentTableCreationArg &arg)
	{
		// check if the table has been created successfully
		cut_assert_exist_path(g_dbPath.c_str());
		string cmd = StringUtils::sprintf("sqlite3 %s \".table\"",
		                                  g_dbPath.c_str());
		string output = executeCommand(cmd);
		assertExist(TABLE_NAME_TEST, output);

		//
		// check table definition
		//
		cmd = StringUtils::sprintf(
		  "sqlite3 %s \"select * from sqlite_master\"",
		  g_dbPath.c_str());
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
		cppcut_assert_equal(string(TABLE_NAME_TEST), outVec[idx++]);
		cppcut_assert_equal(string(TABLE_NAME_TEST), outVec[idx++]);

		// rootpage (we ignore it)
		idx++;

		// table schema
		expected = StringUtils::sprintf("CREATE TABLE %s(",
		                                TABLE_NAME_TEST);
		for (size_t i = 0; i < arg.numColumns; i++) {
			const ColumnDef &columnDef = arg.columnDefs[i];

			// name
			expected += columnDef.columnName;
			expected += " ";

			// type 
			switch(columnDef.type) {
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
			case NUM_SQL_COLUMN_TYPES:
			default:
				cut_fail("Unknwon type: %d\n", columnDef.type);
			}

			// key 
			if (columnDef.keyType == SQL_KEY_PRI)
				expected += "PRIMARY KEY";

			if (i < arg.numColumns - 1)
				expected += ",";
		}
		expected += ")\n";
		cppcut_assert_equal(expected, outVec[idx++]);

	}

	virtual void assertInsert(const DBAgentInsertArg &arg,
	                          uint64_t id, int age, const char *name,
	                          double height)
	{
		assertExistRecord(id, age, name,height);
	}

	virtual void assertUpdate(uint64_t id, int age,
	                          const char *name, double height,
	                          const string &condition)
	{
		assertExistRecord(id, age, name,height);
	}

	virtual void getIDStringVector(const ColumnDef &columnDefId,
	                               vector<string> &actualIds)
	{
		cut_assert_exist_path(g_dbPath.c_str());
		string cmd =
		  StringUtils::sprintf(
		    "sqlite3 %s \"SELECT %s FROM %s ORDER BY %s ASC\"",
		    g_dbPath.c_str(), columnDefId.columnName,
		    TABLE_NAME_TEST, columnDefId.columnName);
		string output = executeCommand(cmd);
		StringUtils::split(actualIds, output, '\n');
	}
};

static DBAgentCheckerSQLite3 dbAgentChecker;

static void deleteDB(void)
{
	unlink(g_dbPath.c_str());
	cut_assert_not_exist_path(g_dbPath.c_str());
}

#define DEFINE_DBAGENT_WITH_INIT(DB_NAME, OBJ_NAME) \
string _path = getFixturesDir() + DB_NAME; \
DBAgentSQLite3::defineDBPath(DEFAULT_DB_DOMAIN_ID, _path); \
DBAgentSQLite3 OBJ_NAME; \

void setup(void)
{
	deleteDB();
	DBAgentSQLite3::defineDBPath(DEFAULT_DB_DOMAIN_ID, g_dbPath);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbPathByEnv(void)
{
	const char *dbDir = getenv(ConfigManager::ASURA_DB_DIR_ENV_VAR_NAME);
	if (!dbDir) {
		cut_omit("Not set: %s", 
	                 ConfigManager::ASURA_DB_DIR_ENV_VAR_NAME);
	}
	cppcut_assert_equal(0, strncmp(dbDir, g_dbPath.c_str(), strlen(dbDir)));
}

void test_testIsTableExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(true, dbAgent.isTableExisting("foo"));
}

void test_testIsTableExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	cppcut_assert_equal(false, dbAgent.isTableExisting("NotExistTable"));
}

void test_testIsRecordExisting(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectTrueCondition = "id=1";
	cppcut_assert_equal
	  (true, dbAgent.isRecordExisting("foo", expectTrueCondition));
}

void test_testIsRecordExistingNotIncluded(void)
{
	DEFINE_DBAGENT_WITH_INIT("FooTable.db", dbAgent);
	string expectFalseCondition = "id=100";
	cppcut_assert_equal
	  (false, dbAgent.isRecordExisting("foo", expectFalseCondition));
}

void test_createTable(void)
{
	DBAgentSQLite3 dbAgent;
	dbAgentTestCreateTable(dbAgent, dbAgentChecker);
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

} // testDBAgentSQLite3

