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
#include <gcutter.h>
#include <unistd.h>
#include <typeinfo>
#include "Helpers.h"
#include "DBClientZabbix.h"
#include "DBClientConfig.h"
#include "DBClientAction.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "CacheServiceDBClient.h"
#include "SQLUtils.h"
using namespace std;
using namespace mlpl;

void _assertStringVector(const StringVector &expected,
                         const StringVector &actual)
{
	cppcut_assert_equal(expected.size(), actual.size());
	for (size_t i = 0; i < expected.size(); i++)
		cppcut_assert_equal(expected[i], actual[i]);
}

void _assertStringVectorVA(const StringVector &actual, ...)
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

struct SpawnSyncContext {
	bool running;
	bool hasError;
	GIOChannel *ioch;
	guint dataCbId;
	guint errCbId;
	string &msg;

	SpawnSyncContext(int fd, string &_msg)
	: running(true),
	  ioch(NULL),
	  msg(_msg)
	{
		ioch = g_io_channel_unix_new(fd);
		cppcut_assert_not_null(ioch);

		GError *error = NULL;
		g_io_channel_set_encoding(ioch, NULL, &error);
		gcut_assert_error(error);

		// non blocking
		GIOFlags flags = G_IO_FLAG_NONBLOCK;
		g_io_channel_set_flags(ioch, flags, &error);
		gcut_assert_error(error);

		// data callback
		GIOCondition cond = G_IO_IN;
		dataCbId = g_io_add_watch(ioch, cond, cbData, this);

		// error callback
		cond = (GIOCondition)(G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL);
		errCbId = g_io_add_watch(ioch, cond, cbErr, this);
	}

	virtual ~SpawnSyncContext()
	{
		Utils::removeEventSourceIfNeeded(dataCbId);
		Utils::removeEventSourceIfNeeded(errCbId);
		g_io_channel_unref(ioch);
	}

	static gboolean cbData(GIOChannel *source, GIOCondition condition,
	                gpointer data)
	{
		SpawnSyncContext *obj = static_cast<SpawnSyncContext *>(data);
		GError *error = NULL;
		gsize bytesRead;
		gsize bufSize = 0x1000;
		gchar buf[bufSize];
		GIOStatus stat =
		  g_io_channel_read_chars(source, buf, bufSize-1, &bytesRead,
		                          &error);
		if (stat != G_IO_STATUS_NORMAL) {
			MLPL_ERR("Failed to call g_io_channel_read_chars: %s\n",
			         error ? error->message : "unknown");
			if (error)
				g_error_free(error);
			obj->running = false;
			obj->dataCbId = INVALID_EVENT_ID;
			return FALSE;
		}
		buf[bytesRead] = '\0';
		obj->msg += buf;
		return TRUE;
	}

	static gboolean cbErr(GIOChannel *source, GIOCondition condition,
	                gpointer data)
	{
		SpawnSyncContext *obj = static_cast<SpawnSyncContext *>(data);
		obj->running = false;
		obj->errCbId = INVALID_EVENT_ID;
		return FALSE;
	}
};

gboolean spawnSync(const string &commandLine, string &stdOut, string &stdErr,
                   GError **error)
{
	gchar **argv = NULL;
	if (!g_shell_parse_argv(commandLine.c_str(), NULL, &argv, error))
		return FALSE;

	const gchar *workingDir = NULL;
	gchar **envp = NULL;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GPid childPid;
	gint stdOutFd;
	gint stdErrFd;
	gboolean ret =
	  g_spawn_async_with_pipes(workingDir, argv, envp, flags,
	                           childSetup, userData, &childPid,
	                           NULL /* stdInFd */, &stdOutFd, &stdErrFd,
	                           error);
	g_strfreev(argv);
	if (!ret)
		return FALSE;

	SpawnSyncContext ctxStd(stdOutFd, stdOut);
	SpawnSyncContext ctxErr(stdErrFd, stdErr);

	while (ctxStd.running && ctxErr.running)
		g_main_context_iteration(NULL, TRUE);
	return TRUE;
}

