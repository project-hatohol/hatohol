/*
 * Copyright (C) 2013-2015 Project Hatohol
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

#include <cppcutter.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "UnifiedDataStore.h"
#include "testDBTablesMonitoring.h"
#include "FaceRestTestUtils.h"
#include "RestResourceHost.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestHost {

static void _assertTestTriggerInfo(
  JSONParser *parser, const TriggerInfo &triggerInfo)
{
	assertValueInParser(parser, "status", triggerInfo.status);
	assertValueInParser(parser, "severity", triggerInfo.severity);
	assertValueInParser(parser, "lastChangeTime",
	                    triggerInfo.lastChangeTime);
	assertValueInParser(parser, "serverId", triggerInfo.serverId);
	assertValueInParser(parser, "hostId", triggerInfo.hostIdInServer);
	assertValueInParser(parser, "brief", triggerInfo.brief);
	assertValueInParser(parser, "extendedInfo", triggerInfo.extendedInfo);
}
#define assertTestTriggerInfo(P, T) cut_trace(_assertTestTriggerInfo(P, T))

static void assertHostsInParser(JSONParser *parser,
                                const ServerIdType &targetServerId,
                                DataQueryContext *dqCtx)
{
	ServerHostDefVect svHostDefs;
	HostsQueryOption option(dqCtx);
	option.setTargetServerId(targetServerId);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK, uds->getServerHostDefs(svHostDefs, option));

	// Pick up defunct servers
	set<size_t> excludeIdSet;
	for (size_t i = 0; i < svHostDefs.size(); i++) {
		if (!dqCtx->isValidServer(svHostDefs[i].serverId))
			excludeIdSet.insert(i);
	}
	const size_t numValidHosts = svHostDefs.size() - excludeIdSet.size();
	assertValueInParser(parser, "numberOfHosts", numValidHosts);

	// make an index
	map<ServerIdType, map<LocalHostIdType, const ServerHostDef *> > hostMap;
	map<LocalHostIdType, const ServerHostDef *>::iterator hostMapIt;
	for (size_t i = 0; i < svHostDefs.size(); i++) {
		if (excludeIdSet.find(i) != excludeIdSet.end())
			continue;
		const ServerHostDef &svHostDef = svHostDefs[i];
		hostMapIt = hostMap[svHostDef.serverId].find(svHostDef.name);
		cppcut_assert_equal(
		  true, hostMapIt == hostMap[svHostDef.serverId].end());
		hostMap[svHostDef.serverId][svHostDef.hostIdInServer]
		  = &svHostDef;
	}

	assertStartObject(parser, "hosts");
	for (size_t i = 0; i < numValidHosts; i++) {
		int64_t var64;
		parser->startElement(i);
		cppcut_assert_equal(true, parser->read("serverId", var64));
		const ServerIdType serverId = static_cast<ServerIdType>(var64);

		string hostIdInServer;
		cppcut_assert_equal(true, parser->read("id", hostIdInServer));
		hostMapIt = hostMap[serverId].find(hostIdInServer);
		cppcut_assert_equal(true, hostMapIt != hostMap[serverId].end());
		const ServerHostDef &svHostDef = *hostMapIt->second;

		assertValueInParser(parser, "hostName", svHostDef.name);
		parser->endElement();
		hostMap[serverId].erase(hostMapIt);
	}
	parser->endObject();
}

static void assertHostsIdNameHashInParser(const TriggerInfo *triggers,
                                          size_t numberOfTriggers,
                                          JSONParser *parser)
{
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < numberOfTriggers; i++) {
		const TriggerInfo &triggerInfo = triggers[i];
		assertStartObject(parser, StringUtils::toString(triggerInfo.serverId));
		assertStartObject(parser, "hosts");
		assertStartObject(parser, triggerInfo.hostIdInServer);
		assertValueInParser(parser, "name", triggerInfo.hostName);
		parser->endObject();
		parser->endObject();
		parser->endObject();
	}
	parser->endObject();
}

static void assertHostsIdNameHashInParser(
  const vector<const EventInfo *> &expectedRecords, JSONParser *parser)
{
	assertStartObject(parser, "servers");
	for (size_t i = 0; i < expectedRecords.size(); i++) {
		const EventInfo &eventInfo = *expectedRecords[i];
		assertStartObject(parser, StringUtils::toString(eventInfo.serverId));
		assertStartObject(parser, "hosts");
		assertStartObject(parser, eventInfo.hostIdInServer);
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
	loadTestDBServerHostDef();
	startFaceRest();

	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		   StringUtils::sprintf("%" PRIu32, serverId);
	}
	RequestArg arg(path, callbackName);
	arg.parameters = queryMap;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	unique_ptr<JSONParser> parserPtr(getResponseAsJSONParser(arg));
	assertErrorCode(parserPtr.get());
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);
	assertHostsInParser(parserPtr.get(), serverId, dqCtxPtr);
}
#define assertHosts(P,...) cut_trace(_assertHosts(P,##__VA_ARGS__))

static void _assertTriggers(
  const string &path, const string &callbackName = "",
  const ServerIdType &serverId = ALL_SERVERS,
  const LocalHostIdType &hostIdInServer = ALL_LOCAL_HOSTS,
  const timespec &beginTime = {0, 0},
  const timespec &endTime = {0, 0},
  const size_t expectedNumTrigger = -1)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);
	size_t expectedNumTrig = 0, allNumTrig = -1;

	// calculate the expected test triggers
	ServerIdTriggerIdIdxMap         indexMap;
	ServerIdTriggerIdIdxMapIterator indexMapIt;
	TriggerIdIdxMapIterator         trigIdIdxIt;
	getTestTriggersIndexes(indexMap, serverId, hostIdInServer);
	if (expectedNumTrigger == allNumTrig) {
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
	} else {
		expectedNumTrig = expectedNumTrigger;
	}

	// request
	StringMap queryMap;
	if (serverId != ALL_SERVERS) {
		queryMap["serverId"] =
		  StringUtils::sprintf("%" PRIu32, serverId);
	}
	if (hostIdInServer != ALL_LOCAL_HOSTS)
		queryMap["hostId"] = hostIdInServer;
	if (beginTime.tv_sec != 0 || beginTime.tv_nsec != 0) {
		queryMap["beginTime"] = StringUtils::sprintf("%ld", beginTime.tv_sec);
	}
	if (endTime.tv_sec != 0 || endTime.tv_nsec != 0) {
		queryMap["endTime"] = StringUtils::sprintf("%ld", endTime.tv_sec);
	}
	arg.parameters = queryMap;
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);

	// Check the reply
	assertErrorCode(parser);
	assertValueInParser(parser, "numberOfTriggers", expectedNumTrig);
	assertValueInParser(parser, "totalNumberOfTriggers", expectedNumTrig);
	assertStartObject(parser, "triggers");
	for (size_t i = 0; i < expectedNumTrig; i++) {
		parser->startElement(i);
		int64_t var64;
		cppcut_assert_equal(true, parser->read("serverId", var64));
		const ServerIdType actSvId =
		  static_cast<ServerIdType>(var64);

		string actTrigId;
		cppcut_assert_equal(true, parser->read("id", actTrigId));
		trigIdIdxIt = indexMap[actSvId].find(actTrigId);
		cppcut_assert_equal(
		  true, trigIdIdxIt != indexMap[actSvId].end());
		size_t idx = trigIdIdxIt->second;
		indexMap[actSvId].erase(trigIdIdxIt);

		const TriggerInfo &triggerInfo = testTriggerInfo[idx];
		assertTestTriggerInfo(parser, triggerInfo);
		parser->endElement();
	}
	parser->endObject();
	assertHostsIdNameHashInParser(testTriggerInfo, expectedNumTrig, parser);
	assertServersIdNameHashInParser(parser);
}
#define assertTriggers(P,...) cut_trace(_assertTriggers(P,##__VA_ARGS__))

static void _assertEvents(const string &path,
			  AssertGetEventsArg &expectedEventsArg,
			  const string &callbackName = "")
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBServerHostDef();
	startFaceRest();

	// check json data
	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);

	// check the reply
	assertErrorCode(parser);
	assertValueInParser(parser, "numberOfEvents",
	                    expectedEventsArg.expectedRecords.size());
	assertValueInParser(parser, "lastUnifiedEventId",
	                    expectedEventsArg.findLastUnifiedId());
	ThreadLocalDBCache cache;
	bool shouldHaveIncident = cache.getAction().isIncidentSenderEnabled();
	assertValueInParser(parser, "haveIncident", shouldHaveIncident);

	assertStartObject(parser, "events");
	vector<const EventInfo*>::reverse_iterator it
	  = expectedEventsArg.expectedRecords.rbegin();
	for (size_t i = 0; it != expectedEventsArg.expectedRecords.rend(); i++, ++it) {
		const EventInfo &eventInfo = *(*it);
		uint64_t unifiedId = expectedEventsArg.idMap[*it];
		parser->startElement(i);
		assertValueInParser(parser, "unifiedId", unifiedId);
		assertValueInParser(parser, "serverId", eventInfo.serverId);
		assertValueInParser(parser, "time", eventInfo.time);
		assertValueInParser(parser, "type", eventInfo.type);
		assertValueInParser(parser, "triggerId", (eventInfo.triggerId));
		assertValueInParser(parser, "eventId",   eventInfo.id);
		assertValueInParser(parser, "status",    eventInfo.status);
		assertValueInParser(parser, "severity",  eventInfo.severity);
		assertValueInParser(parser, "hostId",    eventInfo.hostIdInServer);
		assertValueInParser(parser, "brief",     eventInfo.brief);
		assertValueInParser(parser, "extendedInfo", eventInfo.extendedInfo);
		if (shouldHaveIncident) {
			assertStartObject(parser, "incident");
			const IncidentInfo incident
			  = expectedEventsArg.getExpectedIncidentInfo(eventInfo);
			assertValueInParser(parser, "location",
					    incident.location);
			parser->endElement();
		} else {
			assertNoValueInParser(parser, "incident");
		}
		parser->endElement();
	}
	parser->endObject();
	assertHostsIdNameHashInParser(expectedEventsArg.expectedRecords, parser);
	assertServersIdNameHashInParser(parser);
}

static void _assertEvents(const string &path, const string &callbackName = "")
{
	// build expected data
	AssertGetEventsArg eventsArg(NULL);
	eventsArg.excludeDefunctServers = true;
	eventsArg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	eventsArg.sortType = EventsQueryOption::SORT_TIME;
	eventsArg.sortDirection = EventsQueryOption::SORT_DESCENDING;
	eventsArg.fixup();

	_assertEvents(path, eventsArg, callbackName);
}
#define assertEvents(P,...) cut_trace(_assertEvents(P,##__VA_ARGS__))

static void _assertItems(const string &path, const string &callbackName = "",
			 ssize_t numExpectedItems = -1)
{
	loadTestDBItems();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);
	assertErrorCode(parser);

	// NOTE: Items that belong to the servers that registered in the config
	// DB are returned. I.e. all elements of testItemInfo shall not be
	// in 'parser'.

	// Check the size of ItemInfo
	if (numExpectedItems < 0) {
		numExpectedItems = 0;
		for (size_t i = 0; i < NumTestItemInfo; i++) {
			if (dqCtxPtr->isValidServer(testItemInfo[i].serverId))
				numExpectedItems++;
		}
	}
	// If 'numExpectedItems' is 0, the test matarial is bad.
	cppcut_assert_not_equal((ssize_t)0, numExpectedItems);

	int64_t _numItems = 0;
	cppcut_assert_equal(true, parser->read("numberOfItems", _numItems));
	ssize_t numItems = static_cast<ssize_t>(_numItems);
	cppcut_assert_equal(numExpectedItems, numItems);

	cppcut_assert_equal(true, parser->read("totalNumberOfItems", _numItems));
	numItems = static_cast<ssize_t>(_numItems);
	cppcut_assert_equal(numExpectedItems, numItems);

	// Check each ItemInfo
	ServerIdItemInfoIdIndexMapMap indexMap;
	getTestItemsIndexes(indexMap);
	assertStartObject(parser, "items");
	set<const ItemInfo *> itemInfoPtrSet;
	for (ssize_t i = 0; i < numItems; i++) {
		int64_t serverId = 0;
		ItemIdType itemInfoId;

		parser->startElement(i);
		cppcut_assert_equal(true, parser->read("serverId", serverId));
		cppcut_assert_equal(true, parser->read("id", itemInfoId));
		const ItemInfo *itemInfoPtr =
		  findTestItem(indexMap, serverId, itemInfoId);
		cppcut_assert_not_null(itemInfoPtr);
		const ItemInfo &itemInfo = *itemInfoPtr;

		assertValueInParser(parser, "serverId", itemInfo.serverId);
		assertValueInParser(parser, "hostId", itemInfo.hostIdInServer);
		assertValueInParser(parser, "brief", itemInfo.brief);
		assertValueInParser(parser, "lastValueTime",
		                    itemInfo.lastValueTime);
		assertValueInParser(parser, "lastValue", itemInfo.lastValue);
		assertValueInParser(parser, "unit", itemInfo.unit);
		assertValueInParser(parser, "valueType",
				    static_cast<int>(itemInfo.valueType));
		parser->endElement();

		// Check duplication
		pair<set<const ItemInfo *>::iterator, bool> result =
		  itemInfoPtrSet.insert(itemInfoPtr);
		cppcut_assert_equal(true, result.second);
	}
	parser->endObject();
	assertServersIdNameHashInParser(parser);
}
#define assertItems(P,...) cut_trace(_assertItems(P,##__VA_ARGS__))

static void assertHostgroupsInParser(JSONParser *parser,
                                     const ServerIdType &serverId,
                                     HostgroupIdSet &hostgroupIdSet)
{
	assertStartObject(parser, "hostgroups");
	for (size_t i = 0; i < NumTestHostgroup; i++) {
		const Hostgroup &hostgrp = testHostgroup[i];
		// TODO: fix this inefficient algorithm
		if (hostgrp.serverId != serverId)
			continue;
		assertStartObject(parser, hostgrp.idInServer);
		assertValueInParser(parser, string("name"), hostgrp.name);
		parser->endObject();
		hostgroupIdSet.insert(hostgrp.idInServer);
	}
	parser->endObject();
}

static void assertHostStatusInParser(JSONParser *parser,
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
  JSONParser *parser, const int &severityNum,
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

static void assertSystemStatusInParser(JSONParser *parser,
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

static void _assertOverviewInParser(JSONParser *parser, RequestArg &arg)
{
	assertValueInParser(parser, "numberOfServers", NumTestServerInfo);
	assertStartObject(parser, "serverStatus");
	size_t numGoodServers = 0, numBadServers = 0;
	// We assume that the caller can access all serers and hosts.
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		HostgroupIdSet hostgroupIdSet;
		parser->startElement(i);
		const MonitoringServerInfo &svInfo = testServerInfo[i];
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
		string nvps = "0.00";
		if (i < NumTestServerStatus) {
			const MonitoringServerStatus &serverStatus
			  = testServerStatus[i];
			nvps = StringUtils::sprintf("%.2f", serverStatus.nvps);
		}
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

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBUser();
}

void cut_teardown(void)
{
	stopFaceRest();

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
	               testTriggerInfo[1].serverId,
	               testTriggerInfo[1].hostIdInServer);
}

void test_triggersWithTimeRange(void)
{
	timespec beginTime = {1362957197,0}, endTime = {1362957200,0};
	size_t numExpectedTriggers = 5;
	assertTriggers("/trigger", "foo", ALL_SERVERS, ALL_LOCAL_HOSTS,
		       beginTime, endTime, numExpectedTriggers);
}

void test_events(void)
{
	assertEvents("/event");
}

void test_eventsWithIncidents(void)
{
	 // incident info will be added when a IncidentSender action exists
	loadTestDBIncidents();

	assertEvents("/event");
}

void test_eventsJSONP(void)
{
	assertEvents("/event", "foo");
}

void test_eventsForOneServerOneHost(void)
{
	// build expected data
	AssertGetEventsArg eventsArg(NULL);
	eventsArg.excludeDefunctServers = true;
	eventsArg.targetServerId = testEventInfo[1].serverId;
	eventsArg.targetHostId = testEventInfo[1].hostIdInServer;
	eventsArg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	eventsArg.sortType = EventsQueryOption::SORT_TIME;
	eventsArg.sortDirection = EventsQueryOption::SORT_DESCENDING;
	eventsArg.fixup();

	string query =
	  StringUtils::sprintf(
	    "/event?serverId=%" FMT_SERVER_ID "&hostId=%" FMT_LOCAL_HOST_ID,
	    testEventInfo[1].serverId,
	    testEventInfo[1].hostIdInServer.c_str());

	assertEvents(query, eventsArg);
}

void test_eventsWithTimeRange(void)
{
	AssertGetEventsArg eventsArg(NULL);
	eventsArg.excludeDefunctServers = true;
	eventsArg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	eventsArg.sortType = EventsQueryOption::SORT_TIME;
	eventsArg.sortDirection = EventsQueryOption::SORT_DESCENDING;
	eventsArg.beginTime = {1363000000, 0};
	eventsArg.endTime = {1389123457, 0};
	eventsArg.fixup();

	assertEvents("/event?beginTime=1363000000&endTime=1389123457",
		     eventsArg);
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

void test_itemsJSONWithQuery(void)
{
	assertItems("/item?serverId=3&hostId=5&itemId=1", "", 1);
}

void test_overview(void)
{
	startFaceRest();
	loadTestDBTriggers();
	loadTestDBItems();
	loadTestDBServerStatus();
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();

	RequestArg arg("/overview");
	// It's supposed to be a user with ID:2, who can access all hosts.
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);
	assertErrorCode(parser);
	assertOverviewInParser(parser, arg);
}

void test_getHistoryWithoutParameter(void)
{
	startFaceRest();

	RequestArg arg("/history");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);
	assertErrorCode(parser, HTERR_NOT_FOUND_PARAMETER);
}

void test_getHistoryWithMinimumParameter(void)
{
	startFaceRest();
	loadTestDBItems();
	loadTestDBServerHostDef();

	RequestArg arg("/history");
	StringMap params;
	params["serverId"] = StringUtils::toString(testItemInfo[0].serverId);
	params["itemId"] = testItemInfo[0].id;
	arg.parameters = params;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);
	assertErrorCode(parser, HTERR_OK);

	// TODO: check contents
}

void test_getHistoryWithInvalidItemId(void)
{
	startFaceRest();

	RequestArg arg("/history");
	StringMap params;
	params["serverId"] = StringUtils::toString(testItemInfo[0].serverId);
	params["itemId"] = testItemInfo[0].id;
	arg.parameters = params;
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	JSONParser *parser = getResponseAsJSONParser(arg);
	unique_ptr<JSONParser> parserPtr(parser);
	assertErrorCode(parser, HTERR_NOT_FOUND_TARGET_RECORD);
}

static string getExpectedServers(void)
{
	return string("\"servers\":{"

		      "\"1\":{"
		      "\"name\":\"pochi.dog.com\","
		      "\"nickname\":\"POCHI\","
		      "\"type\":7,"
		      "\"ipAddress\":\"192.168.0.5\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"8e632c14-d1f7-11e4-8350-d43d7e3146fb\","
		      "\"hosts\":{"
		      "\"1129\":{\"name\":\"hostX3\"},"
		      "\"235012\":{\"name\":\"hostX1\"},"
		      "\"235013\":{\"name\":\"hostX2\"}},"
		      "\"groups\":{}},"

		      "\"2\":{"
		      "\"name\":\"mike.dog.com\","
		      "\"nickname\":\"MIKE\","
		      "\"type\":7,"
		      "\"ipAddress\":\"192.168.1.5\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"902d955c-d1f7-11e4-80f9-d43d7e3146fb\","
		      "\"hosts\":{"
		      "\"512\":{\"name\":\"multi-host group\"},"
		      "\"9920249034889494527\":{\"name\":\"hostQ1\"}},"
		      "\"groups\":{}},"

		      "\"3\":{"
		      "\"name\":\"hachi.dog.com\","
		      "\"nickname\":\"8\","
		      "\"type\":7,"
		      "\"ipAddress\":\"192.168.10.1\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"\","
		      "\"hosts\":{"
		      "\"100\":{\"name\":\"dolphin\"},"
		      "\"10001\":{\"name\":\"hostZ1\"},"
		      "\"10002\":{\"name\":\"hostZ2\"},"
		      "\"5\":{\"name\":\"frog\"}},"
		      "\"groups\":{}},"

		      "\"4\":{"
		      "\"name\":\"mosquito.example.com\","
		      "\"nickname\":\"KA\","
		      "\"type\":7,"
		      "\"ipAddress\":\"10.100.10.52\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"\","
		      "\"hosts\":{"
		      "\"100\":{\"name\":\"squirrel\"}},"
		      "\"groups\":{}},"

		      "\"5\":{"
		      "\"name\":\"overture.example.com\","
		      "\"nickname\":\"OIOI\","
		      "\"type\":7,"
		      "\"ipAddress\":\"123.45.67.89\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"\","
		      "\"hosts\":{},"
		      "\"groups\":{}},"

		      "\"211\":{"
		      "\"name\":\"x-men.example.com\","
		      "\"nickname\":\"(^_^)\","
		      "\"type\":7,"
		      "\"ipAddress\":\"172.16.32.51\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"\","
		      "\"hosts\":{"
		      "\"12111\":{\"name\":\"host 12111\"},"
		      "\"12112\":{\"name\":\"host 12112\"},"
		      "\"12113\":{\"name\":\"host 12113\"},"
		      "\"200\":{\"name\":\"host 200\"}},"
		      "\"groups\":{}},"

		      "\"222\":{"
		      "\"name\":\"zoo.example.com\","
		      "\"nickname\":\"Akira\","
		      "\"type\":7,"
		      "\"ipAddress\":\"10.0.0.48\","
		      "\"baseURL\":\"\","
		      "\"uuid\":\"\","
		      "\"hosts\":{"
		      "\"110005\":{\"name\":\"host 110005\"}},"
		      "\"groups\":{}},"

		      "\"301\":{"
		      "\"name\":\"nagios.example.com\","
		      "\"nickname\":\"Akira\","
		      "\"type\":7,"
		      "\"ipAddress\":\"10.0.0.32\","
		      "\"baseURL\":\"http://10.0.0.32/nagios3\","
		      "\"uuid\":\"\","
		      "\"hosts\":{},"
		      "\"groups\":{}}"

		      "}");
}

static string getExpectedIncidentTrackers(void)
{
	return string("\"incidentTrackers\":{"

		       "\"1\":{"
		       "\"type\":0,"
		       "\"nickname\":\"Numerical ID\","
		       "\"baseURL\":\"http://localhost\","
		       "\"projectId\":\"1\","
		       "\"trackerId\":\"3\"},"

		      "\"2\":{"
		      "\"type\":0,"
		      "\"nickname\":\"String project ID\","
		      "\"baseURL\":\"http://localhost\","
		      "\"projectId\":\"hatohol\","
		      "\"trackerId\":\"3\"},"

		      "\"3\":{"
		      "\"type\":0,"
		      "\"nickname\":\"Redmine Emulator\","
		      "\"baseURL\":\"http://localhost:44444\","
		      "\"projectId\":\"hatoholtestproject\","
		      "\"trackerId\":\"1\"},"

		      "\"4\":{"
		      "\"type\":0,\"nickname\":\"Redmine Emulator\","
		      "\"baseURL\":\"http://localhost:44444\","
		      "\"projectId\":\"hatoholtestproject\","
		      "\"trackerId\":\"2\"},"

		      "\"5\":{"
		      "\"type\":1,"
		      "\"nickname\":\"Internal\","
		      "\"baseURL\":\"\","
		      "\"projectId\":\"\","
		      "\"trackerId\":\"\"}"

		      "}");
}

void test_eventsWithHostsFilter(void)
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg("/event"
		       "?excludeHosts%5B0%5D%5BserverId%5D=1"
		       "&excludeHosts%5B1%5D%5BserverId%5D=3"
		       "&selectHosts%5B2%5D%5BserverId%5D=1"
		       "&selectHosts%5B2%5D%5BhostId%5D=235013");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected("{"
			"\"apiVersion\":4,"
			"\"errorCode\":0,"
			"\"lastUnifiedEventId\":7,"
			"\"haveIncident\":false,"
			"\"events\":"
			"[{"
			"\"unifiedId\":5,"
			"\"serverId\":1,"
			"\"time\":1389123457,"
			"\"type\":0,"
			"\"triggerId\":\"3\","
			"\"eventId\":\"3\","
			"\"status\":1,"
			"\"severity\":1,"
			"\"hostId\":\"235013\","
			"\"brief\":\"TEST Trigger 1b\","
			"\"extendedInfo\":\"\""
			"}],"
			"\"numberOfEvents\":1,");
	expected += getExpectedServers() + ",";
	expected += getExpectedIncidentTrackers() + "}";
	assertEqualJSONString(expected, arg.response);
}

void test_eventsWithTypesFilter(void)
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg("/event?types=1%2C2");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"lastUnifiedEventId\":7,"
	  "\"haveIncident\":false,"
	  "\"events\":"
	  "["
	  "{"
	  "\"unifiedId\":7,"
	  "\"serverId\":3,"
	  "\"time\":1390000100,"
	  "\"type\":2,"
	  "\"triggerId\":\"4\","
	  "\"eventId\":\"4\","
	  "\"status\":2,"
	  "\"severity\":4,"
	  "\"hostId\":\"10002\","
	  "\"brief\":\"Status:Unknown, Severity:Critical\","
	  "\"extendedInfo\":\"\""
	  "},"
	  "{"
	  "\"unifiedId\":6,"
	  "\"serverId\":3,"
	  "\"time\":1390000000,"
	  "\"type\":1,"
	  "\"triggerId\":\"2\","
	  "\"eventId\":\"3\","
	  "\"status\":1,"
	  "\"severity\":2,"
	  "\"hostId\":\"10001\","
	  "\"brief\":\"TEST Trigger 2\","
	  "\"extendedInfo\":"
	  "\"{\\\"expandedDescription\\\":\\\"Test Trigger on hostZ1\\\"}\""
	  "}"
	  "],"
	  "\"numberOfEvents\":2,");

	expected += getExpectedServers() + ",";
	expected += getExpectedIncidentTrackers() + "}";
	assertEqualJSONString(expected, arg.response);
}

void test_eventsWithSeveritiesFilter(void)
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg("/event?severities=2%2C4");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"lastUnifiedEventId\":7,"
	  "\"haveIncident\":false,"
	  "\"events\":"
	  "["
	  "{"
	  "\"unifiedId\":7,"
	  "\"serverId\":3,"
	  "\"time\":1390000100,"
	  "\"type\":2,"
	  "\"triggerId\":\"4\","
	  "\"eventId\":\"4\","
	  "\"status\":2,"
	  "\"severity\":4,"
	  "\"hostId\":\"10002\","
	  "\"brief\":\"Status:Unknown, Severity:Critical\","
	  "\"extendedInfo\":\"\""
	  "},"
	  "{"
	  "\"unifiedId\":6,"
	  "\"serverId\":3,"
	  "\"time\":1390000000,"
	  "\"type\":1,"
	  "\"triggerId\":\"2\","
	  "\"eventId\":\"3\","
	  "\"status\":1,"
	  "\"severity\":2,"
	  "\"hostId\":\"10001\","
	  "\"brief\":\"TEST Trigger 2\","
	  "\"extendedInfo\":"
	  "\"{\\\"expandedDescription\\\":\\\"Test Trigger on hostZ1\\\"}\""
	  "},"
	  "{"
	  "\"unifiedId\":1,"
	  "\"serverId\":3,"
	  "\"time\":1362957200,"
	  "\"type\":0,"
	  "\"triggerId\":\"2\","
	  "\"eventId\":\"1\","
	  "\"status\":1,"
	  "\"severity\":2,"
	  "\"hostId\":\"10001\","
	  "\"brief\":\"TEST Trigger 2\","
	  "\"extendedInfo\":"
	  "\"{\\\"expandedDescription\\\":\\\"Test Trigger on hostZ1\\\"}\""
	  "}"
	  "],"
	  "\"numberOfEvents\":3,");

	expected += getExpectedServers() + ",";
	expected += getExpectedIncidentTrackers() + "}";
	assertEqualJSONString(expected, arg.response);
}

void test_eventsWithStatusesFilter(void)
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg("/event?statuses=0%2C2");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"lastUnifiedEventId\":7,"
	  "\"haveIncident\":false,"
	  "\"events\":"
	  "["
	  "{"
	  "\"unifiedId\":7,"
	  "\"serverId\":3,"
	  "\"time\":1390000100,"
	  "\"type\":2,"
	  "\"triggerId\":\"4\","
	  "\"eventId\":\"4\","
	  "\"status\":2,"
	  "\"severity\":4,"
	  "\"hostId\":\"10002\","
	  "\"brief\":\"Status:Unknown, Severity:Critical\","
	  "\"extendedInfo\":\"\""
	  "},"
	  "{"
	  "\"unifiedId\":4,"
	  "\"serverId\":1,"
	  "\"time\":1378900022,"
	  "\"type\":0,"
	  "\"triggerId\":\"1\","
	  "\"eventId\":\"2\","
	  "\"status\":0,"
	  "\"severity\":1,"
	  "\"hostId\":\"235012\","
	  "\"brief\":\"TEST Trigger 1\","
	  "\"extendedInfo\":\"\""
	  "}"
	  "],"
	  "\"numberOfEvents\":2,");

	expected += getExpectedServers() + ",";
	expected += getExpectedIncidentTrackers() + "}";
	assertEqualJSONString(expected, arg.response);
}

void test_eventsWithIncidentStatusesFilter(void)
{
	loadTestDBTriggers();
	loadTestDBEvents();
	loadTestDBIncidents();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg("/event?incidentStatuses=New%2CNONE"
		       "&sortOrder=1&sortType=time");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	getServerResponse(arg);
	string expected(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"lastUnifiedEventId\":7,"
	  "\"haveIncident\":false,"
	  "\"events\":"
	  "["
	  "{"
	  "\"unifiedId\":3,"
	  "\"serverId\":1,"
	  "\"time\":1363123456,"
	  "\"type\":0,"
	  "\"triggerId\":\"2\","
	  "\"eventId\":\"1\","
	  "\"status\":1,"
	  "\"severity\":1,"
	  "\"hostId\":\"235012\","
	  "\"brief\":\"TEST Trigger 1a\","
	  "\"extendedInfo\":"
	  "\"{\\\"expandedDescription\\\":\\\"Test Trigger on hostX1\\\"}\""
	  "},"
	  "{"
	  "\"unifiedId\":4,"
	  "\"serverId\":1,"
	  "\"time\":1378900022,"
	  "\"type\":0,"
	  "\"triggerId\":\"1\","
	  "\"eventId\":\"2\","
	  "\"status\":0,"
	  "\"severity\":1,"
	  "\"hostId\":\"235012\","
	  "\"brief\":\"TEST Trigger 1\","
	  "\"extendedInfo\":\"\""
	  "}"
	  "],"
	  "\"numberOfEvents\":2,");

	expected += getExpectedServers() + ",";
	expected += getExpectedIncidentTrackers() + "}";
	assertEqualJSONString(expected, arg.response);
}

static void incidentInfo2StringMap(
  const IncidentInfo &src, StringMap &dest)
{
	dest["status"] = src.status;
	dest["priority"] = src.priority;
	dest["assignee"] = src.assignee;
	dest["doneRatio"] = StringUtils::toString(src.doneRatio);

	/* Hatohol doesn't allow changing following properties:
	dest["trackerId"] = StringUtils::toString(src.trackerId);
	dest["serverId"] = StringUtils::toString(serc.serverId);
	dest["eventId"] = src.eventId;
	dest["triggerId"] = src.triggerId;
	dest["identifier"] = src.identifier;
	dest["location"] = src.location;
	dest["createdAtSec"] = src.createdAt.tv_sec;
	dest["createdAtNSec"] = src.createdAt.tv_nsec;
	dest["updatedAtSec"] = src.updatedAt.tv_sec;
	dest["updatedAtNSec"] = src.updatedAt.tv_nsec;
	dest["unifiedEventId"] = src.unifiedEventId;
	*/
}

