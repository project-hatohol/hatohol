#include <cppcutter.h>
#include "Asura.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBAgentSQLite3.h"
#include "DBAgentTest.h"

namespace testFaceRest {

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_NAME = "ThreeServers.db";

static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void startFaceRest(const string &testDBName)
{
	string dbPath = getFixturesDir() + testDBName;
	DBAgentSQLite3::init(dbPath);

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
	string dbPath = getFixturesDir() + TEST_DB_NAME;
	DBAgentSQLite3::init(dbPath);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	FaceRest faceRest(arg);
	faceRest.start();

	string getCmd =
	  StringUtils::sprintf("wget -q http://localhost:%u/servers -O -",
	                       TEST_PORT);
	string response = executeCommand(getCmd);
	JsonParserAgent parser(response);
	cppcut_assert_equal(false, parser.hasError(),
	                    cut_message("%s\n%s",
	                                parser.getErrorMessage(),
	                                response.c_str()));
	bool result;
	cppcut_assert_equal(true, parser.read("result", result));
	cppcut_assert_equal(true, result);

	int64_t numServers;
	cppcut_assert_equal(true, parser.read("numberOfServers", numServers));
	cppcut_assert_equal(NumServerInfo, (size_t)numServers);

	parser.startObject("servers");
	for (int i = 0; i < numServers; i++) {
		parser.startElement(i);
		MonitoringServerInfo &svInfo = serverInfo[i];
		// id
		int64_t id;
		cppcut_assert_equal(true, parser.read("id", id));
		cppcut_assert_equal(svInfo.id, (uint32_t)id);

		// type
		int64_t type;
		cppcut_assert_equal(true, parser.read("type", type));
		cppcut_assert_equal(svInfo.type, (MonitoringSystemType)type);

		// host name
		string hostname;
		cppcut_assert_equal(true, parser.read("hostName", hostname));
		cppcut_assert_equal(svInfo.hostName, hostname);

		// IP address
		string ipAddress;
		cppcut_assert_equal(true, parser.read("ipAddress", ipAddress));
		cppcut_assert_equal(svInfo.ipAddress, ipAddress);

		// nickname
		string nickname;
		cppcut_assert_equal(true, parser.read("nickname", nickname));
		cppcut_assert_equal(svInfo.nickname, nickname);
		parser.endElement();
	}
	parser.endObject();
}

void test_triggers(void)
{
	string testDBName = "testTriggerList.db";
	startFaceRest(testDBName);
	g_parser = getResponseAsJsonParser("/triggers");
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfTriggers",
	                    (uint32_t)NumTestTriggerInfo);
	g_parser->startObject("triggers");
	for (int i = 0; i < NumTestTriggerInfo; i++) {
		g_parser->startElement(i);
		TriggerInfo &triggerInfo = testTriggerInfo[i];
		assertValueInParser(g_parser, "status", 
		                    (uint32_t)triggerInfo.status);
		assertValueInParser(g_parser, "severity",
		                    (uint32_t)triggerInfo.severity);
		assertValueInParser(g_parser, "lastChangeTime",
		                    triggerInfo.lastChangeTime);
		assertValueInParser(g_parser, "serverId", triggerInfo.serverId);

	}
	g_parser->endObject();
}

} // namespace testFaceRest