string executeCommand(const string &commandLine)
{
	gboolean ret;
	string errorStr;
	string stdoutStr, stderrStr;
	GError *error = NULL;

	// g_spawn_sync families cannot be used for htohol tests.
	// Because they update the signal handler for SIGCHLD. As a result,
	// Hatohol's SIGCHLD signal handler is ignored.
	ret = spawnSync(commandLine, stdoutStr, stderrStr, &error);
	if (!ret)
		goto err;
	return stdoutStr;

err:
	if (error) {
		errorStr = error->message;
		g_error_free(error);
	}
	cut_fail("<<stdout>>\n%s\n<<stderr>>\n%s\n<<error->message>>\n%s",
	         stdoutStr.c_str(), stderrStr.c_str(), errorStr.c_str());
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

void deleteFileAndCheck(const string &path)
{
	unlink(path.c_str());
	cut_assert_not_exist_path(path.c_str());
}

string getDBPathForDBClientZabbix(const ServerIdType serverId)
{
	struct callgate : public DBClientZabbix, public DBAgentSQLite3 {
		static string getDBPath(const ServerIdType serverId) {
			string dbName = getDBName(serverId);
			string dbPath = makeDBPathFromName(dbName);
			return dbPath;
		}
	};
	return callgate::getDBPath(serverId);
}

string getDBPathForDBClientHatohol(void)
{
	struct callgate : public DBClientHatohol, public DBAgentSQLite3 {
		static string getDBPath(void) {
			return makeDBPathFromName(DEFAULT_DB_NAME);
		}
	};
	return callgate::getDBPath();
}

string deleteDBClientHatoholDB(void)
{
	string dbPath = getDBPathForDBClientHatohol();
	deleteFileAndCheck(dbPath);
	return dbPath;
}

string deleteDBClientZabbixDB(const ServerIdType serverId)
{
	string dbPath = getDBPathForDBClientZabbix(serverId);
	deleteFileAndCheck(dbPath);
	return dbPath;
}

string execSqlite3ForDBClient(const string &dbPath, const string &statement)
{
	cut_assert_exist_path(dbPath.c_str());
	string commandLine =
	  StringUtils::sprintf("sqlite3 %s \"%s\"",
	                       dbPath.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

string execSqlite3ForDBClientHatohol(const string &statement)
{
	string dbPath = getDBPathForDBClientHatohol();
	return execSqlite3ForDBClient(dbPath, statement);
}

string execSqlite3ForDBClientZabbix(const ServerIdType serverId,
                                    const string &statement)
{
	string dbPath = getDBPathForDBClientZabbix(serverId);
	return execSqlite3ForDBClient(dbPath, statement);
}

string execMySQL(const string &dbName, const string &statement, bool showHeader)
{
	const char *headerOpt = showHeader ? "" : "-N";
	//
	// We use a --raw option that disable the escape such as \ -> \\.
	//
	string commandLine =
	  StringUtils::sprintf("mysql --raw -B %s -D %s -e \"%s\"",
	                       headerOpt, dbName.c_str(), statement.c_str());
	string result = executeCommand(commandLine);
	return result;
}

void _assertDatetime(int expectedClock, int actualClock)
{
	if (expectedClock == CURR_DATETIME)
		assertCurrDatetime(actualClock);
	else
		cppcut_assert_equal(expectedClock, actualClock);
}

void _assertCurrDatetime(int clock)
{
	const int MAX_ALLOWD_CURR_TIME_ERROR = 5;
	int currClock = (int)time(NULL);
	cppcut_assert_equal(
	  true, currClock >= clock,
	  cut_message("currClock: %d, clock: %d", currClock, clock));
	cppcut_assert_equal(
	  true, currClock - clock < MAX_ALLOWD_CURR_TIME_ERROR,
	  cut_message( "currClock: %d, clock: %d", currClock, clock));
}

void _assertCurrDatetime(const string &datetime)
{
	ItemDataPtr item = SQLUtils::createFromString(datetime.c_str(),
	                                              SQL_COLUMN_TYPE_DATETIME);
	int clock = *item;
	assertCurrDatetime(clock);
}

string getExpectedNullNotation(DBAgent &dbAgent)
{
	const type_info &tid = typeid(dbAgent);
	if (tid == typeid(DBAgentMySQL))
		return "NULL";
	else if (tid == typeid(DBAgentSQLite3))
		return "";
	else
		cut_fail("Unknown type: %s", DEMANGLED_TYPE_NAME(dbAgent));
	return "";
}

string makeServerInfoOutput(const MonitoringServerInfo &serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s|%d|%d|%d|%s|%s|%s\n",
	                        serverInfo.id, serverInfo.type,
	                        serverInfo.hostName.c_str(),
	                        serverInfo.ipAddress.c_str(),
	                        serverInfo.nickname.c_str(),
	                        serverInfo.port,
	                        serverInfo.pollingIntervalSec,
	                        serverInfo.retryIntervalSec,
	                        serverInfo.userName.c_str(),
	                        serverInfo.password.c_str(),
	                        serverInfo.dbName.c_str());
	return expectedOut;
}

std::string makeUserRoleInfoOutput(const UserRoleInfo &userRoleInfo)
{
	return StringUtils::sprintf(
		 "%"FMT_USER_ROLE_ID"|%s|%"FMT_OPPRVLG"\n",
		 userRoleInfo.id, userRoleInfo.name.c_str(),
		 userRoleInfo.flags);
}

string makeEventOutput(const EventInfo &eventInfo)
{
	string output =
	  mlpl::StringUtils::sprintf(
	    "%"PRIu32"|%"PRIu64"|%ld|%ld|%d|%u|%"PRIu64"|%s|%s\n",
	    eventInfo.serverId, eventInfo.id,
	    eventInfo.time.tv_sec, eventInfo.time.tv_nsec,
	    eventInfo.status, eventInfo.severity,
	    eventInfo.hostId,
	    eventInfo.hostName.c_str(),
	    eventInfo.brief.c_str());
	return output;
}

static void assertDBContentForComponets(const string &expect,
                                        const string &actual,
                                        DBAgent *dbAgent)
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
		if (wordsExpect[i] == DBCONTENT_MAGIC_CURR_DATETIME) {
			assertCurrDatetime(wordsActual[i]);
		} else if(wordsExpect[i] == DBCONTENT_MAGIC_NULL) {
			cppcut_assert_equal(getExpectedNullNotation(*dbAgent),
			                    wordsActual[i]);
		} else {
			cppcut_assert_equal(wordsExpect[i], wordsActual[i],
			  cut_message(
			    "line no: %zd\n<<expect>>: %s\n<<actual>>: %s\n",
	                    i, expect.c_str(), actual.c_str()));
		}
	}
}

