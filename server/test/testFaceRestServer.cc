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
	loadTestDBServerType();
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
	  DBTablesConfig::getDefaultPluginPath(armPluginInfo.type,
					       armPluginInfo.uuid);
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
		const MonitoringServerInfo &svInfo = testServerInfo[i];
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
		assertValueInParser(parser, "baseURL",
				    svInfo.baseURL);
		assertValueInParser(parser, "extendedInfo",
				    svInfo.extendedInfo);
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
	assertAddServer(params, userId, expectCode,
	                testServerInfo[NumTestServerInfo-1].id + 1);
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
		const string serverIdStr = to_string(*serverIdItr);
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
  const MonitoringServerInfo &svInfo, const ArmPluginInfo &armInfo,
  StringMap &dest)
{
	dest["type"] = to_string(svInfo.type);
	dest["hostName"] = svInfo.hostName;
	dest["ipAddress"] = svInfo.ipAddress;
	dest["nickname"] = svInfo.nickname;
	dest["port"] = to_string(svInfo.port);
	dest["userName"] = svInfo.userName;
	dest["password"] = svInfo.password;
	dest["dbName"] = svInfo.dbName;
	dest["pollingInterval"]
	  = to_string(svInfo.pollingIntervalSec);
	dest["retryInterval"]
	  = to_string(svInfo.retryIntervalSec);
	dest["baseURL"] = svInfo.baseURL;
	dest["extendedInfo"] = svInfo.extendedInfo;

	dest["brokerUrl"] = armInfo.brokerUrl;
	dest["staticQueueAddress"] = armInfo.staticQueueAddress;
	dest["tlsCertificatePath"] = armInfo.tlsCertificatePath;
	dest["tlsKeyPath"] = armInfo.tlsKeyPath;
	dest["tlsCACertificatePath"] = armInfo.tlsCACertificatePath;
	if (armInfo.tlsEnableVerify)
		dest["tlsEnableVerify"] = "true";
	dest["uuid"] = armInfo.uuid;
}

