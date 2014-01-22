/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <MutexLock.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include "DBClientTest.h"
#include "Params.h"
#include "MultiLangTest.h"
#include "CacheServiceDBClient.h"
#include "UnifiedDataStore.h"
#include "ZabbixAPIEmulator.h"
#include "SessionManager.h"
using namespace std;
using namespace mlpl;

namespace testFaceRest {

typedef map<string, string>       StringMap;
typedef StringMap::iterator       StringMapIterator;
typedef StringMap::const_iterator StringMapConstIterator;

static const unsigned int TEST_PORT = 53194;
static const char *TEST_DB_HATOHOL_NAME = "testDatabase-hatohol.db";

static StringMap    emptyStringMap;
static StringVector emptyStringVector;
static FaceRest *g_faceRest = NULL;
static JsonParserAgent *g_parser = NULL;

static void setupUserDB(void)
{
	const bool dbRecreate = true;
	// If loadTestDat is true, not only the user DB but also
	// the access info DB will be loaded. So we set false here
	// and load the user DB later.
	const bool loadTestDat = false;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUser();
}

static void startFaceRest(void)
{
	struct : public FaceRestParam {
		MutexLock mutex;
		virtual void setupDoneNotifyFunc(void) 
		{
			mutex.unlock();
		}
	} param;

	string dbPathHatohol  = getFixturesDir() + TEST_DB_HATOHOL_NAME;
	setupTestDBServers();

	defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPathHatohol);

	CommandLineArg arg;
	arg.push_back("--face-rest-port");
	arg.push_back(StringUtils::sprintf("%u", TEST_PORT));
	g_faceRest = new FaceRest(arg, &param);
	g_faceRest->setNumberOfPreLoadWorkers(1);

	param.mutex.lock();
	g_faceRest->start();

	// wait for the setup completion of FaceReset
	param.mutex.lock();
}

static string makeSessionIdHeader(const string &sessionId)
{
	string header =
	  StringUtils::sprintf("%s:%s", FaceRest::SESSION_ID_HEADER_NAME,
	                                sessionId.c_str());
	return header;
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

struct RequestArg {
	// input
	string       url;
	string       callbackName;
	string       request;
	StringMap    parameters;
	StringVector headers;
	UserIdType   userId;

	// output
	string response;
	StringVector responseHeaders;

	RequestArg(const string &_url, const string &_cbname = "")
	: url(_url),
	  callbackName(_cbname),
	  request("GET"),
	  userId(INVALID_USER_ID)
	{
	}
};

static void getSessionId(RequestArg &arg);
static void getServerResponse(RequestArg &arg)
{
	getSessionId(arg);

	// make encoded query parameters
	string joinedQueryParams;
	string postDataArg;
	if (arg.request == "POST" || arg.request == "PUT") {
		postDataArg =
		   makeQueryStringForCurlPost(arg.parameters, arg.callbackName);
	} else {
		joinedQueryParams = makeQueryString(arg.parameters,
		                                    arg.callbackName);
		if (!joinedQueryParams.empty())
			joinedQueryParams = "?" + joinedQueryParams;
	}

	// headers
	string headers;
	for (size_t i = 0; i < arg.headers.size(); i++) {
		headers += "-H \"";
		headers += arg.headers[i];
		headers += "\" ";
	}

	// get reply with wget
	string getCmd =
	  StringUtils::sprintf("curl -X %s %s %s -i http://localhost:%u%s%s",
	                       arg.request.c_str(), headers.c_str(),
	                       postDataArg.c_str(), TEST_PORT, arg.url.c_str(),
	                       joinedQueryParams.c_str());
	arg.response = executeCommand(getCmd);
	const string separator = "\r\n\r\n";
	size_t pos = arg.response.find(separator);
	if (pos != string::npos) {
		string headers = arg.response.substr(0, pos);
		arg.response = arg.response.substr(pos + separator.size());
		gchar **tmp = g_strsplit(headers.c_str(), "\r\n", -1);
		for (size_t i = 0; tmp[i]; i++)
			arg.responseHeaders.push_back(tmp[i]);
		g_strfreev(tmp);
	}
}

static JsonParserAgent *getResponseAsJsonParser(RequestArg &arg)
{
	getServerResponse(arg);

	// if JSONP, check the callback name
	if (!arg.callbackName.empty()) {
		size_t lenCallbackName = arg.callbackName.size();
		size_t minimumLen = lenCallbackName + 2; // +2 for ''(' and ')'
		cppcut_assert_equal(true, arg.response.size() > minimumLen,
		  cut_message("length: %zd, minmumLen: %zd\n%s",
		              arg.response.size(), minimumLen,
		              arg.response.c_str()));

		cut_assert_equal_substring(
		  arg.callbackName.c_str(), arg.response.c_str(),
		  lenCallbackName);
		cppcut_assert_equal(')', arg.response[arg.response.size()-1]);
		arg.response = string(arg.response, lenCallbackName+1,
		                      arg.response.size() - minimumLen);
	}

	// check the JSON body
	if (isVerboseMode())
		cut_notify("<<response>>\n%s\n", arg.response.c_str());
	JsonParserAgent *parser = new JsonParserAgent(arg.response);
	if (parser->hasError()) {
		string parserErrMsg = parser->getErrorMessage();
		delete parser;
		cut_fail("%s\n%s", parserErrMsg.c_str(), arg.response.c_str());
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
	RequestArg arg("/test");
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "testMode", expectedMode);
}
#define assertTestMode(E,...) cut_trace(_assertTestMode(E,##__VA_ARGS__))

static void _assertServers(const string &path, const string &callbackName = "")
{
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertServersInParser(g_parser);
}
#define assertServers(P,...) cut_trace(_assertServers(P,##__VA_ARGS__))

static void _assertHosts(const string &path, const string &callbackName = "",
                         uint32_t serverId = ALL_SERVERS)
{
	setupUserDB();
	startFaceRest();
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		   StringUtils::sprintf("%"PRIu32, serverId); 
	}
	RequestArg arg(path, callbackName);
	arg.parameters = queryMap;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertHostsInParser(g_parser, serverId);
}
#define assertHosts(P,...) cut_trace(_assertHosts(P,##__VA_ARGS__))