void _assertDBContent(DBAgent *dbAgent, const string &statement,
                      const string &expect)
{
	const string actual = execSQL(dbAgent, statement);
	StringVector linesExpect;
	StringVector linesActual;
	StringUtils::split(linesExpect, expect, '\n');
	StringUtils::split(linesActual, actual, '\n');
	cppcut_assert_equal(linesExpect.size(), linesActual.size(),
	  cut_message("<<expect>>\n%s\n\n<<actual>>\n%s\n",
	              expect.c_str(), actual.c_str()));
	for (size_t i = 0; i < linesExpect.size(); i++) {
		cut_trace(
		  assertDBContentForComponets(linesExpect[i], linesActual[i],
		                              dbAgent));
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

void _assertTimeIsNow(const SmartTime &smtime, double allowedError)
{
	SmartTime diff(SmartTime::INIT_CURR_TIME);
	diff -= smtime;
	cppcut_assert_equal(
	  true, diff.getAsSec() < allowedError,
	  cut_message("time: %e, diff: %e, allowedError: %e",
	              smtime.getAsSec(), diff.getAsSec(), allowedError));
}

void _assertHatoholError(const HatoholErrorCode &code,
                         const HatoholError &err)
{
	cppcut_assert_equal(code, err.getCode());
}

void _assertServersInDB(const ServerIdSet &excludeServerIdSet)
{
	string statement = "select * from servers ";
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		ServerIdType serverId = i + 1;
		MonitoringServerInfo serverInfo = testServerInfo[i];
		serverInfo.id = serverId;
		ServerIdSetIterator it = excludeServerIdSet.find(serverId);
		if (it != excludeServerIdSet.end())
			continue;
		expect += makeServerInfoOutput(serverInfo);
	}
	CacheServiceDBClient cache;
	assertDBContent(cache.getConfig()->getDBAgent(), statement, expect);
}

void _assertUsersInDB(const UserIdSet &excludeUserIdSet)
{
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USERS;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		UserIdType userId = i + 1;
		if (excludeUserIdSet.find(userId) != excludeUserIdSet.end())
			continue;
		const UserInfo &userInfo = testUserInfo[i];
		expect += StringUtils::sprintf(
		  "%"FMT_USER_ID"|%s|%s|%"FMT_OPPRVLG"\n",
		  userId, userInfo.name.c_str(),
		  Utils::sha256(userInfo.password).c_str(),
		  userInfo.flags);
	}
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
}

