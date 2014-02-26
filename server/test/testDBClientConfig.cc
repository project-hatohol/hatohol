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
#include "Hatohol.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"
using namespace std;
using namespace mlpl;

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
  (const string &ipAddr, const string &hostName, const char *expectValue,
   bool forURI = false)
{
	MonitoringServerInfo svInfo;
	svInfo.ipAddress = ipAddr;
	svInfo.hostName  = hostName;
	string actualValue = svInfo.getHostAddress(forURI);
	cppcut_assert_equal(expectValue, actualValue.c_str());
}
#define assertGetHostAddress(IP, HOSTNAME, EXPECTED, ...) \
cut_trace(_assertGetHostAddress(IP, HOSTNAME, EXPECTED, ##__VA_ARGS__))

static void addTargetServer(MonitoringServerInfo *serverInfo)
{
	DBClientConfig dbConfig;
	OperationPrivilege privilege(ALL_PRIVILEGES);
	dbConfig.addTargetServer(serverInfo, privilege);
}
#define assertAddServerToDB(X) \
cut_trace(_assertAddToDB<MonitoringServerInfo>(X, addTargetServer))

static void getTargetServersData(void)
{
	gcut_add_datum("By Admin",
		       "userId", G_TYPE_INT, USER_ID_SYSTEM,
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

static const char *TEST_DB_USER = "hatohol_test_user";
static const char *TEST_DB_PASSWORD = ""; // empty: No password is used
void cut_setup(void)
{
	hatoholInit();
	bool dbRecreate = true;
	setupTestDBConfig(dbRecreate);
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

void test_getHostAddressIPv4ForURI(void)
{
	const char *ipAddr = "192.168.1.1";
	bool forURI = true;
	assertGetHostAddress(ipAddr, "example.com", ipAddr, forURI);
}

void test_getHostAddressIPv6(void)
{
	const char *ipAddr = "::1";
	assertGetHostAddress(ipAddr, "", ipAddr);
}

void test_getHostAddressIPv6ForURI(void)
{
	const char *ipAddr = "::1";
	bool forURI = true;
	assertGetHostAddress(ipAddr, "", "[::1]", forURI);
}

void test_getHostAddressHostName(void)
{
	const char *hostName = "example.com";
	assertGetHostAddress("", hostName, hostName);
}

void test_getHostAddressHostNameForURI(void)
{
	const char *hostName = "example.com";
	bool forURI = true;
	assertGetHostAddress("", hostName, hostName, forURI);
}

void test_getHostAddressBothNotSet(void)
{
	assertGetHostAddress("", "", "");
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

void _assertAddTargetServer(
  MonitoringServerInfo serverInfo, const HatoholErrorCode expectedErrorCode,
  OperationPrivilege privilege = ALL_PRIVILEGES)
{
	DBClientConfig dbConfig;
	HatoholError err;
	err = dbConfig.addTargetServer(&serverInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string expectedOut("");
	if (expectedErrorCode == HTERR_OK)
		expectedOut = makeServerInfoOutput(serverInfo);
	string statement("select * from servers");
	assertDBContent(dbConfig.getDBAgent(), statement, expectedOut);
}
#define assertAddTargetServer(I,E,...) \
cut_trace(_assertAddTargetServer(I,E,##__VA_ARGS__))

void test_addTargetServer(void)
{
	MonitoringServerInfo *testInfo = testServerInfo;
	assertAddTargetServer(*testInfo, HTERR_OK);
}

void test_addTargetServerWithoutPrivilege(void)
{
	MonitoringServerInfo *testInfo = testServerInfo;
	OperationPrivilegeFlag privilege = ALL_PRIVILEGES;
	privilege &= ~(1 << OPPRVLG_CREATE_SERVER);
	assertAddTargetServer(*testInfo, HTERR_NO_PRIVILEGE, privilege);
}

void test_addTargetServerWithInvalidServerType(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.type = NUM_MONITORING_SYSTEMS;
	assertAddTargetServer(testInfo, HTERR_INVALID_MONITORING_SYSTEM_TYPE);
}

void test_addTargetServerWithInvalidPortNumber(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.port = 65536;
	assertAddTargetServer(testInfo, HTERR_INVALID_PORT_NUMBER);
}

void test_addTargetServerWithLackedIPv4Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "192.168.1.";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithOverflowIPv4Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "192.168.1.256";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithTooManyIPv4Fields(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "1.192.168.1.1";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithValidIPv6Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "FE80:0000:0000:0000:0202:B3FF:FE1E:8329";
	assertAddTargetServer(testInfo, HTERR_OK);
}

void test_addTargetServerWithShortenIPv6Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "FE80::0202:B3FF:FE1E:8329";
	assertAddTargetServer(testInfo, HTERR_OK);
}

void test_addTargetServerWithMixedIPv6Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "fe80::0202:b3ff:fe1e:192.168.1.1";
	string expectedOut = makeServerInfoOutput(testInfo);
	assertAddTargetServer(testInfo, HTERR_OK);
}

void test_addTargetServerWithTooManyIPv6AddressFields(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "FE80:0000:0000:0000:0202:B3FF:FE1E:8329::1";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithOverflowIPv6Address(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "FE80::02:B3FF:10000:8329";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithInvalidIPv6AddressCharacter(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "FE80::02:B3FG:8329";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithIPv6Loopback(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "::1";
	assertAddTargetServer(testInfo, HTERR_OK);
}

void test_addTargetServerWithSimpleNumber(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "123";
	assertAddTargetServer(testInfo, HTERR_INVALID_IP_ADDRESS);
}

void test_addTargetServerWithEmptyIPAddress(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.ipAddress = "";
	// Allow empty IP address when the host name isn't empty.
	assertAddTargetServer(testInfo, HTERR_OK);
}

void test_addTargetServerWithEmptyIPAddressAndHostname(void)
{
	MonitoringServerInfo testInfo = testServerInfo[0];
	testInfo.hostName = "";
	testInfo.ipAddress = "";
	assertAddTargetServer(testInfo, HTERR_NO_IP_ADDRESS_AND_HOST_NAME);
}

void _assertUpdateTargetServer(
  MonitoringServerInfo serverInfo, const HatoholErrorCode expectedErrorCode,
  OperationPrivilege privilege = ALL_PRIVILEGES)
{
	loadTestDBServer();

	int targetId = serverInfo.id;
	int targetIdx = targetId - 1;

	string expectedOut;
	if (expectedErrorCode == HTERR_OK)
		expectedOut = makeServerInfoOutput(serverInfo);
	else
		expectedOut = makeServerInfoOutput(testServerInfo[targetIdx]);

	DBClientConfig dbConfig;
	HatoholError err;
	err = dbConfig.updateTargetServer(&serverInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string statement = StringUtils::sprintf(
	                     "select * from servers where id=%d",
			     targetId);
	assertDBContent(dbConfig.getDBAgent(), statement, expectedOut);
}
#define assertUpdateTargetServer(I,E,...) \
cut_trace(_assertUpdateTargetServer(I,E,##__VA_ARGS__))

void test_updateTargetServer(void)
{
	int targetId = 2;
	MonitoringServerInfo serverInfo = testServerInfo[0];
	serverInfo.id = targetId;
	assertUpdateTargetServer(serverInfo, HTERR_OK);
}

void test_updateTargetServerWithoutPrivilege(void)
{
	int targetId = 2;
	MonitoringServerInfo serverInfo = testServerInfo[0];
	serverInfo.id = targetId;
	OperationPrivilegeFlag privilege = ALL_PRIVILEGES;
	OperationPrivilegeFlag updateFlags = 
	 (1 << OPPRVLG_UPDATE_SERVER) | (1 << OPPRVLG_UPDATE_ALL_SERVER);
	privilege &= ~updateFlags;
	assertUpdateTargetServer(
	  serverInfo, HTERR_NO_PRIVILEGE, privilege);
}

void test_updateTargetServerWithNoHostNameAndIPAddress(void)
{
	int targetId = 2;
	MonitoringServerInfo serverInfo = testServerInfo[0];
	serverInfo.id = targetId;
	serverInfo.hostName = "";
	serverInfo.ipAddress = "";
	assertUpdateTargetServer(
	  serverInfo, HTERR_NO_IP_ADDRESS_AND_HOST_NAME);
}

void test_deleteTargetServer(void)
{
	setupTestDBUser(true, true);
	loadTestDBServer();
	ServerIdType targetServerId = 1;
	OperationPrivilege privilege(findUserWith(OPPRVLG_DELETE_ALL_SERVER));
	DBClientConfig dbConfig;
	HatoholError err = dbConfig.deleteTargetServer(targetServerId,
						       privilege);
	assertHatoholError(HTERR_OK, err);

	ServerIdSet serverIdSet;
	serverIdSet.insert(targetServerId);
	assertServersInDB(serverIdSet);
}

void test_deleteTargetServerWithoutPrivilege(void)
{
	setupTestDBUser(true, true);
	loadTestDBServer();
	ServerIdType targetServerId = 1;
	OperationPrivilege privilege;
	DBClientConfig dbConfig;
	HatoholError err = dbConfig.deleteTargetServer(targetServerId,
						       privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	ServerIdSet serverIdSet;
	assertServersInDB(serverIdSet);
}

static void prepareGetServer(
  const UserIdType &userId, ServerHostGrpSetMap &authMap,
  MonitoringServerInfoList &expected)
{
	makeServerHostGrpSetMap(authMap, userId);
	for (size_t i = 0; i < NumTestServerInfo; i++)
		if (isAuthorized(authMap, userId, testServerInfo[i].id))
			expected.push_back(testServerInfo[i]);

	for (size_t i = 0; i < NumTestServerInfo; i++)
		assertAddServerToDB(&testServerInfo[i]);
}

void _assertGetTargetServers(UserIdType userId)
{
	ServerHostGrpSetMap authMap;
	MonitoringServerInfoList expected;
	prepareGetServer(userId, authMap, expected);

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
		expectedText += makeServerInfoOutput(*it_expected);
		actualText += makeServerInfoOutput(*it_actual);
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetTargetServers(U) \
  cut_trace(_assertGetTargetServers(U))

static void _assertGetServerIdSet(const UserIdType &userId)
{
	ServerHostGrpSetMap authMap;
	ServerIdSet expectIdSet;
	MonitoringServerInfoList svInfoList;
	prepareGetServer(userId, authMap, svInfoList);
	MonitoringServerInfoListIterator serverInfo = svInfoList.begin();
	for (; serverInfo != svInfoList.end(); ++serverInfo)
		expectIdSet.insert(serverInfo->id);

	ServerIdSet actualIdSet;
	ServerQueryOption option(userId);
	DBClientConfig dbConfig;
	dbConfig.getServerIdSet(actualIdSet, option);
	cppcut_assert_equal(expectIdSet.size(), actualIdSet.size());

	ServerIdSetIterator expectId = expectIdSet.begin();
	ServerIdSetIterator actualId = actualIdSet.begin();
	for (; expectId != expectIdSet.end(); ++expectId, ++actualId)
		cppcut_assert_equal(*expectId, *actualId);
}
#define assertGetServerIdSet(U) cut_trace(_assertGetServerIdSet(U))

void data_getTargetServers(void)
{
	getTargetServersData();
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

void data_getServerIdSet(void)
{
	getTargetServersData();
}

void test_getServerIdSet(gconstpointer data)
{
	const UserIdType userId = gcut_data_get_int(data, "userId");
	setupTestDBUser(true, true);
	assertGetServerIdSet(userId);
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
	ServerQueryOption option(USER_ID_SYSTEM);
	cppcut_assert_equal(string(""), option.getCondition());
}

void test_serverQueryOptionForAdminWithTarget(void)
{
	uint32_t serverId = 2;
	ServerQueryOption option(USER_ID_SYSTEM);
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


void test_serverQueryOptionConstructorTakingOperationPrivilege(void)
{
	const UserIdType &userId = 5;
	OperationPrivilege privilege(userId);
	ServerQueryOption option(privilege);
	cppcut_assert_equal(userId, option.getUserId());
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
