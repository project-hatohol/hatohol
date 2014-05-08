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
using namespace std;
using namespace mlpl;

static const ServerIdType defunctServerId1 = 0x7fff0001;
const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST =
  static_cast<MonitoringSystemType>(NUM_MONITORING_SYSTEMS + 100);

MonitoringServerInfo testServerInfo[] = 
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
size_t NumTestServerInfo = sizeof(testServerInfo) / sizeof(MonitoringServerInfo);

MonitoringServerStatus testServerStatus[] =
{{
	1,                        // id
	1.1,                      // nvps
},{
	2,                        // id
	1.2,                      // nvps
},{
	3,                        // id
	1.3,                      // nvps
}};
size_t NumTestServerStatus = sizeof(testServerStatus) / sizeof(MonitoringServerStatus);

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
	1,                        // serverId
	4,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957197,0},           // lastChangeTime
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1c",        // brief,
},{
	1,                        // serverId
	5,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957198,0},           // lastChangeTime
	1129,                     // hostId,
	"hostX3",                 // hostName,
	"TEST Trigger 1d",        // brief,
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
},{
	2,                        // serverId
	0xfedcba987654321,        // id
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_WARNING, // severity
	{1362951000,0},           // lastChangeTime
	0x89abcdeffffffff,       // hostId,
	"hostQ1",                 // hostName,
	"TEST Trigger Action",    // brief,
},{
	// This entry is for tests with a defunct server
	defunctServerId1,         // serverId
	3,                        // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957117,0},           // lastChangeTime
	10002,                    // hostId,
	"defunctSv1Host1",        // hostName,
	"defunctSv1Host1 material", // brief,
},
};
size_t NumTestTriggerInfo = sizeof(testTriggerInfo) / sizeof(TriggerInfo);

static const TriggerInfo &trigInfoDefunctSv1 =
  testTriggerInfo[NumTestTriggerInfo-1];

EventInfo testEventInfo[] = {
{
	AUTO_INCREMENT_VALUE,     // unifiedId
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
	AUTO_INCREMENT_VALUE,     // unifiedId
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
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	1,                        // id
	{1362957198,0},           // time
	EVENT_TYPE_GOOD,          // type
	2,                        // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1a",        // brief,
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	2,                        // id
	{1362951000,0},           // time
	EVENT_TYPE_GOOD,          // type
	1,                        // triggerId
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	235012,                   // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	3,                        // id
	{1362957117,0},           // time
	EVENT_TYPE_GOOD,          // type
	3,                        // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	235013,                   // hostId,
	"hostX2",                 // hostName,
	"TEST Trigger 1b",        // brief,
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,     // unifiedId
	trigInfoDefunctSv1.serverId, // serverId
	1,                        // id
	trigInfoDefunctSv1.lastChangeTime, // time
	EVENT_TYPE_BAD,           // type
	3,                        // triggerId
	trigInfoDefunctSv1.status,   // status
	trigInfoDefunctSv1.severity, // severity
	trigInfoDefunctSv1.hostId,   // hostId,
	trigInfoDefunctSv1.hostName, // hostName,
	trigInfoDefunctSv1.brief,    // brief,
},
};
size_t NumTestEventInfo = sizeof(testEventInfo) / sizeof(EventInfo);

