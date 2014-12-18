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
		"%" PRIu32 "|%" PRIu64 "|%s|%s|%lu|%ld|%" PRIu64 "|%s|%s\n",
		info.serverId,
		info.id,
		triggerStatusToString(info.status).c_str(),
		triggerSeverityToString(info.severity).c_str(),
		info.lastChangeTime.tv_sec,
		info.lastChangeTime.tv_nsec,
		info.hostId,
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
		"%" PRIu32 "|%" PRIu64 "|%lu|%ld|%s|%s|%s|%" PRIu64 "|%s|%s\n",
		info.serverId,
		info.id,
		info.time.tv_sec,
		info.time.tv_nsec,
		eventTypeToString(info.type).c_str(),
		triggerStatusToString(info.status).c_str(),
		triggerSeverityToString(info.severity).c_str(),
		info.hostId,
		info.hostName.c_str(),
		info.brief.c_str());
}

static string dumpItemInfo(const ItemInfo &info)
{
	return StringUtils::sprintf(
		"%" PRIu32 "|%" PRIu64 "|%" PRIu64 "|%s|%lu|%ld|%s|%s|%s\n",
		info.serverId,
		info.id,
		info.hostId,
		info.brief.c_str(),
		info.lastValueTime.tv_sec,
		info.lastValueTime.tv_nsec,
		info.lastValue.c_str(),
		info.prevValue.c_str(),
		info.itemGroupName.c_str());
}

template<class T>
static void collectValidResourceInfoString(
  vector<string> &outStringVect,
  const size_t &numTestResourceData, const T *testResourceData,
  const bool &filterForDataOfDefunctSv, const DataQueryOption &option,
  string (*dumpResourceFunc)(const T &)) 
{
	for (size_t i = 0; i < numTestResourceData; i++) {
		const T &testResource = testResourceData[i];
		if (filterForDataOfDefunctSv) {
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
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerList(gconstpointer data)
{
	loadTestDBTriggers();

	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	vector<string> expectedStrVec;
	TriggersQueryOption option(USER_ID_SYSTEM);
	collectValidResourceInfoString<TriggerInfo>(
	  expectedStrVec, NumTestTriggerInfo, testTriggerInfo,
	  filterForDataOfDefunctSv, option, dumpTriggerInfo);
	
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList list;
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	dataStore->getTriggerList(list, option);

	assertLines<TriggerInfo>(expectedStrVec, list, dumpTriggerInfo);
}

void data_getEventList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventList(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBEvents();

	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	vector<string> expectedStrVec;
	EventsQueryOption option(USER_ID_SYSTEM);
	collectValidResourceInfoString<EventInfo>(
	  expectedStrVec, NumTestEventInfo, testEventInfo,
	  filterForDataOfDefunctSv, option, dumpEventInfo);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList list;
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	dataStore->getEventList(list, option);

	assertLines<EventInfo>(expectedStrVec, list, dumpEventInfo);
}

void data_getItemList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemList(gconstpointer data)
{
	loadTestDBItems();

	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	ItemsQueryOption option(USER_ID_SYSTEM);
	vector<string> expectedStrVec;
	collectValidResourceInfoString<ItemInfo>(
	  expectedStrVec, NumTestItemInfo, testItemInfo,
	  filterForDataOfDefunctSv, option, dumpItemInfo);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList list;
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
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
	cppcut_assert_equal(false, uds->getDataStore(svInfo.id).hasData());

	// Create a DataStore instance.
	ArmPluginInfo *armPluginInfo = NULL;
	assertHatoholError(
	  HTERR_OK, uds->addTargetServer(svInfo, *armPluginInfo,
	                                 USER_ID_SYSTEM, false));

	DataStorePtr dataStore0 = uds->getDataStore(svInfo.id);
	cppcut_assert_equal(true, dataStore0.hasData());

	// The obtained DataStore instance should be the same as the previous.
	DataStorePtr dataStore1 = uds->getDataStore(svInfo.id);
	cppcut_assert_equal((DataStore *)dataStore0,
	                    (DataStore *)dataStore1);
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
		const ServerIdType expectId = i + 1;
		expectIdSet.insert(expectId);
	}
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		const ServerIdType svId = svConnStatVect[i].serverId;
		ServerIdSetIterator it = expectIdSet.find(svId);
		cppcut_assert_equal(true, it != expectIdSet.end());
		expectIdSet.erase(it);
	}
}

} // testUnifiedDataStore
