#include <cppcutter.h>
#include "Asura.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBAgentSQLite3.h"
#include "DBAgentTest.h"

namespace testFaceRest {

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_NAME = "testDatabase.db";

static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void startFaceRest(const string &testDBName)
{
	string dbPath = getFixturesDir() + testDBName;
	// TODO: remove the direct call of DBAgentSQLite3's API.
	DBAgentSQLite3::defineDBPath(DefaultDBDomainId, dbPath);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg);
	bool autoDeleteObject = true;
	g_faceRest->start(autoDeleteObject);
}

static JsonParserAgent *getResponseAsJsonParser(const string url)
{
	string getCmd =
	  StringUtils::sprintf("wget -q http://localhost:%u%s -O -",
	                       TEST_PORT, url.c_str());
	string response = executeCommand(getCmd);
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
                                 const string &member, timespec &expected)
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

void setup(void)
{
	asuraInit();
}

void teardown(void)
{
	if (g_faceRest) {
		g_faceRest->stop();
		// g_face will be automatically destroyed, because it is starte
		// with autoDeleteObject flag 
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
	startFaceRest(TEST_DB_NAME);
	g_parser = getResponseAsJsonParser("/servers");
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfServers",
	                    (uint32_t)NumServerInfo);
	g_parser->startObject("servers");
	for (size_t i = 0; i < NumServerInfo; i++) {
		g_parser->startElement(i);
		MonitoringServerInfo &svInfo = serverInfo[i];
		assertValueInParser(g_parser, "id",        svInfo.id);
		assertValueInParser(g_parser, "type", (uint32_t)svInfo.type);
		assertValueInParser(g_parser, "hostName",  svInfo.hostName);
		assertValueInParser(g_parser, "ipAddress", svInfo.ipAddress);
		assertValueInParser(g_parser, "nickname",  svInfo.nickname);
		g_parser->endElement();
	}
	g_parser->endObject();
}

void test_triggers(void)
{
	startFaceRest(TEST_DB_NAME);
	g_parser = getResponseAsJsonParser("/triggers");
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfTriggers",
	                    (uint32_t)NumTestTriggerInfo);
	g_parser->startObject("triggers");
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		g_parser->startElement(i);
		TriggerInfo &triggerInfo = testTriggerInfo[i];
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
		g_parser->endElement();
	}
	g_parser->endObject();
}

} // namespace testFaceRest
