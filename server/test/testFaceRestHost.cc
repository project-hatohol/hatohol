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
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "UnifiedDataStore.h"
#include "testDBClientHatohol.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestHost {

static JSONParserAgent *g_parser = NULL;

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

static void assertHostsInParser(JSONParserAgent *parser,
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
                                          JSONParserAgent *parser)
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
  const vector<EventInfo *> &expectedRecords, JSONParserAgent *parser)
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
	g_parser = getResponseAsJSONParser(arg);
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
	g_parser = getResponseAsJSONParser(arg);

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
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfEvents",
	                    eventsArg.expectedRecords.size());
	assertValueInParser(g_parser, "lastUnifiedEventId",
	                    eventsArg.expectedRecords.size());
	DBClientAction dbAction;
	bool shouldHaveIncident = dbAction.isIncidentSenderEnabled();
	assertValueInParser(g_parser, "haveIncident", shouldHaveIncident);

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
		if (shouldHaveIncident) {
			assertStartObject(g_parser, "incident");
			IncidentInfo incident
			  = eventsArg.getExpectedIncidentInfo(eventInfo);
			assertValueInParser(g_parser, "location",
					    incident.location);
			g_parser->endElement();
		} else {
			assertNoValueInParser(g_parser, "incident");
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
	g_parser = getResponseAsJSONParser(arg);
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

static void assertHostgroupsInParser(JSONParserAgent *parser,
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

static void assertHostStatusInParser(JSONParserAgent *parser,
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
  JSONParserAgent *parser, const int &severityNum,
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

static void assertSystemStatusInParser(JSONParserAgent *parser,
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

static void _assertOverviewInParser(JSONParserAgent *parser, RequestArg &arg)
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

static void setupActionDB(void)
{
	bool recreate = true;
	bool loadData = true;
	setupTestDBAction(recreate, loadData);
}

void cut_setup(void)
{
	hatoholInit();

	// Make sure to clear actions on intial state because the incident object
	// in an event object depends on them.
	bool recreate = true;
	bool loadData = false;
	setupTestDBAction(recreate, loadData);
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->setCopyOnDemandEnabled(false);
}

void test_hosts(void)
{
	assertHosts("/host");
}

void test_hostsJSONP(void)
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

void test_triggersJSONP(void)
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

void test_eventsWithIncidents(void)
{
	 // incident info will be added when a IncidentSender action exists
	setupActionDB();
	assertEvents("/event");
}

void test_eventsJSONP(void)
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

void test_itemsJSONP(void)
{
	assertItems("/item", "foo");
}

void test_overview(void)
{
	setupUserDB();
	startFaceRest();
	RequestArg arg("/overview");
	// It's supposed to be a user with ID:2, who can access all hosts.
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertOverviewInParser(g_parser, arg);
}

} // namespace testFaceRestHost
