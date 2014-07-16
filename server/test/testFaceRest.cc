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
#include "SessionManager.h"
#include "testDBClientHatohol.h"
#include "HatoholArmPluginInterface.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRest {

static JsonParserAgent *g_parser = NULL;

static void _assertTestTriggerInfo(const TriggerInfo &triggerInfo)
{
	assertValueInParser(g_parser, "status", triggerInfo.status);
	assertValueInParser(g_parser, "severity", triggerInfo.severity);
	assertValueInParser(g_parser, "lastChangeTime",
	                    triggerInfo.lastChangeTime);
	assertValueInParser(g_parser, "serverId", triggerInfo.serverId);
	assertValueInParser(g_parser, "hostId", triggerInfo.hostId);
	assertValueInParser(g_parser, "brief", triggerInfo.brief);
}
#define assertTestTriggerInfo(T) cut_trace(_assertTestTriggerInfo(T))

static void assertHostsInParser(JsonParserAgent *parser,
                                const ServerIdType &targetServerId,
                                DataQueryContext *dqCtx)
{
	HostInfoList hostInfoList;
	getDBCTestHostInfo(hostInfoList, targetServerId);
	// Remove data of defunct servers
	HostInfoListIterator hostInfoItr = hostInfoList.begin();
	while (hostInfoItr != hostInfoList.end()) {
		HostInfoListIterator currHostInfoItr = hostInfoItr;
		++hostInfoItr;
		if (dqCtx->isValidServer(currHostInfoItr->serverId))
			continue;
		hostInfoList.erase(currHostInfoItr);
	}
	assertValueInParser(parser, "numberOfHosts", hostInfoList.size());

	// make an index
	map<ServerIdType, map<uint64_t, const HostInfo *> > hostInfoMap;
	map<uint64_t, const HostInfo *>::iterator hostMapIt;
	HostInfoListConstIterator it = hostInfoList.begin();
	for (; it != hostInfoList.end(); ++it) {
		const HostInfo &hostInfo = *it;
		hostMapIt = hostInfoMap[hostInfo.serverId].find(hostInfo.id);
		cppcut_assert_equal(
		  true, hostMapIt == hostInfoMap[hostInfo.serverId].end());
		hostInfoMap[hostInfo.serverId][hostInfo.id] = &hostInfo;
	}

	assertStartObject(parser, "hosts");
	for (size_t i = 0; i < hostInfoList.size(); i++) {
		int64_t var64;
		parser->startElement(i);
		cppcut_assert_equal(true, parser->read("serverId", var64));
		const ServerIdType serverId = static_cast<ServerIdType>(var64);
		cppcut_assert_equal(true, parser->read("id", var64));
		uint64_t hostId = (uint64_t)var64;

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
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < numberOfTriggers; i++) {
		TriggerInfo &triggerInfo = triggers[i];
		assertStartObject(parser, StringUtils::toString(triggerInfo.serverId));
		assertStartObject(parser, "hosts");
		assertStartObject(parser, StringUtils::toString(triggerInfo.hostId));
		assertValueInParser(parser, "name", triggerInfo.hostName);
		parser->endObject();
		parser->endObject();
		parser->endObject();
	}
	parser->endObject();
}

static void assertHostsIdNameHashInParser(
  const vector<EventInfo *> &expectedRecords, JsonParserAgent *parser)
{
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < expectedRecords.size(); i++) {
		const EventInfo &eventInfo = *expectedRecords[i];
		assertStartObject(parser, StringUtils::toString(eventInfo.serverId));
		assertStartObject(parser, "hosts");
		assertStartObject(parser, StringUtils::toString(eventInfo.hostId));
		assertValueInParser(parser, "name", eventInfo.hostName);
		parser->endObject();
		parser->endObject();
		parser->endObject();
	}
	parser->endObject();
}

static void assertServersIdNameHashInParser(JsonParserAgent *parser)
{
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		MonitoringServerInfo &svInfo = testServerInfo[i];
		assertStartObject(parser, StringUtils::toString(svInfo.id));
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

static void _assertHosts(const string &path, const string &callbackName = "",
                         const ServerIdType &serverId = ALL_SERVERS)
{
	setupUserDB();
	startFaceRest();
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		   StringUtils::sprintf("%" PRIu32, serverId); 
	}
	RequestArg arg(path, callbackName);
	arg.parameters = queryMap;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);
	assertHostsInParser(g_parser, serverId, dqCtxPtr);
}
#define assertHosts(P,...) cut_trace(_assertHosts(P,##__VA_ARGS__))

