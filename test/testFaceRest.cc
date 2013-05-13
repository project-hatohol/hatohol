#include <cppcutter.h>
#include "Asura.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBAgentSQLite3.h"
#include "DBClientTest.h"
#include "ConfigManager.h"

namespace testFaceRest {

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_CONFIG_NAME = "testDatabase-config.db";
static const char *TEST_DB_ASURA_NAME = "testDatabase-asura.db";

static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void startFaceRest(void)
{
	string dbPathConfig = getFixturesDir() + TEST_DB_CONFIG_NAME;
	string dbPathAsura  = getFixturesDir() + TEST_DB_ASURA_NAME;
	// TODO: remove the direct call of DBAgentSQLite3's API.
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG, dbPathConfig);
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_ASURA, dbPathAsura);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg);
	g_faceRest->start();
}

static JsonParserAgent *getResponseAsJsonParser(const string &url,
                                                const string &callbackName = "")
{
	string callbackParam;
	if (!callbackName.empty()) {
		callbackParam = "?callback=";
		callbackParam += callbackName;
	}

	// get reply with wget
	string getCmd =
	  StringUtils::sprintf("wget http://localhost:%u%s%s -O -",
	                       TEST_PORT, url.c_str(), callbackParam.c_str());
	string response = executeCommand(getCmd);

	// if JSONP, check the callback name
	if (!callbackParam.empty()) {
		size_t lenCallbackName = callbackName.size();
		size_t minimumLen = lenCallbackName + 2; // +2 for ''(' and ')'
		cppcut_assert_equal(true, response.size() > minimumLen);

		cut_assert_equal_substring(
		  callbackName.c_str(), response.c_str(), lenCallbackName);
		cppcut_assert_equal(')', response[response.size()-1]);
		response = string(response, lenCallbackName+1,
		                  response.size() - minimumLen);
	}

	// check the JSON body
	if (isVerboseMode())
		cut_notify("<<response>>\n%s\n", response.c_str());
	JsonParserAgent *parser = new JsonParserAgent(response);
	if (parser->hasError()) {
		string parserErrMsg = parser->getErrorMessage();
		delete parser;
		cut_fail("%s\n%s", parserErrMsg.c_str(), response.c_str());
	}
	return parser;
}

static void _assertValueInParser(JsonParserAgent *parser,
                                 const string &member, bool expected)
{
	bool val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, val);
}

static void _assertValueInParser(JsonParserAgent *parser,
                                 const string &member, uint32_t expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, (uint32_t)val);
}

static void _assertValueInParser(JsonParserAgent *parser,
                                 const string &member, const timespec &expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal((uint32_t)expected.tv_sec, (uint32_t)val);
}

static void _assertValueInParser(JsonParserAgent *parser,
                                 const string &member, const string &expected)
{
	string val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, val);
}

#define assertValueInParser(P,M,E) cut_trace(_assertValueInParser(P,M,E));

static void _assertTestTriggerInfo(const TriggerInfo &triggerInfo)
{
	assertValueInParser(g_parser, "status", 
	                    (uint32_t)triggerInfo.status);
	assertValueInParser(g_parser, "severity",
	                    (uint32_t)triggerInfo.severity);
	assertValueInParser(g_parser, "lastChangeTime",
	                    triggerInfo.lastChangeTime);
	assertValueInParser(g_parser, "serverId", triggerInfo.serverId);
	assertValueInParser(g_parser, "hostId", triggerInfo.hostId);
	assertValueInParser(g_parser, "hostName", triggerInfo.hostName);
	assertValueInParser(g_parser, "brief", triggerInfo.brief);
}
#define assertTestTriggerInfo(T) cut_trace(_assertTestTriggerInfo(T))

static void _assertServers(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_SERVERS);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfServers",
	                    (uint32_t)NumServerInfo);
	g_parser->startObject("servers");
	for (size_t i = 0; i < NumServerInfo; i++) {
		g_parser->startElement(i);
		MonitoringServerInfo &svInfo = serverInfo[i];
		assertValueInParser(g_parser, "id",   (uint32_t)svInfo.id);
		assertValueInParser(g_parser, "type", (uint32_t)svInfo.type);
		assertValueInParser(g_parser, "hostName",  svInfo.hostName);
		assertValueInParser(g_parser, "ipAddress", svInfo.ipAddress);
		assertValueInParser(g_parser, "nickname",  svInfo.nickname);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertServers(P,...) cut_trace(_assertServers(P,##__VA_ARGS__))

static void _assertTriggers(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_TRIGGERS);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfTriggers",
	                    (uint32_t)NumTestTriggerInfo);
	g_parser->startObject("triggers");
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		g_parser->startElement(i);
		TriggerInfo &triggerInfo = testTriggerInfo[i];
		assertTestTriggerInfo(triggerInfo);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertTriggers(P,...) cut_trace(_assertTriggers(P,##__VA_ARGS__))

static void _assertEvents(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_EVENTS);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfEvents",
	                    (uint32_t)NumTestEventInfo);
	g_parser->startObject("events");
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		g_parser->startElement(i);
		EventInfo &eventInfo = testEventInfo[i];
		assertValueInParser(g_parser, "serverId", eventInfo.serverId);
		assertValueInParser(g_parser, "time", eventInfo.time);
		assertValueInParser(g_parser, "eventValue",
		                    (uint32_t)eventInfo.eventValue);
		assertValueInParser(g_parser, "triggerId",
		                    (uint32_t)eventInfo.triggerId);

		// check the trigger part
		const TriggerInfo &expectedTriggerInfo =
			searchTestTriggerInfo(eventInfo);
		assertTestTriggerInfo(expectedTriggerInfo);

		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertEvents(P,...) cut_trace(_assertEvents(P,##__VA_ARGS__))

void setup(void)
{
	asuraInit();
}

void teardown(void)
{
	if (g_faceRest) {
		try {
			g_faceRest->stop();
		} catch (const AsuraException &e) {
			printf("Got exception: %s\n",
			       e.getFancyMessage().c_str());
		}
		delete g_faceRest;
		g_faceRest = NULL;
	}

	if (g_parser) {
		delete g_parser;
		g_parser = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_servers(void)
{
	assertServers("/servers.json");
}

void test_serversJsonp(void)
{
	assertServers("/servers.jsonp", "foo");
}

void test_triggers(void)
{
	assertTriggers("/triggers.json");
}

void test_triggersJsonp(void)
{
	assertTriggers("/triggers.jsonp", "foo");
}

void test_events(void)
{
	assertEvents("/events.json");
}

void test_eventsJsonp(void)
{
	assertEvents("/events.jsonp", "foo");
}

} // namespace testFaceRest