void test_putIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	size_t index = 2;
	IncidentInfo expectedIncident = testIncidentInfo[index];
	expectedIncident.status = "IN PROGRESS";
	expectedIncident.priority = "HIGH";
	expectedIncident.assignee = "taro";
	expectedIncident.doneRatio = 30;
	string path(
	  StringUtils::sprintf("/incident/%" FMT_UNIFIED_EVENT_ID,
			       expectedIncident.unifiedEventId));
	RequestArg arg(path);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "PUT";
	incidentInfo2StringMap(expectedIncident, arg.parameters);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"unifiedEventId\":123"
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=123");
	string expected =
		"^5\\|2\\|2\\|3\\|123\\|\\|IN PROGRESS\\|taro\\|"
		"1412957360\\|0\\|\\d+\\|\\d+\\|HIGH\\|30\\|123$";
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_putInvalidIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	size_t index = 2;
	IncidentInfo expectedIncident = testIncidentInfo[index];
	expectedIncident.status = "INVALID STATUS";
	string path(
	  StringUtils::sprintf("/incident/%" FMT_UNIFIED_EVENT_ID,
			       expectedIncident.unifiedEventId));
	RequestArg arg(path);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "PUT";
	incidentInfo2StringMap(expectedIncident, arg.parameters);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":42,"
	  "\"errorMessage\":\"Invalid parameter.\","
	  "\"optionMessages\":\"Unknown status: INVALID STATUS\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=123");
	string expected = makeIncidentOutput(testIncidentInfo[index]);
	cppcut_assert_equal(expected, actual);
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_postDuplicateIncident(void)
{
	loadTestDBEvents();
	loadTestDBIncidents();
	startFaceRest();

	IncidentTrackerIdType trackerId
	  = findIncidentTrackerIdByType(INCIDENT_TRACKER_HATOHOL);
	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "POST";
	arg.parameters["unifiedEventId"] = "3";
	arg.parameters["incidentTrackerId"]
	  = StringUtils::sprintf("%" FMT_INCIDENT_TRACKER_ID, trackerId);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":65,"
	  "\"errorMessage\":\"The record already exists.\","
	  "\"optionMessages\":\"id: 3\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	assertEqualJSONString(expectedResponse, arg.response);

	// check the content in the DB
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where unified_event_id=3");
	string expected = makeIncidentOutput(testIncidentInfo[0]);
	cppcut_assert_equal(expected, actual);
}