static void _assertTriggers(const string &path, const string &callbackName = "",
                            uint32_t serverId = ALL_SERVERS,
                            uint64_t hostId = ALL_HOSTS)
{
	setupUserDB();
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
	RequestArg arg(path, callbackName);
	arg.parameters = queryMap;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);

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
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfEvents",
	                    (uint32_t)NumTestEventInfo);
	g_parser->startObject("events");
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		g_parser->startElement(i);
		EventInfo &eventInfo = testEventInfo[i];
		assertValueInParser(g_parser, "unifiedId", i + 1);
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
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
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
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_USER);
	g_parser = getResponseAsJsonParser(arg);
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
                      const UserIdType &userId = INVALID_USER_ID,
                      const HatoholErrorCode &expectCode = HTERR_OK,
                      uint32_t expectedId = 1)
{
	startFaceRest();
	RequestArg arg(url, "foo");
	arg.parameters = params;
	arg.request = "POST";
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, expectCode);
	if (expectCode != HTERR_OK)
		return;
	assertValueInParser(g_parser, "id", expectedId);
}

void _assertUpdateRecord(const StringMap &params, const string &baseUrl,
                         uint32_t targetId = 1,
                         const UserIdType &userId = INVALID_USER_ID,
                         const HatoholErrorCode &expectCode = HTERR_OK)
{
	startFaceRest();
	string url;
	uint32_t invalidId = -1;
	if (targetId == invalidId)
		url = baseUrl;
	else
		url = baseUrl + StringUtils::sprintf("/%"PRIu32, targetId);
	RequestArg arg(url, "foo");
	arg.parameters = params;
	arg.request = "PUT";
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, expectCode);
	if (expectCode != HTERR_OK)
		return;
	assertValueInParser(g_parser, "id", targetId);
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
	RequestArg arg("/login", "cbname");
	arg.parameters = query;
	g_parser = getResponseAsJsonParser(arg);
}
#define assertLogin(U,P) cut_trace(_assertLogin(U,P))

static void getSessionId(RequestArg &arg)
{
	if (arg.userId == INVALID_USER_ID)
		return;
	SessionManager *sessionMgr = SessionManager::getInstance();
	const string sessionId = sessionMgr->create(arg.userId);
	arg.headers.push_back(makeSessionIdHeader(sessionId));
}

static void _assertUser(JsonParserAgent *parser, const UserInfo &userInfo,
                        uint32_t expectUserId = 0)
{
	if (expectUserId)
		assertValueInParser(parser, "userId", expectUserId);
	assertValueInParser(parser, "name", userInfo.name);
	assertValueInParser(parser, "flags", userInfo.flags);
}
#define assertUser(P,I,...) cut_trace(_assertUser(P,I,##__VA_ARGS__))

