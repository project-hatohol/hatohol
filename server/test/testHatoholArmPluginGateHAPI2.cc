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

const MonitoringServerInfo &monitoringServerInfo = testServerInfo[7];

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBServer();
	loadTestDBArmPlugin();
}

void cut_teardown(void)
{
}

void test_new(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	cut_assert_not_null(gate);
}

void test_procedureHandlerExchangeProfile(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"exchangeProfile\","
		" \"params\":{\"procedures\":[\"getMonitoringServerInfo\","
		" \"getLastInfo\", \"putItems\", \"updateArmInfo\", \"fetchItems\"],"
		" \"name\":\"exampleName\"}, \"id\":123}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_EXCHANGE_PROFILE, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":{\"name\":\"" PACKAGE_NAME "\","
		  "\"procedures\":"
		  "[\"getMonitoringServerInfo\",\"getLastInfo\",\"putItems\","
		  "\"putHistory\",\"updateHosts\",\"updateHostGroups\","
		  "\"updateHostGroupMembership\",\"updateTriggers\","
		  "\"updateEvents\",\"updateHostParent\",\"updateArmInfo\""
		"]},\"id\":123}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerMonitoringServerInfo(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"getMonitoringServerInfo\","
		" \"params\":\"\", \"id\":456}";
	JSONParser parser(json);
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
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(serverInfo, false), false);
	string json =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"getLastInfo\","
			       " \"params\":\"%s\", \"id\":789}",
			       gcut_data_get_string(data, "params"));
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_LAST_INFO, parser);
	string expected =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\",\"result\":\"%s\",\"id\":789}",
			       gcut_data_get_string(data, "value"));
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerPutItems(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"putItems\","
		" \"params\":{\"items\":[{\"itemId\":\"1\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175523\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":\"example name\", \"unit\":\"example unit\"},"
		" {\"itemId\":\"2\", \"hostId\":\"1\","
		" \"brief\":\"example brief\", \"lastValueTime\":\"20150410175531\","
		" \"lastValue\":\"example value\","
		" \"itemGroupName\":\"example name\", \"unit\":\"example unit\"}],"
		" \"fetchId\":\"1\"}, \"id\":83241245}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_PUT_ITEMS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"\",\"id\":83241245}";
	cppcut_assert_equal(expected, actual);
	// TODO: add DB assertion
}

void test_procedureHandlerPutHistory(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"putHistory\","
		" \"params\":{\"itemId\":\"1\","
		" \"histories\":[{\"value\":\"exampleValue\","
		" \"time\":\"20150323113032.000000000\"},"
		"{\"value\":\"exampleValue2\",\"time\":\"20150323113033.000000000\"}],"
		" \"fetchId\":\"1\"}, \"id\":-83241245}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_PUT_HISTORY, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"\",\"id\":-83241245}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateHosts(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"updateHosts\", \"params\":"
		"{\"hosts\":[{\"hostId\":\"1\", \"hostName\":\"exampleHostName1\"}],"
		" \"updateType\":\"UPDATED\",\"lastInfo\":\"201504091052\"},"
		" \"id\":\"deadbeaf\"}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_HOSTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"deadbeaf\"}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateHostGroups(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"updateHostGroups\","
		" \"params\":{\"hostGroups\":[{\"groupId\":\"1\","
		" \"groupName\":\"Group2\"}],\"updateType\":\"ALL\","
		" \"lastInfo\":\"20150409104900\"}, \"id\":\"123abc\"}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_HOST_GROUPS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":\"123abc\"}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateHostGroupMembership(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\",\"method\":\"updateHostGroupMembership\","
		" \"params\":{\"hostGroupsMembership\":[{\"hostId\":\"1\","
		" \"groupIds\":[\"1\", \"2\", \"5\"]}],"
		" \"lastInfo\":\"20150409105600\", \"updateType\":\"ALL\"},"
		" \"id\":9342}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(
	  HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":9342}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateTriggers(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"updateTriggers\","
		" \"params\":{\"updateType\":\"UPDATED\","
		" \"lastInfo\":\"201504061606\", \"fetchId\":\"1\","
		" \"triggers\":[{\"triggerId\":\"1\", \"status\":\"OK\","
		" \"severity\":\"INFO\",\"lastChangeTime\":\"20150323175800\","
		" \"hostId\":\"1\", \"hostName\":\"exampleName\","
		" \"brief\":\"example brief\","
		" \"extendedInfo\": \"sample extended info\"}]},\"id\":34031}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_TRIGGERS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":34031}";
	cppcut_assert_equal(expected, actual);
}

void data_procedureHandlerUpdateEvents(void)
{
	gcut_add_datum("WithTriggerId",
	               "triggerIdContents", G_TYPE_STRING, " \"triggerId\":2,", NULL);
	gcut_add_datum("WithoutTriggerId",
	               "triggerIdContents", G_TYPE_STRING, "", NULL);
}

