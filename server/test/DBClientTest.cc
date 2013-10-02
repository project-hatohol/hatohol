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
	1,                        // serverId
	2,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957198,0},           // lastChangeTime
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1a",        // brief,
},{
	1,                        // serverId
	3,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957117,0},           // lastChangeTime
	235013,                   // hostId,
	"hostX2",                 // hostName,
	"TEST Trigger 1b",        // brief,
},{
	3,                        // serverId
	2,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
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
	EVENT_TYPE_GOOD,          // type
	2,                        // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
	10001,                    // hostId,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
}, {
	3,                        // serverId
	2,                        // id
	{1362951000,0},           // time
	EVENT_TYPE_GOOD,          // type
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
	EVENT_TYPE_GOOD,          // type
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

ActionDef testActionDef[] = {
{
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID |
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  1,                        // serverId
	  10,                       // hostId
	  5,                        // hostGroupId
	  3,                        // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_INVALID               // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"",                // working dir
	"/bin/hoge",       // command
	300,               // timeout
}, {
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  0,                        // serverId
	  0,                        // hostId
	  0,                        // hostGroupId
	  0x12345,                  // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_EQ_GT                 // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"/home/%%\"'@#!()+-~<>?:;",  // working dir
	"/usr/libexec/w",  // command
	30,                // timeout
}, {
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS,   // enableBits
	  100,                      // serverId
	  0x7fffffffffffffff,       // hostId
	  0x8000000000000000,       // hostGroupId
	  0xfedcba9876543210,       // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_WARNING, // triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_RESIDENT,   // type
	"/tmp",            // working dir
	"/usr/lib/liba.so",// command
	60,                // timeout
}, {
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS |
	   ACTCOND_TRIGGER_SEVERITY,   // enableBits
	  2,                        // serverId
	  0x89abcdefffffffff,       // hostId
	  0x8000000000000000,       // hostGroupId
	  0xfedcba9876543210,       // triggerId
	  TRIGGER_STATUS_OK,        // triggerStatus
	  TRIGGER_SEVERITY_WARNING, // triggerSeverity
	  CMP_EQ_GT                 // triggerSeverityCompType;
	), // condition
	ACTION_RESIDENT,   // type
	"/home/hatohol",   // working dir
	"/usr/lib/liba.so",// command
	0,                 // timeout
},
};

const size_t NumTestActionDef = sizeof(testActionDef) / sizeof(ActionDef);

UserInfo testUserInfo[] = {
{
	0,                 // id
	"cheesecake",      // name
	"CDEF~!@#$%^&*()", // password
	0,                 // flags
}, {
	0,                 // id
	"pineapple",       // name
	"Po+-\\|}{\":?><", // password
	USER_FLAG_ADMIN,   // flags
}, {
	0,                 // id
	"m1ffy@v@",        // name
	"",                // password
	0,                 // flags
}
};
const size_t NumTestUserInfo = sizeof(testUserInfo) / sizeof(UserInfo);

AccessInfo testAccessInfo[] = {
{
	0,                 // id
	1,                 // userId
	1,                 // serverId
	0,                 // hostGroupId
}, {
	0,                 // id
	1,                 // userId
	1,                 // serverId
	1,                 // hostGroupId
}, {
	0,                 // id
	2,                 // userId
	ALL_SERVERS,       // serverId
	ALL_HOST_GROUPS,   // hostGroupId
}, {
	0,                 // id
	3,                 // userId
	1,                 // serverId
	ALL_HOST_GROUPS,   // hostGroupId
}, {
	0,                 // id
	3,                 // userId
	2,                 // serverId
	1,                 // hostGroupId
}, {
	0,                 // id
	3,                 // userId
	2,                 // serverId
	2,                 // hostGroupId
}, {
	0,                 // id
	3,                 // userId
	4,                 // serverId
	1,                 // hostGroupId
}
};
const size_t NumTestAccessInfo = sizeof(testAccessInfo) / sizeof(AccessInfo);

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

void getTestTriggersIndexes(
  map<uint32_t, map<uint64_t, size_t> > &indexMap,
  uint32_t serverId, uint64_t hostId)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (serverId != ALL_SERVERS) {
			if (trigInfo.serverId != serverId)
				continue;
		}
		if (hostId != ALL_HOSTS) {
			if (trigInfo.hostId != hostId)
				continue;
		}
		// If the following assertion fails, the test data is illegal.
		cppcut_assert_equal(
		  (size_t)0, indexMap[trigInfo.serverId].count(trigInfo.id));
		indexMap[trigInfo.serverId][trigInfo.id] = i;
	}
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

