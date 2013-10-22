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

class TestFaceRest : public FaceRest {
public:
	static const SessionInfo *callGetSessionInfo(const string &sessionId)
	{
		return getSessionInfo(sessionId);
	}
};

typedef map<string, string>       StringMap;
typedef StringMap::iterator       StringMapIterator;
typedef StringMap::const_iterator StringMapConstIterator;

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_HATOHOL_NAME = "testDatabase-hatohol.db";

static StringMap    emptyStringMap;
static StringVector emptyStringVector;
static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void startFaceRest(void)
{
	string dbPathHatohol  = getFixturesDir() + TEST_DB_HATOHOL_NAME;
	setupTestDBServers();

	// TODO: remove the direct call of DBAgentSQLite3's API.
	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPathHatohol);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg);
	g_faceRest->start();
}

static string makeQueryString(const StringMap &parameters,
                              const string &callbackName)
{
	GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);
	cppcut_assert_not_null(hashTable);
	StringMapConstIterator it = parameters.begin();
	for (; it != parameters.end(); ++it) {
		string key = it->first;
		string val = it->second;
		g_hash_table_insert(hashTable,
		                    (void *)key.c_str(), (void *)val.c_str());
	}
	if (!callbackName.empty()) {
		g_hash_table_insert(hashTable, (void *)"callback",
		                    (void *)callbackName.c_str());
		g_hash_table_insert(hashTable, (void *)"fmt", (void *)"jsonp");
	}

	char *encoded = soup_form_encode_hash(hashTable);
	string queryString = encoded;
	g_free(encoded);
	g_hash_table_unref(hashTable);
	return queryString;
}

static string makeQueryStringForCurlPost(const StringMap &parameters,
                                         const string &callbackName)
{
	string postDataArg;
	StringVector queryVect;
	string joinedString = makeQueryString(parameters, callbackName);
	StringUtils::split(queryVect, joinedString, '&');
	for (size_t i = 0; i < queryVect.size(); i++) {
		postDataArg += " -d ";
		postDataArg += queryVect[i];
	}
	return postDataArg;
}

static JsonParserAgent *getResponseAsJsonParser(
  const string &url, const string &callbackName = "",
  const StringMap &parameters = emptyStringMap,
  const string &request = "GET",
  const StringVector &headersVect = emptyStringVector)
{
	// make encoded query parameters
	string joinedQueryParams;
	string postDataArg;
	if (request == "POST") {
		postDataArg =
		   makeQueryStringForCurlPost(parameters, callbackName);
	} else {
		joinedQueryParams = makeQueryString(parameters, callbackName);
		if (!joinedQueryParams.empty())
			joinedQueryParams = "?" + joinedQueryParams;
	}

	// headers
	string headers;
	for (size_t i = 0; i < headersVect.size(); i++) {
		headers += "-H \"";
		headers += headersVect[i];
		headers += "\" ";
	}

	// get reply with wget
	string getCmd =
	  StringUtils::sprintf("curl -X %s %s %s http://localhost:%u%s%s",
	                       request.c_str(), headers.c_str(),
	                       postDataArg.c_str(), TEST_PORT, url.c_str(),
	                       joinedQueryParams.c_str());
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
                                 const string &member, const bool expected)
{
	bool val;
	cppcut_assert_equal(true, parser->read(member, val),
	                    cut_message("member: %s, expect: %d",
	                                member.c_str(), expected));
	cppcut_assert_equal(expected, val);
}

