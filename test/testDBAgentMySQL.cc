#include <cppcutter.h>
#include "DBAgentMySQL.h"

namespace testDBAgentMySQL {

static const char *TEST_DB_NAME = "test_db_agent_mysql";

DBAgentMySQL *g_dbAgent = NULL;

static void makeTestDBIfNeeded(void)
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
	int error = 0;
	if (!found) {
		string query = "CREATE DATABASE ";
		query += TEST_DB_NAME;
		error = mysql_query(&mysql, query.c_str());
	}

	// close the connection
	string errmsg;
	if (error)
		errmsg = mysql_error(&mysql);
	mysql_close(&mysql);
	cppcut_assert_equal(0, error,
	   cut_message("Failed to query: %s", errmsg.c_str()));
}

static void _createGlobalDBAgent(void)
{
	makeTestDBIfNeeded();
	try {
		g_dbAgent = new DBAgentMySQL(TEST_DB_NAME);
	} catch (const exception &e) {
		cut_fail("%s", e.what());
	}
}
#define createGlobalDBAgent() cut_trace(_createGlobalDBAgent())

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

} // testDBAgentMySQL