static void _assertTriggers(const string &path, const string &callbackName = "",
                            const ServerIdType &serverId = ALL_SERVERS,
                            uint64_t hostId = ALL_HOSTS)
{
	setupUserDB();
	startFaceRest();

	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);

	// calculate the expected test triggers
	map<ServerIdType, map<uint64_t, size_t> > indexMap;
	map<ServerIdType, map<uint64_t, size_t> >::iterator indexMapIt;
	map<uint64_t, size_t>::iterator trigIdIdxIt;
	getTestTriggersIndexes(indexMap, serverId, hostId);
	size_t expectedNumTrig = 0;
	indexMapIt = indexMap.begin();
	while (indexMapIt != indexMap.end()) {
		// Remove element whose key points a defunct server.
		const ServerIdType serverId = indexMapIt->first;
		if (!dqCtxPtr->isValidServer(serverId)) {
			indexMap.erase(indexMapIt++); // must be post-increment
			continue;
		}
		expectedNumTrig += indexMapIt->second.size();
		++indexMapIt;
	}

	// request
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		  StringUtils::sprintf("%" PRIu32, serverId);
	}
	if (hostId != ALL_HOSTS)
		queryMap["hostId"] = StringUtils::sprintf("%" PRIu64, hostId); 
	arg.parameters = queryMap;
	g_parser = getResponseAsJsonParser(arg);

	// Check the reply
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfTriggers", expectedNumTrig);
	assertValueInParser(g_parser, "totalNumberOfTriggers", expectedNumTrig);
	assertStartObject(g_parser, "triggers");
	for (size_t i = 0; i < expectedNumTrig; i++) {
		g_parser->startElement(i);
		int64_t var64;
		cppcut_assert_equal(true, g_parser->read("serverId", var64));
		const ServerIdType actSvId =
		  static_cast<ServerIdType>(var64);
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

	// build expected data
	AssertGetEventsArg eventsArg(NULL);
	eventsArg.filterForDataOfDefunctSv = true;
	eventsArg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	eventsArg.sortType = EventsQueryOption::SORT_TIME;
	eventsArg.sortDirection = EventsQueryOption::SORT_DESCENDING;
	eventsArg.fixup();

	// check json data
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfEvents",
	                    eventsArg.expectedRecords.size());
	assertValueInParser(g_parser, "lastUnifiedEventId",
	                    eventsArg.expectedRecords.size());
	DBClientAction dbAction;
	bool shouldHaveIssue = dbAction.isIssueSenderEnabled();
	assertValueInParser(g_parser, "haveIssue", shouldHaveIssue);

	assertStartObject(g_parser, "events");
	vector<EventInfo*>::reverse_iterator it
	  = eventsArg.expectedRecords.rbegin();
	for (size_t i = 0; it != eventsArg.expectedRecords.rend(); i++, ++it) {
		EventInfo &eventInfo = *(*it);
		uint64_t unifiedId = eventsArg.idMap[*it];
		g_parser->startElement(i);
		assertValueInParser(g_parser, "unifiedId", unifiedId);
		assertValueInParser(g_parser, "serverId", eventInfo.serverId);
		assertValueInParser(g_parser, "time", eventInfo.time);
		assertValueInParser(g_parser, "type", eventInfo.type);
		assertValueInParser(g_parser, "triggerId", eventInfo.triggerId);
		assertValueInParser(g_parser, "status",    eventInfo.status);
		assertValueInParser(g_parser, "severity",  eventInfo.severity);
		assertValueInParser(g_parser, "hostId",    eventInfo.hostId);
		assertValueInParser(g_parser, "brief",     eventInfo.brief);
		if (shouldHaveIssue) {
			assertStartObject(g_parser, "issue");
			IssueInfo issue
			  = eventsArg.getExpectedIssueInfo(eventInfo);
			assertValueInParser(g_parser, "location",
					    issue.location);
			g_parser->endElement();
		} else {
			assertNoValueInParser(g_parser, "issue");
		}
		g_parser->endElement();
	}
	g_parser->endObject();
	assertHostsIdNameHashInParser(eventsArg.expectedRecords, g_parser);
	assertServersIdNameHashInParser(g_parser);
}
#define assertEvents(P,...) cut_trace(_assertEvents(P,##__VA_ARGS__))

