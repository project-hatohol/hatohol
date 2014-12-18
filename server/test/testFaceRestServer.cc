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
#include "FaceRest.h"
#include "Helpers.h"
#include "ThreadLocalDBCache.h"
#include "DBTablesTest.h"
#include "UnifiedDataStore.h"
#include "HatoholArmPluginInterface.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestServer {

static JSONParser *g_parser = NULL;

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBUser();
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;
}

static void setupArmPluginInfo(
  ArmPluginInfo &armPluginInfo, const MonitoringServerInfo &serverInfo)
{
	armPluginInfo.id = AUTO_INCREMENT_VALUE;
	armPluginInfo.type = serverInfo.type;
	armPluginInfo.path =
	  DBTablesConfig::getDefaultPluginPath(armPluginInfo.type);
	armPluginInfo.brokerUrl = "abc.example.com:22222";
	armPluginInfo.staticQueueAddress = "";
	armPluginInfo.serverId = serverInfo.id;
	armPluginInfo.tlsEnableVerify = 1;
}

static void assertServersInParser(
  JSONParser *parser, bool shouldHaveAccountInfo = true)
{
	assertValueInParser(parser, "numberOfServers", NumTestServerInfo);
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		parser->startElement(i);
		MonitoringServerInfo &svInfo = testServerInfo[i];
		assertValueInParser(parser, "id",   svInfo.id);
		assertValueInParser(parser, "type", svInfo.type);
		assertValueInParser(parser, "hostName",  svInfo.hostName);
		assertValueInParser(parser, "ipAddress", svInfo.ipAddress);
		assertValueInParser(parser, "nickname",  svInfo.nickname);
		assertValueInParser(parser, "port",  svInfo.port);
		assertValueInParser(parser, "pollingInterval",
				    svInfo.pollingIntervalSec);
		assertValueInParser(parser, "retryInterval",
				    svInfo.retryIntervalSec);
		if (shouldHaveAccountInfo) {
			assertValueInParser(parser, "userName",
					    svInfo.userName);
			assertValueInParser(parser, "password",
					    svInfo.password);
			assertValueInParser(parser, "dbName",
					    svInfo.dbName);
		} else {
			assertNoValueInParser(parser, "userName");
			assertNoValueInParser(parser, "password");
			assertNoValueInParser(parser, "dbName");
		}
		parser->endElement();
	}
	parser->endObject();
}

static void _assertServers(const string &path, const string &callbackName = "")
{
	startFaceRest();

	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertServersInParser(g_parser);
}
#define assertServers(P,...) cut_trace(_assertServers(P,##__VA_ARGS__))

#define assertAddServer(P, ...) \
cut_trace(_assertAddRecord(P, "/server", ##__VA_ARGS__))

void _assertAddServerWithSetup(const StringMap &params,
			       const HatoholErrorCode &expectCode)
{
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_SERVER);
	assertAddServer(params, userId, expectCode, NumTestServerInfo + 1);
}
#define assertAddServerWithSetup(P,C) cut_trace(_assertAddServerWithSetup(P,C))

static void _assertServerConnStat(JSONParser *parser)
{
	// Make expected data
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	ServerIdSet expectIdSet;
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(USER_ID_SYSTEM),
	                             false);
	dbConfig.getServerIdSet(expectIdSet, dqCtxPtr);
	cppcut_assert_equal(true, expectIdSet.size() > 1);

	// Check
	const string initTimeStr = (string)SmartTime();
	ServerIdSetIterator serverIdItr;
	assertStartObject(parser, "serverConnStat");
	while (!expectIdSet.empty()) {
		serverIdItr = expectIdSet.begin();
		const string serverIdStr = StringUtils::toString(*serverIdItr);
		assertStartObject(parser, serverIdStr);
		assertValueInParser(parser, "running", false);
		assertValueInParser(parser, "status", ARM_WORK_STAT_INIT);
		assertValueInParser(parser, "statUpdateTime",  initTimeStr);
		assertValueInParser(parser, "lastSuccessTime", initTimeStr);
		assertValueInParser(parser, "lastFailureTime", initTimeStr);
		assertValueInParser(parser, "failureComment", string(""));
		assertValueInParser(parser, "numUpdate",      0);
		assertValueInParser(parser, "numFailure",     0);
		parser->endObject(); // serverId
		expectIdSet.erase(serverIdItr);
	}
	parser->endObject(); // serverConnStat
}
#define assertServerConnStat(P) cut_trace(_assertServerConnStat(P))

