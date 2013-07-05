#include <cutter.h>
#include <cppcutter.h>
#include "DBClientTest.h"

MonitoringServerInfo serverInfo[] = 
{{
	1,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"pochi.dog.com",          // hostname
	"192.168.0.5",            // ip_address
	"POCHI",                  // nickname
	80,                       // port
	10,                       // polling_interval_sec
	5,                        // retry_interval_sec
	"foo",                    // user_name
	"goo",                    // password
	"dbX",                    // db_name
},{
	2,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"mike.dog.com",           // hostname
	"192.168.1.5",            // ip_address
	"MIKE",                   // nickname
	80,                       // port
	30,                       // polling_interval_sec
	15,                       // retry_interval_sec
	"Einstein",               // user_name
	"Albert",                 // password
	"gravity",                // db_name
},{
	3,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"hachi.dog.com",          // hostname
	"192.168.10.1",           // ip_address
	"8",                      // nickname
	8080,                     // port
	60,                       // polling_interval_sec
	60,                       // retry_interval_sec
	"Fermi",                  // user_name
	"fermion",                // password
	"",                       // db_name
}};
size_t NumServerInfo = sizeof(serverInfo) / sizeof(MonitoringServerInfo);

TriggerInfo testTriggerInfo[] = 
{{
	1,                        // serverId
	1,                        // id
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957197,0},           // lastChangeTime
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
},{
	3,                        // serverId
	2,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARN,    // severity
	{1362957200,0},           // lastChangeTime
	10001,                    // hostId,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
},{
	3,                        // serverId
	3,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362951000,0},           // lastChangeTime
	10002,                    // hostId,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
}};
size_t NumTestTriggerInfo = sizeof(testTriggerInfo) / sizeof(TriggerInfo);

EventInfo testEventInfo[] = {
{
	3,                        // serverId
	1,                        // id
	{1362957200,0},           // time
	EVENT_TYPE_ACTIVATED,        // type
	2,                        // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARN,    // severity
	10001,                    // hostId,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
}, {
	3,                        // serverId
	2,                        // id
	{1362951000,0},           // time
	EVENT_TYPE_ACTIVATED,        // type
	3,                        // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	10002,                    // hostId,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
}, {
	1,                        // serverId
	1,                        // id
	{1362951000,0},           // time
	EVENT_TYPE_ACTIVATED,        // type
	1,                        // triggerId
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
},
};
size_t NumTestEventInfo = sizeof(testEventInfo) / sizeof(EventInfo);

ItemInfo testItemInfo[] = {
{
	3,                        // serverId
	1,                        // id
	5,                        // hostId
	"The age of the cat.",    // brief
	{1362957200,0},           // lastValueTime
	"1",                      // lastValue
	"5",                      // prevValue
	"number",                 // itemGroupName,
}, {
	3,                        // serverId
	2,                        // id
	100,                      // hostId
	"All roads lead to Rome.",// brief
	{1362951000,0},           // lastValueTime
	"Osaka",                  // lastValue
	"Ichikawa",               // prevValue
	"City",                   // itemGroupName,
}, {
	4,                        // serverId
	1,                        // id
	100,                      // hostId
	"All roads lead to Rome.",// brief
	{1362951000,0},           // lastValueTime
	"Osaka",                  // lastValue
	"Ichikawa",               // prevValue
	"City",                   // itemGroupName,
},
};
size_t NumTestItemInfo = sizeof(testItemInfo) / sizeof(ItemInfo);

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo &trigInfo = testTriggerInfo[i];
		if (trigInfo.serverId != eventInfo.serverId)
			continue;
		if (trigInfo.id != eventInfo.triggerId)
			continue;
		return trigInfo;
	}
	cut_fail("Not found: server ID: %u, trigger ID: %"PRIu64,
	         eventInfo.serverId, eventInfo.triggerId);
	return *(new TriggerInfo()); // never exectuted, just to pass build
}

