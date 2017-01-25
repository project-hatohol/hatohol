/*
 * Copyright (C) 2013 Project Hatohol
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
#include <cutter.h>
#include <gcutter.h>
#include <unistd.h>
#include "Hatohol.h"
#include "Params.h"
#include "UnifiedDataStore.h"
#include "Reaper.h"
#include "DBTablesTest.h"
#include "LabelUtils.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testUnifiedDataStore {

static const string triggerStatusToString(TriggerStatusType type)
{
	return LabelUtils::getTriggerStatusLabel(type);
}

static const string triggerSeverityToString(TriggerSeverityType type)
{
	return LabelUtils::getTriggerSeverityLabel(type);
}

static string dumpTriggerInfo(const TriggerInfo &info)
{
	return StringUtils::sprintf(
		"%" FMT_SERVER_ID "|%" FMT_TRIGGER_ID "|%s|%s"
		"|%lu|%ld|%" FMT_LOCAL_HOST_ID
		"|%s|%s\n",
		info.serverId,
		info.id.c_str(),
		triggerStatusToString(info.status).c_str(),
		triggerSeverityToString(info.severity).c_str(),
		info.lastChangeTime.tv_sec,
		info.lastChangeTime.tv_nsec,
		info.hostIdInServer.c_str(),
		info.hostName.c_str(),
		info.brief.c_str());
}

static string eventTypeToString(EventType type)
{
	return LabelUtils::getEventTypeLabel(type);
}

static string dumpEventInfo(const EventInfo &info)
{
	return StringUtils::sprintf(
		"%" FMT_SERVER_ID "|%" FMT_EVENT_ID
		"|%lu|%ld|%s|%s|%s|%" FMT_LOCAL_HOST_ID
		"|%s|%s\n",
		info.serverId,
		info.id.c_str(),
		info.time.tv_sec,
		info.time.tv_nsec,
		eventTypeToString(info.type).c_str(),
		triggerStatusToString(info.status).c_str(),
		triggerSeverityToString(info.severity).c_str(),
		info.hostIdInServer.c_str(),
		info.hostName.c_str(),
		info.brief.c_str());
}

static string dumpItemInfo(const ItemInfo &info)
{
	return StringUtils::sprintf(
		"%" FMT_SERVER_ID "|%" FMT_ITEM_ID "|%" FMT_HOST_ID
		"|%" FMT_LOCAL_HOST_ID "|%s|%lu|%ld|%s|%s\n",
		info.serverId,
		info.id.c_str(),
		info.globalHostId,
		info.hostIdInServer.c_str(),
		info.brief.c_str(),
		info.lastValueTime.tv_sec,
		info.lastValueTime.tv_nsec,
		info.lastValue.c_str(),
		info.prevValue.c_str());
}

template<class T>
static void collectValidResourceInfoString(
  vector<string> &outStringVect,
  const size_t &numTestResourceData, const T *testResourceData,
  const bool &excludeDefunctServers, const DataQueryOption &option,
  string (*dumpResourceFunc)(const T &)) 
{
	for (size_t i = 0; i < numTestResourceData; i++) {
		const T &testResource = testResourceData[i];
		if (excludeDefunctServers) {
			if (!option.isValidServer(testResource.serverId))
				continue;
		}
		outStringVect.push_back((*dumpResourceFunc)(testResource));
	}
}

template<class T>
static void assertLines(
  const vector<string> &expectedStrVec, const list<T> &actualList,
  string (*dumpResourceFunc)(const T &)) 
{
	cppcut_assert_equal(expectedStrVec.size(), actualList.size());
	LinesComparator linesComparator;
	vector<string>::const_iterator expectedStrItr = expectedStrVec.begin();
	typename list<T>::const_iterator actualItr = actualList.begin();
	for (; actualItr != actualList.end(); ++actualItr, ++expectedStrItr) {
		linesComparator.add(*expectedStrItr,
		                    (*dumpResourceFunc)(*actualItr));
	}
	const bool strictOrder = false;
	linesComparator.assert(strictOrder);
}

void cut_setup(void)
{
	hatoholInit();

	setupTestDB();
	loadTestDBTablesConfig();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_singleton(void) {
	UnifiedDataStore *dataStore1 = UnifiedDataStore::getInstance();
	UnifiedDataStore *dataStore2 = UnifiedDataStore::getInstance();
	cut_assert_not_null(dataStore1);
	cppcut_assert_equal(dataStore1, dataStore2);
}

void data_getTriggerList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerList(gconstpointer data)
{
	loadTestDBTriggers();

	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	vector<string> expectedStrVec;
	TriggersQueryOption option(USER_ID_SYSTEM);
	collectValidResourceInfoString<TriggerInfo>(
	  expectedStrVec, NumTestTriggerInfo, testTriggerInfo,
	  excludeDefunctServers, option, dumpTriggerInfo);
	
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList list;
	option.setExcludeDefunctServers(excludeDefunctServers);
	dataStore->getTriggerList(list, option);

	assertLines<TriggerInfo>(expectedStrVec, list, dumpTriggerInfo);
}

void data_getEventList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventList(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBEvents();

	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	vector<string> expectedStrVec;
	EventsQueryOption option(USER_ID_SYSTEM);
	collectValidResourceInfoString<EventInfo>(
	  expectedStrVec, NumTestEventInfo, testEventInfo,
	  excludeDefunctServers, option, dumpEventInfo);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList list;
	option.setExcludeDefunctServers(excludeDefunctServers);
	dataStore->getEventList(list, option);

	assertLines<EventInfo>(expectedStrVec, list, dumpEventInfo);
}

void data_getItemList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getItemList(gconstpointer data)
{
	loadTestDBItems();

	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	ItemsQueryOption option(USER_ID_SYSTEM);
	vector<string> expectedStrVec;
	collectValidResourceInfoString<ItemInfo>(
	  expectedStrVec, NumTestItemInfo, testItemInfo,
	  excludeDefunctServers, option, dumpItemInfo);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList list;
	option.setExcludeDefunctServers(excludeDefunctServers);
	dataStore->getItemList(list, option);

	assertLines<ItemInfo>(expectedStrVec, list, dumpItemInfo);
}

void test_serverIdDataStoreMap(void)
{
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	MonitoringServerInfo svInfo;
	MonitoringServerInfo::initialize(svInfo);
	svInfo.type = MONITORING_SYSTEM_FAKE;
	svInfo.id   = 100;

	// Before a DataStore is created.
	cppcut_assert_equal(false, uds->getDataStore(svInfo.id) != nullptr);

	// Create a DataStore instance.
	ArmPluginInfo *armPluginInfo = NULL;
	assertHatoholError(
	  HTERR_OK, uds->addTargetServer(svInfo, *armPluginInfo,
	                                 USER_ID_SYSTEM, false));

	shared_ptr<DataStore> dataStore0 = uds->getDataStore(svInfo.id);
	cppcut_assert_equal(true, dataStore0.get() != nullptr);

	// The obtained DataStore instance should be the same as the previous.
	shared_ptr<DataStore> dataStore1 = uds->getDataStore(svInfo.id);
	cppcut_assert_equal(dataStore0.get(), dataStore1.get());
}

void test_getServerConnStatusVector(void)
{
	// setup
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	uds->start(false);

	MonitoringServerInfo svInfo;
	MonitoringServerInfo::initialize(svInfo);
	svInfo.type = MONITORING_SYSTEM_FAKE;

	// Call the target method
	ServerConnStatusVector svConnStatVect;
	DataQueryContextPtr dqCtxP(new DataQueryContext(USER_ID_SYSTEM), false);
	uds->getServerConnStatusVector(svConnStatVect, dqCtxP);

	// Check
	cppcut_assert_equal(NumTestServerInfo, svConnStatVect.size());
	ServerIdSet expectIdSet;
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		const ServerIdType expectId = testServerInfo[i].id;
		expectIdSet.insert(expectId);
	}
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		const ServerIdType svId = svConnStatVect[i].serverId;
		ServerIdSetIterator it = expectIdSet.find(svId);
		cppcut_assert_equal(true, it != expectIdSet.end());
		expectIdSet.erase(it);
	}
}

void data_upsertHost(void)
{
	gcut_add_datum("Getting returned host ID",
	               "getHostId", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("Ignore returned host ID",
	               "getHostId", G_TYPE_BOOLEAN, FALSE, NULL);
}

void test_upsertHost(gconstpointer data)
{
	const bool getHostId = gcut_data_get_boolean(data, "getHostId");
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = AUTO_ASSIGNED_ID;
	svHostDef.serverId = 5;
	svHostDef.hostIdInServer = "AiDee";
	svHostDef.name = "GOHOUMEI";
	svHostDef.status = HOST_STAT_NORMAL;

	HostIdType returnedId = INVALID_HOST_ID;
	HatoholError err =
	  uds->upsertHost(svHostDef, getHostId ? & returnedId : NULL);
	assertHatoholError(HTERR_OK, err);
	if (getHostId) {
		cppcut_assert_not_equal(INVALID_HOST_ID, returnedId);
		cppcut_assert_equal(true, returnedId > 0,
		                    cut_message("%" FMT_HOST_ID, returnedId));
	} else {
		cppcut_assert_equal(INVALID_HOST_ID, returnedId);
	}
}

} // testUnifiedDataStore