static void _assertValueInParser(JsonParserAgent *parser,
                                 const string &member, uint32_t expected)
{
	int64_t val;
	cppcut_assert_equal(true, parser->read(member, val),
	                    cut_message("member: %s, expect: %"PRIu32,
	                                member.c_str(), expected));
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

static void _assertErrorCode(JsonParserAgent *parser,
                             const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertValueInParser(parser, "apiVersion",
	                    (uint32_t)FaceRest::API_VERSION);
	assertValueInParser(parser, "errorCode", (uint32_t)expectCode);
}
#define assertErrorCode(P, ...) cut_trace(_assertErrorCode(P, ##__VA_ARGS__))

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

static void assertHostsInParser(JsonParserAgent *parser,
                                uint32_t targetServerId)
{
	HostInfoList hostInfoList;
	getDBCTestHostInfo(hostInfoList, targetServerId);
	assertValueInParser(parser, "numberOfHosts",
	                    (uint32_t)hostInfoList.size());

	// make an index
	map<uint32_t, map<uint64_t, const HostInfo *> > hostInfoMap;
	map<uint64_t, const HostInfo *>::iterator hostMapIt;
	HostInfoListConstIterator it = hostInfoList.begin();
	for (; it != hostInfoList.end(); ++it) {
		const HostInfo &hostInfo = *it;
		hostMapIt = hostInfoMap[hostInfo.serverId].find(hostInfo.id);
		cppcut_assert_equal(
		  true, hostMapIt == hostInfoMap[hostInfo.serverId].end());
		hostInfoMap[hostInfo.serverId][hostInfo.id] = &hostInfo;
	}

	parser->startObject("hosts");
	uint32_t serverId;
	uint64_t hostId;
	for (size_t i = 0; i < hostInfoList.size(); i++) {
		int64_t var64;
		parser->startElement(i);
		cppcut_assert_equal(true, parser->read("serverId", var64));
		serverId = (uint32_t)var64;
		cppcut_assert_equal(true, parser->read("id", var64));
		hostId = (uint64_t)var64;

		hostMapIt = hostInfoMap[serverId].find(hostId);
		cppcut_assert_equal(true,
		                    hostMapIt != hostInfoMap[serverId].end());
		const HostInfo &hostInfo = *hostMapIt->second;

		assertValueInParser(parser, "hostName", hostInfo.hostName);
		parser->endElement();
		hostInfoMap[serverId].erase(hostMapIt);
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

static void _assertTestMode(const bool expectedMode = false,
                            const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser("/test");
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "testMode", expectedMode);
}
#define assertTestMode(E,...) cut_trace(_assertTestMode(E,##__VA_ARGS__))

static void _assertServers(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertErrorCode(g_parser);
	assertServersInParser(g_parser);
}
#define assertServers(P,...) cut_trace(_assertServers(P,##__VA_ARGS__))

static void _assertHosts(const string &path, const string &callbackName = "",
                         uint32_t serverId = ALL_SERVERS)
{
	startFaceRest();
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		   StringUtils::sprintf("%"PRIu32, serverId); 
	}
	g_parser = getResponseAsJsonParser(path, callbackName, queryMap);
	assertErrorCode(g_parser);
	assertHostsInParser(g_parser, serverId);
}
#define assertHosts(P,...) cut_trace(_assertHosts(P,##__VA_ARGS__))

static void _assertTriggers(const string &path, const string &callbackName = "",
                            uint32_t serverId = ALL_SERVERS,
                            uint64_t hostId = ALL_HOSTS)
{
	startFaceRest();

	// calculate the expected test triggers
	map<uint32_t, map<uint64_t, size_t> > indexMap;
	map<uint32_t, map<uint64_t, size_t> >::iterator indexMapIt;
	map<uint64_t, size_t>::iterator trigIdIdxIt;
	getTestTriggersIndexes(indexMap, serverId, hostId);
	size_t expectedNumTrig = 0;
	indexMapIt = indexMap.begin();
	for (; indexMapIt != indexMap.end(); ++indexMapIt)
		expectedNumTrig += indexMapIt->second.size();

	// request
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		   StringUtils::sprintf("%"PRIu32, serverId); 
	}
	if (hostId != ALL_HOSTS)
		queryMap["hostId"] = StringUtils::sprintf("%"PRIu64, hostId); 
	g_parser = getResponseAsJsonParser(path, callbackName, queryMap);

	// Check the reply
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfTriggers",
	                    (uint32_t)expectedNumTrig);
	g_parser->startObject("triggers");
	for (size_t i = 0; i < expectedNumTrig; i++) {
		g_parser->startElement(i);
		int64_t var64;
		cppcut_assert_equal(true, g_parser->read("serverId", var64));
		uint32_t actSvId = (uint32_t)var64;
		cppcut_assert_equal(true, g_parser->read("id", var64));
		uint64_t actTrigId = (uint64_t)var64;

		trigIdIdxIt = indexMap[actSvId].find(actTrigId);
		cppcut_assert_equal(
		  true, trigIdIdxIt != indexMap[actSvId].end());
		size_t idx = trigIdIdxIt->second;
		indexMap[actSvId].erase(trigIdIdxIt);

		TriggerInfo &triggerInfo = testTriggerInfo[idx];
		assertTestTriggerInfo(triggerInfo);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertHostsIdNameHashInParser(testTriggerInfo, expectedNumTrig,
	                              g_parser);
	assertServersIdNameHashInParser(g_parser);
}
#define assertTriggers(P,...) cut_trace(_assertTriggers(P,##__VA_ARGS__))