static void _assertItems(const string &path, const string &callbackName = "")
{
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);

	// NOTE: Items that belong to the servers that registered in the config
	// DB are returned. I.e. all elements of testItemInfo shall not be
	// in 'g_parser'.

	// Check the size of ItemInfo
	size_t numExpectedItems = 0;
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		if (dqCtxPtr->isValidServer(testItemInfo[i].serverId))
			numExpectedItems++;
	}
	// If 'numExpectedItems' is 0, the test matarial is bad.
	cppcut_assert_not_equal((size_t)0, numExpectedItems);

	int64_t _numItems = 0;
	cppcut_assert_equal(true, g_parser->read("numberOfItems", _numItems));
	size_t numItems = static_cast<size_t>(_numItems);
	cppcut_assert_equal(numExpectedItems, numItems);

	cppcut_assert_equal(true, g_parser->read("totalNumberOfItems", _numItems));
	numItems = static_cast<size_t>(_numItems);
	cppcut_assert_equal(numExpectedItems, numItems);

	// Check each ItemInfo
	ServerIdItemInfoIdIndexMapMap indexMap;
	getTestItemsIndexes(indexMap);
	assertStartObject(g_parser, "items");
	set<ItemInfo *> itemInfoPtrSet;
	for (size_t i = 0; i < numItems; i++) {
		int64_t serverId = 0;
		int64_t itemInfoId = 0;

		g_parser->startElement(i);
		cppcut_assert_equal(true, g_parser->read("serverId", serverId));
		cppcut_assert_equal(true, g_parser->read("id", itemInfoId));
		ItemInfo *itemInfoPtr =
		  findTestItem(indexMap, serverId, itemInfoId);
		cppcut_assert_not_null(itemInfoPtr);
		const ItemInfo &itemInfo = *itemInfoPtr;

		assertValueInParser(g_parser, "serverId", itemInfo.serverId);
		assertValueInParser(g_parser, "hostId", itemInfo.hostId);
		assertValueInParser(g_parser, "brief", itemInfo.brief);
		assertValueInParser(g_parser, "lastValueTime",
		                    itemInfo.lastValueTime);
		assertValueInParser(g_parser, "lastValue", itemInfo.lastValue);
		assertValueInParser(g_parser, "prevValue", itemInfo.prevValue);
		g_parser->endElement();

		// Check duplication
		pair<set<ItemInfo *>::iterator, bool> result = 
		  itemInfoPtrSet.insert(itemInfoPtr);
		cppcut_assert_equal(true, result.second);
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

static void _assertActions(const string &path, const string &callbackName = "",
			   const ActionType actionType = ACTION_USER_DEFINED)
{
	setupUserDB();
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.parameters["type"]
	  = StringUtils::sprintf("%d", actionType);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_USER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfActions",
			    getNumberOfTestActions(actionType));
	assertStartObject(g_parser, "actions");
	size_t idx = 0;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actionDef = testActionDef[i];
		if (filterOutAction(actionDef, actionType))
			continue;
		g_parser->startElement(idx++);
		const ActionCondition &cond = actionDef.condition;
		assertValueInParser(g_parser, "actionId", actionDef.id);

		assertValueInParser(g_parser, "enableBits", cond.enableBits);
		asssertActionCondition(
		  g_parser, cond, "serverId", ACTCOND_SERVER_ID,
		  ServerIdType, cond.serverId);
		asssertActionCondition(
		  g_parser, cond, "hostId", ACTCOND_HOST_ID,
		  uint64_t, cond.hostId);
		asssertActionCondition(
		  g_parser, cond, "hostgroupId", ACTCOND_HOST_GROUP_ID,
		  uint64_t, cond.hostgroupId);
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

		assertValueInParser(g_parser, "type", actionDef.type);
		assertValueInParser(g_parser, "workingDirectory",
		                    actionDef.workingDir);
		assertValueInParser(g_parser, "command", actionDef.command);
		assertValueInParser(g_parser, "timeout", actionDef.timeout);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertServersIdNameHashInParser(g_parser);
}
#define assertActions(P,...) cut_trace(_assertActions(P,##__VA_ARGS__))

#define assertAddAction(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/action", ##__VA_ARGS__))

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

static void _assertUser(JsonParserAgent *parser, const UserInfo &userInfo,
                        uint32_t expectUserId = 0)
{
	if (expectUserId)
		assertValueInParser(parser, "userId", expectUserId);
	assertValueInParser(parser, "name", userInfo.name);
	assertValueInParser(parser, "flags", userInfo.flags);
}
#define assertUser(P,I,...) cut_trace(_assertUser(P,I,##__VA_ARGS__))

