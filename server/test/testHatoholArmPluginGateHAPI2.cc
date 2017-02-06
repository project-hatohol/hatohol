/*
 * Copyright (C) 2015 Project Hatohol
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

#include <gcutter.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <algorithm>
#include <Hatohol.h>
#include <HatoholArmPluginGateHAPI2.h>
#include <AMQPConnection.h>
#include <AMQPPublisher.h>
#include <AMQPConsumer.h>
#include <AMQPMessageHandler.h>
#include <ThreadLocalDBCache.h>
#include "Helpers.h"
#include "DBTablesTest.h"

#include <StringUtils.h>

using namespace std;
using namespace mlpl;

const static MonitoringServerInfo monitoringServerInfo = {
	302,                      // id
	MONITORING_SYSTEM_HAPI2,  // type
	"HAPI2 Zabbix",           // hostname
	"10.0.0.33",              // ip_address
	"HAPI2 Zabbix",           // nickname
	80,                       // port
	300,                      // polling_interval_sec
	60,                       // retry_interval_sec
	"Admin",                  // user_name
	"zabbix",                 // password
	"",                       // db_name
	"http://10.0.0.33/zabbix/", // base_url
	"test extended info",     // exteneded_info
};

const static ArmPluginInfo armPluginInfo = {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"",                              // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	302,                             // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"8e632c14-d1f7-11e4-8350-d43d7e3146fb", // uuid
};

const static ServerHostDef dummyHosts[] = {
{
	AUTO_INCREMENT_VALUE, // 1       // id
	10,                              // hostId
	302,                             // serverId
	"1",                             // hostIdInServer
	"exampleHostName1",              // name
}, {
	AUTO_INCREMENT_VALUE, // 2       // id
	13,                              // hostId
	302,                             // serverId
	"3",                             // hostIdInServer
	"exampleHostName",               // name
}
};

static void loadDummyHosts(void)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < ARRAY_SIZE(dummyHosts); i++)
		dbHost.upsertServerHostDef(dummyHosts[i]);
}

namespace testHAPI2ParseTimeStamp {

void test_fullFormat(void) {
	timespec timeStamp;
	// UTC: 2015-05017 16:00:00
	// JST: 2015-05018 01:00:00
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp("20150517160000.123456789",
						    timeStamp);
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(static_cast<time_t>(1431878400),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(123456789),
			    timeStamp.tv_nsec);
}

void test_noNanoSecondField(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp("20150517160000",
						    timeStamp);
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(static_cast<time_t>(1431878400),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(0),
			    timeStamp.tv_nsec);
}

void test_shortNanoSecondField(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp("20150517160000.12",
						    timeStamp);
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(static_cast<time_t>(1431878400),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(120000000),
			    timeStamp.tv_nsec);
}

void test_nanoSecondFieldWithLeadingZero(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp("20150517160000.012",
						    timeStamp);
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(static_cast<time_t>(1431878400),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(12000000),
			    timeStamp.tv_nsec);
}

void test_emptyTimeStamp(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp(string(), timeStamp);
	cppcut_assert_equal(false, succeeded);
	cppcut_assert_equal(static_cast<time_t>(0),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(0),
			    timeStamp.tv_nsec);
}

void test_allowEmptyTimeStamp(void) {
	const bool allowEmpty = true;
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp(string(), timeStamp,
						    allowEmpty);
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(static_cast<time_t>(0),
			    timeStamp.tv_sec);
	cppcut_assert_equal(static_cast<long>(0),
			    timeStamp.tv_nsec);
}

void test_invalidDateTime(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp(
	    "2015-05-17 16:00:00.123456789", timeStamp);
	cppcut_assert_equal(false, succeeded);
	cppcut_assert_equal((time_t) 0,
			    timeStamp.tv_sec);
	cppcut_assert_equal((long) 0,
			    timeStamp.tv_nsec);
}

void test_invalidNanoSecond(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp(
	    "20150517160000.1234a6789", timeStamp);
	cppcut_assert_equal(false, succeeded);
	cppcut_assert_equal((time_t) 1431878400,
			    timeStamp.tv_sec);
	cppcut_assert_equal((long) 0,
			    timeStamp.tv_nsec);
}

void test_tooLongNanoSecond(void) {
	timespec timeStamp;
	bool succeeded =
	  HatoholArmPluginGateHAPI2::parseTimeStamp(
	    "20150517160000.1234567890", timeStamp);
	cppcut_assert_equal(false, succeeded);
	cppcut_assert_equal((time_t) 1431878400,
			    timeStamp.tv_sec);
	cppcut_assert_equal((long) 0,
			    timeStamp.tv_nsec);
}

}

namespace testHatoholArmPluginGateHAPI2 {

namespace testProcedureHandlers {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();

	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	MonitoringServerInfo serverInfo = monitoringServerInfo;
	ArmPluginInfo pluginInfo = armPluginInfo;
	dbConfig.addTargetServer(serverInfo, pluginInfo, privilege);
}

void cut_teardown(void)
{
}

void test_new(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
		make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	cut_assert_not_null(gate.get());
}

void test_procedureHandlerExchangeProfile(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
		make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"exchangeProfile\","
		" \"params\":{\"procedures\":[\"getMonitoringServerInfo\","
		" \"getLastInfo\", \"putItems\", \"putArmInfo\", \"fetchItems\"],"
		" \"name\":\"examplePlugin\"}, \"id\":123}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_EXCHANGE_PROFILE, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":{\"name\":\"" PACKAGE_NAME "\","
		  "\"procedures\":"
		  "[\"exchangeProfile\",\"getMonitoringServerInfo\",\"getLastInfo\","
		  "\"putItems\",\"putHistory\",\"putHosts\",\"putHostGroups\","
		  "\"putHostGroupMembership\",\"putTriggers\","
		  "\"putEvents\",\"putHostParents\",\"putArmInfo\""
		"]},\"id\":123}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerMonitoringServerInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"getMonitoringServerInfo\","
		" \"params\":\"\", \"id\":456}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_MONITORING_SERVER_INFO,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":{"
		 "\"serverId\":302,\"url\":\"http://10.0.0.33/zabbix/\","
		 "\"type\":\"8e632c14-d1f7-11e4-8350-d43d7e3146fb\","
		 "\"nickName\":\"HAPI2 Zabbix\",\"userName\":\"Admin\","
		 "\"password\":\"zabbix\",\"pollingIntervalSec\":300,"
		 "\"retryIntervalSec\":60,\"extendedInfo\":\"test extended info\""
		"},\"id\":456}";
	cppcut_assert_equal(expected, actual);
}

void data_procedureHandlerLastInfo(void)
{
	gcut_add_datum("host",
		       "serverId", G_TYPE_INT, 11,
	               "params", G_TYPE_STRING, "host",
	               "value", G_TYPE_STRING, "1431232440", NULL);
	gcut_add_datum("hostGroup",
		       "serverId", G_TYPE_INT, 1001,
	               "params", G_TYPE_STRING, "hostGroup",
	               "value", G_TYPE_STRING, "1431221640", NULL);
	gcut_add_datum("hostGroupMembership",
		       "serverId", G_TYPE_INT, 1002,
	               "params", G_TYPE_STRING, "hostGroupMembership",
	               "value", G_TYPE_STRING, "1431567240", NULL);
	gcut_add_datum("trigger",
		       "serverId", G_TYPE_INT, 1003,
	               "params", G_TYPE_STRING, "trigger",
	               "value", G_TYPE_STRING, "1431671640", NULL);
	gcut_add_datum("event",
		       "serverId", G_TYPE_INT, 10001,
	               "params", G_TYPE_STRING, "event",
	               "value", G_TYPE_STRING, "1431585240", NULL);
	gcut_add_datum("hostParent",
		       "serverId", G_TYPE_INT, 10002,
	               "params", G_TYPE_STRING, "hostParent",
	               "value", G_TYPE_STRING, "1431930840", NULL);
	gcut_add_datum("empty",
		       "serverId", G_TYPE_INT, 11,
	               "params", G_TYPE_STRING, "event",
	               "value", G_TYPE_STRING, "", NULL);
}

void test_procedureHandlerLastInfo(gconstpointer data)
{
	MonitoringServerInfo serverInfo = monitoringServerInfo;
	serverInfo.id = gcut_data_get_int(data, "serverId");
	loadTestDBLastInfo();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(serverInfo, false);
	string json =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"getLastInfo\","
			       " \"params\":\"%s\", \"id\":789}",
			       gcut_data_get_string(data, "params"));
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_LAST_INFO, parser);
	string expected =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\",\"result\":\"%s\",\"id\":789}",
			       gcut_data_get_string(data, "value"));
	cppcut_assert_equal(expected, actual);

	// DB assertions
	LastInfoType type;
	HatoholArmPluginGateHAPI2::convertLastInfoStrToType(
	  gcut_data_get_string(data, "params"), type);
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();

	LastInfoDefList lastInfoList;
	LastInfoQueryOption option(USER_ID_SYSTEM);
	option.setLastInfoType(type);
	option.setTargetServerId(serverInfo.id);
	auto err = dbLastInfo.getLastInfoList(lastInfoList, option);
	assertHatoholError(HTERR_OK, err);

	for (auto lastInfo : lastInfoList) {
		cppcut_assert_equal(lastInfo.dataType, type);
		string value = gcut_data_get_string(data, "value");
		cppcut_assert_equal(lastInfo.value, value);
		cppcut_assert_equal(lastInfo.serverId, serverInfo.id);
	}
}

void test_procedureHandlerLastInfoInvalidJSON(void)
{
	MonitoringServerInfo serverInfo = monitoringServerInfo;
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(serverInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"getLastInfo\","
		"\"id\":789}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_LAST_INFO, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":789,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member: 'params' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();

	LastInfoDefList lastInfoList;
	LastInfoQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverInfo.id);
	auto err = dbLastInfo.getLastInfoList(lastInfoList, option);
	assertHatoholError(HTERR_OK, err);
	cppcut_assert_equal(lastInfoList.size(), static_cast<size_t>(0));
}

void test_procedureHandlerPutItems(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":["
		// 1st item
		"{\"itemId\":\"1\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175523\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"},"
		// 2nd item
		" {\"itemId\":\"2\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175531\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\", \"category2\", \"category3\"], \"unit\":\"example unit\"},"
		// 3rd item
		" {\"itemId\":\"3\", \"hostId\":\"1\","
		" \"brief\":\"example wiht empty itemGroupName array\","
		" \"lastValueTime\":\"20151117095531\","
		" \"lastValue\":\"Alpha Beta Gamma\","
		" \"itemGroupName\":[], \"unit\":\"Kelvin\"}],"
		// others
		" \"fetchId\":\"1\"}, \"id\":83241245}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_ITEMS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":83241245}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	ItemInfoList itemInfoList;
	ItemsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getItemInfoList(itemInfoList, option);
	ItemInfoList expectedItemInfoList;
	ItemInfo item1, item2, item3;
	timespec timeStamp;
	string lastValueTime;

	lastValueTime = "20150410175523";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item1.globalId       = 1;
	item1.serverId       = 302;
	item1.id             = "1";
	item1.globalHostId   = 10;
	item1.hostIdInServer = "1";
	item1.brief          = "example brief";
	item1.lastValueTime  = timeStamp;
	item1.lastValue      = "example value";
	item1.categoryNames  = {"example name"};
	item1.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item1.delay          = 0;
	item1.unit           = "example unit";
	expectedItemInfoList.push_back(item1);

	lastValueTime = "20150410175531";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item2.globalId       = 2;
	item2.serverId       = 302;
	item2.id             = "2";
	item2.globalHostId   = 10;
	item2.hostIdInServer = "1";
	item2.brief          = "example brief";
	item2.lastValueTime  = timeStamp;
	item2.lastValue      = "example value";
	item2.categoryNames  = {"example name", "category2", "category3"};
	item2.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item2.delay          = 0;
	item2.unit           = "example unit";
	expectedItemInfoList.push_back(item2);

	lastValueTime = "20151117095531";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item3.globalId       = 3;
	item3.serverId       = 302;
	item3.id             = "3";
	item3.globalHostId   = 10;
	item3.hostIdInServer = "1";
	item3.brief          = "example wiht empty itemGroupName array";
	item3.lastValueTime  = timeStamp;
	item3.lastValue      = "Alpha Beta Gamma";
	item3.categoryNames.clear();
	item3.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item3.delay          = 0;
	item3.unit           = "Kelvin";
	expectedItemInfoList.push_back(item3);

	string actualOutput;
	for (auto itemInfo : itemInfoList) {
		actualOutput += makeItemOutput(itemInfo);
	}
	string expectedOutput;
	for (auto itemInfo : expectedItemInfoList) {
		expectedOutput += makeItemOutput(itemInfo);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutItemsWithDivideInfo(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":["
		// 1st item
		"{\"itemId\":\"1\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175523\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"},"
		// 2nd item
		" {\"itemId\":\"2\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175531\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"}],"
		// others
		" \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"  \"requestId\":\"2029dcdd-db29-4ac4-8006-3d975874b5a8\"}"
		" }, \"id\":83241245}";
	string json2 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":["
		// 3rd item
		" {\"itemId\":\"3\", \"hostId\":\"1\","
		" \"brief\":\"example wiht empty itemGroupName array\","
		" \"lastValueTime\":\"20151117095531\","
		" \"lastValue\":\"Alpha Beta Gamma\","
		" \"itemGroupName\":[], \"unit\":\"Kelvin\"}],"
		// others
		" \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"  \"requestId\":\"2029dcdd-db29-4ac4-8006-3d975874b5a8\"}"
		" }, \"id\":83241245}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_ITEMS, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":83241245}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_ITEMS, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":83241245}";
	cppcut_assert_equal(expected2, actual2);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	ItemInfoList itemInfoList;
	ItemsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getItemInfoList(itemInfoList, option);
	ItemInfoList expectedItemInfoList;
	ItemInfo item1, item2, item3;
	timespec timeStamp;
	string lastValueTime;

	lastValueTime = "20150410175523";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item1.globalId       = 1;
	item1.serverId       = 302;
	item1.id             = "1";
	item1.globalHostId   = 10;
	item1.hostIdInServer = "1";
	item1.brief          = "example brief";
	item1.lastValueTime  = timeStamp;
	item1.lastValue      = "example value";
	item1.categoryNames  = {"example name"};
	item1.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item1.delay          = 0;
	item1.unit           = "example unit";
	expectedItemInfoList.push_back(item1);

	lastValueTime = "20150410175531";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item2.globalId       = 2;
	item2.serverId       = 302;
	item2.id             = "2";
	item2.globalHostId   = 10;
	item2.hostIdInServer = "1";
	item2.brief          = "example brief";
	item2.lastValueTime  = timeStamp;
	item2.lastValue      = "example value";
	item2.categoryNames  = {"example name"};
	item2.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item2.delay          = 0;
	item2.unit           = "example unit";
	expectedItemInfoList.push_back(item2);

	lastValueTime = "20151117095531";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	item3.globalId       = 3;
	item3.serverId       = 302;
	item3.id             = "3";
	item3.globalHostId   = 10;
	item3.hostIdInServer = "1";
	item3.brief          = "example wiht empty itemGroupName array";
	item3.lastValueTime  = timeStamp;
	item3.lastValue      = "Alpha Beta Gamma";
	item3.categoryNames.clear();
	item3.valueType      = ITEM_INFO_VALUE_TYPE_UNKNOWN;
	item3.delay          = 0;
	item3.unit           = "Kelvin";
	expectedItemInfoList.push_back(item3);

	string actualOutput;
	for (auto itemInfo : itemInfoList) {
		actualOutput += makeItemOutput(itemInfo);
	}
	string expectedOutput;
	for (auto itemInfo : expectedItemInfoList) {
		expectedOutput += makeItemOutput(itemInfo);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutItemsInvalidJSON(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":[{\"itemId\":\"1\", "
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175523\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"},"
		" {\"itemId\":\"2\", \"hostId\":\"1\","
		" \"lastValueTime\":\"20150410175531\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"}],"
		" \"fetchId\":\"1\"}, \"id\":83241245}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_ITEMS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":83241245,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'hostId' does not exist.\","
		"\"Failed to parse mandatory member:"
		" 'brief' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	ItemInfoList itemInfoList;
	ItemsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getItemInfoList(itemInfoList, option);
	cppcut_assert_equal(itemInfoList.size(), static_cast<size_t>(0));
}

void test_procedureHandlerPutHistory(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"1\","
		" \"samples\":[{\"value\":\"exampleValue\","
		" \"time\":\"20150323113032.000000000\"},"
		"{\"value\":\"exampleValue2\",\"time\":\"20150323113033.000000000\"}],"
		" \"fetchId\":\"1\"}, \"id\":-83241245}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HISTORY, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":-83241245}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerPutHistoryWithDivideInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"1\","
		" \"samples\":[{\"value\":\"exampleValue\","
		"  \"time\":\"20150323113032.000000000\"},"
		"  {\"value\":\"exampleValue2\",\"time\":\"20150323113033.000000000\"}],"
		" \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"   \"requestId\":\"3aa730a1-53dd-4e58-a327-f486c841da6e\"}"
		"}, \"id\":-83241245}";
	string json2 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"1\","
		" \"samples\":[{\"value\":\"exampleValue\","
		" \"time\":\"20150323113034.000000000\"},"
		"{\"value\":\"exampleValue2\",\"time\":\"20150323113035.000000000\"}],"
		" \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"   \"requestId\":\"3aa730a1-53dd-4e58-a327-f486c841da6e\"}"
		"}, \"id\":-83241245}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_HISTORY, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":-83241245}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_HISTORY, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":-83241245}";
	cppcut_assert_equal(expected2, actual2);
}

void test_procedureHandlerPutHistoryInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"1\","
		" \"samples\":[{"
		" \"time\":\"20150323113032.000000000\"},"
		"{\"value\":\"exampleValue2\",\"time\":\"20150323113033.000000000\"}],"
		" \"fetchId\":\"1\"}, \"id\":-83241245}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HISTORY, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":-83241245,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member: 'value' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerPutHosts(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHosts\", \"params\":"
		"{\"hosts\":[{\"hostId\":\"1\", \"hostName\":\"exampleHostName1\"}],"
		" \"updateType\":\"UPDATED\",\"lastInfo\":\"201504091052\"},"
		" \"id\":\"deadbeaf\"}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOSTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"deadbeaf\"}";
	cppcut_assert_equal(expected, actual);

	ServerHostDefVect expectedHostVect =
	{
		{
			1,                       // id
			10,                      // hostId
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"exampleHostName1",      // name
		},
	};

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	ServerHostDefVect hostDefVect;
	HostsQueryOption option(USER_ID_SYSTEM);
	option.setStatusSet({HOST_STAT_NORMAL});
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getServerHostDefs(hostDefVect, option);
	string actualOutput;
	{
		size_t i = 0;
		for (auto host : hostDefVect) {
			actualOutput += makeHostsOutput(host, i);
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto host : expectedHostVect) {
			expectedOutput += makeHostsOutput(host, i);
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostsWithDivideInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHosts\", \"params\":"
		"{\"hosts\":[{\"hostId\":\"1\", \"hostName\":\"exampleHostName1\"}],"
		" \"updateType\":\"UPDATED\",\"lastInfo\":\"201504091052\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"   \"requestId\":\"aee4039b-29fe-4478-a2a2-99c0f4a4dbfd\"}"
		"}, \"id\":\"deadbeaf\"}";
	string json2 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHosts\", \"params\":"
		"{\"hosts\":[{\"hostId\":\"2\", \"hostName\":\"exampleHostName2\"}],"
		" \"updateType\":\"UPDATED\",\"lastInfo\":\"201604091500\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"   \"requestId\":\"aee4039b-29fe-4478-a2a2-99c0f4a4dbfd\"}"
		"}, \"id\":\"deadbeaf\"}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_HOSTS, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"deadbeaf\"}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_HOSTS, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"deadbeaf\"}";
	cppcut_assert_equal(expected2, actual2);

	ServerHostDefVect expectedHostVect =
	{
		{
			1,                       // id
			10,                      // hostId
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"exampleHostName1",      // name
		},
		{
			2,                       // id
			10,                      // hostId
			monitoringServerInfo.id, // serverId
			"2",                     // hostIdInServer
			"exampleHostName2",      // name
		},

	};

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	ServerHostDefVect hostDefVect;
	HostsQueryOption option(USER_ID_SYSTEM);
	option.setStatusSet({HOST_STAT_NORMAL});
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getServerHostDefs(hostDefVect, option);
	string actualOutput;
	{
		size_t i = 0;
		sort(begin(hostDefVect), end(hostDefVect),
		     [](ServerHostDef lhs, ServerHostDef rhs) -> int {
			     return lhs.hostIdInServer < rhs.hostIdInServer;
		     });
		for (auto host : hostDefVect) {
			actualOutput += makeHostsOutput(host, i);
			++i;
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto host : expectedHostVect) {
			expectedOutput += makeHostsOutput(host, i);
			++i;
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostsInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"updateHosts\","
		" \"params\":{\"hosts\":[{\"hostId\":\"1\"}],"
		" \"updateType\":\"UPDATED\",\"lastInfo\":\"201504091052\"},"
		" \"id\":\"deadbeaf\"}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOSTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":\"deadbeaf\","
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'hostName' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	ServerHostDefVect hostDefVect;
	HostsQueryOption option(USER_ID_SYSTEM);
	option.setStatusSet({HOST_STAT_NORMAL});
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getServerHostDefs(hostDefVect, option);
	cppcut_assert_equal(hostDefVect.size(), static_cast<size_t>(0));
}

void test_procedureHandlerPutHostGroups(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroups\","
		" \"params\":{\"hostGroups\":[{\"groupId\":\"1\","
		" \"groupName\":\"Group2\"}],\"updateType\":\"ALL\","
		" \"lastInfo\":\"20150409104900\"}, \"id\":\"123abc\"}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOST_GROUPS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"123abc\"}";
	cppcut_assert_equal(expected, actual);

	HostgroupVect expectedHostgroupVect =
	{
		{
			1,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // idInServer
			"Group2",                // name
		},
	};
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupVect hostgroupVect;
	HostgroupsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroups(hostgroupVect, option);
	string actualOutput;
	{
		size_t i = 0;
		for (auto hostgroup : hostgroupVect) {
			actualOutput += makeHostgroupsOutput(hostgroup, i);
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto hostgroup : expectedHostgroupVect) {
			expectedOutput += makeHostgroupsOutput(hostgroup, i);
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostGroupsWithDivideInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroups\","
		" \"params\":{\"hostGroups\":[{\"groupId\":\"1\","
		" \"groupName\":\"Group2\"}],\"updateType\":\"ALL\","
		" \"lastInfo\":\"20150409104900\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"   \"requestId\":\"7701d99e-94d3-4a71-9ad8-e8f0584d6b08\"}"
		"}, \"id\":\"123abc\"}";
	string json2 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroups\","
		" \"params\":{\"hostGroups\":[{\"groupId\":\"2\","
		" \"groupName\":\"Group3\"}],\"updateType\":\"ALL\","
		" \"lastInfo\":\"20160407152000\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"   \"requestId\":\"7701d99e-94d3-4a71-9ad8-e8f0584d6b08\"}"
		"}, \"id\":\"123abc\"}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_HOST_GROUPS,
					       parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"123abc\"}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_HOST_GROUPS,
					       parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"123abc\"}";
	cppcut_assert_equal(expected2, actual2);

	HostgroupVect expectedHostgroupVect =
	{
		{
			1,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // idInServer
			"Group2",                // name
		},
		{
			1,                       // id
			monitoringServerInfo.id, // serverId
			"2",                     // idInServer
			"Group3",                // name
		},
	};
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupVect hostgroupVect;
	HostgroupsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroups(hostgroupVect, option);
	string actualOutput;
	{
		size_t i = 0;
		for (auto hostgroup : hostgroupVect) {
			actualOutput += makeHostgroupsOutput(hostgroup, i);
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto hostgroup : expectedHostgroupVect) {
			expectedOutput += makeHostgroupsOutput(hostgroup, i);
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostGroupsInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroups\","
		" \"params\":{\"hostGroups\":[{\"groupId\":\"1\"}],"
		" \"updateType\":\"ALL\","
		" \"lastInfo\":\"20150409104900\"}, \"id\":\"123abc\"}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOST_GROUPS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":\"123abc\","
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'groupName' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupVect hostgroupVect;
	HostgroupsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroups(hostgroupVect, option);
	cppcut_assert_equal(hostgroupVect.size(), static_cast<size_t>(0));
}

void test_procedureHandlerPutHostGroupMembership(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroupMembership\","
		" \"params\":{\"hostGroupMembership\":[{\"hostId\":\"1\","
		" \"groupIds\":[\"1\", \"2\"]}, {\"hostId\":\"2\","
		" \"groupIds\":[\"2\", \"5\"]}, {\"hostId\":\"4\","
		" \"groupIds\":[\"5\"]}],"
		" \"lastInfo\":\"20150409105600\", \"updateType\":\"ALL\"},"
		" \"id\":9342}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":9342}";
	cppcut_assert_equal(expected, actual);

	HostgroupMemberVect expectedHostgroupMemberVect =
	{
		{
			1,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"1",                     // hostGroupIdInServer
			10,                      // hostId
		},
		{
			2,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"2",                     // hostGroupIdInServer
			10,                      // hostId
		},
		{
			3,                       // id
			monitoringServerInfo.id, // serverId
			"2",                     // hostIdInServer
			"2",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
		{
			4,                       // id
			monitoringServerInfo.id, // serverId
			"2",                     // hostIdInServer
			"5",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
		{
			5,                       // id
			monitoringServerInfo.id, // serverId
			"4",                     // hostIdInServer
			"5",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
	};

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupMemberVect hostgroupMemberVect;
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroupMembers(hostgroupMemberVect, option);
	string actualOutput;
	{
		size_t i = 0;
		for (auto hostgroupMember : hostgroupMemberVect) {
			actualOutput += makeMapHostsHostgroupsOutput(hostgroupMember, i);
			i++;
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto hostgroupMember : expectedHostgroupMemberVect) {
			expectedOutput += makeMapHostsHostgroupsOutput(hostgroupMember, i);
			i++;
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostGroupMembershipWithDivideInfo(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroupMembership\","
		" \"params\":{\"hostGroupMembership\":[{\"hostId\":\"1\","
		" \"groupIds\":[\"1\", \"2\"]}],"
		" \"lastInfo\":\"20150409105600\", \"updateType\":\"ALL\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"   \"requestId\":\"b6f13d37-0adc-4ded-aaec-823f8cf19bff\"}"
		"},"
		" \"id\":9342}";
	string json2 =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroupMembership\","
		" \"params\":{\"hostGroupMembership\":[{\"hostId\":\"2\","
		" \"groupIds\":[\"2\", \"5\"]}, {\"hostId\":\"4\","
		" \"groupIds\":[\"5\"]}],"
		" \"lastInfo\":\"20160407152500\", \"updateType\":\"ALL\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"   \"requestId\":\"b6f13d37-0adc-4ded-aaec-823f8cf19bff\"}"
		"},"
		" \"id\":9342}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":9342}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":9342}";
	cppcut_assert_equal(expected2, actual2);

	HostgroupMemberVect expectedHostgroupMemberVect =
	{
		{
			1,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"1",                     // hostGroupIdInServer
			10,                      // hostId
		},
		{
			2,                       // id
			monitoringServerInfo.id, // serverId
			"1",                     // hostIdInServer
			"2",                     // hostGroupIdInServer
			10,                      // hostId
		},
		{
			3,                       // id
			monitoringServerInfo.id, // serverId
			"2",                     // hostIdInServer
			"2",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
		{
			4,                       // id
			monitoringServerInfo.id, // serverId
			"2",                     // hostIdInServer
			"5",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
		{
			5,                       // id
			monitoringServerInfo.id, // serverId
			"4",                     // hostIdInServer
			"5",                     // hostGroupIdInServer
			INVALID_HOST_ID,         // hostId
		},
	};

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupMemberVect hostgroupMemberVect;
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroupMembers(hostgroupMemberVect, option);
	string actualOutput;
	{
		size_t i = 0;
		for (auto hostgroupMember : hostgroupMemberVect) {
			actualOutput += makeMapHostsHostgroupsOutput(hostgroupMember, i);
			i++;
		}
	}
	string expectedOutput;
	{
		size_t i = 0;
		for (auto hostgroupMember : expectedHostgroupMemberVect) {
			expectedOutput += makeMapHostsHostgroupsOutput(hostgroupMember, i);
			i++;
		}
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutHostGroupMembershipInvalidJSON(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putHostGroupMembership\","
		" \"params\":{\"hostGroupMembership\":[{\"hostId\":\"1\"}],"
		" \"lastInfo\":\"20150409105600\", \"updateType\":\"ALL\"},"
		" \"id\":9342}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(
	  HAPI2_PUT_HOST_GROUP_MEMEBRSHIP, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":9342,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":[\"Failed to parse mandatory object:"
		"  'groupIds' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	HostgroupMemberVect hostgroupMemberVect;
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbHost.getHostgroupMembers(hostgroupMemberVect, option);
	cppcut_assert_equal(hostgroupMemberVect.size(), static_cast<size_t>(0));
}

void test_procedureHandlerPutTriggers(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201504061606\", \"fetchId\":\"1\","
		" \"triggers\":[{\"triggerId\":\"1\", \"status\":\"OK\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323175800\","
		" \"hostId\":\"1\", \"hostName\":\"exampleHostName\","
		" \"brief\":\"example brief\","
		" \"extendedInfo\": \"sample extended info\"}]},\"id\":34031}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_TRIGGERS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":34031}";
	cppcut_assert_equal(expected, actual);
	timespec timeStamp;
	HatoholArmPluginGateHAPI2::parseTimeStamp("20150323175800", timeStamp);
	TriggerInfoList expectedTriggerInfoList =
	{
		{
			monitoringServerInfo.id, // serverId
			"1",                     // id
			TRIGGER_STATUS_OK,       // status
			TRIGGER_SEVERITY_INFO,   // severity
			timeStamp,               // lastChangeTime
			10,                      // globalHostId
			"1",                     // hostIdInServer
			"exampleHostName",       // hostName
			"example brief",         // brief
			"sample extended info",  // extendedInfo
			TRIGGER_VALID,           // validity
		}
	};

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	option.setExcludeFlags(EXCLUDE_SELF_MONITORING);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	string actualOutput;
	for (auto trigger : triggerInfoList) {
		actualOutput += makeTriggerOutput(trigger);
	}
	string expectedOutput;
	for (auto trigger : expectedTriggerInfoList) {
		expectedOutput += makeTriggerOutput(trigger);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutTriggersWithDivideInfo(void)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201504061606\", \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"   \"requestId\":\"62cf68a6-e6b4-4b88-9e0a-9a494b6b5d19\"},"
		" \"triggers\":[{\"triggerId\":\"1\", \"status\":\"OK\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323175800\","
		" \"hostId\":\"1\", \"hostName\":\"exampleHostName\","
		" \"brief\":\"example brief\","
		" \"extendedInfo\": \"sample extended info\"}]},\"id\":34031}";
	string json2 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201604071600\", \"fetchId\":\"1\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"   \"requestId\":\"62cf68a6-e6b4-4b88-9e0a-9a494b6b5d19\"},"
		" \"triggers\":[{\"triggerId\":\"2\", \"status\":\"NG\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323275800\","
		" \"hostId\":\"1\", \"hostName\":\"exampleHostName\","
		" \"brief\":\"example problem brief\","
		" \"extendedInfo\": \"sample extended problem info\"}]},\"id\":34031}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_TRIGGERS, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":34031}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_TRIGGERS, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":34031}";
	cppcut_assert_equal(expected2, actual2);
	timespec timeStamp1;
	HatoholArmPluginGateHAPI2::parseTimeStamp("20150323175800", timeStamp1);
	timespec timeStamp2;
	HatoholArmPluginGateHAPI2::parseTimeStamp("20150323275800", timeStamp2);
	TriggerInfoList expectedTriggerInfoList =
	{
		{
			monitoringServerInfo.id, // serverId
			"1",                     // id
			TRIGGER_STATUS_OK,       // status
			TRIGGER_SEVERITY_INFO,   // severity
			timeStamp1,              // lastChangeTime
			10,                      // globalHostId
			"1",                     // hostIdInServer
			"exampleHostName",       // hostName
			"example brief",         // brief
			"sample extended info",  // extendedInfo
			TRIGGER_VALID,           // validity
		},
		{
			monitoringServerInfo.id,       // serverId
			"2",                           // id
			TRIGGER_STATUS_PROBLEM,        // status
			TRIGGER_SEVERITY_INFO,         // severity
			timeStamp2,                    // lastChangeTime
			10,                            // globalHostId
			"1",                           // hostIdInServer
			"exampleHostName",             // hostName
			"example problem brief",       // brief
			"sample extended problem info", // extendedInfo
			TRIGGER_VALID,                 // validity
		}
	};

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	option.setExcludeFlags(EXCLUDE_SELF_MONITORING);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	string actualOutput;
	for (auto trigger : triggerInfoList) {
		actualOutput += makeTriggerOutput(trigger);
	}
	string expectedOutput;
	for (auto trigger : expectedTriggerInfoList) {
		expectedOutput += makeTriggerOutput(trigger);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutTriggersInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201504061606\", \"fetchId\":\"1\","
		" \"triggers\":[{\"triggerId\":\"1\", \"status\":\"OK\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323175800\","
		" \"hostName\":\"exampleHostName\","
		" \"extendedInfo\": \"sample extended info\"}]},\"id\":34031}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_TRIGGERS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":34031,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'hostId' does not exist.\","
		"\"Failed to parse mandatory member:"
		" 'brief' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	option.setExcludeFlags(EXCLUDE_SELF_MONITORING);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	cppcut_assert_equal(static_cast<size_t>(0), triggerInfoList.size());
}

void data_procedureHandlerPutEvents(void)
{
	gcut_add_datum("WithTriggerId",
	               "triggerIdContents", G_TYPE_STRING, " \"triggerId\":\"2\",",
		       "triggerId", G_TYPE_STRING, "2", NULL);
	gcut_add_datum("WithoutTriggerId",
	               "triggerIdContents", G_TYPE_STRING, "",
		       "triggerId", G_TYPE_STRING, DO_NOT_ASSOCIATE_TRIGGER_ID.c_str(), NULL);
}

void test_procedureHandlerPutEvents(gconstpointer data)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"1\","
			       " \"time\":\"20150323151300\", \"type\":\"GOOD\","
			       " %s \"status\": \"OK\", \"severity\":\"INFO\","
			       " \"hostId\":\"3\", \"hostName\":\"exampleHostName\","
			       " \"brief\":\"example brief\","
			       " \"extendedInfo\": \"sample extended info\"}],"
			       " \"lastInfo\":\"20150401175900\","
			       " \"fetchId\":\"1\"},\"id\":2374234}",
			       gcut_data_get_string(data, "triggerIdContents"));
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_EVENTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":2374234}";
	cppcut_assert_equal(expected, actual);

	timespec timeStamp;
	string lastValueTime = "20150323151300";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime, timeStamp);
	EventInfoList expectedEventInfoList =
	{
		{
			1,                                               // unifiedId
			monitoringServerInfo.id,                         // serverId
			"1",                                             // id
			timeStamp,                                       // time
			EVENT_TYPE_GOOD,                                 // type
			gcut_data_get_string(data, "triggerId"),         // triggerId
			TRIGGER_STATUS_OK,                               // status
			TRIGGER_SEVERITY_INFO,                           // severity
			13,                                              // globalHostId
			"3",                                             // hostIdInServer
			"exampleHostName",                               // hostName
			"example brief",                                 // brief
			"sample extended info",                         // extendedInfo
		},
	};

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getEventInfoList(eventInfoList, option);
	string actualOutput;
	for (auto eventInfo : eventInfoList) {
		actualOutput += makeEventOutput(eventInfo);
	}
	string expectedOutput;
	for (auto eventInfo : expectedEventInfoList) {
		expectedOutput += makeEventOutput(eventInfo);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void data_procedureHandlerPutEventsWithDivideInfo(void)
{
	gcut_add_datum("WithTriggerId",
	               "triggerIdContents", G_TYPE_STRING, " \"triggerId\":\"2\",",
		       "triggerId", G_TYPE_STRING, "2", NULL);
	gcut_add_datum("WithoutTriggerId",
	               "triggerIdContents", G_TYPE_STRING, "",
		       "triggerId", G_TYPE_STRING, DO_NOT_ASSOCIATE_TRIGGER_ID.c_str(), NULL);
}

void test_procedureHandlerPutEventsWithDivideInfo(gconstpointer data)
{
	loadDummyHosts();
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"1\","
			       " \"time\":\"20150323151300\", \"type\":\"GOOD\","
			       " %s \"status\": \"OK\", \"severity\":\"INFO\","
			       " \"hostId\":\"3\", \"hostName\":\"exampleHostName\","
			       " \"brief\":\"example brief\","
			       " \"extendedInfo\": \"sample extended info\"}],"
			       " \"lastInfo\":\"20150401175900\","
			       " \"fetchId\":\"1\","
			       " \"divideInfo\":"
			       "  {\"isLast\":false,\"serialId\":0,"
			       "  \"requestId\":\"979c30ce-2c60-428a-a17a-e8c4fe6960e4\"}"
			       " },\"id\":2374234}",
			       gcut_data_get_string(data, "triggerIdContents"));
	string json2 =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"2\","
			       " \"time\":\"20160407160000\", \"type\":\"GOOD\","
			       " %s \"status\": \"NG\", \"severity\":\"INFO\","
			       " \"hostId\":\"3\", \"hostName\":\"exampleHostName\","
			       " \"brief\":\"example problem brief\","
			       " \"extendedInfo\": \"sample extended problem info\"}],"
			       " \"lastInfo\":\"20160407161000\","
			       " \"fetchId\":\"1\","
			       " \"divideInfo\":"
			       "  {\"isLast\":true,\"serialId\":1,"
			       "  \"requestId\":\"979c30ce-2c60-428a-a17a-e8c4fe6960e4\"}"
			       " },\"id\":2374234}",
			       gcut_data_get_string(data, "triggerIdContents"));
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_EVENTS, parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":2374234}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_EVENTS, parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":2374234}";
	cppcut_assert_equal(expected2, actual2);

	timespec timeStamp1;
	string lastValueTime1 = "20150323151300";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime1, timeStamp1);
	timespec timeStamp2;
	string lastValueTime2 = "20160407160000";
	HatoholArmPluginGateHAPI2::parseTimeStamp(lastValueTime2, timeStamp2);
	EventInfoList expectedEventInfoList =
	{
		{
			1,                                               // unifiedId
			monitoringServerInfo.id,                         // serverId
			"1",                                             // id
			timeStamp1,                                      // time
			EVENT_TYPE_GOOD,                                 // type
			gcut_data_get_string(data, "triggerId"),         // triggerId
			TRIGGER_STATUS_OK,                               // status
			TRIGGER_SEVERITY_INFO,                           // severity
			13,                                              // globalHostId
			"3",                                             // hostIdInServer
			"exampleHostName",                               // hostName
			"example brief",                                 // brief
			"sample extended info",                         // extendedInfo
		},
		{
			2,                                               // unifiedId
			monitoringServerInfo.id,                         // serverId
			"2",                                             // id
			timeStamp2,                                      // time
			EVENT_TYPE_GOOD,                                 // type
			gcut_data_get_string(data, "triggerId"),         // triggerId
			TRIGGER_STATUS_PROBLEM,                          // status
			TRIGGER_SEVERITY_INFO,                           // severity
			13,                                              // globalHostId
			"3",                                             // hostIdInServer
			"exampleHostName",                               // hostName
			"example problem brief",                         // brief
			"sample extended problem info",                  // extendedInfo
		},
	};

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getEventInfoList(eventInfoList, option);
	string actualOutput;
	for (auto eventInfo : eventInfoList) {
		actualOutput += makeEventOutput(eventInfo);
	}
	string expectedOutput;
	for (auto eventInfo : expectedEventInfoList) {
		expectedOutput += makeEventOutput(eventInfo);
	}
	cppcut_assert_equal(expectedOutput, actualOutput);
}

void test_procedureHandlerPutEventsInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
		" \"params\":{\"events\":[{\"eventId\":\"1\","
		" \"time\":\"20150323151300\", \"type\":\"GOOD\","
		" \"status\": \"OK\", \"severity\":\"INFO\","
		" \"hostName\":\"exampleHostName\","
		" \"extendedInfo\": \"sample extended info\"}],"
		" \"lastInfo\":\"20150401175900\","
		" \"fetchId\":\"1\"},\"id\":2374234}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_EVENTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":2374234,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member: 'brief' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(monitoringServerInfo.id);
	dbMonitoring.getEventInfoList(eventInfoList, option);
	cppcut_assert_equal(eventInfoList.size(), static_cast<size_t>(1));
}

void test_procedureHandlerPutHostParents(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHostParent\","
		" \"params\":{\"hostParents\":"
		" [{\"childHostId\":\"12\",\"parentHostId\":\"10\"},"
		" {\"childHostId\":\"11\",\"parentHostId\":\"20\"}],"
		" \"updateType\":\"ALL\", \"lastInfo\":\"201504152246\"},"
		" \"id\":6234093}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOST_PARENTS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":6234093}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerPutHostParentsWithDivideInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json1 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHostParent\","
		" \"params\":{\"hostParents\":"
		" [{\"childHostId\":\"12\",\"parentHostId\":\"10\"}],"
		" \"updateType\":\"ALL\", \"lastInfo\":\"201504152246\","
		" \"divideInfo\":"
		"  {\"isLast\":false,\"serialId\":0,"
		"  \"requestId\":\"f2dac8bd-8dc1-4bee-9a48-a1aa2a5f3bb0\"}"
		"},"
		" \"id\":6234093}";
	string json2 =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHostParent\","
		" \"params\":{\"hostParents\":"
		" [{\"childHostId\":\"11\",\"parentHostId\":\"20\"}],"
		" \"updateType\":\"ALL\", \"lastInfo\":\"201504152246\","
		" \"divideInfo\":"
		"  {\"isLast\":true,\"serialId\":1,"
		"  \"requestId\":\"f2dac8bd-8dc1-4bee-9a48-a1aa2a5f3bb0\"}"
		"},"
		" \"id\":6234093}";
	JSONParser parser1(json1);
	gate->setEstablished(true);
	string actual1 = gate->interpretHandler(HAPI2_PUT_HOST_PARENTS,
					       parser1);
	string expected1 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":6234093}";
	cppcut_assert_equal(expected1, actual1);
	JSONParser parser2(json2);
	string actual2 = gate->interpretHandler(HAPI2_PUT_HOST_PARENTS,
					       parser2);
	string expected2 =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":6234093}";
	cppcut_assert_equal(expected2, actual2);
}

void test_procedureHandlerPutHostParentsInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHostParent\","
		" \"params\":{\"hostParents\":"
		" [{\"childHostId\":\"12\"},"
		" {\"parentHostId\":\"20\"}],"
		" \"updateType\":\"ALL\", \"lastInfo\":\"201504152246\"},"
		" \"id\":6234093}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_HOST_PARENTS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":6234093,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'parentHostId' does not exist.\","
		"\"Failed to parse mandatory member:"
		" 'childHostId' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerPutArmInfo(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putArmInfo\","
		" \"params\":{\"lastStatus\":\"INIT\","
		" \"failureReason\":\"Example reason\","
		" \"lastSuccessTime\":\"20150313161100\","
		" \"lastFailureTime\":\"20150313161530\","
		" \"numSuccess\":165, \"numFailure\":10}, \"id\":234}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_ARM_INFO, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":234}";
	cppcut_assert_equal(expected, actual);

	timespec successTimeStamp, failureTimeStamp;
	HatoholArmPluginGateHAPI2::parseTimeStamp("20150313161100", successTimeStamp);
	HatoholArmPluginGateHAPI2::parseTimeStamp("20150313161530", failureTimeStamp);
	ArmInfo expectedArmInfo;
	expectedArmInfo.running         = true;
	expectedArmInfo.stat            = ARM_WORK_STAT_INIT;
	expectedArmInfo.statUpdateTime  = SmartTime(SmartTime::INIT_CURR_TIME);
	expectedArmInfo.failureComment  = "Example reason";
	expectedArmInfo.lastSuccessTime = successTimeStamp;
	expectedArmInfo.lastFailureTime = failureTimeStamp;
	expectedArmInfo.numUpdate       = 165;
	expectedArmInfo.numFailure      = 10;

	const ArmStatus &armStatus = gate->getArmStatus();

	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	gate->start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	cppcut_assert_equal(expectedArmInfo.stat, armStatus.getArmInfo().stat);
	cppcut_assert_equal(expectedArmInfo.failureComment,
			    armStatus.getArmInfo().failureComment);
	cppcut_assert_equal(expectedArmInfo.lastSuccessTime,
			    armStatus.getArmInfo().lastSuccessTime);
	cppcut_assert_equal(expectedArmInfo.lastFailureTime,
			    armStatus.getArmInfo().lastFailureTime);
	cppcut_assert_equal(expectedArmInfo.numUpdate, armStatus.getArmInfo().numUpdate);
	cppcut_assert_equal(expectedArmInfo.numFailure, armStatus.getArmInfo().numFailure);
}

void test_procedureHandlerPutArmInfoInvalidJSON(void)
{
	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo, false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putArmInfo\","
		" \"params\":{\"lastStatus\":\"INIT\","
		" \"failureReason\":\"Example reason\","
		" \"lastSuccessTime\":\"20150313161100\","
		" \"lastFailureTime\":\"20150313161530\"}, \"id\":234}";
	JSONParser parser(json);
	gate->setEstablished(true);
	string actual = gate->interpretHandler(HAPI2_PUT_ARM_INFO, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":234,"
		"\"error\":{\"code\":-32602,"
		"\"message\":\"Invalid method parameter(s).\","
		"\"data\":["
		"\"Failed to parse mandatory member:"
		" 'numSuccess' does not exist.\","
		"\"Failed to parse mandatory member:"
		" 'numFailure' does not exist.\""
		"]}}";
	cppcut_assert_equal(expected, actual);
}

} // testProcedureHandlers

namespace testCommunication {

AMQPConnectionInfo *connectionInfo;
shared_ptr<AMQPConnection> connection;

void prepareDB(const char *amqpURL)
{
	hatoholInit();
	setupTestDB();
	loadDummyHosts();

	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	MonitoringServerInfo serverInfo = monitoringServerInfo;
	ArmPluginInfo pluginInfo = armPluginInfo;
	pluginInfo.brokerUrl = amqpURL;
	pluginInfo.staticQueueAddress = "test.1";
	dbConfig.addTargetServer(serverInfo, pluginInfo, privilege);
}

AMQPConnectionInfo &getConnectionInfo(void)
{
	if (!connectionInfo)
		cut_omit("TEST_AMQP_URL isn't set");
	return *connectionInfo;
}

void omitIfNoURL(void)
{
	getConnectionInfo();
}

GTimer *startTimer(void)
{
	GTimer *timer = g_timer_new();
	cut_take(timer, (CutDestroyFunction)g_timer_destroy);
	g_timer_start(timer);
	return timer;
}

void sendMessage(const string body)
{
	AMQPPublisher publisher(getConnectionInfo());
	AMQPJSONMessage message;
	message.body = body;
	publisher.setMessage(message);
	publisher.publish();
}

string popServerMessage(void)
{
	class TestMessageHandler : public AMQPMessageHandler {
	public:
		TestMessageHandler()
		: m_gotMessage(false)
		{
		}

		virtual bool handle(AMQPConsumer &consumer,
				    const AMQPMessage &message) override
		{
			if (m_gotMessage)
				return true;
			m_gotMessage = true;
			m_message = message;
			return true;
		}

		atomic<bool> m_gotMessage;
		AMQPMessage m_message;
	} handler;
	AMQPConsumer consumer(getConnectionInfo(), &handler);

	consumer.start();
	gdouble timeout = 2.0, elapsed = 0.0;
	GTimer *timer = startTimer();
	while (!handler.m_gotMessage && elapsed < timeout) {
		g_usleep(0.01 * G_USEC_PER_SEC);
		elapsed = g_timer_elapsed(timer, NULL);
	}
	consumer.exitSync();

	return handler.m_message.body;
}

void receiveFetchRequest(const string &expectedMethod,
			 string &fetchId, int64_t &id)
{
	string request = popServerMessage();
	JSONParser parser(request);
	cppcut_assert_equal(false, parser.hasError());
	string actualMethod;
	parser.read("method", actualMethod);
	if (parser.startObject("params")) {
		parser.read("fetchId", fetchId);
		parser.endObject();
	}
	parser.read("id", id);
	cppcut_assert_equal(expectedMethod, actualMethod);
	cppcut_assert_equal(true, !fetchId.empty() && id);
}

void waitConnection(shared_ptr<HatoholArmPluginGateHAPI2> gate) {
	constexpr const size_t maxSleepCount = 10;
	for (size_t sleepCount = 0; ;sleepCount++) {
		if (gate->isEstablished()) {
			break;
		}
		this_thread::sleep_for(chrono::milliseconds(200));
		if (sleepCount >= maxSleepCount)
			cut_fail("HatoholArmPluginGateHAPI2 "
				 "does not aquire connection\n");
	}
}

void acceptProcedure(shared_ptr<HatoholArmPluginGateHAPI2> gate,
		     const string procedureName)
{
	string exchangeProfileMethod = popServerMessage();
	JSONParser parser(exchangeProfileMethod);
	cppcut_assert_equal(false, parser.hasError());
	int64_t id = 0;
	parser.read("id", id);
	string response = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\","
		" \"result\":{\"procedures\":[\"%s\"],"
		" \"name\":\"examplePlugin\"}, \"id\":%" PRId64 "}",
		procedureName.c_str(), id);
	sendMessage(response);
	waitConnection(gate);
}

void cut_setup(void)
{
	// e.g.) url = "amqp://hatohol:hatohol@localhost:5672/hatohol";
	const char *url = getenv("TEST_AMQP_URL");
	if (url) {
		connectionInfo = new AMQPConnectionInfo();
		connectionInfo->setURL(url);
		connectionInfo->setPublisherQueueName("test.1-S");
		connectionInfo->setConsumerQueueName("test.1-T");
		connection = AMQPConnection::create(*connectionInfo);
		connection->connect();
		connection->deleteAllQueues();
		connection = NULL;
		prepareDB(url);
	} else {
		connectionInfo = NULL;
	}
}

void cut_teardown(void)
{
	if (connectionInfo) {
		connection = AMQPConnection::create(*connectionInfo);
		connection->connect();
		connection->deleteAllQueues();
		connection = NULL;
	}
	delete connectionInfo;
	connectionInfo = NULL;
}

void test_callExchangeProfile(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"exchangeProfile\","
		"\"params\":\\{"
		"\"name\":\"hatohol\","
		"\"procedures\":\\["
		"\"exchangeProfile\","
		"\"getMonitoringServerInfo\","
		"\"getLastInfo\","
		"\"putItems\","
		"\"putHistory\","
		"\"putHosts\","
		"\"putHostGroups\","
		"\"putHostGroupMembership\","
		"\"putTriggers\","
		"\"putEvents\","
		"\"putHostParents\","
		"\"putArmInfo\""
		"\\]\\},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_exchangeProfile(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	sendMessage(
		"{"
		"  \"id\": 1,"
		"  \"params\": {"
		"    \"name\": \"examplePlugin\","
		"    \"procedures\": ["
		"      \"exchangeProfile\""
		"    ]"
		"  },"
		"  \"method\": \"exchangeProfile\","
		"  \"jsonrpc\": \"2.0\""
		"}");

	string expected =
		"{\"jsonrpc\":\"2.0\","
		  "\"result\":{\"name\":\"" PACKAGE_NAME "\","
		  "\"procedures\":"
		  "[\"exchangeProfile\",\"getMonitoringServerInfo\","
		  "\"getLastInfo\",\"putItems\"," "\"putHistory\","
		  "\"putHosts\",\"putHostGroups\","
		  "\"putHostGroupMembership\",\"putTriggers\","
		  "\"putEvents\",\"putHostParents\",\"putArmInfo\""
		"]},\"id\":1}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
	const ArmStatus &armStatus = gate->getArmStatus();
	cppcut_assert_equal(ARM_WORK_STAT_OK, armStatus.getArmInfo().stat);
}

void test_brokenJSON(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	sendMessage("Broken JSON");

	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":null,"
		"\"error\":{"
		"\"code\":-32700,"
		"\"message\":\"Invalid JSON\""
		"}"
		"}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
}

void test_unknownProcedure(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	sendMessage(
		"{\"jsonrpc\":\"2.0\", \"method\":\"conquerTheWorld\","
		" \"params\":{\"Austoralia\":\"mine\","
		" \"North America\":\"mine\","
		" \"South America\":\"mine\","
		" \"Eurasia\":\"mine\","
		" \"Africa\":\"mine\","
		" \"Antarctica\":\"mine\"}, \"id\":3}");
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":3,"
		"\"error\":{"
		"\"code\":-32601,"
		"\"message\":\"Method not found: conquerTheWorld\""
		"}"
		"}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
}

void test_noMethod(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	sendMessage(
		"{\"jsonrpc\":\"2.0\","
		" \"params\":{\"Austoralia\":\"mine\","
		" \"North America\":\"mine\","
		" \"South America\":\"mine\","
		" \"Eurasia\":\"mine\","
		" \"Africa\":\"mine\","
		" \"Antarctica\":\"mine\"}, \"id\":5}");
	string expected =
		"{\"jsonrpc\":\"2.0\",\"id\":5,"
		"\"error\":{"
		"\"code\":-32600,"
		"\"message\":\"Invalid JSON-RPC object: "
		"None of method, result, error exist!\""
		"}"
		"}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
}

void test_invalidTypeForMethodName(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	sendMessage(
		"{\"jsonrpc\":\"2.0\","
		" \"method\":{ \"name\": \"exchangeProfile\" },"
		" \"params\":{},"
		" \"id\":5}");
	string expected =
		"{\"jsonrpc\":\"2.0\","
		"\"id\":5,"
		"\"error\":{"
		"\"code\":-32600,"
		"\"message\":\"Invalid request: Invalid type for \\\"method\\\"!\""
		"}"
		"}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
}

void test_callMethodWithoutExchangeProfile(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	popServerMessage(); // eat exchangeProfile

	sendMessage(
		"{\"jsonrpc\":\"2.0\", \"method\":\"getMonitoringServerInfo\","
		" \"params\":\"\", \"id\":456}");
	string expected =
		"{\"jsonrpc\":\"2.0\","
		"\"result\":\"FAILURE\","
		"\"id\":456"
		"}";
	string actual = popServerMessage();
	cppcut_assert_equal(expected, actual);
}

#if 0
// TODO: Need to add a method to hook the response
void test_errorResponse(void)
{
	omitIfNoURL();

	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo), false);
	string exchangeProfile = popServerMessage();
	JSONParser parser(exchangeProfile);
	int64_t id;
	parser.read("id", id);
	string errorResponse = StringUtils::sprintf(
		"{\"jsonrpc\": \"2.0\","
		"\"error\":{"
		"\"code\": -32603,"
		"\"message\": \"Internal Error\""
		"},"
		"\"id\": %" PRId64 "}", id);
	sendMessage(errorResponse.c_str());
	cppcut_assert_equal(true, true);
}
#endif

void test_fetchItems(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchItems");
	cppcut_assert_equal(true, gate->startOnDemandFetchItems({}, NULL));

	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchItems\","
		"\"params\":\\{\"fetchId\":\"\\d+\"\\},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_fetchItemsWithHostIds(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchItems");
	cppcut_assert_equal(true,
			    gate->startOnDemandFetchItems({"3", "5", "8"}, NULL));

	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchItems\","
		"\"params\":\\{\"hostIds\":\\[\"3\",\"5\",\"8\"\\],"
		"\"fetchId\":\"\\d+\"\\},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_notSupportfetchItems(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");
	cppcut_assert_equal(false, gate->startOnDemandFetchItems({}, NULL));
}

void test_fetchItemsCallback(void)
{
	omitIfNoURL();

	struct TestContext {
		atomic<bool> m_called;
		TestContext()
		:m_called(false)
		{
		}
		void onFetchItems(Closure0 *closure)
		{
			m_called = true;
		}
	} context;
	struct FetchClosure : ClosureTemplate0<TestContext>
	{
		FetchClosure(TestContext *receiver, callback func)
		: ClosureTemplate0<TestContext>(receiver, func)
		{
		}
	};
	Closure0 *closure = new FetchClosure(&context, &TestContext::onFetchItems);

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchItems");
	cppcut_assert_equal(true, gate->startOnDemandFetchItems({}, closure));

	string fetchId;
	int64_t id = 0;
	receiveFetchRequest("fetchItems", fetchId, id);

	string fetchItemsResponse = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\","
		" \"result\": \"SUCCESS\","
		" \"id\":%" PRId64 "}",
		id);
	sendMessage(fetchItemsResponse);
	string response = popServerMessage();
	cppcut_assert_equal(false, context.m_called.load());

	string putItemsJSON = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":[{\"itemId\":\"1\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175523\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\"], \"unit\":\"example unit\"},"
		" {\"itemId\":\"2\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175531\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":[\"example name\", \"example2\"], \"unit\":\"example unit\"}],"
		" \"fetchId\":\"%s\"}, \"id\":%" PRId64 "}",
		fetchId.c_str(), id);
	sendMessage(putItemsJSON);

	response = popServerMessage(); // for waiting the callback
	cppcut_assert_equal(true, context.m_called.load());
}

void test_fetchItemsCallbackOnError(void)
{
	omitIfNoURL();

	struct TestContext {
		atomic<bool> m_called;
		TestContext()
		:m_called(false)
		{
		}
		void onFetchItems(Closure0 *closure)
		{
			m_called = true;
		}
	} context;
	struct FetchClosure : ClosureTemplate0<TestContext>
	{
		FetchClosure(TestContext *receiver, callback func)
		: ClosureTemplate0<TestContext>(receiver, func)
		{
		}
	};
	Closure0 *closure = new FetchClosure(&context, &TestContext::onFetchItems);

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchItems");
	cppcut_assert_equal(true, gate->startOnDemandFetchItems({}, closure));

	string fetchId;
	int64_t id = 0;
	receiveFetchRequest("fetchItems", fetchId, id);

	string fetchItemsErrorResponse = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\","
		" \"result\": \"FAILURE\","
		" \"id\":%" PRId64 "}",
		id);
	sendMessage(fetchItemsErrorResponse);

	string response = popServerMessage(); // for waiting the callback
	cppcut_assert_equal(true, context.m_called.load());
}

void test_fetchHistory(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchHistory");
	ItemInfo itemInfo = testItemInfo[0];
	gate->startOnDemandFetchHistory(itemInfo, 1433748751, 1433752340, NULL);

	string expected = StringUtils::sprintf(
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchHistory\","
		"\"params\":\\{"
		"\"hostId\":\"%" FMT_LOCAL_HOST_ID "\","
		"\"itemId\":\"%" FMT_ITEM_ID "\","
		"\"beginTime\":\"20150608073231\","
		"\"endTime\":\"20150608083220\","
		"\"fetchId\":\"\\d+\""
		"\\},"
		"\"id\":\\d+"
		"\\}$",
		itemInfo.hostIdInServer.c_str(), itemInfo.id.c_str());
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_fetchHistoryCallback(void)
{
	omitIfNoURL();

	struct TestContext {
		HistoryInfoVect m_historyInfoVect;
		TestContext()
		{
		}
		void onFetchHistory(Closure1<HistoryInfoVect> *closure,
				    const HistoryInfoVect &historyInfoVect)
		{
			m_historyInfoVect = historyInfoVect;
		}
	} context;
	struct FetchClosure : ClosureTemplate1<TestContext, HistoryInfoVect>
	{
		FetchClosure(TestContext *receiver, callback func)
		: ClosureTemplate1<TestContext, HistoryInfoVect>(receiver, func)
		{
		}
	};
	FetchClosure *closure =
	  new FetchClosure(&context, &TestContext::onFetchHistory);

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchHistory");
	ItemInfo itemInfo = testItemInfo[0];
	gate->startOnDemandFetchHistory(itemInfo, 1433748751, 1433752340,
					closure);

	string fetchId;
	int64_t id = 0;
	receiveFetchRequest("fetchHistory", fetchId, id);

	string putHistoryJSON = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"%" FMT_ITEM_ID "\","
		" \"samples\":[{\"value\":\"exampleValue\","
		" \"time\":\"20150323113032.000000000\"},"
		"{\"value\":\"exampleValue2\",\"time\":\"20150323113033.000000000\"}],"
		" \"fetchId\":\"%s\"}, \"id\":%" PRId64 "}",
		itemInfo.id.c_str(), fetchId.c_str(), id);
	sendMessage(putHistoryJSON);

	string reply = popServerMessage(); // for waiting the callback
	HistoryInfoVect expectedHistoryInfoVect = {
		{
			monitoringServerInfo.id,
			itemInfo.id,
			"exampleValue",
			{ 1427110232, 0 }
		},
		{
			monitoringServerInfo.id,
			itemInfo.id,
			"exampleValue2",
			{ 1427110233, 0 }
		},
	};
	string expected, actual;
	for (auto &historyInfo: expectedHistoryInfoVect)
		expected += makeHistoryOutput(historyInfo);
	for (auto &historyInfo: context.m_historyInfoVect)
		actual += makeHistoryOutput(historyInfo);
	cppcut_assert_equal(expected, actual);
}

void test_fetchTriggers(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchTriggers");
	cppcut_assert_equal(true, gate->startOnDemandFetchTriggers({}, NULL));

	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchTriggers\","
		"\"params\":\\{\"fetchId\":\"\\d+\"\\},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_fetchTriggersWithHostIds(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchTriggers");
	cppcut_assert_equal(true,
			    gate->startOnDemandFetchTriggers({"2", "3", "5"}, NULL));

	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchTriggers\","
		"\"params\":\\{\"hostIds\":\\[\"2\",\"3\",\"5\"\\],"
		"\"fetchId\":\"\\d+\"\\},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_fetchTriggersCallback(void)
{
	omitIfNoURL();

	struct TestContext {
		atomic<bool> m_called;
		TestContext()
		:m_called(false)
		{
		}
		void onFetchTriggers(Closure0 *closure)
		{
			m_called = true;
		}
	} context;
	struct FetchClosure : ClosureTemplate0<TestContext>
	{
		FetchClosure(TestContext *receiver, callback func)
		: ClosureTemplate0<TestContext>(receiver, func)
		{
		}
	};
	Closure0 *closure =
	  new FetchClosure(&context, &TestContext::onFetchTriggers);

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchTriggers");
	cppcut_assert_equal(true, gate->startOnDemandFetchTriggers({}, closure));

	string fetchId;
	int64_t id = 0;
	receiveFetchRequest("fetchTriggers", fetchId, id);

	string putTriggersJSON = StringUtils::sprintf(
		"{\"jsonrpc\":\"2.0\", \"method\":\"putTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201504061606\", \"fetchId\":\"%s\","
		" \"triggers\":[{\"triggerId\":\"1\", \"status\":\"OK\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323175800\","
		" \"hostId\":\"1\", \"hostName\":\"exampleHostName\","
		" \"brief\":\"example brief\","
		" \"extendedInfo\": \"sample extended info\"}]},"
		"\"id\":%" PRId64 "}",
		fetchId.c_str(), id);
	sendMessage(putTriggersJSON);

	string reply = popServerMessage(); // for waiting the callback
	cppcut_assert_equal(true, context.m_called.load());
}

void test_notSupportFetchTriggers(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");
	cppcut_assert_equal(false, gate->startOnDemandFetchTriggers({}, NULL));
}

void test_fetchEvents(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchEvents");

	string lastInfo = "2015061218152100";
	size_t count = 1000;
	bool ascending = true;
	bool succeeded =
	  gate->startOnDemandFetchEvents(lastInfo, count, ascending, NULL);
	cppcut_assert_equal(true, succeeded);

	string expected =
		"^\\{"
		"\"jsonrpc\":\"2\\.0\","
		"\"method\":\"fetchEvents\","
		"\"params\":\\{"
		"\"lastInfo\":\"2015061218152100\","
		"\"count\":1000,"
		"\"direction\":\"ASC\","
		"\"fetchId\":\"\\d+\"\\"
		"},"
		"\"id\":\\d+"
		"\\}$";
	string actual = popServerMessage();
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_fetchEventsCallback(void)
{
	omitIfNoURL();

	struct TestContext {
		atomic<bool> m_called;
		TestContext()
		:m_called(false)
		{
		}
		void onFetchEvents(Closure0 *closure)
		{
			m_called = true;
		}
	} context;
	struct FetchClosure : ClosureTemplate0<TestContext>
	{
		FetchClosure(TestContext *receiver, callback func)
		: ClosureTemplate0<TestContext>(receiver, func)
		{
		}
	};

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "fetchEvents");

	Closure0 *closure =
	  new FetchClosure(&context, &TestContext::onFetchEvents);
	string lastInfo = "20150616214400";
	size_t maxEvents = 1000;
	bool ascending = true;
	bool succeeded =
	  gate->startOnDemandFetchEvents(lastInfo, maxEvents, ascending,
					 closure);
	cppcut_assert_equal(true, succeeded);

	string fetchId;
	int64_t id = 0;
	receiveFetchRequest("fetchEvents", fetchId, id);

	// mayMoreFlag: true
	string putEventsJSON =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"1\","
			       " \"time\":\"20150323151300\", \"type\":\"GOOD\","
			       " \"triggerId\":2, \"status\": \"OK\","
			       " \"severity\":\"INFO\","
			       " \"hostId\":\"3\", \"hostName\":\"exampleHostName\","
			       " \"brief\":\"example brief\","
			       " \"extendedInfo\": \"sample extended info\"}],"
			       " \"lastInfo\":\"20150323151299\","
			       " \"mayMoreFlag\": true,"
			       " \"fetchId\":\"%s\"},\"id\":%" PRId64 "}",
			       fetchId.c_str(), id);
	sendMessage(putEventsJSON);

	string reply = popServerMessage(); // for waiting the reply
	cppcut_assert_equal(false, context.m_called.load());

	// mayMoreFlag: false
	putEventsJSON =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"putEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"2\","
			       " \"time\":\"20150323151301\", \"type\":\"GOOD\","
			       " \"triggerId\":2, \"status\": \"OK\","
			       " \"severity\":\"INFO\","
			       " \"hostId\":\"3\", \"hostName\":\"exampleHostName\","
			       " \"brief\":\"example brief\","
			       " \"extendedInfo\": \"sample extended info\"}],"
			       " \"lastInfo\":\"20150323151300\","
			       " \"mayMoreFlag\": false,"
			       " \"fetchId\":\"%s\"},\"id\":%" PRId64 "}",
			       fetchId.c_str(), id);
	sendMessage(putEventsJSON);

	reply = popServerMessage(); // for waiting the callback
	cppcut_assert_equal(true, context.m_called.load());
}

void test_notSupportFetchEvents(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	string lastInfo = "2015061218152100";
	size_t count = 1000;
	bool ascending = true;
	bool succeeded =
	  gate->startOnDemandFetchEvents(lastInfo, count, ascending, NULL);
	cppcut_assert_equal(false, succeeded);
}

void test_updateMonitoringServerInfo(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	// Update MonitoringServerInfo
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	MonitoringServerInfo serverInfo = monitoringServerInfo;
	ArmPluginInfo pluginInfo = armPluginInfo;
	dbConfig.getArmPluginInfo(pluginInfo, serverInfo.id);
	serverInfo.baseURL = "http://www.example.net/zabbix/";
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	dbConfig.updateTargetServer(serverInfo, pluginInfo, privilege);

	// delete the gate to emit updateMonitoringServerInfo notification
	gate = nullptr;

	string expected(
		"{\"jsonrpc\":\"2.0\","
		"\"method\":\"updateMonitoringServerInfo\","
		"\"params\":{"
		 "\"serverId\":302,\"url\":\"http://www.example.net/zabbix/\","
		 "\"type\":\"8e632c14-d1f7-11e4-8350-d43d7e3146fb\","
		 "\"nickName\":\"HAPI2 Zabbix\",\"userName\":\"Admin\","
		 "\"password\":\"zabbix\",\"pollingIntervalSec\":300,"
		 "\"retryIntervalSec\":60,\"extendedInfo\":\"test extended info\""
		"}}");
	string actual(popServerMessage());
	cppcut_assert_equal(expected, actual);
}

void test_noUpdateMonitoringServerInfo(void)
{
	omitIfNoURL();

	shared_ptr<HatoholArmPluginGateHAPI2> gate =
	  make_shared<HatoholArmPluginGateHAPI2>(monitoringServerInfo);
	acceptProcedure(gate, "exchangeProfile");

	gate = nullptr;

	string expected("");
	string actual(popServerMessage());
	cppcut_assert_equal(expected, actual);
}

} // testCommunication

} // testHatoholArmPluginGateHAPI2
