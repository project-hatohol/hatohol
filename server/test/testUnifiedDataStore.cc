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
#include <cutter.h>
#include <unistd.h>

#include "Hatohol.h"
#include "Params.h"
#include "DBAgentSQLite3.h"
#include "UnifiedDataStore.h"
#include "DBClientTest.h"
#include "LabelUtils.h"
#include "Helpers.h"

namespace testUnifiedDataStore {

void test_singleton(void) {
	UnifiedDataStore *dataStore1 = UnifiedDataStore::getInstance();
	UnifiedDataStore *dataStore2 = UnifiedDataStore::getInstance();
	cut_assert_not_null(dataStore1);
	cppcut_assert_equal(dataStore1, dataStore2);
}

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
		"%"PRIu32"|%"PRIu64"|%s|%s|%lu|%ld|%"PRIu64"|%s|%s\n",
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

void test_getTriggerList(void)
{
	string expected, actual;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		expected += dumpTriggerInfo(testTriggerInfo[i]);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList list;
	dataStore->getTriggerList(list);

	TriggerInfoListIterator it;
	for (it = list.begin(); it != list.end(); it++)
		actual += dumpTriggerInfo(*it);
	cppcut_assert_equal(expected, actual);
}

static const char *eventTypeToString(EventType type)
{
	return LabelUtils::getEventTypeLabel(type).c_str();
}

static string dumpEventInfo(const EventInfo &info)
{
	return StringUtils::sprintf(
		"%"PRIu32"|%"PRIu64"|%lu|%ld|%s|%s|%s|%"PRIu64"|%s|%s\n",
		info.serverId,
		info.id,
		info.time.tv_sec,
		info.time.tv_nsec,
		eventTypeToString(info.type),
		triggerStatusToString(info.status).c_str(),
		triggerSeverityToString(info.severity).c_str(),
		info.hostId,
		info.hostName.c_str(),
		info.brief.c_str());
}

static string dumpItemInfo(const ItemInfo &info)
{
	return StringUtils::sprintf(
		"%"PRIu32"|%"PRIu64"|%"PRIu64"|%s|%lu|%ld|%s|%s|%s\n",
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

void cut_setup(void)
{
	hatoholInit();
	const gchar *dbPath = cut_build_path(cut_get_test_directory(),
	                                     "fixtures",
	                                     "testDatabase-hatohol.db",
	                                     NULL);
 	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPath);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getEventList(void)
{
	string expected, actual;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		expected += dumpEventInfo(testEventInfo[i]);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList list;
	EventQueryOption option;
	option.setUserId(USER_ID_ADMIN);
	dataStore->getEventList(list, option);

	EventInfoListIterator it;
	for (it = list.begin(); it != list.end(); it++)
		actual += dumpEventInfo(*it);
	cppcut_assert_equal(expected, actual);
}

void test_getItemList(void)
{
	string expected, actual;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		expected += dumpItemInfo(testItemInfo[i]);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList list;
	dataStore->getItemList(list);

	ItemInfoListIterator it;
	for (it = list.begin(); it != list.end(); it++)
		actual += dumpItemInfo(*it);
	cppcut_assert_equal(expected, actual);
}

} // testUnifiedDataStore