void test_postIncidentForUnknownEvent(void)
{
	startFaceRest();

	IncidentTrackerIdType trackerId
	  = findIncidentTrackerIdByType(INCIDENT_TRACKER_HATOHOL);
	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "POST";
	arg.parameters["unifiedEventId"] = "3";
	arg.parameters["incidentTrackerId"]
	  = StringUtils::sprintf("%" FMT_INCIDENT_TRACKER_ID, trackerId);

	// check the response
	getServerResponse(arg);
	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":10,"
	  "\"errorMessage\":\"Not found target record.\","
	  "\"optionMessages\":\"id: 3\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	assertEqualJSONString(expectedResponse, arg.response);
}

void test_getIncident(void)
{
	loadTestDBIncidents();
	startFaceRest();

	RequestArg arg("/incident");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);
	cppcut_assert_equal(405, arg.httpStatusCode); // Method Not Allowed
}

void test_getNumberOfEvents(void)
{
	loadTestDBEvents();
	startFaceRest();

	RequestArg arg("/events?countOnly=true&serverId=3");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);

	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":0,"
	  "\"numberOfEvents\":4"
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);
}

void test_getNumberOfEventsWithInvalidQuery(void)
{
	loadTestDBEvents();
	startFaceRest();

	RequestArg arg("/events?countOnly=TRUE&serverId=3");
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	arg.request = "GET";
	getServerResponse(arg);

	string expectedResponse(
	  "{"
	  "\"apiVersion\":4,"
	  "\"errorCode\":42,"
	  "\"errorMessage\":\"Invalid parameter.\","
	  "\"optionMessages\":\"Invalid value for countOnly: TRUE\""
	  "}");
	cppcut_assert_equal(200, arg.httpStatusCode);
	cppcut_assert_equal(expectedResponse, arg.response);
}

} // namespace testFaceRestHost
