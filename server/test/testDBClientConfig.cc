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
#include <gcutter.h>

#include "Hatohol.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"

static const char *TEST_DB_NAME = "test_db_config";

struct TestDBClientConfig : public DBClientConfig {

	static bool callParseCommandLineArgument(const CommandLineArg &cmdArg)
	{
		bool ret = parseCommandLineArgument(cmdArg);
		// Some of the parameters in the above are effective on reset().
		TestDBClientConfig::reset();
		return ret;
	}
};

namespace testDBClientConfig {

void _assertGetHostAddress
  (const string &ipAddr, const string &hostName, const char *expectValue)
{
	MonitoringServerInfo svInfo;
	svInfo.ipAddress = ipAddr;
	svInfo.hostName  = hostName;
	cppcut_assert_equal(expectValue, svInfo.getHostAddress());
}
#define assertGetHostAddress(IP, HOSTNAME, EXPECT) \
cut_trace(_assertGetHostAddress(IP, HOSTNAME, EXPECT))

static void addTargetServer(MonitoringServerInfo *serverInfo)
{
	DBClientConfig dbConfig;
	dbConfig.addOrUpdateTargetServer(serverInfo);
}
#define assertAddServerToDB(X) \
cut_trace(_assertAddToDB<MonitoringServerInfo>(X, addTargetServer))

static string makeExpectedOutput(MonitoringServerInfo *serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s|%d|%d|%d|%s|%s|%s\n",
	                        serverInfo->id, serverInfo->type,
	                        serverInfo->hostName.c_str(),
	                        serverInfo->ipAddress.c_str(),
	                        serverInfo->nickname.c_str(),
	                        serverInfo->port,
	                        serverInfo->pollingIntervalSec,
	                        serverInfo->retryIntervalSec,
	                        serverInfo->userName.c_str(),
	                        serverInfo->password.c_str(),
	                        serverInfo->dbName.c_str());
	return expectedOut;
}

static const char *TEST_DB_USER = "hatohol_test_user";
static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
void cut_setup(void)
{
	hatoholInit();
	DBClient::setDefaultDBParams(
	  DB_DOMAIN_ID_CONFIG, TEST_DB_NAME, TEST_DB_USER, TEST_DB_PASSWORD);

	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
}

void cut_teardown(void)
{
	// This commands resets the server address and the port.
	CommandLineArg cmdArg;
	TestDBClientConfig::callParseCommandLineArgument(cmdArg);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientConfig dbConfig;
	cppcut_assert_equal(DB_DOMAIN_ID_CONFIG,
	                    dbConfig.getDBAgent()->getDBDomainId());
}

void test_setDefaultDBParams(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(string(TEST_DB_USER), connInfo.user);
	cppcut_assert_equal(string(TEST_DB_PASSWORD), connInfo.password);
}

void test_getHostAddressIP(void)
{
	const char *ipAddr = "192.168.1.1";
	assertGetHostAddress(ipAddr, "example.com", ipAddr);
}

void test_getHostAddressHostName(void)
{
	const char *hostName = "example.com";
	assertGetHostAddress("", hostName, hostName);
}

void test_getHostAddressBothNotSet(void)
{
	assertGetHostAddress("", "", NULL);
}

void test_createDB(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DBClientConfig dbConfig;

	// check the version
	string statement = "select * from _dbclient_version";
	string expect =
	  StringUtils::sprintf(
	    "%d|%d\n", DB_DOMAIN_ID_CONFIG,
	               DBClientConfig::CONFIG_DB_VERSION);
	assertDBContent(dbConfig.getDBAgent(), statement, expect);
}

void test_createTableSystem(void)
{
	const string tableName = "system";
	DBClientConfig dbConfig;
	assertCreateTable(dbConfig.getDBAgent(), tableName);
	
	// check content
	string statement = "select * from " + tableName;
	const char *expectedDatabasePath = "";
	int expectedEnableFaceMySQL = 0;
	int expectedFaceRestPort    = 0;
	int expectedEnableCopyOnDemand = 1;
	string expectedOut =
	   StringUtils::sprintf("%s|%d|%d|%d\n",
	                        expectedDatabasePath,
	                        expectedEnableFaceMySQL, expectedFaceRestPort,
	                        expectedEnableCopyOnDemand);
	assertDBContent(dbConfig.getDBAgent(), statement, expectedOut);
}

void test_createTableServers(void)
{
	const string tableName = "servers";
	DBClientConfig dbConfig;
	assertCreateTable(dbConfig.getDBAgent(), tableName);

	// check content
	string statement = "select * from " + tableName;
	string expectedOut = ""; // currently no data
	assertDBContent(dbConfig.getDBAgent(), statement, expectedOut);
}

void test_addTargetServer(void)
{
	// added a record
	MonitoringServerInfo *testInfo = serverInfo;
	assertAddServerToDB(testInfo);

	// confirm with the command line tool
	string statement = "select * from servers";
	string expectedOut = makeExpectedOutput(testInfo);
	DBClientConfig dbConfig;
	assertDBContent(dbConfig.getDBAgent(), statement, expectedOut);
}

