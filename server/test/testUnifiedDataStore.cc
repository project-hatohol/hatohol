#include <cppcutter.h>
#include <cutter.h>
#include <unistd.h>

#include "DBAgentSQLite3.h"
#include "UnifiedDataStore.h"
#include <glib.h>

namespace testUnifiedDataStore {

static string g_dbPath = DBAgentSQLite3::getDBPath(DEFAULT_DB_DOMAIN_ID);

static void deleteDB(void)
{
	unlink(g_dbPath.c_str());
	cut_assert_not_exist_path(g_dbPath.c_str());
}

void setup(void)
{
	deleteDB();
	DBAgentSQLite3::defineDBPath(DEFAULT_DB_DOMAIN_ID, g_dbPath);
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

static string dumpTriggerInfo(TriggerInfo &info)
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
	const string expected =
		"1|1|OK|INFO|1362957197|0|235012|hostX1|TEST Trigger 1\n"
		"3|2|PROBLEM|WARN|1362957200|0|10001|hostZ1|TEST Trigger 2\n"
		"3|3|PROBLEM|INFO|1362951000|0|10002|hostZ2|TEST Trigger 3\n";
	string actual;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	TriggerInfoList list;
	dataStore->getTriggerList(list);
	TriggerInfoListIterator it;
	for (it = list.begin(); it != list.end(); it++)
		actual += dumpTriggerInfo(*it);
	cppcut_assert_equal(expected, actual);
}

} // testUnifiedDataStore