ItemInfo testItemInfo[] = {
{
	1,                        // serverId
	2,                        // id
	1129,                     // hostId
	"Rome wasn't built in a day",// brief
	{1362951129,0},           // lastValueTime
	"Fukuoka",                // lastValue
	"Sapporo",                // prevValue
	"City",                   // itemGroupName,
}, {
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
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID |
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  1,                        // serverId
	  10,                       // hostId
	  5,                        // hostgroupId
	  3,                        // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_INVALID               // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"",                // working dir
	"/bin/hoge",       // command
	300,               // timeout
	1,                 // ownerUserId
}, {
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  0,                        // serverId
	  0,                        // hostId
	  0,                        // hostgroupId
	  0x12345,                  // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_EQ_GT                 // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"/home/%%\"'@#!()+-~<>?:;",  // working dir
	"/usr/libexec/w",  // command
	30,                // timeout
	1,                 // ownerUserId
}, {
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS,   // enableBits
	  100,                      // serverId
	  0x7fffffffffffffff,       // hostId
	  0x8000000000000000,       // hostgroupId
	  0xfedcba9876543210,       // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_WARNING, // triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_RESIDENT,   // type
	"/tmp",            // working dir
	"/usr/lib/liba.so",// command
	60,                // timeout
	2,                 // ownerUserId
}, {
	0,                 // id (this field is ignored)
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
	4,                 // ownerUserId
}, {
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS,   // enableBits
	  101,                      // serverId
	  0x7fffffffffffffff,       // hostId
	  0x8000000000000000,       // hostgroupId
	  0xfedcba9876543210,       // triggerId
	  TRIGGER_STATUS_OK,        // triggerStatus
	  TRIGGER_SEVERITY_CRITICAL,// triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"/home/foo",       // working dir
	"a-dog-meets-food",// command
	3939,              // timeout
	2,                 // ownerUserId
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
	ALL_PRIVILEGES,    // flags
}, {
	0,                 // id
	"m1ffy@v@",        // name
	"S/N R@t10",       // password
	OperationPrivilege::makeFlag(OPPRVLG_DELETE_ACTION), // flags
}, {
	0,                 // id
	"higgs",           // name
	"gg -> h",        // password
	OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_USER), // flags
}, {
	0,                 // id
	"tiramisu",        // name
	"qJN9DBkJRQSQo",   // password
	0,                 // flags
}, {
	0,                    // id
	"cannotUpdateServer", // name
	"qJN9DBkJRQSQo",      // password
	OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER),
}, {
	0,                          // id
	"multipleAuthorizedGroups", // name
	"5XUkuWUlqQs1s",            // password
	0,
}
};
const size_t NumTestUserInfo = sizeof(testUserInfo) / sizeof(UserInfo);
const UserIdType userIdWithMultipleAuthorizedHostgroups = 7;

AccessInfo testAccessInfo[] = {
{
	0,                 // id
	1,                 // userId
	1,                 // serverId
	0,                 // hostgroupId
}, {
	0,                 // id
	1,                 // userId
	1,                 // serverId
	1,                 // hostgroupId
}, {
	0,                 // id
	2,                 // userId
	ALL_SERVERS,       // serverId
	ALL_HOST_GROUPS,   // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	1,                 // serverId
	ALL_HOST_GROUPS,   // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	2,                 // serverId
	1,                 // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	2,                 // serverId
	2,                 // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	4,                 // serverId
	1,                 // hostgroupId
}, {
	0,                 // id
	5,                 // userId
	1,                 // serverId
	ALL_HOST_GROUPS,   // hostgroupId
}, {
	0,                 // id
	7,                 // userId
	1,                 // serverId
	1,                 // hostgroupId
}, {
	0,                 // id
	7,                 // userId
	1,                 // serverId
	2,                 // hostgroupId
}
};
const size_t NumTestAccessInfo = sizeof(testAccessInfo) / sizeof(AccessInfo);

HostgroupInfo testHostgroupInfo[] = {
{
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	1,                     // groupId
	"Monitor Servers"      // groupName
}, {
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	2,                     // groupId
	"Monitored Servers"    // groupName
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	1,                     // groupId
	"Checking Servers"     // groupName
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	2,                     // groupId
	"Checked Servers"      // groupName
}, {
	AUTO_INCREMENT_VALUE,  // id
	4,                     // serverId
	1,                     // groupId
	"Watching Servers"     // groupName
}, {
	AUTO_INCREMENT_VALUE,  // id
	4,                     // serverId
	2,                     // groupId
	"Watched Servers"      // groupName
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,  // id
	trigInfoDefunctSv1.serverId, // serverId
	1,                     // groupId
	"Hostgroup on a defunct servers" // groupName
}
};
const size_t NumTestHostgroupInfo = sizeof(testHostgroupInfo) / sizeof(HostgroupInfo);

