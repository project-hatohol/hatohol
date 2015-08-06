/*
 * Copyright (C) 2013-2014 Project Hatohol
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
  const LocalHostIdType &hostIdInServer = ALL_LOCAL_HOSTS)
{
	loadTestDBArmPlugin();
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	startFaceRest();

	RequestArg arg(path, callbackName);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_SERVER);
	DataQueryContextPtr dqCtxPtr(new DataQueryContext(arg.userId), false);

	// calculate the expected test triggers
	ServerIdTriggerIdIdxMap         indexMap;
	ServerIdTriggerIdIdxMapIterator indexMapIt;
	TriggerIdIdxMapIterator         trigIdIdxIt;
	getTestTriggersIndexes(indexMap, serverId, hostIdInServer);
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
	if (hostIdInServer != ALL_LOCAL_HOSTS)
		queryMap["hostId"] = hostIdInServer;
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
	loadTestDBArmPlugin();
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
	eventsArg.filterForDataOfDefunctSv = true;
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
	loadTestDBArmPlugin();
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
	eventsArg.filterForDataOfDefunctSv = true;
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
	eventsArg.filterForDataOfDefunctSv = true;
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

} // namespace testFaceRestHost
