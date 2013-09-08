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
#include <cutter.h>

#include "Hatohol.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"

static const char *TEST_DB_NAME = "test_db_config";

struct TestDBClientConfig : public DBClientConfig {

	static bool callParseCommandLineArgument(const CommandLineArg &cmdArg)
	{
		return parseCommandLineArgument(cmdArg);
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
	dbConfig.addTargetServer(serverInfo);
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

void cut_setup(void)
{
	hatoholInit();
	static const char *TEST_DB_USER = "hatohol_test_user";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClient::setDefaultDBParams(
	  DB_DOMAIN_ID_CONFIG, TEST_DB_NAME, TEST_DB_USER, TEST_DB_PASSWORD);

	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
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
	string statement = "select * from _dbclient";
	string expect =
	  StringUtils::sprintf(
	    "%d|%d\n", DBClient::DBCLIENT_DB_VERSION,
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
	int expectedEnableCopyOnDemand = 0;
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

void test_getTargetServers(void)
{
	for (size_t i = 0; i < NumServerInfo; i++)
		assertAddServerToDB(&serverInfo[i]);

	MonitoringServerInfoList monitoringServers;
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(monitoringServers);
	cppcut_assert_equal(NumServerInfo, monitoringServers.size());

	string expectedText;
	string actualText;
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (size_t i = 0; i < NumServerInfo; i++, ++it) {
		expectedText += makeExpectedOutput(&serverInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
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
	TestDBClientConfig::callParseCommandLineArgument(arg);
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
	TestDBClientConfig::callParseCommandLineArgument(arg);
	DBConnectInfo connInfo =
	   DBClient::getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	cppcut_assert_equal(serverName, connInfo.host);
	cppcut_assert_equal(port, connInfo.port);
}

void test_isCopyOnDemandEnabledDefault(void)
{
	DBClientConfig dbConfig;
	cppcut_assert_equal(false, dbConfig.isCopyOnDemandEnabled());
}

} // namespace testDBClientConfig