void _assertAccessInfoInDB(const AccessInfoIdSet &excludeAccessInfoIdSet)
{
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_ACCESS_LIST;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfoIdType id = i + 1;
		if (excludeAccessInfoIdSet.find(id) != excludeAccessInfoIdSet.end())
			continue;
		const AccessInfo &accessInfo = testAccessInfo[i];
		expect += StringUtils::sprintf(
		  "%"FMT_ACCESS_INFO_ID"|%"FMT_USER_ID"|%d|%"PRIu64"\n",
		  id, accessInfo.userId, accessInfo.serverId, accessInfo.hostgroupId);
	}
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
}

void _assertUserRoleInfoInDB(UserRoleInfo &userRoleInfo) 
{
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USER_ROLES,
			     userRoleInfo.id);
	string expect = makeUserRoleInfoOutput(userRoleInfo);
	DBClientUser dbUser;
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}
#define assertUserRoleInfoInDB(I) cut_trace(_assertUserRoleInfoInDB(I))

void _assertUserRolesInDB(const UserRoleIdSet &excludeUserRoleIdSet)
{
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USER_ROLES;
	statement += " ORDER BY id ASC";
	string expect;
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleIdType userRoleId = i + 1;
		UserRoleIdSetIterator endIt = excludeUserRoleIdSet.end();
		if (excludeUserRoleIdSet.find(userRoleId) != endIt)
			continue;
		UserRoleInfo userRoleInfo = testUserRoleInfo[i];
		userRoleInfo.id = userRoleId;
		expect += makeUserRoleInfoOutput(userRoleInfo);
	}
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
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

void loadTestDBServer(void)
{
	DBClientConfig dbConfig;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestServerInfo; i++)
		dbConfig.addTargetServer(&testServerInfo[i], privilege);
}

void setupTestDBConfig(bool dbRecreate, bool loadTestData)
{
	static const char *TEST_DB_NAME = "test_config";
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClient::setDefaultDBParams(DB_DOMAIN_ID_CONFIG, TEST_DB_NAME,
	                             TEST_DB_USER, TEST_DB_PASSWORD);
	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
	if (loadTestData)
		loadTestDBServer();
}

void setupTestDBAction(bool dbRecreate, bool loadTestData)
{
	static const char *TEST_DB_NAME = "test_action";
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClient::setDefaultDBParams(DB_DOMAIN_ID_ACTION, TEST_DB_NAME,
	                             TEST_DB_USER, TEST_DB_PASSWORD);
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, dbRecreate);
	if (loadTestData)
		loadTestDBAction();
}