HostInfo testHostInfo[] = {
{
	1,                     // serverId
	235012,                // id(hostId)
	"hostX1"               // hostName
}, {
	1,                     // serverId
	235013,                // id(hostId)
	"hostX2"               // hostName
}, {
	1,                     // serverId
	1129,                  // id(hostId)
	"hostX3"               // hostName
} ,{
	3,                     // serverId
	10001,                 // id(hostId)
	"hostZ1"               // hostName
} ,{
	2,                     // serverId
	512,                   // id(hostId)
	"multi-host group",    // hostName
}, {
	3,                     // serverId
	10002,                 // id(hostId)
	"hostZ2"               // hostName
}, {
	3,                     // serverId
	5,                     // id(hostId)
	"frog"                 // hostName
} ,{
	3,                     // serverId
	100,                   // id(hostId)
	"dolphin"              // hostName
}, {
	4,                     // serverId
	100,                   // id(hostId)
	"squirrel"             // hostName
}, {
	2,                     // serverId
	0x89abcdeffffffff,     // id(hostId)
	"hostQ1"               // hostName
}, {
	// This entry is for tests with a defunct server
	trigInfoDefunctSv1.serverId, // serverId
	trigInfoDefunctSv1.hostId,   // hostId,
	trigInfoDefunctSv1.hostName, // hostName,
}
};
const size_t NumTestHostInfo = sizeof(testHostInfo) / sizeof(HostInfo);

HostgroupElement testHostgroupElement[] = {
{
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	235012,                // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	235012,                // hostId
	2,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	235013,                // hostId
	2,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	1129,                  // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	2,                     // serverId
	512,                   // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	2,                     // serverId
	512,                   // hostId
	2,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	10001,                 // hostId
	2,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	10002,                 // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	5,                     // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	100,                   // hostId
	2,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	4,                     // serverId
	100,                   // hostId
	1,                     // groupId
}, {
	AUTO_INCREMENT_VALUE,  // id
	2,                     // serverId
	0x89abcdefffffffff,    // hostId
	0x8000000000000000,    // hostGroupId
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,        // id
	trigInfoDefunctSv1.serverId, // serverId
	trigInfoDefunctSv1.hostId,   // hostId,
	1,                           // groupId
}
};
const size_t NumTestHostgroupElement = sizeof(testHostgroupElement) / sizeof(HostgroupElement);

UserRoleInfo testUserRoleInfo[] = {
{
	0,                            // id
	"Specific Server maintainer", // name
	(1 << OPPRVLG_UPDATE_SERVER)  // flags
}, {
	0,                     // id
	"We're Action master", // name
	// flags
	(1 << OPPRVLG_CREATE_ACTION)     |
	(1 << OPPRVLG_UPDATE_ACTION)     |
	(1 << OPPRVLG_UPDATE_ALL_ACTION) |
	(1 << OPPRVLG_DELETE_ACTION)     |
	(1 << OPPRVLG_DELETE_ALL_ACTION) |
	(1 << OPPRVLG_GET_ALL_ACTION)
}, {
	0,                 // id
	"Sweeper",         // name
	// flags
	(1 << OPPRVLG_DELETE_USER)          |
	(1 << OPPRVLG_DELETE_ALL_SERVER)    |
	(1 << OPPRVLG_DELETE_ALL_ACTION)    |
	(1 << OPPRVLG_DELETE_ALL_USER_ROLE)
}
};
const size_t NumTestUserRoleInfo = sizeof(testUserRoleInfo) / sizeof(UserRoleInfo);

