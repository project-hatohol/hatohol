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
#include <unistd.h>
#include "Helpers.h"
#include "DBClientZabbix.h"
#include "DBClientConfig.h"
#include "DBClientAction.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "SQLUtils.h"

void _assertStringVector(StringVector &expected, StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}

void _assertStringVectorVA(StringVector &actual, ...)
{
	StringVector expectedVect;
	va_list valist;
	va_start(valist, actual);
	while (true) {
		const char *expected = va_arg(valist, const char *);
		if (!expected)
			break;
		expectedVect.push_back(expected);
	}
	va_end(valist);
	assertStringVector(expectedVect, actual);
}

void _assertExist(const string &target, const string &words)
{
	StringVector splitWords;
	string wordsStripCR = StringUtils::eraseChars(words, "\n");
	StringUtils::split(splitWords, wordsStripCR, ' ');
	for (size_t i = 0; i < splitWords.size(); i++) {
		if (splitWords[i] == target)
			return;
	}
	cut_fail("Not found: %s in %s", target.c_str(), words.c_str());
}

string executeCommand(const string &commandLine)
{
	gboolean ret;
	gchar *standardOutput = NULL;
	gchar *standardError = NULL;
	gint exitStatus;
	GError *error = NULL;
	string errorStr;
	string stdoutStr, stderrStr;

	ret = g_spawn_command_line_sync(commandLine.c_str(),
	                                &standardOutput, &standardError,
	                                &exitStatus, &error);
	if (standardOutput) {
		stdoutStr = standardOutput;
		g_free(standardOutput);
	}
	if (standardError) {
		stderrStr = standardError;
		g_free(standardError);
	}
	if (!ret)
		goto err;
	if (exitStatus != 0)
		goto err;
	return stdoutStr;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("ret: %d, exit status: %d\n<<stdout>>\n%s\n<<stderr>>\n%s\n"
	         "<<error->message>>\n%s",
	         ret, exitStatus, stdoutStr.c_str(), stderrStr.c_str(), 
	         errorStr.c_str());
	return "";
}

string getFixturesDir(void)
{
	char *cwd = get_current_dir_name();
	string dir = cwd;
	free(cwd);
	dir += G_DIR_SEPARATOR;
	dir += "fixtures";
	dir += G_DIR_SEPARATOR;
	return dir;
}

bool isVerboseMode(void)
{
	static bool checked = false;
	static bool verboseMode = false;
	if (!checked) {
		char *env = getenv("VERBOSE");
		if (env && env == string("1"))
			verboseMode = true;
		checked = true;
	}
	return verboseMode;
}

string getDBClientDBPath(DBDomainId domainId)
{
	// TODO: remove the direct call of DBAgentSQLite3's API.
	return DBAgentSQLite3::getDBPath(domainId);
}

string deleteDBClientDB(DBDomainId domainId)
{
	string dbPath = getDBClientDBPath(domainId);
	unlink(dbPath.c_str());
	cut_assert_not_exist_path(dbPath.c_str());
	return dbPath;
}

string deleteDBClientZabbixDB(int serverId)
{
	DBDomainId domainId = DBClientZabbix::getDBDomainId(serverId);
	return  deleteDBClientDB(domainId);
}