static void assertUserRolesMapInParser(JsonParserAgent *parser)
{
	assertStartObject(parser, "userRoles");

	string flagsStr =
	  StringUtils::sprintf("%" FMT_OPPRVLG, NONE_PRIVILEGE);
	assertStartObject(parser, flagsStr);
	assertValueInParser(parser, "name", string("Guest"));
	parser->endObject();

	flagsStr = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	assertStartObject(parser, flagsStr);
	assertValueInParser(parser, "name", string("Admin"));
	parser->endObject();

	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		flagsStr = StringUtils::toString(userRoleInfo.flags);
		assertStartObject(parser, flagsStr);
		assertValueInParser(parser, "name", userRoleInfo.name);
		parser->endObject();
	}
	parser->endObject();
}

static void _assertUsers(const string &path, const UserIdType &userId,
                         const string &callbackName = "")
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.userId = userId;
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfUsers", NumTestUserInfo);
	assertStartObject(g_parser, "users");
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		g_parser->startElement(i);
		const UserInfo &userInfo = testUserInfo[i];
		assertUser(g_parser, userInfo, i + 1);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertUserRolesMapInParser(g_parser);
}
#define assertUsers(P, U, ...) cut_trace(_assertUsers(P, U, ##__VA_ARGS__))

#define assertAddUser(P, ...) \
cut_trace(_assertAddRecord(g_parser, P, "/user", ##__VA_ARGS__))

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
cut_trace(_assertUpdateRecord(g_parser, P, "/user", ##__VA_ARGS__))

void _assertUpdateUserWithSetup(const StringMap &params,
                                uint32_t targetUserId,
                                const HatoholErrorCode &expectCode)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
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
	parameters["name"] = name;
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
	  parameters["name"].c_str(),
	  Utils::sha256( parameters["password"]).c_str(),
	  parameters["flags"].c_str());
	CacheServiceDBClient cache;
	assertDBContent(cache.getUser()->getDBAgent(), statement, expect);
}
#define assertUpdateOrAddUser(U) cut_trace(_assertUpdateOrAddUser(U))

static void _assertServerAccessInfo(JsonParserAgent *parser, HostGrpAccessInfoMap &expected)
{
	assertStartObject(parser, "allowedHostgroups");
	HostGrpAccessInfoMapIterator it = expected.begin();
	for (size_t i = 0; i < expected.size(); i++) {
		uint64_t hostgroupId = it->first;
		string idStr;
		if (hostgroupId == ALL_HOST_GROUPS)
			idStr = "-1";
		else
			idStr = StringUtils::sprintf("%" PRIu64, hostgroupId);
		assertStartObject(parser, idStr);
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
	assertStartObject(g_parser, "allowedServers");
	ServerAccessInfoMapIterator it = srvAccessInfoMap.begin();
	for (; it != srvAccessInfoMap.end(); ++it) {
		const ServerIdType &serverId = it->first;
		string idStr;
		if (serverId == ALL_SERVERS)
			idStr = "-1";
		else
			idStr = StringUtils::toString(serverId);
		assertStartObject(g_parser, idStr);
		HostGrpAccessInfoMap *hostGrpAccessInfoMap = it->second;
		assertServerAccessInfo(g_parser, *hostGrpAccessInfoMap);
		g_parser->endObject();
	}
	g_parser->endObject();
	DBClientUser::destroyServerAccessInfoMap(srvAccessInfoMap);
}
#define assertAllowedServers(P,...) cut_trace(_assertAllowedServers(P,##__VA_ARGS__))

#define assertAddAccessInfo(U, P, USER_ID, ...) \
cut_trace(_assertAddRecord(g_parser, P, U, USER_ID, ##__VA_ARGS__))

void _assertAddAccessInfoWithCond(
  const string &serverId, const string &hostgroupId,
  string expectHostgroupId = "")
{
	setupUserDB();

	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	const UserIdType targetUserId = 1;

	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info", targetUserId);
	StringMap params;
	params["serverId"] = serverId;
	params["hostgroupId"] = hostgroupId;
	assertAddAccessInfo(url, params, userId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_ACCESS_LIST;
	int expectedId = 1;
	if (expectHostgroupId.empty())
		expectHostgroupId = hostgroupId;
	string expect = StringUtils::sprintf(
	  "%" FMT_ACCESS_INFO_ID "|%" FMT_USER_ID "|%s|%s\n",
	  expectedId, targetUserId, serverId.c_str(),
	  expectHostgroupId.c_str());
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
	assertValueInParser(g_parser, "numberOfUserRoles", NumTestUserRoleInfo);
	assertStartObject(g_parser, "userRoles");
	for (size_t i = 0; i < NumTestUserRoleInfo; i++) {
		g_parser->startElement(i);
		const UserRoleInfo &userRoleInfo = testUserRoleInfo[i];
		assertUserRole(g_parser, userRoleInfo, i + 1);
		g_parser->endElement();
	}
	g_parser->endObject();
}
#define assertUserRoles(P, U, ...) \
  cut_trace(_assertUserRoles(P, U, ##__VA_ARGS__))

static void assertHostgroupsInParser(JsonParserAgent *parser,
                                     const ServerIdType &serverId,
                                     HostgroupIdSet &hostgroupIdSet)
{
	assertStartObject(parser, "hostgroups");
	for (size_t i = 0; i < NumTestHostgroupInfo; i++) {
		const HostgroupInfo &hgrpInfo = testHostgroupInfo[i];
		// TODO: fix this inefficient algorithm
		if (hgrpInfo.serverId != serverId)
			continue;
		const HostgroupIdType hostgroupId = hgrpInfo.groupId;
		const string expectKey =
		  StringUtils::sprintf("%" FMT_HOST_GROUP_ID, hostgroupId);
		assertStartObject(parser, expectKey);
		assertValueInParser(parser, string("name"), hgrpInfo.groupName);
		parser->endObject();
		hostgroupIdSet.insert(hostgroupId);
	}
	parser->endObject();
}

static void assertHostStatusInParser(JsonParserAgent *parser,
                                     const ServerIdType &serverId,
                                     const HostgroupIdSet &hostgroupIdSet)
{
	assertStartObject(parser, "hostStatus");
	cppcut_assert_equal(hostgroupIdSet.size(),
	                    (size_t)parser->countElements());
	HostgroupIdSetConstIterator hostgrpIdItr = hostgroupIdSet.begin();
	size_t idx = 0;
	for (; hostgrpIdItr != hostgroupIdSet.end(); ++hostgrpIdItr, ++idx) {
		parser->startElement(idx);
		assertValueInParser(parser, "hostgroupId",  *hostgrpIdItr);
		const size_t expectedGoodHosts = getNumberOfTestHostsWithStatus(
		  serverId, *hostgrpIdItr, true);
		const size_t expectedBadHosts = getNumberOfTestHostsWithStatus(
		  serverId, *hostgrpIdItr, false);
		assertValueInParser(parser, "numberOfGoodHosts",
		                    expectedGoodHosts);
		assertValueInParser(parser, "numberOfBadHosts",
		                    expectedBadHosts);
		parser->endElement();
	}
	parser->endObject();
}

static void assertSystemStatusInParserEach(
  JsonParserAgent *parser, const int &severityNum,
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId)
{
	const TriggerSeverityType severity = 
	  static_cast<TriggerSeverityType>(severityNum);
	const size_t expectedTriggers =
	  getNumberOfTestTriggers(serverId, hostgroupId, severity);
	assertValueInParser(parser, "hostgroupId", hostgroupId);
	assertValueInParser(parser, "severity", severity);
	assertValueInParser(parser, "numberOfTriggers", expectedTriggers);
}

static void assertSystemStatusInParser(JsonParserAgent *parser,
                                       const ServerIdType &serverId,
                                       const HostgroupIdSet &hostgroupIdSet)
{
	assertStartObject(parser, "systemStatus");
	HostgroupIdSetConstIterator hostgrpIdItr = hostgroupIdSet.begin();
	cppcut_assert_equal(hostgroupIdSet.size() * NUM_TRIGGER_SEVERITY,
	                    (size_t)parser->countElements());
	size_t arrayIdx = 0;
	for (; hostgrpIdItr != hostgroupIdSet.end(); ++hostgrpIdItr) {
		for (int i = 0; i < NUM_TRIGGER_SEVERITY; ++i, ++arrayIdx) {
			parser->startElement(arrayIdx);
			assertSystemStatusInParserEach(parser, i, serverId,
			                               *hostgrpIdItr);
			parser->endElement();
		}
	}
	parser->endObject();
}

static void _assertOverviewInParser(JsonParserAgent *parser, RequestArg &arg)
{
	assertValueInParser(parser, "numberOfServers", NumTestServerInfo);
	assertStartObject(parser, "serverStatus");
	size_t numGoodServers = 0, numBadServers = 0;
	// We assume that the caller can access all serers and hosts.
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		HostgroupIdSet hostgroupIdSet;
		parser->startElement(i);
		MonitoringServerInfo &svInfo = testServerInfo[i];
		assertValueInParser(parser, "serverId", svInfo.id);
		assertValueInParser(parser, "serverHostName", svInfo.hostName);
		assertValueInParser(parser, "serverIpAddr", svInfo.ipAddress);
		assertValueInParser(parser, "serverNickname", svInfo.nickname);
		assertValueInParser(parser, "numberOfHosts",
				    getNumberOfTestHosts(svInfo.id));
		size_t numBadHosts = getNumberOfTestHostsWithStatus(
				       svInfo.id, ALL_HOST_GROUPS,
				       false, arg.userId);
		assertValueInParser(parser, "numberOfBadHosts", numBadHosts);
		assertValueInParser(parser, "numberOfItems",
				    getNumberOfTestItems(svInfo.id));
		assertValueInParser(parser, "numberOfTriggers",
				    getNumberOfTestTriggers(svInfo.id));
		assertValueInParser(parser, "numberOfUsers", 0);
		assertValueInParser(parser, "numberOfOnlineUsers", 0);
		MonitoringServerStatus &serverStatus = testServerStatus[i];
		string nvps = StringUtils::sprintf("%.2f", serverStatus.nvps);
		assertValueInParser(parser, "numberOfMonitoredItemsPerSecond", nvps);
		assertHostgroupsInParser(parser, svInfo.id, hostgroupIdSet);
		assertSystemStatusInParser(parser, svInfo.id, hostgroupIdSet);
		assertHostStatusInParser(parser, svInfo.id, hostgroupIdSet);
		parser->endElement();
		size_t badHosts = getNumberOfTestHostsWithStatus(
			svInfo.id, ALL_HOST_GROUPS, false);
		if (badHosts > 0)
			numBadServers++;
		else
			numGoodServers++;
	}
	parser->endObject();
	assertValueInParser(parser, "numberOfGoodServers", numGoodServers);
	assertValueInParser(parser, "numberOfBadServers", numBadServers);

	// TODO: check badServers
}
#define assertOverviewInParser(P,A) cut_trace(_assertOverviewInParser(P,A))

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

	// Make sure to clear actions on intial state because the issue object
	// in an event object depends on them.
	bool recreate = true;
	bool loadData = false;
	setupTestDBAction(recreate, loadData);
}

void cut_teardown(void)
{
	stopFaceRest();

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
	assertStartObject(g_parser, "queryData");
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
	assertHosts("/host", "foo", 1);
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

void test_eventsWithIssues(void)
{
	 // issue info will be added when a IssueSender action exists
	setupActionDB();
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

void data_actionsJsonp(void)
{
	gcut_add_datum("Normal actions",
		       "type", G_TYPE_INT, ACTION_USER_DEFINED,
		       NULL);
	gcut_add_datum("IssueSenderAction",
		       "type", G_TYPE_INT, ACTION_ISSUE_SENDER,
		       NULL);
	gcut_add_datum("All",
		       "type", G_TYPE_INT, ACTION_ALL,
		       NULL);
}

void test_actionsJsonp(gconstpointer data)
{
	const ActionType actionType
	  = static_cast<ActionType>(gcut_data_get_int(data, "type"));
	bool recreate = true;
	bool loadData = true;
	setupTestDBAction(recreate, loadData);
	assertActions("/action", "foo", actionType);
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
	expect += StringUtils::sprintf("|%" FMT_USER_ID, userId);
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
	uint64_t hostgroupId = 1000;
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
	params["hostId"]      = StringUtils::sprintf("%" PRIu64, hostId);
	params["hostgroupId"] = StringUtils::sprintf("%" PRIu64, hostgroupId);
	params["triggerId"]   = StringUtils::sprintf("%" PRIu64, triggerId);
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
	expect += StringUtils::sprintf("%" PRIu64 "|%" PRIu64 "|%" PRIu64 "|",
	  hostId, hostgroupId, triggerId);
	expect += StringUtils::sprintf(
	  "%d|%d|%d|", triggerStatus, triggerSeverity, triggerSeverityCompType);
	expect += StringUtils::sprintf("%d|", type);
	expect += command;
	expect += "|";
	expect += workingDir;
	expect += "|";
	expect += StringUtils::sprintf("%d|%" FMT_USER_ID, timeout, userId);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionParameterOver32bit(void)
{
	setupUserDB();
	setupPostAction();

	const string command = "/usr/bin/pochi";
	uint64_t hostId = 0x89abcdef01234567;
	uint64_t hostgroupId = 0xabcdef0123456789;
	uint64_t triggerId = 0x56789abcdef01234;

	StringMap params;
	params["type"]        = StringUtils::sprintf("%d", ACTION_RESIDENT);
	params["command"]     = command;
	params["hostId"]      = StringUtils::sprintf("%" PRIu64, hostId);
	params["hostgroupId"] = StringUtils::sprintf("%" PRIu64, hostgroupId);
	params["triggerId"]   = StringUtils::sprintf("%" PRIu64, triggerId);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

	// check the content in the DB
	DBClientAction dbAction;
	string statement = "select host_id, host_group_id, trigger_id from ";
	statement += DBClientAction::getTableNameActions();
	string expect;
	expect += StringUtils::sprintf("%" PRIu64 "|%" PRIu64 "|%" PRIu64,
	  hostId, hostgroupId, triggerId);
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
	statement += " order by action_id asc";
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
	assertValueInParser(g_parser, "numberOfUsers", 1);
	assertStartObject(g_parser, "users");
	g_parser->startElement(0);
	assertUser(g_parser, user);
}

void test_addUser(void)
{
	OperationPrivilegeFlag flags = OPPRVLG_GET_ALL_USER;
	const string user = "y@ru0";
	const string password = "w(^_^)d";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertAddUserWithSetup(params, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = "select * from ";
	statement += DBClientUser::TABLE_NAME_USERS;
	statement += " order by id desc limit 1";
	const int expectedId = NumTestUserInfo + 1;
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
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
	params["name"] = user;
	params["password"] = password;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  targetId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithoutFlags(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = testUserInfo[targetId - 1].flags;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
	  targetId, user.c_str(), Utils::sha256(password).c_str(), flags);
	assertDBContent(dbUser.getDBAgent(), statement, expect);
}

void test_updateUserWithInvalidFlags(void)
{
	const UserIdType targetId = 1;
	const string user = "y@r@n@i0";
	const string password = "=(-.-)zzZZ";

	StringMap params;
	params["name"] = user;
	params["password"] = password;
	params["flags"] = "no-number";
	assertUpdateUserWithSetup(params, targetId, HTERR_INVALID_PARAMETER);
}

void test_updateUserWithoutPassword(void)
{
	const UserIdType targetId = 1;
	OperationPrivilegeFlag flags = ALL_PRIVILEGES;
	const string user = "y@r@n@i0";
	const string expectedPassword = testUserInfo[targetId - 1].password;

	StringMap params;
	params["name"] = user;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
	assertUpdateUserWithSetup(params, targetId, HTERR_OK);

	// check the content in the DB
	DBClientUser dbUser;
	string statement = StringUtils::sprintf(
	                     "select * from %s where id=%d",
	                     DBClientUser::TABLE_NAME_USERS, targetId);
	string expect = StringUtils::sprintf("%d|%s|%s|%" FMT_OPPRVLG,
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
	params["name"] = user;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
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
	params["name"] = "y@ru0";
	params["flags"] = "0";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserWithoutFlags(void)
{
	StringMap params;
	params["name"] = "y@ru0";
	params["password"] = "w(^_^)d";
	assertAddUserWithSetup(params, HTERR_NOT_FOUND_PARAMETER);
}

void test_addUserInvalidUserName(void)
{
	StringMap params;
	params["name"] = "!^.^!"; // '!' and '^' are invalid characters
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
	string url = StringUtils::sprintf("/user/%" FMT_USER_ID, userId);
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
	parameters["name"] = "ABC";
	parameters["flags"] = "2";
	assertUpdateAddUserMissing(parameters);
}

void test_updateOrAddUserMissingFlags(void)
{
	StringMap parameters;
	parameters["name"] = "ABC";
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
	const string hostgroupId = "3";
	assertAddAccessInfoWithCond(serverId, hostgroupId);
}

void test_updateAccessInfo(void)
{
	setupUserDB();

	const UserIdType userId = findUserWith(OPPRVLG_UPDATE_USER);
	const UserIdType targetUserId = 1;
	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info/2", targetUserId);
	StringMap params;
	params["serverId"] = "2";
	params["hostgroupId"] = "-1";

	startFaceRest();

	RequestArg arg(url);
	arg.parameters = params;
	arg.request = "PUT";
	arg.userId = userId;
	getServerResponse(arg);
	cppcut_assert_equal(
	  static_cast<int>(SOUP_STATUS_METHOD_NOT_ALLOWED),
	  arg.httpStatusCode);
}

void test_addAccessInfoWithAllHostgroups(void)
{
	const string serverId = "2";
	const string hostgroupId =
	  StringUtils::sprintf("%" PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostgroupId);
}

void test_addAccessInfoWithAllHostgroupsNegativeValue(void)
{
	const string serverId = "2";
	const string hostgroupId = "-1";
	const string expectHostgroup =
	  StringUtils::sprintf("%" PRIu64, ALL_HOST_GROUPS);
	assertAddAccessInfoWithCond(serverId, hostgroupId, expectHostgroup);
}

void test_addAccessInfoWithExistingData(void)
{
	const bool dbRecreate = true;
	const bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);

	const UserIdType userId = findUserWith(OPPRVLG_CREATE_USER);
	const UserIdType targetUserId = 1;
	const string serverId = "1";
	const string hostgroupId = "1";

	StringMap params;
	params["userId"] = StringUtils::sprintf("%" FMT_USER_ID, userId);
	params["serverId"] = serverId;
	params["hostgroupId"] = hostgroupId;

	const string url = StringUtils::sprintf(
	  "/user/%" FMT_USER_ID "/access-info", targetUserId);
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
	  "/user/1/access-info/%" FMT_ACCESS_INFO_ID,
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
cut_trace(_assertAddRecord(g_parser, P, "/user-role", ##__VA_ARGS__))

void _assertAddUserRoleWithSetup(const StringMap &params,
				 const HatoholErrorCode &expectCode,
				 bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
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
	expectedUserRoleInfo.flags =
	  OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER);

	StringMap params;
	params["name"] = expectedUserRoleInfo.name;
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
	assertAddUserRoleWithSetup(params, HTERR_OK);

	// check the content in the DB
	assertUserRoleInfoInDB(expectedUserRoleInfo);
}

void test_addUserRoleWithoutPrivilege(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	bool operatorHasPrivilege = true;
	assertAddUserRoleWithSetup(params, HTERR_NO_PRIVILEGE,
				   !operatorHasPrivilege);

	assertUserRolesInDB();
}

void test_addUserRoleWithoutName(void)
{
	StringMap params;
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
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
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, ALL_PRIVILEGES);
	assertAddUserRoleWithSetup(params, HTERR_EMPTY_USER_ROLE_NAME);

	assertUserRolesInDB();
}

void test_addUserRoleWithInvalidFlags(void)
{
	StringMap params;
	params["name"] = "maintainer";
	params["flags"] = StringUtils::sprintf(
	  "%" FMT_OPPRVLG, ALL_PRIVILEGES + 1);
	assertAddUserRoleWithSetup(params, HTERR_INVALID_PRIVILEGE_FLAGS);

	assertUserRolesInDB();
}

#define assertUpdateUserRole(P, ...) \
cut_trace(_assertUpdateRecord(g_parser, P, "/user-role", ##__VA_ARGS__))

void _assertUpdateUserRoleWithSetup(const StringMap &params,
				    uint32_t targetUserRoleId,
				    const HatoholErrorCode &expectCode,
				    bool operatorHasPrivilege = true)
{
	const bool dbRecreate = true;
	const bool loadTestDat = true;
	setupTestDBUser(dbRecreate, loadTestDat);
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
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
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
	  "%" FMT_OPPRVLG, expectedUserRoleInfo.flags);
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
	params["flags"] = StringUtils::sprintf("%" FMT_OPPRVLG, flags);
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

	if (url.empty())
		url = StringUtils::sprintf("/user-role/%" FMT_USER_ROLE_ID,
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
	// It's supposed to be a user with ID:2, who can access all hosts.
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJsonParser(arg);
	assertErrorCode(g_parser);
	assertOverviewInParser(g_parser, arg);
}

} // namespace testFaceRest