void _assertGetTargetServers(UserIdType userId)
{
	ServerHostGrpSetMap authMap;
	MonitoringServerInfoList expected;
	makeServerHostGrpSetMap(authMap, userId);
	for (size_t i = 0; i < NumServerInfo; i++)
		if (isAuthorized(authMap, userId, serverInfo[i].id))
			expected.push_back(serverInfo[i]);

	for (size_t i = 0; i < NumServerInfo; i++)
		assertAddServerToDB(&serverInfo[i]);

	MonitoringServerInfoList actual;
	ServerQueryOption option(userId);
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(actual, option);
	cppcut_assert_equal(expected.size(), actual.size());

	string expectedText;
	string actualText;
	MonitoringServerInfoListIterator it_expected = expected.begin();
	MonitoringServerInfoListIterator it_actual = actual.begin();
	for (; it_expected != expected.end(); ++it_expected, ++it_actual) {
		expectedText += makeExpectedOutput(&(*it_expected));
		actualText += makeExpectedOutput(&(*it_actual));
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetTargetServers(U) \
  cut_trace(_assertGetTargetServers(U))

void data_getTargetServers(void)
{
	gcut_add_datum("By Admin",
		       "userId", G_TYPE_INT, USER_ID_ADMIN,
		       NULL);
	gcut_add_datum("With no allowed servers",
		       "userId", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("With a few allowed servers",
		       "userId", G_TYPE_INT, 3,
		       NULL);
	gcut_add_datum("With one allowed servers",
		       "userId", G_TYPE_INT, 5,
		       NULL);
}

void test_getTargetServers(gconstpointer data)
{
	UserIdType userId = gcut_data_get_int(data, "userId");
	setupTestDBUser(true, true);
	assertGetTargetServers(userId);
}

void test_getTargetServersByInvalidUser(void)
{
	UserIdType userId = INVALID_USER_ID;
	assertGetTargetServers(userId);
}

void test_setGetDatabaseDir(void)
{
	const string databaseDir = "/dir1/dir2";
	DBClientConfig dbConfig;
	dbConfig.setDatabaseDir(databaseDir);
	cppcut_assert_equal(databaseDir, dbConfig.getDatabaseDir());
}

void test_setGetFaceRestPort(void)
{
	const int portNumber = 501;
	DBClientConfig dbConfig;
	dbConfig.setFaceRestPort(portNumber);
	cppcut_assert_equal(portNumber, dbConfig.getFaceRestPort());
}

void test_parseArgConfigDBServer(void)
{
	const string serverName = "cat.example.com";
	CommandLineArg arg;
	arg.push_back("--config-db-server");
	arg.push_back(serverName);
	cppcut_assert_equal(
	  true, TestDBClientConfig::callParseCommandLineArgument(arg));
	DBConnectInfo connInfo =
	   DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(serverName, connInfo.host);
}

void test_parseArgConfigDBServerWithPort(void)
{
	const string serverName = "cat.example.com";
	const size_t port = 1027;
	CommandLineArg arg;
	arg.push_back("--config-db-server");
	arg.push_back(StringUtils::sprintf("%s:%zd", serverName.c_str(), port));
	cppcut_assert_equal(
	  true, TestDBClientConfig::callParseCommandLineArgument(arg));
	DBConnectInfo connInfo =
	   DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(serverName, connInfo.host);
	cppcut_assert_equal(port, connInfo.port);
}

void test_isCopyOnDemandEnabledDefault(void)
{
	DBClientConfig dbConfig;
	cut_assert_true(dbConfig.isCopyOnDemandEnabled());
}

void test_serverQueryOptionForAdmin(void)
{
	ServerQueryOption option(USER_ID_ADMIN);
	cppcut_assert_equal(string(""), option.getCondition());
}

void test_serverQueryOptionForAdminWithTarget(void)
{
	uint32_t serverId = 2;
	ServerQueryOption option(USER_ID_ADMIN);
	option.setTargetServerId(serverId);
	cppcut_assert_equal(StringUtils::sprintf("id=%"PRIu32, serverId),
			    option.getCondition());
}

void test_serverQueryOptionForInvalidUser(void)
{
	ServerQueryOption option(INVALID_USER_ID);
	cppcut_assert_equal(string("0"), option.getCondition());
}

void test_serverQueryOptionForInvalidUserWithTarget(void)
{
	uint32_t serverId = 2;
	ServerQueryOption option(INVALID_USER_ID);
	option.setTargetServerId(serverId);
	cppcut_assert_equal(string("0"), option.getCondition());
}

static string makeExpectedCondition(UserIdType userId)
{
	string condition;
	set<uint32_t> knownServerIdSet;

	for (size_t i = 0; i < NumTestAccessInfo; ++i) {
		if (testAccessInfo[i].userId != userId)
			continue;

		uint32_t serverId = testAccessInfo[i].serverId;
		if (knownServerIdSet.find(serverId) != knownServerIdSet.end())
			continue;

		if (!condition.empty())
			condition += " OR ";
		condition += StringUtils::sprintf("id=%"PRIu32, serverId);
		knownServerIdSet.insert(serverId);
	}
	return StringUtils::sprintf("(%s)", condition.c_str());
}

void test_serverQueryOptionForGuestUser(void)
{
	setupTestDBUser(true, true);
	UserIdType userId = 3;
	ServerQueryOption option(userId);
	cppcut_assert_equal(makeExpectedCondition(userId),
			    option.getCondition());
}

void test_serverQueryOptionForGuestUserWithTarget(void)
{
	setupTestDBUser(true, true);
	UserIdType userId = 3;
	uint32_t serverId = 2;
	ServerQueryOption option(userId);
	option.setTargetServerId(serverId);
	cppcut_assert_equal(StringUtils::sprintf("id=%"PRIu32, serverId),
			    option.getCondition());
}

} // namespace testDBClientConfig

namespace testDBClientConfigDefault {

void cut_setup(void)
{
	hatoholInit();
}

void test_databaseName(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_DB_NAME),
	                    connInfo.dbName);
}

void test_databaseUser(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_USER_NAME),
	                    connInfo.user);
}

void test_databasePassword(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_USER_NAME),
	                    connInfo.user);
}

} // namespace testDBClientConfigDefault