string execSqlite3ForDBClient(DBDomainId domainId, const string &statement)
{
	string dbPath = getDBClientDBPath(domainId);
	cut_assert_exist_path(dbPath.c_str());
	string commandLine =
	  StringUtils::sprintf("sqlite3 %s \"%s\"",
	                       dbPath.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

string execSqlite3ForDBClientZabbix(int serverId, const string &statement)
{
	DBDomainId domainId = DBClientZabbix::getDBDomainId(serverId);
	return execSqlite3ForDBClient(domainId, statement);
}

string execMySQL(const string &dbName, const string &statement, bool showHeader)
{
	const char *headerOpt = showHeader ? "" : "-N";
	string commandLine =
	  StringUtils::sprintf("mysql -B %s -D %s -e \"%s\"",
	                       headerOpt, dbName.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

void _assertCurrDatetime(const string &datetime)
{
	const int MAX_ALLOWD_CURR_TIME_ERROR = 5;
	ItemDataPtr item = SQLUtils::createFromString(datetime,
	                                              SQL_COLUMN_TYPE_DATETIME);
	int clock = ItemDataUtils::getInt(item);
	int currClock = (int)time(NULL);
	cppcut_assert_equal(
	  true, currClock >= clock,
	  cut_message("currClock: %d, clock: %d", currClock, clock));
	cppcut_assert_equal(
	  true, currClock - clock < MAX_ALLOWD_CURR_TIME_ERROR,
	  cut_message( "currClock: %d, clock: %d", currClock, clock));
}

static void assertDBContentForComponets(const string &expect,
                                        const string &actual)
{
	StringVector wordsExpect;
	StringVector wordsActual;
	bool doMerge = false;
	StringUtils::split(wordsExpect, expect, '|', doMerge);
	StringUtils::split(wordsActual, actual, '|', doMerge);
	cppcut_assert_equal(wordsExpect.size(), wordsActual.size(),
	                    cut_message("<<expect>>: %s\n<<actual>>: %s\n",
	                                expect.c_str(), actual.c_str()));
	for (size_t i = 0; i < wordsExpect.size(); i++) {
		if (wordsExpect[i] == DBCONTENT_MAGIC_CURR_DATETIME)
			assertCurrDatetime(wordsActual[i]);
		else
			cppcut_assert_equal(wordsExpect[i], wordsActual[i]);
	}
}

void _assertDBContent(DBAgent *dbAgent, const string &statement,
                      const string &expect)
{
	string output;
	const type_info& tid = typeid(*dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL *dbMySQL = dynamic_cast<DBAgentMySQL *>(dbAgent);
		output = execMySQL(dbMySQL->getDBName(), statement);
		output = StringUtils::replace(output, "\t", "|");
	}
	else
		cut_fail("Unknown type_info");

	const string &actual = output;
	StringVector linesExpect;
	StringVector linesActual;
	StringUtils::split(linesExpect, expect, '\n');
	StringUtils::split(linesActual, actual, '\n');
	cppcut_assert_equal(linesExpect.size(), linesActual.size(),
	  cut_message("<<expect>>\n%s\n\n<<actual>>%s\n",
	              expect.c_str(), actual.c_str()));
	for (size_t i = 0; i < linesExpect.size(); i++) {
		cut_trace(
		  assertDBContentForComponets(linesExpect[i], linesActual[i]));
	}
}

void _assertCreateTable(DBAgent *dbAgent, const string &tableName)
{
	string output;
	const type_info &tid = typeid(*dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL *dba = dynamic_cast<DBAgentMySQL *>(dbAgent);
		string dbName = dba->getDBName();
		const string statement = StringUtils::sprintf(
		  "show tables like '%s'", tableName.c_str());
		output = execMySQL(dbName, statement);
	} else if (tid == typeid(DBAgentSQLite3)) {
		DBDomainId domainId = dbAgent->getDBDomainId();
		string dbPath = DBAgentSQLite3::getDBPath(domainId);
		string command = "sqlite3 " + dbPath + " \".table\"";
		output = executeCommand(command);
	} else {
		cut_fail("Unkown type: %s", DEMANGLED_TYPE_NAME(*dbAgent));
	}

	assertExist(tableName, output);
}

static bool makeTestDB(MYSQL *mysql, const string &dbName)
{
	string query = "CREATE DATABASE ";
	query += dbName;
	return mysql_query(mysql, query.c_str()) == 0;
}

static bool dropTestDB(MYSQL *mysql, const string &dbName)
{
	string query = "DROP DATABASE ";
	query += dbName;
	return mysql_query(mysql, query.c_str()) == 0;
}

void makeTestMySQLDBIfNeeded(const string &dbName, bool recreate)
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
	MYSQL_RES *result = mysql_list_dbs(&mysql, dbName.c_str());
	if (!result) {
		mysql_close(&mysql);
		cut_fail("Failed to list table: %s: %s\n",
		          db, mysql_error(&mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	size_t lengthTestDBName = dbName.size();
	while ((row = mysql_fetch_row(result))) {
		if (dbName.compare(0, lengthTestDBName, row[0]) == 0) {
			found = true;
			break;
		}
	}
	mysql_free_result(result);

	// make DB if needed
	bool noError = false;
	if (!found) {
		if (!makeTestDB(&mysql, dbName))
			goto exit;
	} else if (recreate) {
		if (!dropTestDB(&mysql, dbName))
			goto exit;
		if (!makeTestDB(&mysql, dbName))
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

void setupTestDBServers(void)
{
	static const char *TEST_DB_NAME = "test_servers_in_helper";
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClientConfig::setDefaultDBParams(TEST_DB_NAME,
	                                   TEST_DB_USER, TEST_DB_PASSWORD);

	static bool dbServerReady = false;
	if (!dbServerReady) {
		bool recreate = true;
		makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);

		DBClientConfig dbConfig;
		for (size_t i = 0; i < NumServerInfo; i++)
			dbConfig.addTargetServer(&serverInfo[i]);
		dbServerReady = true;
	}
}

void setupTestDBAction(bool dbRecreate)
{
	static const char *TEST_DB_NAME = "test_action";
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClientAction::setDefaultDBParams(TEST_DB_NAME,
	                                   TEST_DB_USER, TEST_DB_PASSWORD);

	makeTestMySQLDBIfNeeded(TEST_DB_NAME, dbRecreate);
}