void test_servers(void)
{
	assertServers("/server");
}

void test_serversJSONP(void)
{
	assertServers("/server", "foo");
}

void test_serversWithoutUpdatePrivilege(void)
{
	startFaceRest();

	RequestArg arg("/server");
	OperationPrivilegeFlag excludeFlags =
	  (1 << OPPRVLG_UPDATE_ALL_SERVER) | (1 << OPPRVLG_UPDATE_SERVER);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER, excludeFlags);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	bool shouldHaveAccountInfo = true;
	assertServersInParser(g_parser, !shouldHaveAccountInfo);
}

static void serverInfo2StringMap(
  const MonitoringServerInfo &src, StringMap &dest)
{
	dest["type"] = StringUtils::toString(src.type);
	dest["hostName"] = src.hostName;
	dest["ipAddress"] = src.ipAddress;
	dest["nickname"] = src.nickname;
	dest["port"] = StringUtils::toString(src.port);
	dest["userName"] = src.userName;
	dest["password"] = src.password;
	dest["dbName"] = src.dbName;
	dest["pollingInterval"]
	  = StringUtils::toString(src.pollingIntervalSec);
	dest["retryInterval"]
	  = StringUtils::toString(src.retryIntervalSec);
}

void test_addServer(void)
{
	MonitoringServerInfo expected;
	MonitoringServerInfo::initialize(expected);
	expected.id = NumTestServerInfo + 1;
	expected.type = MONITORING_SYSTEM_FAKE;

	StringMap params;
	serverInfo2StringMap(expected, params);
	assertAddServerWithSetup(params, HTERR_OK);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	string statement = "select * from servers ";
	statement += " order by id desc limit 1";
	string expectedOutput = makeServerInfoOutput(expected);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_addServerWithHapiParams(void)
{
	MonitoringServerInfo expected;
	MonitoringServerInfo::initialize(expected);
	expected.id = NumTestServerInfo + 1;
	expected.type = MONITORING_SYSTEM_HAPI_ZABBIX;

	ArmPluginInfo armPluginInfo;
	setupArmPluginInfo(armPluginInfo, expected);
	armPluginInfo.id = 1; // We suppose all entries have been deleted

	StringMap params;
	serverInfo2StringMap(expected, params);
	params["passiveMode"] = "false";
	params["brokerUrl"] = armPluginInfo.brokerUrl;
	assertAddServerWithSetup(params, HTERR_OK);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	string statement = "select * from arm_plugins";
	statement += " order by id desc limit 1";
	string expectedOutput = makeArmPluginInfoOutput(armPluginInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_addServerWithoutNickname(void)
{
	MonitoringServerInfo serverInfo = testServerInfo[0];
	StringMap params;
	serverInfo2StringMap(serverInfo, params);
	params.erase("nickname");
	assertAddServerWithSetup(params, HTERR_NOT_FOUND_PARAMETER);

	ServerIdSet serverIdSet;
	assertServersInDB(serverIdSet);
}

void data_updateServer(void)
{
	gcut_add_datum("Test server 1", "index", G_TYPE_INT, 0, NULL);
	gcut_add_datum("Test server 3", "index", G_TYPE_INT, 2, NULL);
}

void test_updateServer(gconstpointer data)
{
	startFaceRest();

	// a copy is necessary not to change the source.
	const int serverIndex = gcut_data_get_int(data, "index");
	MonitoringServerInfo srcSvInfo = testServerInfo[serverIndex];
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	ArmPluginInfo *armPluginInfo = NULL;
	assertHatoholError(
	  HTERR_OK,
	  uds->addTargetServer(srcSvInfo, *armPluginInfo,
	                       OperationPrivilege(USER_ID_SYSTEM), false)
	);

	MonitoringServerInfo updateSvInfo = testServerInfo[1]; // make a copy;
	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_ALL_SERVER);
	string url = StringUtils::sprintf("/server/%" FMT_SERVER_ID,
	                                  srcSvInfo.id);
	StringMap params;
	serverInfo2StringMap(updateSvInfo, params);

	// send a request
	RequestArg arg(url);
	arg.parameters = params;
	arg.request = "PUT";
	arg.userId = userId;
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "id", srcSvInfo.id);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	string statement = StringUtils::sprintf(
	                     "select * from servers where id=%d", srcSvInfo.id);
	updateSvInfo.id = srcSvInfo.id;
	string expectedOutput = makeServerInfoOutput(updateSvInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_updateServerWithArmPlugin(void)
{
	startFaceRest();

	// a copy is necessary not to change the source.
	MonitoringServerInfo serverInfo;
	MonitoringServerInfo::initialize(serverInfo);
	serverInfo.type = MONITORING_SYSTEM_HAPI_ZABBIX;
	ArmPluginInfo armPluginInfo;
	setupArmPluginInfo(armPluginInfo, serverInfo);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK,
	  uds->addTargetServer(serverInfo, armPluginInfo,
	                       OperationPrivilege(USER_ID_SYSTEM), false)
	);

	// Make an updated data
	serverInfo.hostName = "ume.tree.com";
	serverInfo.pollingIntervalSec = 30;
	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_ALL_SERVER);
	string url = StringUtils::sprintf("/server/%" FMT_SERVER_ID,
	                                  serverInfo.id);
	StringMap params;
	serverInfo2StringMap(serverInfo, params);
	armPluginInfo.brokerUrl = "tosaken.dog.exmaple.com:3322";
	armPluginInfo.staticQueueAddress = "address-for-cats";
	params["brokerUrl"] = armPluginInfo.brokerUrl;
	params["staticQueueAddress"] = armPluginInfo.staticQueueAddress;

	// send a request
	RequestArg arg(url);
	arg.parameters = params;
	arg.request = "PUT";
	arg.userId = userId;
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "id", serverInfo.id);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	string statement = StringUtils::sprintf(
	  "SELECT * FROM servers WHERE id=%d", serverInfo.id);
	// TODO: serverInfo2StringMap() doesn't set dbName. Is this OK ?
	string expectedOutput = makeServerInfoOutput(serverInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);

	statement = "SELECT * FROM arm_plugins ORDER BY id DESC LIMIT 1";
	expectedOutput = makeArmPluginInfoOutput(armPluginInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_deleteServer(void)
{
	startFaceRest();

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	ArmPluginInfo *armPluginInfo = NULL;

	// a copy is necessary not to change the source.
	MonitoringServerInfo targetSvInfo = testServerInfo[0];

	assertHatoholError(
	  HTERR_OK,
	  uds->addTargetServer(targetSvInfo, *armPluginInfo,
	                       OperationPrivilege(USER_ID_SYSTEM), false)
	);

	const ServerIdType targetServerId = targetSvInfo.id;
	const UserIdType userId = findUserWith(OPPRVLG_DELETE_SERVER);
	string url = StringUtils::sprintf("/server/%" FMT_SERVER_ID,
					  targetServerId);
	RequestArg arg(url);
	arg.request = "DELETE";
	arg.userId = userId;
	g_parser = getResponseAsJSONParser(arg);

	// check the reply
	assertErrorCode(g_parser);
	ServerIdSet serverIdSet;
	serverIdSet.insert(targetServerId);
	assertServersInDB(serverIdSet);
}

void test_getServerType(void)
{
	loadTestDBServerType();
	startFaceRest();
	UnifiedDataStore::getInstance()->start(false);

	RequestArg arg("/server-type");
	arg.userId = 1; // Any user OK.
	g_parser = getResponseAsJSONParser(arg);

	assertStartObject(g_parser, "serverType");
	cppcut_assert_equal(NumTestServerTypeInfo,
	                    (size_t)g_parser->countElements());
	for (size_t i = 0; i < NumTestServerTypeInfo; i++) {
		cppcut_assert_equal(true, g_parser->startElement(i));
		const ServerTypeInfo &svTypeInfo = testServerTypeInfo[i];
		assertValueInParser(g_parser, "type", svTypeInfo.type);
		assertValueInParser(g_parser, "name", svTypeInfo.name);
		assertValueInParser(g_parser, "parameters",
		                    svTypeInfo.parameters);
		g_parser->endElement();
	}
	g_parser->endObject(); // serverType
}

void test_getServerConnStat(void)
{
	startFaceRest();
	UnifiedDataStore::getInstance()->start(false);

	RequestArg arg("/server-conn-stat");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertServerConnStat(g_parser);
}

} // namespace testFaceRestServer