static void _assertUsers(const string &path, const UserIdType &userId,
                         const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers",
	                    (uint32_t)NumTestUserInfo);
	g_parser->startObject("users");
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		g_parser->startElement(i);
		const UserInfo &userInfo = testUserInfo[i];
		assertUser(g_parser, userInfo, (uint32_t)(i + 1));
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertUsers(P, U, ...) cut_trace(_assertUsers(P, U, ##__VA_ARGS__))

#define assertAddUser(P, ...) \
cut_trace(_assertAddRecord(P, "/user", ##__VA_ARGS__))

void _assertAddUserWithSetup(const StringMap &params,
                             const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	assertAddUser(params, userId, expectCode, NumTestUserInfo + 1);
}
#define assertAddUserWithSetup(P,C) cut_trace(_assertAddUserWithSetup(P,C))

#define assertUpdateUser(P, ...) \
cut_trace(_assertUpdateRecord(P, "/user", ##__VA_ARGS__))

void _assertUpdateUserWithSetup(const StringMap &params,
                                uint32_t targetUserId,
                                const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	assertUpdateUser(params, targetUserId, userId, expectCode);
}
#define assertUpdateUserWithSetup(P,U,C) \
cut_trace(_assertUpdateUserWithSetup(P,U,C))

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
	RequestArg arg("/test/user", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, expectErrorCode);
}
#define assertUpdateAddUserMissing(P,...) \
cut_trace(_assertUpdateAddUserMissing(P, ##__VA_ARGS__))

static void _assertUpdateOrAddUser(const string &name)
{
	setupTestMode();
	startFaceRest();

	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);

	StringMap parameters;
	parameters["user"] = name;
	parameters["password"] = "AR2c43fdsaf";
	parameters["flags"] = "0";

	RequestArg arg("/test/user", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_OK);

	// check the data in the DB
	string statement = StringUtils::sprintf(
	  "select name,password,flags from %s where name='%s'",
	  DBClientUser::TABLE_NAME_USERS, name.c_str());
	string expect = StringUtils::sprintf("%s|%s|%s",
	  parameters["user"].c_str(),
	  Utils::sha256( parameters["password"]).c_str(),
	  parameters["flags"].c_str());
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
}
#define assertUpdateOrAddUser(U) cut_trace(_assertUpdateOrAddUser(U))

static void _assertServerAccessInfo(JsonParserAgent *parser, HostGrpAccessInfoMap &expected)
{
	cut_assert_true(parser->startObject("allowedHostGroups"));
	HostGrpAccessInfoMapIterator it = expected.begin();
	for (size_t i = 0; i < expected.size(); i++) {
		uint64_t hostGroupId = it->first;
		string idStr;
		if (hostGroupId == ALL_HOST_GROUPS)
			idStr = "-1";
		else
			idStr = StringUtils::sprintf("%"PRIu64, hostGroupId);
		parser->startObject(idStr);
		AccessInfo *info = it->second;
		assertValueInParser(parser, "accessInfoId",
				    static_cast<uint64_t>(info->id));
		parser->endObject();
	}
	parser->endObject();
}
#define assertServerAccessInfo(P,I,...) cut_trace(_assertServerAccessInfo(P,I,##__VA_ARGS__))

static void _assertAllowedServers(const string &path, const UserIdType &userId,
                                  const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	ServerAccessInfoMap srvAccessInfoMap;
	makeServerAccessInfoMap(srvAccessInfoMap, userId);
	g_parser->startObject("allowedServers");
	ServerAccessInfoMapIterator it = srvAccessInfoMap.begin();
	for (; it != srvAccessInfoMap.end(); ++it) {
		uint32_t serverId = it->first;
		string idStr;
		if (serverId == ALL_SERVERS)
			idStr = "-1";
		else
			idStr = StringUtils::toString(serverId);
		cut_assert_true(g_parser->startObject(idStr));
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		assertServerAccessInfo(g_parser, *hostGrpAccessInfoMap);
		g_parser->endObject();
	}
	g_parser->endObject();
	DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
}
#define assertAllowedServers(P,...) cut_trace(_assertAllowedServers(P,##__VA_ARGS__))

#define assertAddAccessInfo(U, P, USER_ID, ...) \
cut_trace(_assertAddRecord(P, U, USER_ID, ##__VA_ARGS__))

void _assertAddAccessInfoWithCond(
  const string &serverId, const string &hostGroupId,
  string expectHostGroupId = "")
{
	setupUserDB();

	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	const UserIdType targetUserId = 1;

	const string url = StringUtils::sprintf(
	  "/user/%"FMT_USER_ID"/access-info", targetUserId);
	StringMap params;
	params["serverId"] = serverId;
	params["hostGroupId"] = hostGroupId;
	assertAddAccessInfo(url, params, userId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_ACCESS_LIST;
	int expectedId = 1;
	if (expectHostGroupId.empty())
		expectHostGroupId = hostGroupId;
	string expect = StringUtils::sprintf(
	  "%"FMT_ACCESS_INFO_ID"|%"FMT_USER_ID"|%s|%s\n",
	  expectedId, targetUserId, serverId.c_str(),
	  expectHostGroupId.c_str());
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}
#define assertAddAccessInfoWithCond(SVID, HGRP_ID, ...) \
cut_trace(_assertAddAccessInfoWithCond(SVID, HGRP_ID, ##__VA_ARGS__))

static void _assertUserRole(JsonParserAgent *parser,
			    const UserRoleInfo &userRoleInfo,
			    uint32_t expectUserRoleId = 0)
{
	if (expectUserRoleId)
		assertValueInParser(parser, "userRoleId", expectUserRoleId);
	assertValueInParser(parser, "name", userRoleInfo.name);
	assertValueInParser(parser, "flags", userRoleInfo.flags);
}
#define assertUserRole(P,I,...) cut_trace(_assertUserRole(P,I,##__VA_ARGS__))

static void _assertUserRoles(const string &path,
			     const UserIdType &userId,
			     const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUserRoles",
	                    (uint32_t)NumTestUserRoleInfo);
	g_parser->startObject("userRoles");
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		g_parser->startElement(i);
		const UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		assertUserRole(g_parser, userRoleInfo, (uint32_t)(i + 1));
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertUserRoles(P, U, ...) \
  cut_trace(_assertUserRoles(P, U, ##__VA_ARGS__))

static void assertHostGroupsInParser(JsonParserAgent *parser, uint32_t serverId)
{
	// TODO: currently only one hostGroup "No group" exists in the object
	cut_assert_true(parser->startObject("hostGroups"));
	cut_assert_true(parser->startObject("0"));
	assertValueInParser(parser, string("name"), string("No group"));
	parser->endObject();
	parser->endObject();
}

static void assertHostStatusInParser(JsonParserAgent *parser, uint32_t serverId)
{
	parser->startObject("hostStatus");
	// TODO: currently only one hostGroup "No group" exists in the array
	parser->startElement(0);
	assertValueInParser(parser, "hostGroupId",   (uint32_t)0);
	size_t expected_good_hosts = getNumberOfTestHostsWithStatus(
	  serverId, ALL_HOST_GROUPS, true);
	size_t expected_bad_hosts = getNumberOfTestHostsWithStatus(
	  serverId, ALL_HOST_GROUPS, false);
	assertValueInParser(parser, "numberOfGoodHosts", expected_good_hosts);
	assertValueInParser(parser, "numberOfBadHosts", expected_bad_hosts);
	parser->endElement();
	parser->endObject();
}

static void assertSystemStatusInParser(JsonParserAgent *parser, uint32_t serverId)
{
	parser->startObject("systemStatus");
	// TODO: currently only one hostGroup "No group" exists in the array
	uint64_t hostGroupId = 0;
	for (int severity = 0; severity < NUM_TRIGGER_SEVERITY; ++severity) {
		uint32_t expected_triggers = getNumberOfTestTriggers(
		  serverId, hostGroupId, static_cast<TriggerSeverityType>(severity));
		parser->startElement(severity);
		assertValueInParser(parser, "hostGroupId", (uint32_t)0);
		assertValueInParser(parser, "severity", (uint32_t)severity);
		assertValueInParser(parser, "numberOfTriggers", expected_triggers);
		parser->endElement();
	}
	parser->endObject();
}

static void _assertOverviewInParser(JsonParserAgent *parser)
{
	assertValueInParser(parser, "numberOfServers",
	                    (uint32_t)NumServerInfo);
	parser->startObject("serverStatus");
	for (size_t i = 0; i < NumServerInfo; i++) {
		parser->startElement(i);
		MonitoringServerInfo &svInfo = serverInfo[i];
		assertValueInParser(parser, "serverId",   (uint32_t)svInfo.id);
		assertValueInParser(parser, "serverHostName",  svInfo.hostName);
		assertValueInParser(parser, "serverIpAddr", svInfo.ipAddress);
		assertValueInParser(parser, "serverNickname",  svInfo.nickname);
		assertValueInParser(parser, "numberOfHosts",
				    getNumberOfTestHosts(svInfo.id));
		assertValueInParser(parser, "numberOfItems",
				    getNumberOfTestItems(svInfo.id));
		assertValueInParser(parser, "numberOfTriggers",
				    getNumberOfTestTriggers(svInfo.id));
		uint32_t zero = 0;
		assertValueInParser(parser, "numberOfUsers", zero);
		assertValueInParser(parser, "numberOfOnlineUsers", zero);
		assertValueInParser(parser, "numberOfMonitoredItemsPerSecond", zero);
		assertHostGroupsInParser(parser, svInfo.id);
		assertSystemStatusInParser(parser, svInfo.id);
		assertHostStatusInParser(parser, svInfo.id);
		parser->endElement();
	}
	parser->endObject();

	// TODO: check badServers
}
#define assertOverviewInParser(P) cut_trace(_assertOverviewInParser(P))

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

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->setCopyOnDemandEnabled(false);
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

void test_testPost(void)
{
	setupTestMode();
	startFaceRest();
	StringMap parameters;
	parameters["AB"] = "Foo Goo N2";
	parameters["<!>"] = "? @ @ v '";
	parameters["O.O -v@v-"] = "'<< x234 >>'";
	RequestArg arg("/test", "cbname");
	arg.parameters = parameters;
	arg.request = "POST";
	g_parser = getResponseAsJsonParser(arg);
	cppcut_assert_equal(true, g_parser->startObject("queryData"));
	StringMapIterator it = parameters.begin();
	for (; it != parameters.end(); ++it)
		assertValueInParser(g_parser, it->first, it->second);
}

void test_testError(void)
{
	setupTestMode();
	startFaceRest();
	RequestArg arg("/test/error");
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_ERROR_TEST);
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

void test_eventsStartIdWithoutSortOrder(void)
{
	setupUserDB();
	startFaceRest();
	StringMap parameters;
	parameters["startId"] = "5";
	RequestArg arg("/event", "hoge");
	// Any user can be applied (we just have to pass any session ID)
	arg.userId = findUserWithout(OPPRVLG_GET_ALL_SERVER);
	arg.parameters = parameters;
	JsonParserAgent *g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_FOUND_SORT_ORDER);
}

void test_items(void)
{
	assertItems("/item");
}

void test_itemsAsyncWithNoArm(void)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->setCopyOnDemandEnabled(true);
	assertItems("/item");
}

void test_itemsAsyncWithNonExistentServers(void)
{
	cut_omit("This test will take too long time due to long default "
		 "timeout values of ArmZabbixAPI.\n");

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->setCopyOnDemandEnabled(true);
	dataStore->start();
	assertItems("/item");
	dataStore->stop();
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
	setupUserDB();

	int type = ACTION_COMMAND;
	const string command = "makan-kosappo";
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", type);
	params["command"] = command;
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

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
	expect += StringUtils::sprintf("|%"FMT_USER_ID, userId);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionParameterFull(void)
{
	setupPostAction();
	setupUserDB();

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
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

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
	expect += StringUtils::sprintf("%d|%"FMT_USER_ID, timeout, userId);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionParameterOver32bit(void)
{
	setupUserDB();
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
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

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
	setupUserDB();

	const string command =
	   "/usr/bin/@hoge -l '?ABC+{[=:;|.,#*`!$%\\~]}FOX-' --X '$^' --name \"@'v'@\"'";
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION));

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select command from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, command);
}

void test_addActionCommandWithJapanese(void)
{
	setupPostAction();
	setupUserDB();
	changeLocale("en.UTF-8");

	const string command = COMMAND_EX_JP;
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION));

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select command from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, command);
}

void test_addActionWithoutType(void)
{
	setupUserDB();
	StringMap params;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionWithoutCommand(void)
{
	setupUserDB();
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionInvalidType(void)
{
	setupUserDB();
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_RESIDENT+1);
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_INVALID_PARAMETER);
}

void test_deleteAction(void)
{
	startFaceRest();
	setupUserDB();
	setupActionDB(); // make a test action DB.

	int targetId = 2;
	string url = StringUtils::sprintf("/action/%d", targetId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_ACTION);
	g_parser = getResponseAsJsonParser(arg);

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

	SessionManager *sessionMgr = SessionManager::getInstance();
	const SessionPtr session = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, session.hasData());
	cppcut_assert_equal(targetIdx + 1, session->userId);
	assertTimeIsNow(session->loginTime);
	assertTimeIsNow(session->lastAccessTime);
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

	RequestArg arg("/logout", "cbname");
	arg.headers.push_back(makeSessionIdHeader(sessionId));
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	SessionManager *sessionMgr = SessionManager::getInstance();
	const SessionPtr session = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(false, session.hasData());
}

void test_getUser(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_GET_ALL_USER);
	assertUsers("/user", userId, "cbname");
}

void test_getUserMe(void)
{
	const UserInfo &user = testUserInfo[1];
	assertLogin(user.name, user.password);
	assertErrorCode(g_parser);
	string sessionId;
	cppcut_assert_equal(true, g_parser->read("sessionId", sessionId));
	delete g_parser;
	g_parser = NULL;

	RequestArg arg("/user/me", "cbname");
	arg.headers.push_back(makeSessionIdHeader(sessionId));
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers", (uint32_t)1);
	g_parser->startObject("users");
	g_parser->startElement(0);
	assertUser(g_parser, user);
}

void test_addUser(void)
{
	OperationPrivilegeFlag flags = OPPRVLG_GET_ALL_USER;
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
	statement += " order by id desc limit 1";
	const int expectedId = NumTestUserInfo + 1;
	string expect = StringUtils::sprintf("%d|%s|%s|%"FMT_OPPRVLG,
	  expectedId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUser(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["user"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%"FMT_OPPRVLG,
	  targetId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithoutPassword(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string expectedPassword = testUserInfo[targetId - 1].password;

	StringMap params;
	params["user"] = user;
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%"FMT_OPPRVLG,
	  targetId, user.c_str(),
	  Utils::sha256(expectedPassword).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithoutUserId(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string expectedPassword = testUserInfo[targetId - 1].password;

	StringMap params;
	params["user"] = user;
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, -1, HTERR_NOT_FOUND_ID_IN_URL);
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

	const UserIdType userId = findUserWith(OPPRVLG_DELETE_USER);
	string url = StringUtils::sprintf("/user/%"FMT_USER_ID, userId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser);
	UserIdSet userIdSet;
	userIdSet.insert(userId);
	assertUsersInDB(userIdSet);
}

void test_deleteUserWithoutId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	RequestArg arg("/user", "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserWithNonNumericId(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	RequestArg arg("/user/zoo", "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser, HTERR_NOT_FOUND_ID_IN_URL);
}

void test_updateOrAddUserNotInTestMode(void)
{
	setupUserDB();
	startFaceRest();
	RequestArg arg("/test/user", "cbname");
	arg.request = "POST";
	arg.userId = findUserWith(OPPRVLG_CREATE_USER);
	g_parser = getResponseAsJsonParser(arg);
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
	const size_t targetIndex = 2;
	const UserInfo &userInfo = testUserInfo[targetIndex];
	assertUpdateOrAddUser(userInfo.name);
}

void test_getAccessInfo(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertAllowedServers("/user/1/access-info", 1, "cbname");
}

void test_getAccessInfoWithUserId3(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	assertAllowedServers("/user/3/access-info", 3, "cbname");
}

void test_addAccessInfo(void)
{
	const string serverId = "2";
	const string hostGroupId = "3";
	assertAddAccessInfoWithCond(serverId, hostGroupId);
}

void test_addAccessInfoWithAllHostGroups(void)
{
	const string serverId = "2";
	const string hostGroupId =
	  StringUtils::sprintf("%"PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostGroupId);
}

void test_addAccessInfoWithAllHostGroupsNegativeValue(void)
{
	const string serverId = "2";
	const string hostGroupId = "-1";
	const string expectHostGroup =
	  StringUtils::sprintf("%"PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostGroupId, expectHostGroup);
}

void test_addAccessInfoWithExistingData(void)
{
	const bool dbRecreate = true;
	const bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	const UserIdType targetUserId = 1;
	const string serverId = "1";
	const string hostGroupId = "1";

	StringMap params;
	params["userId"] = StringUtils::sprintf("%"FMT_USER_ID, userId);
	params["serverId"] = serverId;
	params["hostGroupId"] = hostGroupId;

	const string url = StringUtils::sprintf(
	  "/user/%"FMT_USER_ID"/access-info", targetUserId);
	assertAddAccessInfo(url, params, userId, HTERR_OK, 2);

	AccessInfoIdSet accessInfoIdSet;
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_deleteAccessInfo(void)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const AccessInfoIdType targetId = 2;
	string url = StringUtils::sprintf(
	  "/user/1/access-info/%"FMT_ACCESS_INFO_ID,
	  targetId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_UPDATE_USER);
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser);
	AccessInfoIdSet accessInfoIdSet;
	accessInfoIdSet.insert(targetId);
	assertAccessInfoInDB(accessInfoIdSet);
}

void test_getUserRole(void)
{
	const bool dbRecreate = true;
	const bool loadTestDat = false;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUserRole();
	assertUserRoles("/user-role", USER_ID_SYSTEM, "cbname");
}

#define assertAddUserRole(P, ...) \
cut_trace(_assertAddRecord(P, "/user-role", ##__VA_ARGS__))

void _assertAddUserRoleWithSetup(const StringMap &params,
				 const HatoholErrorCode &expectCode,
				 bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUserRole();
	UserIdType userId;
	if (operatorHasPrivilege)
		userId = findUserWith(OPPRVLG_CREATE_USER_ROLE);
	else
		userId = findUserWithout(OPPRVLG_CREATE_USER_ROLE);
	assertAddUserRole(params, userId, expectCode, NumTestUserRoleInfo + 1);
}
#define assertAddUserRoleWithSetup(P, C, ...) \
cut_trace(_assertAddUserRoleWithSetup(P, C, ##__VA_ARGS__))

void test_addUserRole(void)
{
	UserRoleInfo expectedUserRoleInfo;
	expectedUserRoleInfo.id = NumTestUserRoleInfo + 1;
	expectedUserRoleInfo.name = "maintainer";
	expectedUserRoleInfo.flags = ALL_PRIVILEGES;

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	params["flags"] = StringUtils::sprintf(
	  "%"FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertAddUserRoleWithSetup(params, HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_addUserRoleWithoutPrivilege(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, ALL_PRIVILEGES);
	bool operatorHasPrivilege = true;
	assertAddUserRoleWithSetup(params, HTERR_NO_PRIVILEGE,
				   !operatorHasPrivilege);

	assertUserRolesInDB();
}

void test_addUserRoleWithoutName(void)
{
	StringMap params;
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, ALL_PRIVILEGES);
	assertAddUserRoleWithSetup(params, HTERR_NOT_FOUND_PARAMETER);

	assertUserRolesInDB();
}

void test_addUserRoleWithoutFlags(void)
{
	StringMap params;
	params["name"] = "maintainer";
	assertAddUserRoleWithSetup(params, HTERR_NOT_FOUND_PARAMETER);

	assertUserRolesInDB();
}

void test_addUserRoleWithEmptyUserName(void)
{
	StringMap params;
	params["name"] = "";
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, ALL_PRIVILEGES);
	assertAddUserRoleWithSetup(params, HTERR_EMPTY_USER_ROLE_NAME);

	assertUserRolesInDB();
}

void test_addUserRoleWithInvalidFlags(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf(
	  "%"FMT_OPPRVLG, ALL_PRIVILEGES + 1);
	assertAddUserRoleWithSetup(params, HTERR_INVALID_USER_FLAGS);

	assertUserRolesInDB();
}

#define assertUpdateUserRole(P, ...) \
cut_trace(_assertUpdateRecord(P, "/user-role", ##__VA_ARGS__))

void _assertUpdateUserRoleWithSetup(const StringMap &params,
				    uint32_t targetUserRoleId,
				    const HatoholErrorCode &expectCode,
				    bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	loadTestDBUserRole();
	UserIdType userId;
	if (operatorHasPrivilege)
		userId = findUserWith(OPPRVLG_UPDATE_ALL_USER_ROLE);
	else
		userId = findUserWithout(OPPRVLG_UPDATE_ALL_USER_ROLE);
	assertUpdateUserRole(params, targetUserRoleId, userId, expectCode);
}
#define assertUpdateUserRoleWithSetup(P,T,E, ...) \
cut_trace(_assertUpdateUserRoleWithSetup(P,T,E, ##__VA_ARGS__))

void test_updateUserRole(void)
{
	UserRoleInfo expectedUserRoleInfo;
	expectedUserRoleInfo.id = 1;
	expectedUserRoleInfo.name = "ServerAdmin";
	expectedUserRoleInfo.flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	params["flags"] = StringUtils::sprintf(
	  "%"FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutName(void)
{
	int targetIdx = 0;
	UserRoleInfo expectedUserRoleInfo = testUserRoleInfo[targetIdx];
	expectedUserRoleInfo.id = targetIdx + 1;
	expectedUserRoleInfo.flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["flags"] = StringUtils::sprintf(
	  "%"FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutFlags(void)
{
	int targetIdx = 0;
	UserRoleInfo expectedUserRoleInfo = testUserRoleInfo[targetIdx];
	expectedUserRoleInfo.id = targetIdx + 1;
	expectedUserRoleInfo.name = "ServerAdmin";

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	assertUpdateUserRoleWithSetup(params, expectedUserRoleInfo.id,
				      HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_updateUserRoleWithoutPrivilege(void)
{
	UserRoleIdType targetUserRoleId = 1;
	OperationPrivilegeFlag flags =
	  (1 << OPPRVLG_UPDATE_SERVER) | ( 1 << OPPRVLG_DELETE_SERVER);

	StringMap params;
	params["name"] = "ServerAdmin";
	params["flags"] = StringUtils::sprintf("%"FMT_OPPRVLG, flags);
	bool operatorHasPrivilege = true;
	assertUpdateUserRoleWithSetup(params, targetUserRoleId,
				      HTERR_NO_PRIVILEGE,
				      !operatorHasPrivilege);

	assertUserRolesInDB();
}

void _assertDeleteUserRoleWithSetup(
  string url = "",
  HatoholErrorCode expectedErrorCode = HTERR_OK,
  bool operatorHasPrivilege = true,
  const UserRoleIdType targetUserRoleId = 2)
{
	startFaceRest();
	bool dbRecreate = true;
	bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);
	loadTestDBUserRole();

	if (url.empty())
		url = StringUtils::sprintf("/user-role/%"FMT_USER_ROLE_ID,
					   targetUserRoleId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	if (operatorHasPrivilege)
		arg.userId = findUserWith(OPPRVLG_DELETE_ALL_USER_ROLE);
	else
		arg.userId = findUserWithout(OPPRVLG_DELETE_ALL_USER_ROLE);
	g_parser = getResponseAsJsonParser(arg);

	// check the reply
	assertErrorCode(g_parser, expectedErrorCode);
	UserRoleIdSet userRoleIdSet;
	if (expectedErrorCode == HTERR_OK)
		userRoleIdSet.insert(targetUserRoleId);
	// check the version
	assertUserRolesInDB(userRoleIdSet);
}
#define assertDeleteUserRoleWithSetup(...) \
cut_trace(_assertDeleteUserRoleWithSetup(__VA_ARGS__))

void test_deleteUserRole(void)
{
	assertDeleteUserRoleWithSetup();
}

void test_deleteUserRoleWithoutId(void)
{
	assertDeleteUserRoleWithSetup("/user-role",
				      HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserRoleWithNonNumericId(void)
{
	assertDeleteUserRoleWithSetup("/user-role/maintainer",
				      HTERR_NOT_FOUND_ID_IN_URL);
}

void test_deleteUserRoleWithoutPrivilege(void)
{
	bool operatorHasPrivilege = true;
	assertDeleteUserRoleWithSetup(string(), // set automatically
				      HTERR_NO_PRIVILEGE,
				      !operatorHasPrivilege);
}

void test_overview(void)
{
	setupUserDB();
	startFaceRest();
	RequestArg arg("/overview");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertOverviewInParser(g_parser);
}

} // namespace testFaceRest

// ---------------------------------------------------------------------------
// testFaceRestNoInit
//
// This namespace contains test cases that don't need to call hatoholInit(),
// which takes a little time.
// ---------------------------------------------------------------------------
namespace testFaceRestNoInit {

class TestFaceRestNoInit : public FaceRest {
public:
	static HatoholError callParseEventParameter(
		EventsQueryOption &option,
		GHashTable *query)
	{
		return parseEventParameter(option, query);
	}
};

template<typename PARAM_TYPE>
void _assertParseEventParameterTempl(
  const PARAM_TYPE &expectValue, const string &fmt, const string &paramName,
  PARAM_TYPE (HostResourceQueryOption::*valueGetter)(void) const,
  const HatoholErrorCode &expectCode = HTERR_OK,
  const string &forceValueStr = "")
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);

	string expectStr;
	if (forceValueStr.empty())
		expectStr =StringUtils::sprintf(fmt.c_str(), expectValue);
	else
		expectStr = forceValueStr;
	g_hash_table_insert(query,
	                    (gpointer) paramName.c_str(),
	                    (gpointer) expectStr.c_str());
	assertHatoholError(
	  expectCode,
	  TestFaceRestNoInit::callParseEventParameter(option, query));
	if (expectCode != HTERR_OK)
		return;
	cppcut_assert_equal(expectValue, (option.*valueGetter)());
}
#define assertParseEventParameterTempl(T, E, FM, PN, GT, ...) \
cut_trace(_assertParseEventParameterTempl<T>(E, FM, PN, GT, ##__VA_ARGS__))

void _assertParseEventParameterSortOrderDontCare(
  const DataQueryOption::SortOrder &sortOrder,
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  DataQueryOption::SortOrder, sortOrder, "%d", "sortOrder",
	  &HostResourceQueryOption::getSortOrder, expectCode);
}
#define assertParseEventParameterSortOrderDontCare(O, ...) \
cut_trace(_assertParseEventParameterSortOrderDontCare(O, ##__VA_ARGS__))

void _assertParseEventParameterMaximumNumber(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  size_t, expectValue, "%zd", "maximumNumber",
	  &HostResourceQueryOption::getMaximumNumber, expectCode, forceValueStr);
}
#define assertParseEventParameterMaximumNumber(E, ...) \
cut_trace(_assertParseEventParameterMaximumNumber(E, ##__VA_ARGS__))

void _assertParseEventParameterStartId(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  uint64_t, expectValue, "%"PRIu64, "startId",
	  &HostResourceQueryOption::getStartId, expectCode, forceValueStr);
}
#define assertParseEventParameterStartId(E, ...) \
cut_trace(_assertParseEventParameterStartId(E, ##__VA_ARGS__))

GHashTable *g_query = NULL;

void cut_teardown(void)
{
	if (g_query) {
		g_hash_table_unref(g_query);
		g_query = NULL;
	}
}

// ---------------------------------------------------------------------------
// test cases
// ---------------------------------------------------------------------------
void test_parseEventParameterWithNullQueryParameter(void)
{
	EventsQueryOption orig;
	EventsQueryOption option(orig);
	GHashTable *query = NULL;
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	// we confirm that the content is not changed.
	cppcut_assert_equal(true, option == orig);
}

void test_parseEventParameterSortOrderNotFound(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(DataQueryOption::SORT_DONT_CARE,
	                    option.getSortOrder());
}

void test_parseEventParameterSortOrderDontCare(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_DONT_CARE);
}

void test_parseEventParameterSortOrderAscending(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_ASCENDING);
}

void test_parseEventParameterSortOrderDescending(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_DESCENDING);
}

void test_parseEventParameterSortInvalidValue(void)
{
	assertParseEventParameterSortOrderDontCare(
	  (DataQueryOption::SortOrder)-1, HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterMaximumNumberNotFound(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal((size_t)0, option.getMaximumNumber());
}

void test_parseEventParameterMaximumNumber(void)
{
	assertParseEventParameterMaximumNumber(100);
}

void test_parseEventParameterMaximumNumberInvalidInput(void)
{
	assertParseEventParameterMaximumNumber(0, "lion",
	                                       HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterStartIdNotFound(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal((uint64_t)0, option.getStartId());
}

void test_parseEventParameterStartId(void)
{
	assertParseEventParameterStartId(345678);
}

void test_parseEventParameterStartIdInvalidInput(void)
{
	assertParseEventParameterStartId(0, "orca", HTERR_INVALID_PARAMETER);
}

} // namespace testFaceRestNoInit
