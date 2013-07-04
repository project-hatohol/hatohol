#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "ConfigManager.h"
#include "DBAgentSQLite3.h"
#include "UnifiedDataStore.h"
#include "DBClientTest.h"

namespace testUnifiedDataStore {

void setup(void)
{
	const gchar *dbPath = cut_build_path(cut_get_test_directory(),
					     "fixtures",
					     "testDatabase-hatohol.db",
					     NULL);
 	DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_HATOHOL, dbPath);
}

void test_singleton(void) {
	UnifiedDataStore *dataStore1 = UnifiedDataStore::getInstance();
	UnifiedDataStore *dataStore2 = UnifiedDataStore::getInstance();
	cut_assert_not_null(dataStore1);
	cppcut_assert_equal(dataStore1, dataStore2);
}

static const char *triggerStatusToString(TriggerStatusType type)
{
	switch(type) {
	case TRIGGER_STATUS_OK:
		return "OK";
	case TRIGGER_STATUS_PROBLEM:
		return "PROBLEM";
	default:
		cut_fail("Unknown TriggerStatusType: %d\n",
			 static_cast<int>(type));
		return "";
	}
}

static const char *triggerSeverityToString(TriggerSeverityType type)
{
	switch(type) {
	case TRIGGER_SEVERITY_INFO:
		return "INFO";
	case TRIGGER_SEVERITY_WARN:
		return "WARN";
	case TRIGGER_SEVERITY_CRITICAL:
		return "CRITICAL";
	case TRIGGER_SEVERITY_UNKNOWN:
		return "UNKNOWN";
	default:
		cut_fail("Unknown TriggerSeverityType: %d\n",
			 static_cast<int>(type));
		return "";
	}

}

static string dumpTriggerInfo(const TriggerInfo &info)
{
	return StringUtils::sprintf(
		"%"PRIu32"|%"PRIu64"|%s|%s|%lu|%ld|%"PRIu64"|%s|%s\n",
		info.serverId,
		info.id,
		triggerStatusToString(info.status),
		triggerSeverityToString(info.severity),
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
	switch(type) {
	case EVENT_TYPE_ACTIVATED:
		return "ACTIVATED";
	case EVENT_TYPE_DEACTIVATED:
		return "DEACTIVATED";
	case EVENT_TYPE_UNKNOWN:
		return "UNKNOWN";
	default:
		cut_fail("Unknown EventType: %d\n",
			 static_cast<int>(type));
		return "";
	}
}

static string dumpEventInfo(const EventInfo &info)
{
	return StringUtils::sprintf(
		"%"PRIu32"|%"PRIu64"|%lu|%ld|%s|%s\%s|%"PRIu64"|%s|%s\n",
		info.serverId,
		info.id,
		info.time.tv_sec,
		info.time.tv_nsec,
		eventTypeToString(info.type),
		triggerStatusToString(info.status),
		triggerSeverityToString(info.severity),
		info.hostId,
		info.hostName.c_str(),
		info.brief.c_str());
}

void test_getEventList(void)
{
	string expected, actual;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		expected += dumpEventInfo(testEventInfo[i]);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList list;
	dataStore->getEventList(list);

	EventInfoListIterator it;
	for (it = list.begin(); it != list.end(); it++)
		actual += dumpEventInfo(*it);
	cppcut_assert_equal(expected, actual);
}

} // testUnifiedDataStore