void loadTestDBUser(void)
{
	DBClientUser dbUser;
	HatoholError err;
	OperationPrivilege opePrivilege(ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		err = dbUser.addUserInfo(testUserInfo[i], opePrivilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBAccessList(void)
{
	DBClientUser dbUser;
	HatoholError err;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		err = dbUser.addAccessInfo(testAccessInfo[i], privilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBUserRole(void)
{
	DBClientUser dbUser;
	HatoholError err;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		err = dbUser.addUserRoleInfo(testUserRoleInfo[i], privilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void setupTestDBUser(bool dbRecreate, bool loadTestData)
{
	static const char *TEST_DB_NAME = "test_db_user";
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClient::setDefaultDBParams(DB_DOMAIN_ID_USERS, TEST_DB_NAME,
	                             TEST_DB_USER, TEST_DB_PASSWORD);
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, dbRecreate);
	if (loadTestData) {
		loadTestDBUser();
		loadTestDBAccessList();
		loadTestDBUserRole();
	}
}

void loadTestDBArmPlugin(void)
{
	DBClientConfig dbConfig;
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		HatoholError err =
		  dbConfig.saveArmPluginInfo(testArmPluginInfo[i]);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBAction(void)
{
	DBClientAction dbAction;
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (size_t i = 0; i < NumTestActionDef; i++)
		dbAction.addAction(testActionDef[i], privilege);
}

string
replaceForUtf8(const string &str, const string &target, const string &newChar)
{
	string ret;
	const gchar *orig = str.c_str();
	const gchar *curr = orig;
	gchar *next;
	while (true)
	{
		next = g_utf8_find_next_char(curr, NULL);
		if (curr == next)
			break;
		size_t len = next - curr;
		string currChar(curr, 0, len);
		if (currChar == target)
			ret += newChar;
		else
			ret += currChar;
		curr = next;
	}
	return ret;
}

string execSQL(DBAgent *dbAgent, const string &statement, bool showHeader)
{

	string output;
	const type_info& tid = typeid(*dbAgent);
	if (tid == typeid(DBAgentMySQL)) {
		DBAgentMySQL *dbMySQL = dynamic_cast<DBAgentMySQL *>(dbAgent);
		output = execMySQL(dbMySQL->getDBName(), statement, showHeader);
		output = replaceForUtf8(output, "\t", "|");
	} else if (tid == typeid(DBAgentSQLite3)) {
		DBAgentSQLite3 *dbSQLite3 =
		  dynamic_cast<DBAgentSQLite3 *>(dbAgent);
		cppcut_assert_equal(false, showHeader,
		  cut_message("Not implemented yet\n"));
		output = execSqlite3ForDBClient(
		           dbSQLite3->getDBPath(), statement);
	} else {
		cut_fail("Unknown type_info");
	}
	return output;
}

string joinStringVector(const StringVector &strVect, const string &pad,
                        bool isPaddingTail)
{
	string output;
	size_t numElem = strVect.size();
	size_t lastIndex = numElem - 1;
	for (size_t i = 0; i < numElem; i++) {
		output += strVect[i];
		if (i < lastIndex)
			output += pad;
		else if (i == lastIndex && isPaddingTail)
			output += pad;
	}
	return output;
}

void crash(void)
{
	// Accessing the invalid address (NULL) finally causes SIGILL
	// when the compiler is clang, although we expect SIGSEGV.
	// So we send the signal directly when using clang.
	// Now hatohol is built with clang as a trial.
#ifdef __clang__
	kill(getpid(), SIGSEGV);
	while (true)
		sleep(1);
#else
	char *p = NULL;
	*p = 'a';
#endif
}

void prepareTestDataForFilterForDataOfDefunctServers(void)
{
	gcut_add_datum("Not filter data of defunct servers",
		       "filterDataOfDefunctServers", G_TYPE_BOOLEAN, FALSE,
		       NULL);
	gcut_add_datum("Filter data of defunct servers.",
		       "filterDataOfDefunctServers", G_TYPE_BOOLEAN, TRUE,
		       NULL);
}

void fixupForFilteringDefunctServer(
  gconstpointer data, string &expected, HostResourceQueryOption &option,
  const string &tableName)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	if (filterForDataOfDefunctSv)
		insertValidServerCond(expected, option, tableName);
}

void insertValidServerCond(
  string &condition, const HostResourceQueryOption &opt,
  const string &tableName)
{
	struct MyOption : public HostResourceQueryOption {
	public:
		MyOption(const HostResourceQueryOption &opt)
		: HostResourceQueryOption(opt)
		{
		}

		string makeServerCond(const string &tableName)
		{
			const ServerIdSet &serverIdSet =
			  getDataQueryContext().getValidServerIdSet();
			string cond = makeConditionServer(
			                serverIdSet, getServerIdColumnName());
			return cond;
		}

		void insertCond(string &svCond, const string &condition)
		{
			addCondition(svCond, condition);
		}
	};

	MyOption myOpt(opt);
	string svCond = myOpt.makeServerCond(tableName);
	myOpt.insertCond(svCond, condition);
	condition = svCond;
}

string makeDoubleFloatFormat(const ColumnDef &columnDef)
{
	return StringUtils::sprintf("%%.%zdlf", columnDef.decFracLength);
}

void initServerInfo(MonitoringServerInfo &serverInfo)
{
	serverInfo.id = 12345;
	serverInfo.pollingIntervalSec = 1;
}

static GMainContext *g_acquiredContext = NULL;
void _acquireDefaultContext(void)
{
	GMainContext *context = g_main_context_default();
	cppcut_assert_equal((gboolean)TRUE, g_main_context_acquire(context));
	g_acquiredContext = context;
}

void releaseDefaultContext(void)
{
	if (g_acquiredContext) {
		g_main_context_release(g_acquiredContext); 
		g_acquiredContext = NULL;
	}
}

void defineDBPath(DBDomainId domainId, const string &dbPath)
{
	if (domainId != DB_DOMAIN_ID_HATOHOL)
		cut_fail("Cannot set a domain ID for %d\n", domainId);
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPath);
}

UserIdType searchMaxTestUserId(void)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	UserInfoList userInfoList;
	UserQueryOption option(USER_ID_SYSTEM);
	dbUser->getUserInfoList(userInfoList, option);
	cppcut_assert_equal(false, userInfoList.empty());

	UserIdType userId = 0;
	UserInfoListIterator userInfo = userInfoList.begin();
	for (; userInfo != userInfoList.end(); ++userInfo) {
		if (userInfo->id > userId)
			userId = userInfo->id;
	}
	return userId;
}

static UserIdType findUserCommon(const OperationPrivilegeType &type,
                                 const bool &shouldHas,
                                 const OperationPrivilegeFlag &excludeFlags = 0)
{
	UserIdType userId = INVALID_USER_ID;
	OperationPrivilegeFlag flag = OperationPrivilege::makeFlag(type);

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	UserInfoList userInfoList;
	UserQueryOption option(USER_ID_SYSTEM);
	dbUser->getUserInfoList(userInfoList, option);
	cppcut_assert_equal(false, userInfoList.empty());

	UserInfoListIterator userInfo = userInfoList.begin();
	for (; userInfo != userInfoList.end(); ++userInfo) {
		if (userInfo->flags & excludeFlags)
			continue;
		bool having = userInfo->flags & flag;
		if ((shouldHas && having) || (!shouldHas && !having)) {
			userId = userInfo->id;
			break;
		}
	}
	cppcut_assert_not_equal(INVALID_USER_ID, userId);
	cppcut_assert_not_equal(USER_ID_SYSTEM, userId);
	return userId;
}


UserIdType findUserWith(const OperationPrivilegeType &type,
                        const OperationPrivilegeFlag &excludeFlags)
{
	return findUserCommon(type, true, excludeFlags);
}

UserIdType findUserWithout(const OperationPrivilegeType &type)
{
	return findUserCommon(type, false);
}

void initActionDef(ActionDef &actionDef)
{
       actionDef.id = 0;
       actionDef.type = ACTION_COMMAND;
       actionDef.timeout = 0;
       actionDef.ownerUserId = INVALID_USER_ID;;
}

string getSyslogTail(size_t numLines)
{
	// TODO: consider the environment that uses /var/log/messages.
	//       mlpl's testLogger does the similar things. We should
	//       create a commonly used method.
	const string cmd =
	  StringUtils::sprintf("tail -%zd /var/log/syslog", numLines);
	return executeCommand(cmd);
}

// ---------------------------------------------------------------------------
// Watcher
// ---------------------------------------------------------------------------
Watcher::Watcher(void)
: expired(false),
  timerId(INVALID_EVENT_ID)
{ 
}

Watcher::~Watcher()
{
	if (timerId != INVALID_EVENT_ID)
		g_source_remove(timerId);
}

gboolean Watcher::_run(gpointer data)
{
	Watcher *obj = static_cast<Watcher *>(data);
	return obj->run();
}

gboolean Watcher::run(void)
{
	expired = true;
	timerId = INVALID_EVENT_ID;
	return G_SOURCE_REMOVE;
}

bool Watcher::start(const size_t &timeout, const size_t &interval)
{
	timerId = g_timeout_add(timeout, _run, this);
	while (!expired) {
		if (interval == 0) {
			g_main_context_iteration(NULL, TRUE);
		} else {
			while (g_main_context_iteration(NULL, FALSE))
				;
			usleep(interval);
		}
		if (watch())
			return true;
	}
	return false;
}

bool Watcher::watch(void)
{
	return false;
}

// ---------------------------------------------------------------------------
// LinesComparator
// ---------------------------------------------------------------------------
struct LinesComparator::PrivateContext
{
	multiset<string> set0;
	multiset<string> set1;

	string str0;
	string str1;

	void assertWithoutOrder(void)
	{
		multiset<string>::iterator line0Itr;
		multiset<string>::iterator line1Itr;
		while (!set0.empty()) {
			line0Itr = set0.begin();
			line1Itr = set1.find(*line0Itr);
			if (line1Itr == set1.end()) {
				cut_fail("Not found line: %s\n"
				         "<<set0>>\n%s\n"
				         "<<set1>>\n%s\n",
				         line0Itr->c_str(),
				         str0.c_str(), str1.c_str());
			}
			set0.erase(line0Itr);
			set1.erase(line1Itr);
		}
	}
};

LinesComparator::LinesComparator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

LinesComparator::~LinesComparator()
{
	if (m_ctx)
		delete m_ctx;
}

void LinesComparator::add(
  const std::string &line0, const std::string &line1)
{
	m_ctx->str0 += line0;
	m_ctx->str0 += "\n";
	m_ctx->str1 += line1;
	m_ctx->str1 += "\n";

	m_ctx->set0.insert(line0);
	m_ctx->set1.insert(line1);
}

void LinesComparator::assert(const bool &strictOrder)
{
	if (strictOrder)
		cppcut_assert_equal(m_ctx->str0, m_ctx->str1);
	else
		m_ctx->assertWithoutOrder();
}

// ---------------------------------------------------------------------------
// GMainLoopWithTimeout
// ---------------------------------------------------------------------------
GMainLoopWithTimeout::GMainLoopWithTimeout(void)
: m_timerTag(0),
  m_loop(NULL)
{
	m_loop = g_main_loop_new(NULL, TRUE);
}

GMainLoopWithTimeout::~GMainLoopWithTimeout()
{
	if (m_loop)
		g_main_loop_unref(m_loop);
	if (m_timerTag)
		g_source_remove(m_timerTag);
}

void GMainLoopWithTimeout::run(void)
{
	static const size_t timeout = 5000; // ms
	m_timerTag = g_timeout_add(timeout, failureDueToTimedOut, NULL);
	g_main_loop_run(m_loop);
}

void GMainLoopWithTimeout::quit(void)
{
	g_main_loop_quit(m_loop);
}

gboolean GMainLoopWithTimeout::failureDueToTimedOut(gpointer data)
{
	cut_fail("Timed out");
	return G_SOURCE_REMOVE;
}
