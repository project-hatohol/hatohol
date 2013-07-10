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

void setup(void)
{
	hatoholInit();

	static const char *TEST_DB_USER = "hatohol";
	static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
	DBClientConfig::setDefaultDBParams(TEST_DB_NAME,
	                                   TEST_DB_USER, TEST_DB_PASSWORD);

	bool recreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, recreate);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
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
	assertCreateTable(DB_DOMAIN_ID_CONFIG, tableName);
	
	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_CONFIG, statement);
	const char *expectedDatabasePath = "";
	int expectedEnableFaceMySQL = 0;
	int expectedFaceRestPort    = 0;
	string expectedOut =
	   StringUtils::sprintf("%s|%d|%d\n",
	                        expectedDatabasePath,
	                        expectedEnableFaceMySQL, expectedFaceRestPort);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableServers(void)
{
	const string tableName = "servers";
	DBClientConfig dbConfig;
	assertCreateTable(DB_DOMAIN_ID_CONFIG, tableName);

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_CONFIG, statement);
	string expectedOut = ""; // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_testAddTargetServer(void)
{
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_CONFIG);

	// added a record
	MonitoringServerInfo *testInfo = serverInfo;
	assertAddServerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from servers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_testGetTargetServers(void)
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

void test_testSetGetDatabaseDir(void)
{
	const string databaseDir = "/dir1/dir2";
	DBClientConfig dbConfig;
	dbConfig.setDatabaseDir(databaseDir);
	cppcut_assert_equal(databaseDir, dbConfig.getDatabaseDir());
}

void test_testSetGetFaceRestPort(void)
{
	const int portNumber = 501;
	DBClientConfig dbConfig;
	dbConfig.setFaceRestPort(portNumber);
	cppcut_assert_equal(portNumber, dbConfig.getFaceRestPort());
}

} // namespace testDBClientConfig