void test_addServer(void)
{
	MonitoringServerInfo svInfo;
	MonitoringServerInfo::initialize(svInfo);
	svInfo.id = AUTO_INCREMENT_VALUE;
	svInfo.type = MONITORING_SYSTEM_HAPI2;

	ArmPluginInfo armInfo;
	ArmPluginInfo::initialize(armInfo);
	armInfo.id = AUTO_INCREMENT_VALUE;
	armInfo.type = MONITORING_SYSTEM_HAPI2;
	armInfo.brokerUrl = "amqp://foo:goo@abc.example.com/vpath";
	armInfo.staticQueueAddress = "static-queue";
	armInfo.tlsCertificatePath = "/etc/hatohol/client-cert.pem";
	armInfo.tlsKeyPath = "/etc/hatohol/key.pem";
	armInfo.tlsCACertificatePath = "/etc/hatohol/ca-cert.pem";

	StringMap params;
	serverInfo2StringMap(svInfo, armInfo, params);
	assertAddServerWithSetup(params, HTERR_OK);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	string statement = "select * from servers ";
	statement += " order by id desc limit 1";

	svInfo.id = testServerInfo[NumTestServerInfo-1].id + 1;
	string expectedOutput = makeServerInfoOutput(svInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);

	statement = "select * from arm_plugins order by id desc limit 1";

	armInfo.id = NumTestArmPluginInfo + 1;
	// Should we search path from testServerTypeInfo ?
	armInfo.path = "/usr/sbin/hatohol-arm-plugin-ver2";
	armInfo.serverId = svInfo.id;
	expectedOutput = makeArmPluginInfoOutput(armInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_addServerHapiJSON(void)
{
	MonitoringServerInfo expected;
	MonitoringServerInfo::initialize(expected);
	expected.id = testServerInfo[NumTestServerInfo-1].id + 1;
	expected.type = MONITORING_SYSTEM_HAPI_JSON;
	expected.nickname = "JSON";
	expected.hostName = "";
	expected.ipAddress = "";

	ArmPluginInfo armPluginInfo;
	setupArmPluginInfo(armPluginInfo, expected);
	armPluginInfo.id = NumTestArmPluginInfo + 1;
	armPluginInfo.tlsCertificatePath = "/etc/hatohol/client-cert.pem";
	armPluginInfo.tlsKeyPath = "/etc/hatohol/key.pem";
	armPluginInfo.tlsCACertificatePath = "/etc/hatohol/ca-cert.pem";

	StringMap params;
	params["type"] = to_string(expected.type);
	params["nickname"] = expected.nickname;
	params["brokerUrl"] = armPluginInfo.brokerUrl;
	params["tlsCertificatePath"] = armPluginInfo.tlsCertificatePath;
	params["tlsKeyPath"] = armPluginInfo.tlsKeyPath;
	params["tlsCACertificatePath"] = armPluginInfo.tlsCACertificatePath;
	if (armPluginInfo.tlsEnableVerify)
		params["tlsEnableVerify"] = "true";
	params["uuid"] = armPluginInfo.uuid;

	assertAddServerWithSetup(params, HTERR_OK);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();

	string statement = "select * from servers";
	statement += " order by id desc limit 1";
	expected.hostName = expected.nickname;
	string expectedOutput = makeServerInfoOutput(expected);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);

	statement = "select * from arm_plugins";
	statement += " order by id desc limit 1";
	expectedOutput = makeArmPluginInfoOutput(armPluginInfo);
	assertDBContent(&dbConfig.getDBAgent(), statement, expectedOutput);
}

void test_addServerWithoutNickname(void)
{
	MonitoringServerInfo serverInfo = testServerInfo[0];
	ArmPluginInfo armInfo = testArmPluginInfo[0];
	StringMap params;
	serverInfo2StringMap(serverInfo, armInfo, params);
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
	ArmPluginInfo armPluginInfo = testArmPluginInfo[serverIndex];

	// Although cut_setup() loads test servers in the DB, updateServer()
	// needs the call of addTargetServer() before it. Or the internal
	// state becomes wrong and an error is returned.
	srcSvInfo.id = AUTO_INCREMENT_VALUE;
	assertHatoholError(
	  HTERR_OK,
	  uds->addTargetServer(srcSvInfo, armPluginInfo,
	                       OperationPrivilege(USER_ID_SYSTEM), false)
	);

	MonitoringServerInfo updateSvInfo = testServerInfo[1]; // make a copy;
	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_ALL_SERVER);
	string url = StringUtils::sprintf("/server/%" FMT_SERVER_ID,
	                                  srcSvInfo.id);
	StringMap params;
	serverInfo2StringMap(updateSvInfo, armPluginInfo, params);

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

void test_deleteServer(void)
{
	startFaceRest();

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	ArmPluginInfo armPluginInfo = testArmPluginInfo[0];

	// a copy is necessary not to change the source.
	MonitoringServerInfo targetSvInfo = testServerInfo[0];
	targetSvInfo.id = AUTO_INCREMENT_VALUE;
	assertHatoholError(
	  HTERR_OK,
	  uds->addTargetServer(targetSvInfo, armPluginInfo,
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

	set<size_t> remainingIndexes;
	for (size_t i = 0; i < NumTestServerTypeInfo; i++)
		remainingIndexes.insert(i);

	auto extractIndex = [&](const int &type, const string &uuid) {
		for (const auto idx: remainingIndexes) {
			const ServerTypeInfo &svti = testServerTypeInfo[idx];
			if (svti.type == type && svti.uuid == uuid) {
				remainingIndexes.erase(idx);
				return idx;
			}
		}
		cut_fail("Not found: type: %d, uuid: %s", type, uuid.c_str());
		return 0ul; // This line is never invoked, just to pass build.
	};

	for (size_t i = 0; i < NumTestServerTypeInfo; i++) {
		int64_t type;
		string uuid;
		cppcut_assert_equal(true, g_parser->startElement(i));
		cppcut_assert_equal(true, g_parser->read("type", type));
		cppcut_assert_equal(true, g_parser->read("uuid", uuid));
		const size_t idx = extractIndex(type, uuid);
		const ServerTypeInfo &svTypeInfo = testServerTypeInfo[idx];
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