void test_procedureHandlerUpdateEvents(gconstpointer data)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
	  StringUtils::sprintf("{\"jsonrpc\":\"2.0\", \"method\":\"updateEvents\","
			       " \"params\":{\"events\":[{\"eventId\":\"1\","
			       " \"time\":\"20150323151300\", \"type\":\"GOOD\","
			       " %s \"status\": \"OK\", \"severity\":\"INFO\","
			       " \"hostId\":3, \"hostName\":\"exampleName\","
			       " \"brief\":\"example brief\","
			       " \"extendedInfo\": \"sample extended info\"}],"
			       " \"lastInfo\":\"20150401175900\","
			       " \"fetchId\":\"1\"},\"id\":2374234}",
			       gcut_data_get_string(data, "triggerIdContents"));
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_EVENTS, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":2374234}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateHostParents(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"updateHostParent\","
		" \"params\":{\"hostParents\":"
		" [{\"childHostId\":\"12\",\"parentHostId\":\"10\"},"
		" {\"childHostId\":\"11\",\"parentHostId\":\"20\"}],"
		" \"updateType\":\"ALL\", \"lastInfo\":\"201504152246\"},"
		" \"id\":6234093}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_HOST_PARENTS,
					       parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":6234093}";
	cppcut_assert_equal(expected, actual);
}

void test_procedureHandlerUpdateArmInfo(void)
{
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo, false), false);
	string json =
		"{\"jsonrpc\":\"2.0\", \"method\":\"updateArmInfo\","
		" \"params\":{\"lastStatus\":\"INIT\","
		" \"failureReason\":\"Example reason\","
		" \"lastSuccessTime\":\"20150313161100\","
		" \"lastFailureTime\":\"20150313161530\","
		" \"numSuccess\":165, \"numFailure\":10}, \"id\":234}";
	JSONParser parser(json);
	string actual = gate->interpretHandler(HAPI2_UPDATE_ARM_INFO, parser);
	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":\"SUCCESS\",\"id\":234}";
	cppcut_assert_equal(expected, actual);
}

} // testProcedureHandlers

namespace testFetch {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBServer();
	loadTestDBArmPlugin();
}

void cut_teardown(void)
{
}

void test_fetchItem(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo = testServerInfo[7];
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(serverInfo), false);
	cppcut_assert_equal(true, gate->startOnDemandFetchItem(NULL));
	// TODO: check reply
}

} // testFetch

namespace testCallServerProcedure {

AMQPConnectionInfo *connectionInfo;
AMQPConnectionPtr connection;
const MonitoringServerInfo &monitoringServerInfo = testServerInfo[7];

void prepareDB(const char *amqpURL)
{
	hatoholInit();
	setupTestDB();
	loadTestDBServer();
	loadTestDBArmPlugin();

	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	ArmPluginInfo armPluginInfo;
	dbConfig.getArmPluginInfo(armPluginInfo, monitoringServerInfo.id);
	armPluginInfo.brokerUrl = amqpURL;
	armPluginInfo.staticQueueAddress = "test.1";
	dbConfig.saveArmPluginInfo(armPluginInfo);
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
		prepareDB(url);
	} else {
		connectionInfo = NULL;
	}
}

AMQPConnectionInfo &getConnectionInfo(void)
{
	if (!connectionInfo)
		cut_omit("TEST_AMQP_URL isn't set");
	return *connectionInfo;
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

string recieveReply(void)
{
	class TestMessageHandler : public AMQPMessageHandler {
	public:
		TestMessageHandler()
		: m_gotMessage(false)
		{
		}

		virtual bool handle(AMQPConnection &connection,
				    const AMQPMessage &message) override
		{
			m_gotMessage = true;
			m_message = message;
			return true;
		}

		AtomicValue<bool> m_gotMessage;
		AMQPMessage m_message;
	} handler;
	AMQPConsumer consumer(getConnectionInfo(), &handler);

	consumer.start();
	gdouble timeout = 2.0, elapsed = 0.0;
	GTimer *timer = startTimer();
	while (!handler.m_gotMessage && elapsed < timeout) {
		g_usleep(0.1 * G_USEC_PER_SEC);
		elapsed = g_timer_elapsed(timer, NULL);
	}
	consumer.exitSync();

	return handler.m_message.body;
}

void test_exchangeProfile(void)
{
	omitIfNoURL();

	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo), true);

	sendMessage(
		"{"
		"  \"id\": 1,"
		"  \"params\": {"
		"    \"name\": \"exampleName\","
		"    \"procedures\": ["
		"      \"exchangeProfile\""
		"    ]"
		"  },"
		"  \"method\": \"exchangeProfile\","
		"  \"jsonrpc\": \"2.0\""
		"}");

	string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":{\"name\":\"exampleName\","
		  "\"procedures\":"
		  "[\"getMonitoringServerInfo\",\"getLastInfo\",\"putItems\","
		  "\"putHistory\",\"updateHosts\",\"updateHostGroups\","
		  "\"updateHostGroupMembership\",\"updateTriggers\","
		  "\"updateEvents\",\"updateHostParent\",\"updateArmInfo\""
		"]},\"id\":1}";
	string actual = recieveReply();
	cppcut_assert_equal(expected, actual);
}

void test_unknownProcedure(void)
{
	omitIfNoURL();

	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo), false);

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
	string actual = recieveReply();
	cppcut_assert_equal(expected, actual);
}

void test_noMethod(void)
{
	omitIfNoURL();

	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(monitoringServerInfo), false);

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
		"\"error\":{\"code\":-32600,\"message\":\"Invalid request\"}"
		"}";
	string actual = recieveReply();
	cppcut_assert_equal(expected, actual);
}

} // testCallServerProcedure

} // testHatoholArmPluginGateHAPI2