static void _assertEvents(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertErrorCode(g_parser);
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
	assertErrorCode(g_parser);
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
	assertErrorCode(g_parser);
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

void _assertAddRecord(const StringMap &params, const string &url,
                      const HatoholErrorCode &expectCode = HTERR_OK)
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(url, "foo", params, "POST");
	assertErrorCode(g_parser, expectCode);
	if (expectCode != HTERR_OK)
		return;

	// This function asummes that the test database is recreated and
	// is empty. So the added action is the first and the ID should one.
	assertValueInParser(g_parser, "id", (uint32_t)1);
}

#define assertAddAction(P, ...) \
cut_trace(_assertAddRecord(P, "/action", ##__VA_ARGS__))

void _assertLogin(const string &user, const string &password)
{
	setupTestDBUser(true, true);
	startFaceRest();
	StringMap query;
	if (!user.empty())
		query["user"] = user;
	if (!password.empty())
		query["password"] = password;
	string url = "/login";
	g_parser = getResponseAsJsonParser(url, "cbname", query);
}
#define assertLogin(U,P) cut_trace(_assertLogin(U,P))

static void _assertUsers(const string &path, const string &callbackName = "")
{
	startFaceRest();
	g_parser = getResponseAsJsonParser(path, callbackName);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers",
	                    (uint32_t)NumTestUserInfo);
	g_parser->startObject("users");
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		g_parser->startElement(i);
		const UserInfo &userInfo = testUserInfo[i];
		assertValueInParser(g_parser, "userId", (uint32_t)(i + 1));
		assertValueInParser(g_parser, "name", userInfo.name);
		assertValueInParser(g_parser, "flags", userInfo.flags);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertUsers(P,...) cut_trace(_assertUsers(P,##__VA_ARGS__))

#define assertAddUser(P, ...) \
cut_trace(_assertAddRecord(P, "/user", ##__VA_ARGS__))

void _assertAddUserWithSetup(const StringMap &params,
                             const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = false;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertAddUser(params, expectCode);
}
#define assertAddUserWithSetup(P,C) cut_trace(_assertAddUserWithSetup(P,C))

static void setupTestMode(void)
{
	CommandLineArg arg;
	arg.push_back("--test-mode");
	hatoholInit(&arg);
}

static void _assertUpdateAddUserMissing(
  const StringMap &parameters,
  const HatoholErrorCode expectErrorCode = HTERR_NOT_FOUND_PARAMETER)
{
	setupTestMode();
	startFaceRest();
	string url = "/test/user";
	g_parser = getResponseAsJsonParser(url, "cbname", parameters, "POST");
	assertErrorCode(g_parser, expectErrorCode);
}
#define assertUpdateAddUserMissing(P,...) \
cut_trace(_assertUpdateAddUserMissing(P, ##__VA_ARGS__))

static void _assertUpdateOrAddUser(const string &name)
{
	setupTestMode();
	startFaceRest();

	string url = "/test/user";
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);

	StringMap parameters;
	parameters["user"] = name;
	parameters["password"] = "AR2c43fdsaf";
	parameters["flags"] = "0";

	g_parser = getResponseAsJsonParser(url, "cbname", parameters, "POST");
	assertErrorCode(g_parser, HTERR_OK);
}
#define assertUpdateOrAddUser(U) cut_trace(_assertUpdateOrAddUser(U))

static void setupPostAction(void)
{
	bool recreate = true;
	bool loadData = false;
	setupTestDBAction(recreate, loadData);
}

static void setupActionDB(void)
{
	bool recreate = true;
	bool loadData = true;
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
void test_isTestModeDefault(void)
{
	cppcut_assert_equal(false, FaceRest::isTestMode());
	assertTestMode(false);
}

void test_isTestModeSet(void)
{
	CommandLineArg arg;
	arg.push_back("--test-mode");
	hatoholInit(&arg);
	cppcut_assert_equal(true, FaceRest::isTestMode());
	assertTestMode(true);
}

void test_isTestModeReset(void)
{
	test_isTestModeSet();
	cut_teardown();
	hatoholInit();
	cppcut_assert_equal(false, FaceRest::isTestMode());
	assertTestMode(false);
}

void test_servers(void)
{
	assertServers("/server");
}

void test_serversJsonp(void)
{
	assertServers("/server", "foo");
}

void test_hosts(void)
{
	assertHosts("/host");
}

void test_hostsJsonp(void)
{
	assertHosts("/host", "foo");
}

void test_hostsForOneServer(void)
{
	assertHosts("/host", "foo", testTriggerInfo[0].serverId);
}

void test_triggers(void)
{
	assertTriggers("/trigger");
}

void test_triggersJsonp(void)
{
	assertTriggers("/trigger", "foo");
}

void test_triggersForOneServerOneHost(void)
{
	assertTriggers("/trigger", "foo",
	               testTriggerInfo[1].serverId, testTriggerInfo[1].hostId);
}

void test_events(void)
{
	assertEvents("/event");
}

void test_eventsJsonp(void)
{
	assertEvents("/event", "foo");
}

void test_items(void)
{
	assertItems("/item");
}

void test_itemsJsonp(void)
{
	assertItems("/item", "foo");
}

void test_actionsJsonp(void)
{
	bool recreate = true;
	bool loadData = true;
	setupTestDBAction(recreate, loadData);
	assertActions("/action", "foo");
}

void test_addAction(void)
{
	setupPostAction();

	int type = ACTION_COMMAND;
	const string command = "makan-kosappo";
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", type);
	params["command"] = command;
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
	int triggerStatus = TRIGGER_STATUS_PROBLEM;
	int triggerSeverity = TRIGGER_SEVERITY_CRITICAL;
	int triggerSeverityCompType = CMP_EQ_GT;

	StringMap params;
	params["type"]        = StringUtils::sprintf("%d", type);
	params["command"]     = command;
	params["workingDirectory"] = workingDir;
	params["timeout"]     = StringUtils::sprintf("%d", timeout);
	params["serverId"]    = StringUtils::sprintf("%d", serverId);
	params["hostId"]      = StringUtils::sprintf("%"PRIu64, hostId);
	params["hostGroupId"] = StringUtils::sprintf("%"PRIu64, hostGroupId);
	params["triggerId"]   = StringUtils::sprintf("%"PRIu64, triggerId);
	params["triggerStatus"]   = StringUtils::sprintf("%d", triggerStatus);
	params["triggerSeverity"] = StringUtils::sprintf("%d", triggerSeverity);
	params["triggerSeverityCompType"] =
	   StringUtils::sprintf("%d", triggerSeverityCompType);
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

	StringMap params;
	params["type"]        = StringUtils::sprintf("%d", ACTION_RESIDENT);
	params["command"]     = command;
	params["hostId"]      = StringUtils::sprintf("%"PRIu64, hostId);
	params["hostGroupId"] = StringUtils::sprintf("%"PRIu64, hostGroupId);
	params["triggerId"]   = StringUtils::sprintf("%"PRIu64, triggerId);
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
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
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
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
	assertAddAction(params);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select command from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, command);
}

void test_addActionWithoutType(void)
{
	StringMap params;
	assertAddAction(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionWithoutCommand(void)
{
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	assertAddAction(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionInvalidType(void)
{
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_RESIDENT+1);
	assertAddAction(params, HTERR_INVALID_PARAMETER);
}

void test_deleteAction(void)
{
	startFaceRest();
	setupActionDB(); // make a test action DB.

	int targetId = 2;
	string url = StringUtils::sprintf("/action/%d", targetId);
	g_parser =
	  getResponseAsJsonParser(url, "cbname", emptyStringMap, "DELETE");

	// check the reply
	assertErrorCode(g_parser);

	// check DB
	string expect;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const int expectedId = i + 1;
		if (expectedId == targetId)
			continue;
		expect += StringUtils::sprintf("%d\n", expectedId);
	}
	string statement = "select action_id from ";
	statement += DBClientAction::getTableNameActions();
	DBClientAction dbAction;
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_login(void)
{
	const int targetIdx = 1;
	const UserInfo &userInfo = testUserInfo[targetIdx];
	assertLogin(userInfo.name, userInfo.password);
	assertErrorCode(g_parser);
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	cppcut_assert_equal(false, sessionId.empty());

	const SessionInfo *sessionInfo =
	  TestFaceRest::callGetSessionInfo(sessionId);
	cppcut_assert_not_null(sessionInfo);
	cppcut_assert_equal(targetIdx + 1, sessionInfo->userId);
	assertTimeIsNow(sessionInfo->loginTime);
	assertTimeIsNow(sessionInfo->lastAccessTime);
}

void test_loginFailure(void)
{
	assertLogin(testUserInfo[1].name, testUserInfo[0].password);
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoUserName(void)
{
	assertLogin("", testUserInfo[0].password);
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoPassword(void)
{
	assertLogin(testUserInfo[0].name, "");
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_loginNoUserNameAndPassword(void)
{
	assertLogin("", "");
	assertErrorCode(g_parser, HTERR_AUTH_FAILED);
}

void test_logout(void)
{
	test_login();
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	delete g_parser;
	string url = "/logout";
	StringVector headers;
	headers.push_back(
	  StringUtils::sprintf("%s:%s", FaceRest::SESSION_ID_HEADER_NAME,
	                                sessionId.c_str()));
	g_parser = getResponseAsJsonParser(url, "cbname", emptyStringMap,
	                                   "GET", headers);
	assertErrorCode(g_parser);
	cppcut_assert_null(TestFaceRest::callGetSessionInfo(sessionId));
}

void test_getUser(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertUsers("/user", "cbname");
}

void test_addUser(void)
{
	OperationPrivilegeFlag flags = OPPRVLG_GET_ALL_USERS;
	const string user = "y@ru0";
	const string password = "w(^_^)d";

	StringMap params;
	params["user"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, flags);
	assertAddUserWithSetup(params, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USERS;
	int expectedId = 1;
	string expect = StringUtils::sprintf("%d|%s|%s|%"FMT_OPPRVLG,
	  expectedId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_addUserWithoutUser(void)
{
	StringMap params;
	params["password"] = "w(^_^)d";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserWithoutPassword(void)
{
	StringMap params;
	params["user"] = "y@ru0";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserWithoutFlags(void)
{
	StringMap params;
	params["user"] = "y@ru0";
	params["password"] = "w(^_^)d";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserInvalidUserName(void)
{
	StringMap params;
	params["user"] = "!^.^!"; // '!' and '^' are invalid characters
	params["password"] = "w(^_^)d";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_INVALID_CHAR);
}

void test_deleteUser(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const UserIdType targetId = 2;
	string url = StringUtils::sprintf("/user/%"FMT_USER_ID, targetId);
	g_parser =
	  getResponseAsJsonParser(url, "cbname", emptyStringMap, "DELETE");

	// check the reply
	assertErrorCode(g_parser);
	UserIdSet userIdSet;
	userIdSet.insert(targetId);
	assertUsersInDB(userIdSet);
}

void test_deleteUserWithoutId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	string url = StringUtils::sprintf("/user");
	g_parser =
	  getResponseAsJsonParser(url, "cbname", emptyStringMap, "DELETE");
	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserWithNonNumericId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	string url = StringUtils::sprintf("/user/zoo");
	g_parser =
	  getResponseAsJsonParser(url, "cbname", emptyStringMap, "DELETE");
	assertErrorCode(g_parser, HTERR_INVALID_PARAMETER);
}

void test_updateOrAddUserNotInTestMode(void)
{
	startFaceRest();
	string url = StringUtils::sprintf("/test/user");
	g_parser =
	  getResponseAsJsonParser(url, "cbname", emptyStringMap, "POST");
	assertErrorCode(g_parser, HTERR_NOT_TEST_MODE);
}

void test_updateOrAddUserMissingUser(void)
{
	StringMap parameters;
	parameters["password"] = "foo";
	parameters["flags"] = "0";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserMissingPassword(void)
{
	StringMap parameters;
	parameters["user"] = "ABC";
	parameters["flags"] = "2";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserMissingFlags(void)
{
	StringMap parameters;
	parameters["user"] = "ABC";
	parameters["password"] = "AR2c43fdsaf";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserAdd(void)
{
	const string name = "Tux";
	// make sure the name is not in test data.
	for (size_t i = 0; i < NumTestUserInfo; i++)
		cppcut_assert_not_equal(name, testUserInfo[i].name);
	assertUpdateOrAddUser(name);
}

void test_updateOrAddUserUpdate(void)
{
	setupTestMode();
	startFaceRest();

	string url = "/test/user";
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const size_t targetIndex = 2;
	const UserInfo &userInfo = testUserInfo[targetIndex];

	StringMap parameters;
	parameters["user"] = userInfo.name;
	parameters["password"] = "AR2c43fdsaf";
	parameters["flags"] = "0";

	g_parser = getResponseAsJsonParser(url, "cbname", parameters, "POST");
	assertErrorCode(g_parser, HTERR_OK);
}

} // namespace testFaceRest