ArmPluginInfo testArmPluginInfo[] = {
{
	MONITORING_SYSTEM_HAPI_ZABBIX,   // type
	"Zabbix API",                    // name
	"/usr/local/lib/hatohol/hapi/hapi-zabbix-api",       // path
}, {
	MONITORING_SYSTEM_HAPI_NAGIOS,   // type
	"Nagios NDO",                    // name
	"/usr/local/lib/hatohol/hapi/hapi-nagios-ndoutils",  // path
}, {
	MONITORING_SYSTEM_HAPI_TEST,     // type
	"Test monitoring system",        // name
	"hapi-test-plugin",              // path
}
};
const size_t NumTestArmPluginInfo = sizeof(testArmPluginInfo) / sizeof(ArmPluginInfo);

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

uint64_t findLastEventId(const ServerIdType &serverId)
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

void getTestTriggersIndexes(
  map<ServerIdType, map<uint64_t, size_t> > &indexMap,
  const ServerIdType &serverId, uint64_t hostId)
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

void getTestItemsIndexes(ServerIdItemInfoIdIndexMapMap &indexMap)
{
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		const ItemInfo &itemInfo = testItemInfo[i];
		indexMap[itemInfo.serverId][itemInfo.id] = i;
	}
}

ItemInfo *findTestItem(
  const ServerIdItemInfoIdIndexMapMap &indexMap,
  const ServerIdType &serverId, const uint64_t itemId)
{
	ServerIdItemInfoIdIndexMapMapConstIterator it = indexMap.find(serverId);
	if (it == indexMap.end())
		return NULL;
	ItemInfoIdIndexMapConstIterator indexIt = it->second.find(itemId);
	if (indexIt == it->second.end())
		return NULL;
	return &testItemInfo[indexIt->second];
}

size_t getNumberOfTestItems(const ServerIdType &serverId)
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

static string makeHostgroupElementPack(
  const ServerIdType &serverId,
  const HostIdType &hostId, const HostgroupIdType &hostgroupId)
{
	string s;
	s.append((char *)&serverId,    sizeof(serverId));
	s.append((char *)&hostId,      sizeof(hostId));
	s.append((char *)&hostgroupId, sizeof(hostgroupId));
	return s;
}

/**
 * This functions return a kind of a perfect hash set that is made from
 * a server Id, a host Id, and a host group ID. We call it
 * 'HostGroupElementPack'.
 *
 * @return a set of HostGroupElementPack.
 */
static const set<string> &getHostgroupElementPackSet(void)
{
	static set<string> hostgroupElementPackSet;
	if (!hostgroupElementPackSet.empty())
		return hostgroupElementPackSet;
	for (size_t i = 0; i < NumTestHostgroupElement; i++) {
		const HostgroupElement &hgrpElem = testHostgroupElement[i];
		const string mash =
		  makeHostgroupElementPack(
		    hgrpElem.serverId, hgrpElem.hostId, hgrpElem.groupId);
		pair<set<string>::iterator, bool> result = 
		  hostgroupElementPackSet.insert(mash);
		cppcut_assert_equal(true, result.second);
	}
	return hostgroupElementPackSet;
}

static bool isInHostgroup(const TriggerInfo &trigInfo,
                          const HostgroupIdType &hostgroupId)
{
	if (hostgroupId == ALL_HOST_GROUPS)
		return true;

	const set<string> &hostgroupElementPackSet = 
	  getHostgroupElementPackSet();

	const string pack =
	  makeHostgroupElementPack(trigInfo.serverId,
	                           trigInfo.hostId, hostgroupId);
	set<string>::const_iterator it = hostgroupElementPackSet.find(pack);
	return it != hostgroupElementPackSet.end();
}

size_t getNumberOfTestTriggers(const ServerIdType &serverId,
                               const HostgroupIdType &hostgroupId, 
                               const TriggerSeverityType &severity)
{
	size_t count = 0;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (serverId != ALL_SERVERS && trigInfo.serverId != serverId)
			continue;
		if (severity != NUM_TRIGGER_SEVERITY) {
			if (trigInfo.severity != severity)
				continue;
			if (trigInfo.status == TRIGGER_STATUS_OK)
				continue;
		}
		if (!isInHostgroup(trigInfo, hostgroupId))
			continue;
		count++;
	}
	return count;
}

