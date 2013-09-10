/*
 * Copyright (C) 2013 Project Hatohol
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
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBAgentSQLite3.h"
#include "DBClientTest.h"
#include "Params.h"
#include "MultiLangTest.h"

namespace testFaceRest {

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_CONFIG_NAME = "test_db_config";
static const char *TEST_DB_HATOHOL_NAME = "testDatabase-hatohol.db";

static StringVector emptyStringVector;
static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void startFaceRest(void)
{
	string dbPathConfig = getFixturesDir() + TEST_DB_CONFIG_NAME;
	string dbPathHatohol  = getFixturesDir() + TEST_DB_HATOHOL_NAME;
	// TODO: remove the direct call of DBAgentSQLite3's API.
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG, dbPathConfig);
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPathHatohol);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg);
	g_faceRest->start();
}

static JsonParserAgent *getResponseAsJsonParser(
  const string &url, const string &callbackName = "",
  const StringVector &parameters = emptyStringVector,
  bool post = false)
{
	string joinedQueryParams;
	if (!callbackName.empty()) {
		joinedQueryParams = "?callback=";
		joinedQueryParams += callbackName;
	}
	
	for (size_t i = 0; i < parameters.size(); i++) {
		const string &oneQuery = parameters[i];
		if (!joinedQueryParams.empty())
			joinedQueryParams += "&";
		joinedQueryParams += StringUtils::replace(oneQuery, "'", "\\'");
	}

	string postDataArg;
	if (post)
		postDataArg = "--post-data ''";

	// get reply with wget
	string getCmd =
	  StringUtils::sprintf("wget http://localhost:%u%s%s %s -O -",
	                       TEST_PORT, url.c_str(),
	                       joinedQueryParams.c_str(),
	                       postDataArg.c_str());
	string response = executeCommand(getCmd);

	// if JSONP, check the callback name
	if (!callbackName.empty()) {
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
                                 const string &member, uint64_t expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val));
	cppcut_assert_equal(expected, (uint64_t)val);
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

static void _assertNullInParser(JsonParserAgent *parser, const string &member)
{
	bool result = false;
	cppcut_assert_equal(true, parser->isNull(member, result));
	cppcut_assert_equal(true, result);
}
#define assertNullInParser(P,M) cut_trace(_assertNullInParser(P,M))

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
	assertValueInParser(g_parser, "brief", triggerInfo.brief);
}
#define assertTestTriggerInfo(T) cut_trace(_assertTestTriggerInfo(T))

static void assertServersInParser(JsonParserAgent *parser)
{
	assertValueInParser(parser, "numberOfServers",
	                    (uint32_t)NumServerInfo);
	parser->startObject("servers");
	for (size_t i = 0; i < NumServerInfo; i++) {
		parser->startElement(i);
		MonitoringServerInfo &svInfo = serverInfo[i];
		assertValueInParser(parser, "id",   (uint32_t)svInfo.id);
		assertValueInParser(parser, "type", (uint32_t)svInfo.type);
		assertValueInParser(parser, "hostName",  svInfo.hostName);
		assertValueInParser(parser, "ipAddress", svInfo.ipAddress);
		assertValueInParser(parser, "nickname",  svInfo.nickname);
		parser->endElement();
	}
	parser->endObject();
}

static void assertHostsIdNameHashInParser(TriggerInfo *triggers,
                                          size_t numberOfTriggers,
                                          JsonParserAgent *parser)
{
	parser->startObject("servers");
	for (size_t i = 0; i < numberOfTriggers; i++) {
		TriggerInfo &triggerInfo = triggers[i];
		parser->startObject(StringUtils::toString(triggerInfo.serverId));
		parser->startObject("hosts");
		parser->startObject(StringUtils::toString(triggerInfo.hostId));
		assertValueInParser(parser, "name", triggerInfo.hostName);
		parser->endObject();
		parser->endObject();
		parser->endObject();
	}
	parser->endObject();
}

static void assertHostsIdNameHashInParser(EventInfo *events,
                                          size_t numberOfEvents,
                                          JsonParserAgent *parser)
{
	parser->startObject("servers");
	for (size_t i = 0; i < numberOfEvents; i++) {
		EventInfo &eventInfo = events[i];
		parser->startObject(StringUtils::toString(eventInfo.serverId));
		parser->startObject("hosts");
		parser->startObject(StringUtils::toString(eventInfo.hostId));
		assertValueInParser(parser, "name", eventInfo.hostName);
		parser->endObject();
		parser->endObject();
		parser->endObject();
	}
	parser->endObject();
}

static void assertServersIdNameHashInParser(JsonParserAgent *parser)
{
	parser->startObject("servers");
	for (size_t i = 0; i < NumServerInfo; i++) {
		MonitoringServerInfo &svInfo = serverInfo[i];
		parser->startObject(StringUtils::toString(svInfo.id));
		assertValueInParser(parser, "name", svInfo.hostName);
		parser->endObject();
	}
	parser->endObject();
}

static void _assertServers(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_SERVERS);
	assertValueInParser(g_parser, "result", true);
	assertServersInParser(g_parser);
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
	assertHostsIdNameHashInParser(testTriggerInfo, NumTestTriggerInfo,
				      g_parser);
	assertServersIdNameHashInParser(g_parser);
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
		assertValueInParser(g_parser, "type", (uint32_t)eventInfo.type);
		assertValueInParser(g_parser, "triggerId",
		                    (uint32_t)eventInfo.triggerId);
		assertValueInParser(g_parser, "status",
		                    (uint32_t)eventInfo.status);
		assertValueInParser(g_parser, "severity",
		                    (uint32_t)eventInfo.severity);
		assertValueInParser(g_parser, "hostId",   eventInfo.hostId);
		assertValueInParser(g_parser, "brief",    eventInfo.brief);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertHostsIdNameHashInParser(testEventInfo, NumTestEventInfo,
				      g_parser);
	assertServersIdNameHashInParser(g_parser);
}
#define assertEvents(P,...) cut_trace(_assertEvents(P,##__VA_ARGS__))

static void _assertItems(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_ITEMS);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfItems",
	                    (uint32_t)NumTestItemInfo);
	g_parser->startObject("items");
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		g_parser->startElement(i);
		ItemInfo &itemInfo = testItemInfo[i];
		assertValueInParser(g_parser, "serverId", itemInfo.serverId);
		assertValueInParser(g_parser, "hostId", itemInfo.hostId);
		assertValueInParser(g_parser, "brief", itemInfo.brief);
		assertValueInParser(g_parser, "lastValueTime",
		                    itemInfo.lastValueTime);
		assertValueInParser(g_parser, "lastValue", itemInfo.lastValue);
		assertValueInParser(g_parser, "prevValue", itemInfo.prevValue);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertServersIdNameHashInParser(g_parser);
}
#define assertItems(P,...) cut_trace(_assertItems(P,##__VA_ARGS__))

template<typename T>
static void _assertActionCondition(
  JsonParserAgent *parser, const ActionCondition &cond,
   const string &member, ActionConditionEnableFlag bit, T expect)
{
	if (cond.isEnable(bit)) {
		assertValueInParser(parser, member, expect);
	} else {
		assertNullInParser(parser, member);
	}
}
#define asssertActionCondition(PARSER, COND, MEMBER, BIT, T, EXPECT) \
cut_trace(_assertActionCondition<T>(PARSER, COND, MEMBER, BIT, EXPECT))

static void _assertActions(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_ACTIONS);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "numberOfActions",
	                    (uint32_t)NumTestActionDef);
	g_parser->startObject("actions");
	for (size_t i = 0; i < NumTestActionDef; i++) {
		g_parser->startElement(i);
		const ActionDef &actionDef = testActionDef[i];
		const ActionCondition &cond = actionDef.condition;
		assertValueInParser(g_parser, "actionId",
		                    (uint32_t)actionDef.id);

		assertValueInParser(g_parser, "enableBits", cond.enableBits);
		asssertActionCondition(
		  g_parser, cond, "serverId", ACTCOND_SERVER_ID,
		  uint32_t, cond.serverId);
		asssertActionCondition(
		  g_parser, cond, "hostId", ACTCOND_HOST_ID,
		  uint64_t, cond.hostId);
		asssertActionCondition(
		  g_parser, cond, "hostGroupId", ACTCOND_HOST_GROUP_ID,
		  uint64_t, cond.hostGroupId);
		asssertActionCondition(
		  g_parser, cond, "triggerId", ACTCOND_TRIGGER_ID,
		  uint64_t, cond.triggerId);
		asssertActionCondition(
		  g_parser, cond, "triggerStatus", ACTCOND_TRIGGER_STATUS,
		  uint32_t, cond.triggerStatus);
		asssertActionCondition(
		  g_parser, cond, "triggerSeverity", ACTCOND_TRIGGER_SEVERITY,
		  uint32_t, cond.triggerSeverity);

		uint32_t expectCompType
		  = cond.isEnable(ACTCOND_TRIGGER_SEVERITY) ?
		    (uint32_t)cond.triggerSeverityCompType : CMP_INVALID;
		assertValueInParser(g_parser,
		                    "triggerSeverityComparatorType",
		                    expectCompType);

		assertValueInParser(g_parser, "type", (uint32_t)actionDef.type);
		assertValueInParser(g_parser, "workingDirectory",
		                    actionDef.workingDir);
		assertValueInParser(g_parser, "command", actionDef.command);
		assertValueInParser(g_parser, "timeout",
		                    (uint32_t)actionDef.timeout);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertServersIdNameHashInParser(g_parser);
}
#define assertActions(P,...) cut_trace(_assertActions(P,##__VA_ARGS__))

void _assertAddAction(const StringVector &params)
{
	bool post = true;
	startFaceRest();
	g_parser = getResponseAsJsonParser("/actions.jsonp", "foo",
	                                   params, post);
	assertValueInParser(g_parser, "result", true);
	assertValueInParser(g_parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION_ACTIONS);

	// This function asummes that the test database is recreated and
	// is empty. So the added action is the first and the ID should one.
	assertValueInParser(g_parser, "id", (uint32_t)1);
}
#define assertAddAction(P) cut_trace(_assertAddAction(P))

void _assertAddActionError(const StringVector &params)
{
	bool post = true;
	startFaceRest();
	g_parser = getResponseAsJsonParser("/actions.jsonp", "foo",
	                                   params, post);
	assertValueInParser(g_parser, "result", false);
}
#define assertAddActionError(P) cut_trace(_assertAddActionError(P))

static void setupPostAction(void)
{
	bool recreate = true;
	bool loadData = false;
	setupTestDBAction(recreate, loadData);
}

struct LocaleInfo {
	string lcAll;
	string lang;
};

static LocaleInfo *g_localeInfo = NULL;

static void changeLocale(const char *locale)
{
	// save the current locale
	if (!g_localeInfo) {
		g_localeInfo = new LocaleInfo();
		char *env;
		env = getenv("LC_ALL");
		if (env)
			g_localeInfo->lcAll = env;
		env = getenv("LANG");
		if (env)
			g_localeInfo->lang = env;
	}

	// set new locale
	setenv("LC_ALL", locale, 1);
	setenv("LANG", locale, 1);
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDBServers();
}

void cut_teardown(void)
{
	if (g_faceRest) {
		try {
			g_faceRest->stop();
		} catch (const HatoholException &e) {
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

	if (g_localeInfo) {
		if (!g_localeInfo->lcAll.empty())
			setenv("LC_ALL", g_localeInfo->lcAll.c_str(), 1);
		if (!g_localeInfo->lang.empty())
			setenv("LANG", g_localeInfo->lang.c_str(), 1);
		delete g_localeInfo;
		g_localeInfo = NULL;
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

void test_items(void)
{
	assertItems("/items.json");
}

void test_itemsJsonp(void)
{
	assertItems("/items.jsonp", "foo");
}

void test_actionsJsonp(void)
{
	bool recreate = true;
	bool loadData = true;
	setupTestDBAction(recreate, loadData);
	assertActions("/actions.jsonp", "foo");
}

void test_addAction(void)
{
	setupPostAction();

	int type = ACTION_COMMAND;
	const string command = "makan-kosappo";
	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", type));
	params.push_back("command=" + command);
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select * from ";
	statement += DBClientAction::getTableNameActions();
	string expect;
	int expectedId = 1;
	expect += StringUtils::sprintf("%d|", expectedId);
	expect += "#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|";
	expect += StringUtils::sprintf("%d|",type);
	expect += command;
	expect += "||0"; /* workingDirectory and timeout */
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionParamterFull(void)
{
	setupPostAction();

	const string command = "/usr/bin/pochi";
	const string workingDir = "/usr/local/wani";
	int type = ACTION_COMMAND;
	int timeout = 300;
	int serverId= 50;
	uint64_t hostId = 50;
	uint64_t hostGroupId = 1000;
	uint64_t triggerId = 333;
	int triggerStatus = TRIGGER_STATUS_PROBLEM;;
	int triggerSeverity = TRIGGER_SEVERITY_CRITICAL;
	int triggerSeverityCompType = CMP_EQ_GT;

	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", type));
	params.push_back("command=" + command);
	params.push_back("workingDirectory=" + workingDir);
	params.push_back(StringUtils::sprintf("timeout=%d", timeout));
	params.push_back(StringUtils::sprintf("serverId=%d", serverId));
	params.push_back(StringUtils::sprintf("hostId=%"PRIu64, hostId));
	params.push_back(
	  StringUtils::sprintf("hostGroupId=%"PRIu64, hostGroupId));
	params.push_back(StringUtils::sprintf("triggerId=%"PRIu64, triggerId));
	params.push_back(
	  StringUtils::sprintf("triggerStatus=%d", triggerStatus));
	params.push_back(
	  StringUtils::sprintf("triggerSeverity=%d", triggerSeverity));
	params.push_back(
	  StringUtils::sprintf("triggerSeverityCompType=%d",
	                       triggerSeverityCompType));
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select * from ";
	statement += DBClientAction::getTableNameActions();
	string expect;
	int expectedId = 1;
	expect += StringUtils::sprintf("%d|%d|", expectedId, serverId);
	expect += StringUtils::sprintf("%"PRIu64"|%"PRIu64"|%"PRIu64"|",
	  hostId, hostGroupId, triggerId);
	expect += StringUtils::sprintf(
	  "%d|%d|%d|", triggerStatus, triggerSeverity, triggerSeverityCompType);
	expect += StringUtils::sprintf("%d|", type);
	expect += command;
	expect += "|";
	expect += workingDir;
	expect += "|";
	expect += StringUtils::sprintf("%d", timeout);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionParamterOver32bit(void)
{
	setupPostAction();

	const string command = "/usr/bin/pochi";
	uint64_t hostId = 0x89abcdef01234567;
	uint64_t hostGroupId = 0xabcdef0123456789;
	uint64_t triggerId = 0x56789abcdef01234;

	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", ACTION_RESIDENT));
	params.push_back("command=" + command);
	params.push_back(StringUtils::sprintf("hostId=%"PRIu64, hostId));
	params.push_back(
	  StringUtils::sprintf("hostGroupId=%"PRIu64, hostGroupId));
	params.push_back(StringUtils::sprintf("triggerId=%"PRIu64, triggerId));
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select host_id, host_group_id, trigger_id from ";
	statement += DBClientAction::getTableNameActions();
	string expect;
	expect += StringUtils::sprintf("%"PRIu64"|%"PRIu64"|%"PRIu64"",
	  hostId, hostGroupId, triggerId);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}


void test_addActionComplicatedCommand(void)
{
	setupPostAction();

	const string command =
	   "/usr/bin/@hoge -l '?ABC+{[=:;|.,#*`!$%\\~]}FOX-' --X '$^' --name \"@'v'@\"'";
	gchar *encodedCommand = soup_uri_encode(command.c_str(), "+");
	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", ACTION_COMMAND));
	params.push_back(string("command=") + encodedCommand);
	g_free(encodedCommand);
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select command from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, command);
}

void test_addActionCommandWithJapanese(void)
{
	setupPostAction();
	changeLocale("en.UTF-8");

	const string command = COMMAND_EX_JP;
	gchar *encodedCommand = soup_uri_encode(command.c_str(), "+");
	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", ACTION_COMMAND));
	params.push_back(string("command=") + encodedCommand);
	g_free(encodedCommand);
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select command from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, command);
}

void test_addActionWithoutType(void)
{
	StringVector params;
	assertAddActionError(params);
}

void test_addActionWithoutCommand(void)
{
	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", ACTION_COMMAND));
	assertAddActionError(params);
}

void test_addActionInvalidType(void)
{
	StringVector params;
	params.push_back(StringUtils::sprintf("type=%d", ACTION_RESIDENT+1));
	assertAddActionError(params);
}

} // namespace testFaceRest
