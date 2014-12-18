/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "DBTablesConfig.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBHatohol.h"
#include "DBTablesTest.h"
using namespace std;
using namespace mlpl;

namespace testDBTablesConfig {

#define DECLARE_DBTABLES_CONFIG(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesConfig &VAR_NAME = _dbHatohol.getDBTablesConfig();

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

void _assertArmPluginInfo(
  const ArmPluginInfo &expect, const ArmPluginInfo &actual,
  const int *expectId = NULL)
{
	if (expectId)
		cppcut_assert_equal(*expectId, actual.id);
	else
		cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.type, actual.type);
	cppcut_assert_equal(expect.path, actual.path);
	cppcut_assert_equal(expect.brokerUrl, actual.brokerUrl);
	cppcut_assert_equal(expect.staticQueueAddress,
	                    actual.staticQueueAddress);
}
#define assertArmPluginInfo(E,A,...) \
  cut_trace(_assertArmPluginInfo(E,A, ##__VA_ARGS__))

static void addTargetServer(MonitoringServerInfo *serverInfo)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
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

static string makeExpectedDBOutLine(
  const size_t &idx, const ArmPluginInfo &armPluginInfo)
{
	string s = StringUtils::sprintf(
	  "%zd|%d|%s|%s|%s|%" FMT_SERVER_ID "|%s|%s|%s|%d",
	  (idx + 1), // armPluginInfo.id
	  armPluginInfo.type,
	  armPluginInfo.path.c_str(),
	  armPluginInfo.brokerUrl.c_str(),
	  armPluginInfo.staticQueueAddress.c_str(),
	  armPluginInfo.serverId,
	  armPluginInfo.tlsCertificatePath.c_str(),
	  armPluginInfo.tlsKeyPath.c_str(),
	  armPluginInfo.tlsCACertificatePath.c_str(),
	  armPluginInfo.tlsEnableVerify);
	return s;
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DECLARE_DBTABLES_CONFIG(dbConfig);
	assertDBTablesVersion(
	  dbConfig.getDBAgent(),
	  DB_TABLES_ID_CONFIG, DBTablesConfig::CONFIG_DB_VERSION);
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

void test_createTableSystem(void)
{
	const string tableName = "system";
	DECLARE_DBTABLES_CONFIG(dbConfig);
	assertCreateTable(&dbConfig.getDBAgent(), tableName);
	
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
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}

void test_createTableServerTypes(void)
{
	const string tableName = "server_types";
	DECLARE_DBTABLES_CONFIG(dbConfig);
	assertCreateTable(&dbConfig.getDBAgent(), tableName);

	// check content
	const string statement = "SELECT * FROM " + tableName;
	const string expectedOut = ""; // currently no data
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}

void test_registerServerType(void)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	ServerTypeInfo svTypeInfo;
	svTypeInfo.type = MONITORING_SYSTEM_FAKE;
	svTypeInfo.name = "Fake Monitor";
	svTypeInfo.parameters = "IP address|port|Username|Password";
	svTypeInfo.pluginPath = "/usr/bin/hoge-ta-hoge-taro";
	dbConfig.registerServerType(svTypeInfo);

	// check content
	const string statement = "SELECT * FROM server_types";
	const string expectedOut =
	  StringUtils::sprintf("%d|%s|%s|%s\n",
	                       svTypeInfo.type, svTypeInfo.name.c_str(),
	                       svTypeInfo.parameters.c_str(),
	                       svTypeInfo.pluginPath.c_str());
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}

void test_registerServerTypeUpdate(void)
{
	test_registerServerType(); // insert the record

	DECLARE_DBTABLES_CONFIG(dbConfig);
	ServerTypeInfo svTypeInfo;
	svTypeInfo.type = MONITORING_SYSTEM_FAKE;
	svTypeInfo.name = "Fake Monitor (updated version)";
	svTypeInfo.parameters = "IP address|port|Username|Password|Group";
	svTypeInfo.pluginPath = "/usr/bin/hoge-ta-hoge-taro";
	dbConfig.registerServerType(svTypeInfo);

	// check content
	const string statement = "SELECT * FROM server_types";
	const string expectedOut =
	  StringUtils::sprintf("%d|%s|%s|%s\n",
	                       svTypeInfo.type, svTypeInfo.name.c_str(),
	                       svTypeInfo.parameters.c_str(),
	                       svTypeInfo.pluginPath.c_str());
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}

void data_getDefaultPluginPath(void)
{
	gcut_add_datum("MONITORING_SYSTEM_ZABBIX",
	               "type", G_TYPE_INT, (int)MONITORING_SYSTEM_ZABBIX,
	               "expect", G_TYPE_STRING, NULL, NULL);
	gcut_add_datum("MONITORING_SYSTEM_NAGIOS",
	               "type", G_TYPE_INT, (int)MONITORING_SYSTEM_NAGIOS,
	               "expect", G_TYPE_STRING, NULL, NULL);
	gcut_add_datum("MONITORING_SYSTEM_HAPI_ZABBIX",
	               "type", G_TYPE_INT, (int)MONITORING_SYSTEM_HAPI_ZABBIX,
	               "expect", G_TYPE_STRING, "hatohol-arm-plugin-zabbix",
	               NULL);
	gcut_add_datum("MONITORING_SYSTEM_HAPI_NAGIOS",
	               "type", G_TYPE_INT, (int)MONITORING_SYSTEM_HAPI_NAGIOS,
	               "expect", G_TYPE_STRING, "hatohol-arm-plugin-nagios",
	               NULL);
}

void test_getDefaultPluginPath(gconstpointer data)
{
	const string expect = gcut_data_get_string(data, "expect") ? : "";
	const string actual = DBTablesConfig::getDefaultPluginPath(
	    (MonitoringSystemType)gcut_data_get_int(data, "type"));
	cppcut_assert_equal(expect, actual);
}

void test_getServerTypes(void)
{
	test_registerServerType(); // insert the record

	DECLARE_DBTABLES_CONFIG(dbConfig);
	ServerTypeInfoVect svTypeVect;
	dbConfig.getServerTypes(svTypeVect);

	// check content
	cppcut_assert_equal((size_t)1, svTypeVect.size());
}

void test_getServerType(void)
{
	loadTestDBServerType();

	DECLARE_DBTABLES_CONFIG(dbConfig);
	for (size_t i = 0; i < NumTestServerTypeInfo; i++) {
		const ServerTypeInfo &expect = testServerTypeInfo[i];
		ServerTypeInfo actual;
		cppcut_assert_equal(true,
		   dbConfig.getServerType(actual, expect.type));
		cppcut_assert_equal(expect.type, actual.type);
		cppcut_assert_equal(expect.name, actual.name);
		cppcut_assert_equal(expect.parameters, actual.parameters);
		cppcut_assert_equal(expect.pluginPath, actual.pluginPath);
	}
}

void test_getServerTypeExpectFalse(void)
{
	loadTestDBServerType();

	const MonitoringSystemType type =
	  static_cast<MonitoringSystemType>(NumTestServerTypeInfo);
	DECLARE_DBTABLES_CONFIG(dbConfig);
	ServerTypeInfo actual;
	cppcut_assert_equal(false, dbConfig.getServerType(actual, type));
}

void test_createTableServers(void)
{
	const string tableName = "servers";
	DECLARE_DBTABLES_CONFIG(dbConfig);
	assertCreateTable(&dbConfig.getDBAgent(), tableName);

	// check content
	string statement = "select * from " + tableName;
	string expectedOut = ""; // currently no data
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}

void _assertAddTargetServer(
  MonitoringServerInfo serverInfo, const HatoholErrorCode expectedErrorCode,
  OperationPrivilege privilege = ALL_PRIVILEGES)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err;
	err = dbConfig.addTargetServer(&serverInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string expectedOut("");
	if (expectedErrorCode == HTERR_OK)
		expectedOut = makeServerInfoOutput(serverInfo);
	string statement("select * from servers");
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
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
	OperationPrivilege::removeFlag(privilege, OPPRVLG_CREATE_SERVER);
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

	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err;
	err = dbConfig.updateTargetServer(&serverInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string statement = StringUtils::sprintf(
	                     "select * from servers where id=%d",
			     targetId);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
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

void data_deleteTargetServer(void)
{
	prepareDataWithAndWithoutArmPlugin();
}

void test_deleteTargetServer(gconstpointer data)
{
	loadTestDBTablesUser();
	loadTestDBServer();

	const bool withArmPlugin = gcut_data_get_boolean(data, "withArmPlugin");
	if (withArmPlugin)
		loadTestDBArmPlugin();

	ServerIdType targetServerId = 1;
	OperationPrivilege privilege(findUserWith(OPPRVLG_DELETE_ALL_SERVER));
	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err = dbConfig.deleteTargetServer(targetServerId,
						       privilege);
	assertHatoholError(HTERR_OK, err);

	ServerIdSet serverIdSet;
	serverIdSet.insert(targetServerId);
	assertServersInDB(serverIdSet);
	if (withArmPlugin) {
		int armPluginId =
		  findIndexOfTestArmPluginInfo(targetServerId);
		cppcut_assert_not_equal(-1, armPluginId);
		armPluginId += 1; // ID should be index + 1
		set<int> armPluginIdSet;
		armPluginIdSet.insert(armPluginId);
		assertArmPluginsInDB(serverIdSet);
	}
}

void test_deleteTargetServerWithoutPrivilege(void)
{
	loadTestDBTablesUser();
	loadTestDBServer();

	ServerIdType targetServerId = 1;
	OperationPrivilege privilege;
	DECLARE_DBTABLES_CONFIG(dbConfig);
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

void _assertGetTargetServers(
  UserIdType userId, ArmPluginInfoVect *armPluginInfoVect = NULL,
  MonitoringServerInfoList *copyDest = NULL)
{
	ServerHostGrpSetMap authMap;
	MonitoringServerInfoList expected;
	prepareGetServer(userId, authMap, expected);

	MonitoringServerInfoList actual;
	ServerQueryOption option(userId);
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.getTargetServers(actual, option, armPluginInfoVect);
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
	if (copyDest)
		*copyDest = actual;
}
#define assertGetTargetServers(U, ...) \
  cut_trace(_assertGetTargetServers(U, ##__VA_ARGS__))

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
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(userId), false);
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.getServerIdSet(actualIdSet, dqCtxPtr);
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
	loadTestDBTablesUser();

	UserIdType userId = gcut_data_get_int(data, "userId");
	assertGetTargetServers(userId);
}

void test_getTargetServersByInvalidUser(void)
{
	UserIdType userId = INVALID_USER_ID;
	assertGetTargetServers(userId);
}

void test_getTargetServersWithArmPlugin(void)
{
	// loadTestDBServer() is not needed. data server data is added in
	// prepareGetServer().
	loadTestDBArmPlugin();

	const UserIdType userId = USER_ID_SYSTEM;
	ArmPluginInfoVect armPluginInfoVect;
	MonitoringServerInfoList serverInfoList;
	assertGetTargetServers(userId, &armPluginInfoVect, &serverInfoList);
	
	cppcut_assert_equal(serverInfoList.size(), armPluginInfoVect.size());
	MonitoringServerInfoListIterator svInfoIt = serverInfoList.begin();
	ArmPluginInfoVectConstIterator pluginIt = armPluginInfoVect.begin();
	size_t numValidPluginInfo = 0;
	for (; svInfoIt != serverInfoList.end(); ++svInfoIt, ++pluginIt) {
		const int index = findIndexOfTestArmPluginInfo(svInfoIt->id);
		if (index == -1) {
			cppcut_assert_equal(INVALID_ARM_PLUGIN_INFO_ID,
			                    pluginIt->id);
			continue;
		}
		const ArmPluginInfo &expect = testArmPluginInfo[index];
		const int expectId = index + 1;
		assertArmPluginInfo(expect, *pluginIt, &expectId);
		numValidPluginInfo++;
	}
	cppcut_assert_equal(
	  true, numValidPluginInfo > 0,
	  cut_message("The test materials are inappropriate\n"));
	if (isVerboseMode())
		cut_notify("<<numValidPluginInfo>>: %zd", numValidPluginInfo);
}

void data_getServerIdSet(void)
{
	getTargetServersData();
}

void test_getServerIdSet(gconstpointer data)
{
	loadTestDBTablesUser();

	const UserIdType userId = gcut_data_get_int(data, "userId");
	assertGetServerIdSet(userId);
}

void test_setGetDatabaseDir(void)
{
	const string databaseDir = "/dir1/dir2";
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.setDatabaseDir(databaseDir);
	cppcut_assert_equal(databaseDir, dbConfig.getDatabaseDir());
}

void test_setGetFaceRestPort(void)
{
	const int portNumber = 501;
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.setFaceRestPort(portNumber);
	cppcut_assert_equal(portNumber, dbConfig.getFaceRestPort());
}

void test_isCopyOnDemandEnabledDefault(void)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
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
	cppcut_assert_equal(StringUtils::sprintf("id=%" PRIu32, serverId),
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
		condition += StringUtils::sprintf("id=%" PRIu32, serverId);
		knownServerIdSet.insert(serverId);
	}
	return StringUtils::sprintf("(%s)", condition.c_str());
}

void test_serverQueryOptionForGuestUser(void)
{
	loadTestDBTablesUser();

	UserIdType userId = 3;
	ServerQueryOption option(userId);
	cppcut_assert_equal(makeExpectedCondition(userId),
			    option.getCondition());
}

void test_serverQueryOptionForGuestUserWithTarget(void)
{
	loadTestDBTablesUser();

	UserIdType userId = 3;
	uint32_t serverId = 2;
	ServerQueryOption option(userId);
	option.setTargetServerId(serverId);
	cppcut_assert_equal(StringUtils::sprintf("id=%" PRIu32, serverId),
			    option.getCondition());
}


void test_serverQueryOptionConstructorTakingDataQueryContext(void)
{
	loadTestDBTablesUser();

	const UserIdType &userId = 5;
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(userId), false);
	ServerQueryOption option(dqCtxPtr);
	cppcut_assert_equal(userId, option.getUserId());
}

void data_isHatoholArmPlugin(void)
{
	gcut_add_datum("MONITORING_SYSTEM_ZABBIX",
	               "data", G_TYPE_INT, (int)MONITORING_SYSTEM_ZABBIX,
	               "expect", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("MONITORING_SYSTEM_NAGIOS",
	               "data", G_TYPE_INT, (int)MONITORING_SYSTEM_NAGIOS,
	               "expect", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("MONITORING_SYSTEM_HAPI_ZABBIX",
	               "data", G_TYPE_INT, (int)MONITORING_SYSTEM_HAPI_ZABBIX,
	               "expect", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("MONITORING_SYSTEM_HAPI_NAGIOS",
	               "data", G_TYPE_INT, (int)MONITORING_SYSTEM_HAPI_NAGIOS,
	               "expect", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_isHatoholArmPlugin(gconstpointer data)
{
	const bool expect = gcut_data_get_boolean(data, "expect");
	const bool actual =
	  DBTablesConfig::isHatoholArmPlugin(
	    (MonitoringSystemType)gcut_data_get_int(data, "data"));
	cppcut_assert_equal(expect, actual);
}

void test_getArmPluginInfo(void)
{
	loadTestDBArmPlugin();

	ArmPluginInfoVect armPluginInfoVect;
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.getArmPluginInfo(armPluginInfoVect);

	// check
	cppcut_assert_equal((size_t)NumTestArmPluginInfo,
	                    armPluginInfoVect.size());
	for (size_t i = 0; i < armPluginInfoVect.size(); i++) {
		const ArmPluginInfo &expect = testArmPluginInfo[i];
		const ArmPluginInfo &actual = armPluginInfoVect[i];
		const int expectId = i + 1;
		assertArmPluginInfo(expect, actual, &expectId);
	}
}

void test_getArmPluginInfoWithType(void)
{
	loadTestDBServer();
	loadTestDBArmPlugin();

	DECLARE_DBTABLES_CONFIG(dbConfig);
	const int targetIdx = 0;
	const ArmPluginInfo &expect = testArmPluginInfo[targetIdx];
	ArmPluginInfo armPluginInfo;
	cppcut_assert_equal(true, dbConfig.getArmPluginInfo(armPluginInfo,
	                                                    expect.serverId));
	const int expectId = targetIdx + 1;
	assertArmPluginInfo(expect, armPluginInfo, &expectId);
}

void test_getArmPluginInfoWithTypeFail(void)
{
	loadTestDBArmPlugin();

	DECLARE_DBTABLES_CONFIG(dbConfig);
	ArmPluginInfo armPluginInfo;
	MonitoringSystemType invalidType =
	  static_cast<MonitoringSystemType>(NUM_MONITORING_SYSTEMS + 3);
	cppcut_assert_equal(
	  false, dbConfig.getArmPluginInfo(armPluginInfo, invalidType));
}

void test_saveArmPluginInfo(void)
{
	loadTestDBArmPlugin();
}

void test_saveArmPluginInfoWithInvalidType(void)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	ArmPluginInfo armPluginInfo = testArmPluginInfo[0];
	armPluginInfo.type = MONITORING_SYSTEM_NAGIOS;
	assertHatoholError(HTERR_INVALID_ARM_PLUGIN_TYPE,
	                   dbConfig.saveArmPluginInfo(armPluginInfo));
}

void test_saveArmPluginInfoWithNoPath(void)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	ArmPluginInfo armPluginInfo = testArmPluginInfo[0];
	armPluginInfo.path = "";
	assertHatoholError(HTERR_INVALID_ARM_PLUGIN_PATH,
	                   dbConfig.saveArmPluginInfo(armPluginInfo));
}

void test_saveArmPluginInfoInvalidId(void)
{
	loadTestDBArmPlugin();

	DECLARE_DBTABLES_CONFIG(dbConfig);
	ArmPluginInfo armPluginInfo = testArmPluginInfo[0];
	armPluginInfo.id = NumTestArmPluginInfo + 100;
	HatoholError err = dbConfig.saveArmPluginInfo(armPluginInfo);
	assertHatoholError(HTERR_INVALID_ARM_PLUGIN_ID, err);
}

void test_saveArmPluginInfoUpdate(void)
{
	loadTestDBArmPlugin();

	DECLARE_DBTABLES_CONFIG(dbConfig);
	const size_t targetIdx = 1;
	ArmPluginInfo armPluginInfo = testArmPluginInfo[targetIdx];
	armPluginInfo.id = targetIdx + 1;
	armPluginInfo.path = "/usr/lib/dog";
	armPluginInfo.brokerUrl = "abc.example.com:28765";
	armPluginInfo.staticQueueAddress =
	  "b4e28ba-2fa1-11d2-883f-b9a761bde3fb";
	armPluginInfo.serverId = 52343;
	assertHatoholError(HTERR_OK, dbConfig.saveArmPluginInfo(armPluginInfo));

	// check
	const string statement = "SELECT * FROM arm_plugins ORDER BY type ASC";
	string expect;
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		if (i == targetIdx)
			expect += makeExpectedDBOutLine(i, armPluginInfo);
		else
			expect += makeExpectedDBOutLine(i, testArmPluginInfo[i]);
		if (i < NumTestArmPluginInfo - 1)
			expect += "\n";
	}
	assertDBContent(&dbConfig.getDBAgent(), statement, expect);
}

void test_incidentTrackerQueryOptionForAdmin(void)
{
	IncidentTrackerQueryOption option(USER_ID_SYSTEM);
	cppcut_assert_equal(string(), option.getCondition());
}

void test_incidentTrackerQueryOptionForInvalidUser(void)
{
	IncidentTrackerQueryOption option(INVALID_USER_ID);
	cppcut_assert_equal(string("0"), option.getCondition());
}

void test_incidentTrackerQueryOptionWithId(void)
{
	IncidentTrackerQueryOption option(USER_ID_SYSTEM);
	option.setTargetId(2);
	cppcut_assert_equal(string("id=2"), option.getCondition());
}

void _assertAddIncidentTracker(
  IncidentTrackerInfo incidentTrackerInfo,
  const HatoholErrorCode expectedErrorCode,
  OperationPrivilege privilege = ALL_PRIVILEGES)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err;
	err = dbConfig.addIncidentTracker(incidentTrackerInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string expectedOut;
	if (expectedErrorCode == HTERR_OK)
		expectedOut = makeIncidentTrackerInfoOutput(
				incidentTrackerInfo);
	string statement("select * from incident_trackers");
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}
#define assertAddIncidentTracker(I,E,...) \
cut_trace(_assertAddIncidentTracker(I,E,##__VA_ARGS__))

void test_addIncidentTracker(void)
{
	IncidentTrackerInfo *testInfo = testIncidentTrackerInfo;
	assertAddIncidentTracker(*testInfo, HTERR_OK);
}

void test_addIncidentTrackerWithoutPrivilege(void)
{
	OperationPrivilegeFlag privilege = ALL_PRIVILEGES;
	OperationPrivilege::removeFlag(
	  privilege, OPPRVLG_CREATE_INCIDENT_SETTING);
	IncidentTrackerInfo *testInfo = testIncidentTrackerInfo;
	assertAddIncidentTracker(*testInfo, HTERR_NO_PRIVILEGE, privilege);
}

void test_addIncidentTrackerWithEmptyLoction(void)
{
	IncidentTrackerInfo testInfo = testIncidentTrackerInfo[0];
	testInfo.baseURL = string();
	assertAddIncidentTracker(testInfo, HTERR_NO_INCIDENT_TRACKER_LOCATION);
}

void test_addIncidentTrackerWithInvalieType(void)
{
	IncidentTrackerInfo testInfo = testIncidentTrackerInfo[0];
	testInfo.type = NUM_INCIDENT_TRACKERS;
	assertAddIncidentTracker(testInfo, HTERR_INVALID_INCIDENT_TRACKER_TYPE);
}

void _assertUpdateIncidentTracker(
  IncidentTrackerInfo incidentTrackerInfo,
  const HatoholErrorCode expectedErrorCode,
  OperationPrivilege privilege = ALL_PRIVILEGES)
{
	loadTestDBIncidentTracker();

	int targetId = incidentTrackerInfo.id;
	int targetIdx = targetId - 1;

	string expectedOut;
	if (expectedErrorCode == HTERR_OK)
		expectedOut = makeIncidentTrackerInfoOutput(
				incidentTrackerInfo);
	else
		expectedOut = makeIncidentTrackerInfoOutput(
				testIncidentTrackerInfo[targetIdx]);

	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err;
	err = dbConfig.updateIncidentTracker(incidentTrackerInfo, privilege);
	assertHatoholError(expectedErrorCode, err);

	string statement = StringUtils::sprintf(
	                     "select * from incident_trackers where id=%d",
			     targetId);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOut);
}
#define assertUpdateIncidentTracker(I,E,...) \
cut_trace(_assertUpdateIncidentTracker(I,E,##__VA_ARGS__))

void test_updateIncidentTracker(void)
{
	int targetId = 2;
	IncidentTrackerInfo incidentTrackerInfo = testIncidentTrackerInfo[0];
	incidentTrackerInfo.id = targetId;
	assertUpdateIncidentTracker(incidentTrackerInfo, HTERR_OK);
}

void test_updateIncidentTrackerWithoutPrivilege(void)
{
	int targetId = 2;
	IncidentTrackerInfo incidentTrackerInfo = testIncidentTrackerInfo[0];
	incidentTrackerInfo.id = targetId;
	OperationPrivilegeFlag privilege = ALL_PRIVILEGES;
	OperationPrivilege::removeFlag(
	  privilege, OPPRVLG_UPDATE_INCIDENT_SETTING);
	assertUpdateIncidentTracker(
	  incidentTrackerInfo, HTERR_NO_PRIVILEGE, privilege);
}

void test_updateIncidentTrackerWithEmptyLoction(void)
{
	int targetId = 1;
	IncidentTrackerInfo incidentTrackerInfo
	  = testIncidentTrackerInfo[targetId - 1];
	incidentTrackerInfo.id = targetId;
	incidentTrackerInfo.baseURL = string();
	assertUpdateIncidentTracker(
	  incidentTrackerInfo, HTERR_NO_INCIDENT_TRACKER_LOCATION);
}

static void addIncidentTracker(IncidentTrackerInfo *info)
{
	DECLARE_DBTABLES_CONFIG(dbConfig);
	OperationPrivilege privilege(ALL_PRIVILEGES);
	dbConfig.addIncidentTracker(*info, privilege);
}
#define assertAddIncidentTrackerToDB(X) \
cut_trace(_assertAddToDB<IncidentTrackerInfo>(X, addIncidentTracker))

void _assertGetIncidentTrackers(
  UserIdType userId, IncidentTrackerIdType targetId = ALL_INCIDENT_TRACKERS)
{
	string expectedText;
	size_t expectedSize = 0;
	OperationPrivilege privilege(userId);
	for (size_t i = 0; i < NumTestIncidentTrackerInfo; i++) {
		IncidentTrackerInfo &info = testIncidentTrackerInfo[i];
		assertAddIncidentTrackerToDB(&info);
		if (!privilege.has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS))
			continue;
		if (targetId != ALL_INCIDENT_TRACKERS && targetId != info.id)
			continue;
		expectedText += makeIncidentTrackerInfoOutput(info);
		++expectedSize;
	}

	IncidentTrackerInfoVect actual;
	IncidentTrackerQueryOption option(userId);
	if (targetId != ALL_INCIDENT_TRACKERS)
		option.setTargetId(targetId);
	DECLARE_DBTABLES_CONFIG(dbConfig);
	dbConfig.getIncidentTrackers(actual, option);
	cppcut_assert_equal(expectedSize, actual.size());

	string actualText;
	IncidentTrackerInfoVectIterator it = actual.begin();
	for (; it != actual.end(); ++it)
		actualText += makeIncidentTrackerInfoOutput(*it);
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetIncidentTrackers(U, ...) \
  cut_trace(_assertGetIncidentTrackers(U, ##__VA_ARGS__))

void test_getIncidentTrackers(void)
{
	loadTestDBTablesUser();

	assertGetIncidentTrackers(USER_ID_SYSTEM);
}

void test_getIncidentTrackersWithoutPrivilege(void)
{
	loadTestDBTablesUser();

	UserIdType userIdWithoutPrivilege
	  = findUserWithout(OPPRVLG_GET_ALL_INCIDENT_SETTINGS);
	assertGetIncidentTrackers(userIdWithoutPrivilege);
}

void test_getIncidentTrackersWithId(void)
{
	loadTestDBTablesUser();

	assertGetIncidentTrackers(USER_ID_SYSTEM,
				  NumTestIncidentTrackerInfo - 1);
}

void test_getIncidentTrackersByInvalidUser(void)
{
	loadTestDBTablesUser();

	assertGetIncidentTrackers(INVALID_USER_ID);
}

void test_deleteIncidentTracker(void)
{
	loadTestDBTablesUser();
	loadTestDBIncidentTracker();

	IncidentTrackerIdType incidentTrackerId = 1;
	OperationPrivilege privilege(
	  findUserWith(OPPRVLG_DELETE_INCIDENT_SETTING));
	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err = dbConfig.deleteIncidentTracker(incidentTrackerId,
							  privilege);
	assertHatoholError(HTERR_OK, err);

	IncidentTrackerIdSet incidentTrackerIdSet;
	incidentTrackerIdSet.insert(incidentTrackerId);
	assertIncidentTrackersInDB(incidentTrackerIdSet);
}

void test_deleteIncidentTrackerWithoutPrivilege(void)
{
	loadTestDBTablesUser();
	loadTestDBIncidentTracker();

	IncidentTrackerIdType incidentTrackerId = 1;
	OperationPrivilege privilege;
	DECLARE_DBTABLES_CONFIG(dbConfig);
	HatoholError err = dbConfig.deleteIncidentTracker(incidentTrackerId,
							  privilege);
	assertHatoholError(HTERR_NO_PRIVILEGE, err);

	assertIncidentTrackersInDB(EMPTY_INCIDENT_TRACKER_ID_SET);
}

} // namespace testDBTablesConfig