static bool isGoodStatus(const TriggerInfo &triggerInfo)
{
	return triggerInfo.status == TRIGGER_STATUS_OK;
}

static void removeHostIdIfNeeded(ServerIdHostgroupHostIdMap &svIdHostGrpIdMap,
                                 uint64_t hostGrpIdForTrig,
                                 const TriggerInfo &trigInfo)
{
	ServerIdHostgroupHostIdMapIterator svIt;
	HostgroupHostIdMapIterator         hostIt;
	HostIdSetIterator                  idIt;
	svIt = svIdHostGrpIdMap.find(trigInfo.serverId);
	if (svIt == svIdHostGrpIdMap.end())
		return;
	HostgroupHostIdMap &hostGrpIdMap = svIt->second;
	hostIt = hostGrpIdMap.find(hostGrpIdForTrig);
	if (hostIt == hostGrpIdMap.end())
		return;
	HostIdSet &hostIdSet = hostIt->second;
	hostIdSet.erase(trigInfo.hostId);
}

size_t getNumberOfTestHosts(
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId)
{
	size_t numberOfTestHosts = 0;
	for (size_t i = 0; i < NumTestHostInfo; i++) {
		HostInfo hostInfo = testHostInfo[i];
		ServerIdType hostInfoServerId = hostInfo.serverId;
		if (hostInfoServerId == serverId)
			numberOfTestHosts++;
	}
	return numberOfTestHosts;
}

size_t getNumberOfTestHostsWithStatus(
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId,
  const bool &status, const UserIdType &userId)
{
	ServerIdHostgroupHostIdMap svIdHostGrpIdMap;
	ServerIdHostgroupHostIdMapIterator svIt;
	HostgroupHostIdMapIterator         hostIt;
	ServerHostGrpSetMap authMap;

	makeServerHostGrpSetMap(authMap, userId);

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (serverId != ALL_SERVERS && trigInfo.serverId != serverId)
			continue;
		if (!isAuthorized(authMap, userId, serverId, trigInfo.hostId))
			continue;
		if (!isInHostgroup(trigInfo, hostgroupId))
			continue;
		if (isGoodStatus(trigInfo) != status) {
			if (status) {
				// We have to remove inserted host ID.
				// When a host has at least one bad triggeer,
				// the host is not good.
				removeHostIdIfNeeded(svIdHostGrpIdMap,
				                     hostgroupId, trigInfo);
			}
			continue;
		}

		svIt = svIdHostGrpIdMap.find(trigInfo.serverId);
		if (svIt == svIdHostGrpIdMap.end()) {
			// svIdHostGrpMap doesn't have value pair
			// for this server
			HostIdSet hostIdSet;
			hostIdSet.insert(trigInfo.hostId);
			HostgroupHostIdMap hostMap;
			hostMap[hostgroupId] = hostIdSet;
			svIdHostGrpIdMap[trigInfo.serverId] = hostMap;
			continue;
		}

		HostgroupHostIdMap &hostGrpIdMap = svIt->second;
		hostIt = hostGrpIdMap.find(hostgroupId);
		if (hostIt == hostGrpIdMap.end()) {
			// svIdHostGrpMap doesn't have value pair
			// for this host group
			HostIdSet hostIdSet;
			hostIdSet.insert(trigInfo.hostId);
			hostGrpIdMap[hostgroupId] = hostIdSet;
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
	HostgroupHostIdMap &hostGrpIdMap = svIt->second;
	hostIt = hostGrpIdMap.find(hostgroupId);
	if (hostIt == hostGrpIdMap.end())
		return 0;
	HostIdSet &hostIdSet = hostIt->second;
	return hostIdSet.size();
}

void getDBCTestHostInfo(HostInfoList &hostInfoList,
                        const ServerIdType &targetServerId)
{
	for (size_t i = 0; i < NumTestHostInfo; i++) {
		const HostInfo hostInfo = testHostInfo[i];
		const ServerIdType &serverId = hostInfo.serverId;
		if (targetServerId != ALL_SERVERS && serverId != targetServerId)
			continue;
		hostInfoList.push_back(hostInfo);
	}
}

void makeTestUserIdIndexMap(UserIdIndexMap &userIdIndexMap)
{

	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo &accessInfo = testAccessInfo[i];
		userIdIndexMap[accessInfo.userId].insert(i);
	}
}

void makeServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
			     UserIdType userId)
{
	for (size_t i = 0; i < NumTestAccessInfo; ++i) {
		AccessInfo *accessInfo = &testAccessInfo[i];
		if (testAccessInfo[i].userId != userId)
			continue;

		HostGrpAccessInfoMap *hostGrpAccessInfoMap = NULL;
		ServerAccessInfoMapIterator it =
			srvAccessInfoMap.find(accessInfo->serverId);
		if (it == srvAccessInfoMap.end()) {
			hostGrpAccessInfoMap = new HostGrpAccessInfoMap();
			srvAccessInfoMap[accessInfo->serverId] =
				hostGrpAccessInfoMap;
		} else {
			hostGrpAccessInfoMap = it->second;
		}

		AccessInfo *destAccessInfo = NULL;
		HostGrpAccessInfoMapIterator it2
			= hostGrpAccessInfoMap->find(accessInfo->hostgroupId);
		if (it2 == hostGrpAccessInfoMap->end()) {
			destAccessInfo = new AccessInfo();
			(*hostGrpAccessInfoMap)[accessInfo->hostgroupId] =
				destAccessInfo;
		} else {
			destAccessInfo = it2->second;
		}
		*destAccessInfo = *accessInfo;
	}
}

void makeServerHostGrpSetMap(ServerHostGrpSetMap &map, const UserIdType &userId)
{
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		if (testAccessInfo[i].userId != userId)
			continue;
		map[testAccessInfo[i].serverId].insert(
		  testAccessInfo[i].hostgroupId);
	}
}

bool isAuthorized(
  ServerHostGrpSetMap &authMap, const UserIdType &userId,
  const ServerIdType &serverId, const HostIdType &hostId)
{
	if (userId == USER_ID_SYSTEM)
		return true;
	if (userId == INVALID_USER_ID)
		return false;

	ServerHostGrpSetMap::iterator serverIt
		= authMap.find(ALL_SERVERS);
	if (serverIt != authMap.end())
		return true;

	serverIt = authMap.find(serverId);
	if (serverIt == authMap.end())
		return false;

	if (hostId == ALL_HOSTS)
		return true;

	const HostgroupIdSet &hostgroupIds = serverIt->second;
	if (hostgroupIds.find(ALL_HOST_GROUPS) != hostgroupIds.end())
		return true;

	// check if the user is allowed to access to the host
	const set<string> &hgrpElementPackSet = getHostgroupElementPackSet();
	HostgroupIdSetConstIterator hostgroupIdItr = hostgroupIds.begin();
	for (; hostgroupIdItr != hostgroupIds.end(); ++hostgroupIdItr) {
		const string pack =
		  makeHostgroupElementPack(serverId, hostId, *hostgroupIdItr);
		if (hgrpElementPackSet.find(pack) != hgrpElementPackSet.end())
			return true;
	}

	return false;
}

size_t findIndexFromTestActionDef(const UserIdType &userId)
{
	size_t idx = 0;
	for (; idx < NumTestActionDef; idx++) {
		const ActionDef &actDef = testActionDef[idx];
		if (actDef.ownerUserId == userId)
			break;
	}
	cppcut_assert_not_equal(
	  NumTestActionDef, idx,
	  cut_message("Not found a testActionDef entry owned "
	              "by user: %"FMT_USER_ID, userId));
	return idx;
}

const HostgroupIdSet &getTestHostgroupIdSet(void)
{
	static HostgroupIdSet testHostgroupIdSet;
	if (!testHostgroupIdSet.empty()) 
		return testHostgroupIdSet;

	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		testHostgroupIdSet.insert(testHostgroupElement[i].groupId);
	return testHostgroupIdSet;
}
