#include <cppcutter.h>
#include "DBAgentMySQL.h"
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
		string cmd = "mysql -D ";
		cmd += TEST_DB_NAME;
		cmd += " -B -e \"desc ";
		cmd += TABLE_NAME_TEST;
		cmd += "\"";
		string result = executeCommand(cmd);

		// check the number of obtained lines
		size_t linesIdx = 0;
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		cppcut_assert_equal(NUM_COLUMNS_TEST+1, lines.size());

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
		for (size_t  i = 0; i < NUM_COLUMNS_TEST; i++) {
			size_t idx = 0;
			StringVector words;
			const bool doMerge = false;
			StringUtils::split(words, lines[linesIdx++], '\t',
			                   doMerge);
			cppcut_assert_equal(numHeaders, words.size());
			const ColumnDef &columnDef = COLUMN_DEF_TEST[i];

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
			expected = "";
			cppcut_assert_equal(expected, words[idx++]);
		}
	}

	virtual void assertInsert(const DBAgentInsertArg &arg,
	                          uint64_t id, int age, const char *name,
	                          double height)
	{
		// get the table information with mysql command.
		string cmd = "mysql -D ";
		cmd += TEST_DB_NAME;
		cmd += " -B -e \"select * from ";
		cmd += TABLE_NAME_TEST;
		cmd += "\"";
		string result = executeCommand(cmd);

		// check the number of obtained lines
		size_t linesIdx = 0;
		StringVector lines;
		StringUtils::split(lines, result, '\n');
		size_t numExpectedLines = 2;
		cppcut_assert_equal(numExpectedLines, lines.size());

		// assert header output
		StringVector actualHeaders;
		StringUtils::split(actualHeaders, lines[linesIdx++], '\t');
		cppcut_assert_equal(arg.numColumns, actualHeaders.size());
		for (size_t i = 0; i < arg.numColumns; i++) {
			const ColumnDef &columnDefs = arg.columnDefs[i];
			cppcut_assert_equal(string(columnDefs.columnName),
			                    actualHeaders[i]);
		}

		// value
		size_t idx = 0;
		string expected;
		StringVector words;
		StringUtils::split(words, lines[linesIdx++], '\t');
		cppcut_assert_equal(arg.numColumns, words.size());

		// id
		expected = StringUtils::sprintf("%"PRIu64, id);
		cppcut_assert_equal(expected, words[idx++]);

		// age
		expected = StringUtils::sprintf("%d", age);
		cppcut_assert_equal(expected, words[idx++]);

		// name
		expected = name;
		cppcut_assert_equal(expected, words[idx++]);

		// height
		const ColumnDef &columnDef = arg.columnDefs[idx];
		string fmt = StringUtils::sprintf("%%.%zdlf",
		               columnDef.decFracLength);
		expected = StringUtils::sprintf(fmt.c_str(), height);
		cppcut_assert_equal(expected, words[idx++]);
	}
};

static bool dropTestDB(MYSQL *mysql)
{
	string query = "DROP DATABASE ";
	query += TEST_DB_NAME;
	return mysql_query(mysql, query.c_str()) == 0;
}

static bool makeTestDB(MYSQL *mysql)
{
	string query = "CREATE DATABASE ";
	query += TEST_DB_NAME;
	return mysql_query(mysql, query.c_str()) == 0;
}

static void makeTestDBIfNeeded(bool recreate = false)
{
	// make a connection
	const char *host = NULL; // localhost is used.
	const char *user = NULL; // current user name is used.
	const char *passwd = NULL; // passwd is not checked.
	const char *db = NULL;
	unsigned int port = 0; // default port is used.
	const char *unixSocket = NULL;
	unsigned long clientFlag = 0;
	MYSQL mysql;
	mysql_init(&mysql);
	MYSQL *succeeded = mysql_real_connect(&mysql, host, user, passwd,
	                                      db, port, unixSocket, clientFlag);
	if (!succeeded) {
		cut_fail("Failed to connect to MySQL: %s: %s\n",
		          db, mysql_error(&mysql));
	}

	// try to find the test database
	MYSQL_RES *result = mysql_list_dbs(&mysql, TEST_DB_NAME);
	if (!result) {
		mysql_close(&mysql);
		cut_fail("Failed to list table: %s: %s\n",
		          db, mysql_error(&mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	size_t lengthTestDBName = strlen(TEST_DB_NAME);
	while ((row = mysql_fetch_row(result))) {
		if (strncmp(TEST_DB_NAME, row[0], lengthTestDBName) == 0) {
			found = true;
			break;
		}
	}
	mysql_free_result(result);

	// make DB if needed
	bool noError = false;
	if (!found) {
		if (!makeTestDB(&mysql))
			goto exit;
	} else if (recreate) {
		if (!dropTestDB(&mysql))
			goto exit;
		if (!makeTestDB(&mysql))
			goto exit;
	}
	noError = true;

exit:
	// close the connection
	string errmsg;
	if (!noError)
		errmsg = mysql_error(&mysql);
	mysql_close(&mysql);
	cppcut_assert_equal(true, noError,
	   cut_message("Failed to query: %s", errmsg.c_str()));
}

static void _createGlobalDBAgent(void)
{
	try {
		g_dbAgent = new DBAgentMySQL(TEST_DB_NAME);
	} catch (const exception &e) {
		cut_fail("%s", e.what());
	}
}
#define createGlobalDBAgent() cut_trace(_createGlobalDBAgent())

void _assertInsert(uint64_t id, int age, const char *name, double height)
{
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_TEST;
	arg.numColumns = NUM_COLUMNS_TEST;
	arg.columnDefs = COLUMN_DEF_TEST;
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Uint64, id);
	row->ADD_NEW_ITEM(Int, age);
	row->ADD_NEW_ITEM(String, name);
	row->ADD_NEW_ITEM(Double, height);
	arg.row = row;
	g_dbAgent->insert(arg);

	DBAgentCheckerMySQL checker;
	checker.assertInsert(arg, id, age, name, height);
}
#define assertInsert(ID,AGE,NAME,HEIGHT) \
cut_trace(_assertInsert(ID,AGE,NAME,HEIGHT));

void setup(void)
{
	bool recreate = true;
	makeTestDBIfNeeded(recreate);
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

void test_createTable(void)
{
	createGlobalDBAgent();
	createTable<DBAgentMySQL, DBAgentCheckerMySQL>(*g_dbAgent);
}

void test_insert(void)
{
	// create table
	test_createTable();

	// insert a row
	const uint64_t ID = 1;
	const int AGE = 14;
	const char *NAME = "rei";
	const double HEIGHT = 158.2;
	assertInsert(ID, AGE, NAME, HEIGHT);
}

} // testDBAgentMySQL