uint64_t findLastEventId(uint32_t serverId)
{
	bool found = false;
	uint64_t maxId = 0;
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		EventInfo &eventInfo = testEventInfo[i];
		if (eventInfo.serverId != serverId)
			continue;
		if (eventInfo.id >= maxId) {
			maxId = eventInfo.id;
			found = true;
		}
	}
	if (!found)
		return DBClientHatohol::EVENT_NOT_FOUND;
	return maxId;
}

static void addHostInfoToList(HostInfoList &hostInfoList,
                              const TriggerInfo &trigInfo)
{
	hostInfoList.push_back(HostInfo());
	HostInfo &hostInfo = hostInfoList.back();
	hostInfo.serverId = trigInfo.serverId;
	hostInfo.id       = trigInfo.hostId;
	hostInfo.hostName = trigInfo.hostName;
}

size_t getNumberOfTestTriggers(uint32_t serverId)
{
	if (serverId == ALL_SERVERS)
		return NumTestTriggerInfo;

	size_t count = 0;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		if (testTriggerInfo[i].serverId == serverId)
			count++;
	}
	return count;
}

size_t getNumberOfTestItems(uint32_t serverId)
{
	if (serverId == ALL_SERVERS)
		return NumTestItemInfo;

	size_t count = 0;
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		if (testItemInfo[i].serverId == serverId)
			count++;
	}
	return count;
}

void getTestHostInfoList(HostInfoList &hostInfoList,
                         uint32_t targetServerId,
                         ServerIdHostIdMap *serverIdHostIdMap)
{
	size_t numHostInfo0 =  hostInfoList.size();
	ServerIdHostIdMap *svIdHostIdMap;
	if (serverIdHostIdMap)
		svIdHostIdMap = serverIdHostIdMap; // use given data
	else
		svIdHostIdMap = new ServerIdHostIdMap();

	ServerIdHostIdMapIterator svIdIt;
	HostIdSetIterator         hostIdIt;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo &trigInfo = testTriggerInfo[i];
		if (targetServerId != ALL_SERVERS) {
			if (trigInfo.serverId != targetServerId)
				continue;
		}

		svIdIt = svIdHostIdMap->find(trigInfo.serverId);
		if (svIdIt == svIdHostIdMap->end()) {
			addHostInfoToList(hostInfoList, trigInfo);
			HostIdSet newHostIdSet;
			newHostIdSet.insert(trigInfo.hostId);
			(*svIdHostIdMap)[trigInfo.serverId] = newHostIdSet;
			continue;
		}

		HostIdSet &hostIdSet = svIdIt->second;
		hostIdIt = hostIdSet.find(trigInfo.hostId);
		if (hostIdIt == hostIdSet.end()) {
			addHostInfoToList(hostInfoList, trigInfo);
			hostIdSet.insert(trigInfo.hostId);
		}
	}

	// consistency check:
	// The number of hostInfoList elements we added and that of
	// svIdHostIdMap shall be the same.
	size_t numHostInfo = hostInfoList.size() - numHostInfo0;
	size_t numTotalHostInfoSet = 0;
	svIdIt = svIdHostIdMap->begin();
	for (; svIdIt != svIdHostIdMap->end(); ++svIdIt) {
		HostIdSet &hostIdSet = svIdIt->second;
		numTotalHostInfoSet += hostIdSet.size();
	}
	cppcut_assert_equal(numHostInfo, numTotalHostInfoSet);

	if (!serverIdHostIdMap)
		delete svIdHostIdMap;
}

int getNumberOfTestHosts(uint32_t serverId, uint64_t hostGroupId, 
                         TriggerSeverityType severity)
{
	// TODO: use hostGroupId after Hatohol support it.
	int count = 0;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo &trigInfo = testTriggerInfo[i];
		if (serverId != ALL_SERVERS && trigInfo.serverId != serverId)
			continue;
		if (trigInfo.severity != severity)
			continue;
		count++;
	}
	return count;
}