size_t getNumberOfTestTriggers(uint32_t serverId, uint64_t hostGroupId, 
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
		if (trigInfo.status == TRIGGER_STATUS_OK)
			continue;
		count++;
	}
	return count;
}

static bool isGoodStatus(const TriggerInfo &triggerInfo)
{
	return triggerInfo.status == TRIGGER_STATUS_OK;
}

static void removeHostIdIfNeeded(ServerIdHostGroupHostIdMap &svIdHostGrpIdMap,
                                 uint64_t hostGrpIdForTrig,
                                 const TriggerInfo &trigInfo)
{
	ServerIdHostGroupHostIdMapIterator svIt;
	HostGroupHostIdMapIterator         hostIt;
	HostIdSetIterator                  idIt;
	svIt = svIdHostGrpIdMap.find(trigInfo.serverId);
	if (svIt == svIdHostGrpIdMap.end())
		return;
	HostGroupHostIdMap &hostGrpIdMap = svIt->second;
	hostIt = hostGrpIdMap.find(hostGrpIdForTrig);
	if (hostIt == hostGrpIdMap.end())
		return;
	HostIdSet &hostIdSet = hostIt->second;
	hostIdSet.erase(trigInfo.hostId);
}

size_t getNumberOfTestHostsWithStatus(uint32_t serverId, uint64_t hostGroupId,
                                      bool status)
{
	ServerIdHostGroupHostIdMap svIdHostGrpIdMap;
	ServerIdHostGroupHostIdMapIterator svIt;
	HostGroupHostIdMapIterator         hostIt;

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		TriggerInfo &trigInfo = testTriggerInfo[i];
		// TODO: use the correct hostGroupId after Hatohol support it.
		uint64_t hostGrpIdForTrig = hostGroupId;

		if (serverId != ALL_SERVERS && trigInfo.serverId != serverId)
			continue;
		if (isGoodStatus(trigInfo) != status) {
			if (status) {
				// We have to remove inserted host ID.
				// When a host has at least one bad triggeer,
				// the host is not good.
				removeHostIdIfNeeded(svIdHostGrpIdMap,
				                     hostGrpIdForTrig,
				                     trigInfo);
			}
			continue;
		}

		svIt = svIdHostGrpIdMap.find(trigInfo.serverId);
		if (svIt == svIdHostGrpIdMap.end()) {
			// svIdHostGrpMap doesn't have value pair
			// for this server
			HostIdSet hostIdSet;
			hostIdSet.insert(trigInfo.hostId);
			HostGroupHostIdMap hostMap;
			hostMap[hostGrpIdForTrig] = hostIdSet;
			svIdHostGrpIdMap[trigInfo.serverId] = hostMap;
			continue;
		}

		HostGroupHostIdMap &hostGrpIdMap = svIt->second;
		hostIt = hostGrpIdMap.find(hostGrpIdForTrig);
		if (hostIt == hostGrpIdMap.end()) {
			// svIdHostGrpMap doesn't have value pair
			// for this host group
			HostIdSet hostIdSet;
			hostIdSet.insert(trigInfo.hostId);
			hostGrpIdMap[hostGrpIdForTrig] = hostIdSet;
			continue;
		}

		HostIdSet &hostIdSet = hostIt->second;
		// Even when hostIdSet already has an element with
		// trigInfo.hostId, insert() just fails and doesn't
		// cause other side effects.
		// This behavior is no problem for this function and we
		// can skip the check of the existence in the set.
		hostIdSet.insert(trigInfo.hostId);
	}

	// get the number of hosts that matches with the requested condition
	svIt = svIdHostGrpIdMap.find(serverId);
	if (svIt == svIdHostGrpIdMap.end())
		return 0;
	HostGroupHostIdMap &hostGrpIdMap = svIt->second;
	hostIt = hostGrpIdMap.find(hostGroupId);
	if (hostIt == hostGrpIdMap.end())
		return 0;
	HostIdSet &hostIdSet = hostIt->second;
	return hostIdSet.size();
}

void getDBCTestHostInfo(HostInfoList &hostInfoList, uint32_t targetServerId)
{
	map<uint32_t, set<uint64_t> > svIdHostIdsMap;
	map<uint32_t, set<uint64_t> >::iterator it;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		const uint32_t svId = trigInfo.serverId;
		const uint64_t hostId = trigInfo.hostId;
		if (targetServerId != ALL_SERVERS && svId != targetServerId)
			continue;
		if (svIdHostIdsMap[svId].count(hostId))
			continue;

		HostInfo hostInfo;
		hostInfo.serverId = svId;
		hostInfo.id       = hostId;
		hostInfo.hostName = trigInfo.hostName;
		hostInfoList.push_back(hostInfo);
		svIdHostIdsMap[svId].insert(hostId);
	}
}
