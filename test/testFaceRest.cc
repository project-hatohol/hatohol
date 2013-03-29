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

void setup(void)
{
	asuraInit();
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

} // namespace testFaceRest
