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

#ifndef DBClientTest_h
#define DBClientTest_h

#include <map>
#include <set>
#include <SmartTime.h>
#include "DBTablesConfig.h"
#include "DBTablesMonitoring.h"
#include "DBTablesUser.h"
#include "DBTablesAction.h"
#include "DBTablesHost.h"

typedef std::set<uint64_t>  HostIdSet;
typedef HostIdSet::iterator HostIdSetIterator;
typedef std::map<uint32_t, HostIdSet> ServerIdHostIdMap;
typedef ServerIdHostIdMap::iterator   ServerIdHostIdMapIterator;

typedef std::map<HostgroupIdType, HostIdSet> HostgroupHostIdMap;
typedef HostgroupHostIdMap::iterator  HostgroupHostIdMapIterator;
typedef std::map<uint32_t, HostgroupHostIdMap> ServerIdHostgroupHostIdMap;
typedef ServerIdHostgroupHostIdMap::iterator ServerIdHostgroupHostIdMapIterator;

extern ServerTypeInfo testServerTypeInfo[];
extern size_t NumTestServerTypeInfo;

extern const MonitoringServerInfo testServerInfo[];
extern const size_t NumTestServerInfo;

extern MonitoringServerStatus testServerStatus[];
extern size_t NumTestServerStatus;

extern TriggerInfo testTriggerInfo[];
extern size_t NumTestTriggerInfo;

extern const EventInfo testEventInfo[];
extern const size_t NumTestEventInfo;
extern EventIdType findLastEventId(const ServerIdType &serverId);
extern mlpl::SmartTime findTimeOfLastEvent(
  const ServerIdType &serverId, const TriggerIdType &triggerId = ALL_TRIGGERS);

extern EventInfo testDupEventInfo[];
extern size_t NumTestDupEventInfo;

extern ItemInfo testItemInfo[];
extern size_t NumTestItemInfo;

extern ActionDef testActionDef[];
extern const size_t NumTestActionDef;

extern ActionDef testUpdateActionDef;

extern UserInfo testUserInfo[];
extern const size_t NumTestUserInfo;
extern const UserIdType userIdWithMultipleAuthorizedHostgroups;

extern AccessInfo testAccessInfo[];
extern const size_t NumTestAccessInfo;

extern UserRoleInfo testUserRoleInfo[];
extern const size_t NumTestUserRoleInfo;

extern ArmPluginInfo testArmPluginInfo[];
extern const size_t NumTestArmPluginInfo;

extern IncidentTrackerInfo testIncidentTrackerInfo[];
extern size_t NumTestIncidentTrackerInfo;

extern IncidentInfo testIncidentInfo[];
extern size_t NumTestIncidentInfo;

extern HistoryInfo testHistoryInfo[];
extern size_t NumTestHistoryInfo;

extern const ServerHostDef testServerHostDef[];
extern const size_t NumTestServerHostDef;

extern const VMInfo testVMInfo[];
extern const size_t NumTestVMInfo;

extern const Hostgroup testHostgroup[];
extern const size_t NumTestHostgroup;

extern const HostgroupMember testHostgroupMember[];
extern const size_t NumTestHostgroupMember;

/**
 * get the test trigger data indexes whose serverId and hostId are
 * matched with the specified.
 *
 * @param indexMap
 * The key of the outside map is a server Id. The key of the inside map is
 * a trigger ID. The value of the inside map is the index of test trigger data.
 *
 * @param serverId
 * A server ID. ALL_SERVERS can be specified.
 *
 * @param hostIdInServer
 * A host ID in a server. ALL_LOCAL_HOSTS can be specified.
 */
void getTestTriggersIndexes(
  std::map<ServerIdType, std::map<uint64_t, size_t> > &indexMap,
  const ServerIdType &serverId, const LocalHostIdType &hostIdInServer);
size_t getNumberOfTestTriggers(
  const ServerIdType &serverId,
  const HostgroupIdType &hostgroupId = ALL_HOST_GROUPS,
  const TriggerSeverityType &severity = NUM_TRIGGER_SEVERITY);

typedef std::map<uint64_t, size_t>         ItemInfoIdIndexMap;
typedef ItemInfoIdIndexMap::iterator       ItemInfoIdIndexMapIterator;
typedef ItemInfoIdIndexMap::const_iterator ItemInfoIdIndexMapConstIterator;

typedef std::map<ServerIdType, ItemInfoIdIndexMap>
  ServerIdItemInfoIdIndexMapMap;
typedef ServerIdItemInfoIdIndexMapMap::iterator
  ServerIdItemInfoIdIndexMapMapIterator;
typedef ServerIdItemInfoIdIndexMapMap::const_iterator
  ServerIdItemInfoIdIndexMapMapConstIterator;

void getTestItemsIndexes(ServerIdItemInfoIdIndexMapMap &indexMap);
ItemInfo *findTestItem(
  const ServerIdItemInfoIdIndexMapMap &indexMap,
  const ServerIdType &serverId, const uint64_t itemId);
size_t getNumberOfTestItems(const ServerIdType &serverId);

size_t getNumberOfTestHosts(
  const ServerIdType &serverId,
  const HostgroupIdType &hostgroupId = ALL_HOST_GROUPS);
size_t getNumberOfTestHostsWithStatus(
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId,
  const bool &status, const UserIdType &userId = USER_ID_SYSTEM);

bool filterOutAction(const ActionDef &actionDef, const ActionType &targetType);
size_t getNumberOfTestActions(
  const ActionType &actionType = ACTION_USER_DEFINED);

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo);
mlpl::SmartTime getTimestampOfLastTestTrigger(const ServerIdType &serverId);

typedef std::map<UserIdType, std::set<int> > UserIdIndexMap;
typedef UserIdIndexMap::iterator UserIdIndexMapIterator;
void makeTestUserIdIndexMap(UserIdIndexMap &userIdIndexMap);
void makeServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
			     UserIdType userId);
void makeServerHostGrpSetMap(ServerHostGrpSetMap &map,
                             const UserIdType &userId);
std::string makeEventIncidentMapKey(const EventInfo &eventInfo);
void makeEventIncidentMap(std::map<std::string, IncidentInfo*> &eventIncidentMap);
bool isAuthorized(ServerHostGrpSetMap &authMap,
                  const UserIdType &userId,
                  const ServerIdType &serverId,
                  const LocalHostIdType &hostIdInServer = ALL_LOCAL_HOSTS,
                  const std::set<std::string> *hgrpElementPackSet = NULL);

/**
 * Check if the user can access to the host.
 *
 * @param userId A user ID.
 * @param hostId A host ID in host_list table.
 *
 * @return true if the user is allowed to access to the host.
 */
bool isAuthorized(const UserIdType &userId, const HostIdType &hostId);

/**
 * @return if the test server is not in the test data, true is returned.
 */
bool isDefunctTestServer(const ServerIdType &serverId);

size_t findIndexFromTestActionDef(const UserIdType &userId);
size_t findIndexFromTestActionDef(const ActionType &type);

void getTestHistory(HistoryInfoVect &historyInfoVect,
		    const ServerIdType &serverId,
		    const ItemIdType itemId,
		    const time_t &beginTime, const time_t &endTime);

/**
 * Get a set of Hostgroup ID for the test material.
 */
const HostgroupIdSet &getTestHostgroupIdSet(void);

/**
 * Find an index of the record that has the specified type.
 *
 * @return
 * An index if returned when the record is found. Otherwise -1 is returned.
 */
int findIndexOfTestArmPluginInfo(const MonitoringSystemType &type);
int findIndexOfTestArmPluginInfo(const ServerIdType &serverId);
const ArmPluginInfo &getTestArmPluginInfo(const MonitoringSystemType &type);

extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST;
extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST_PASSIVE;

/**
 * Setup database for test.
 *
 * Guild line to use the following functions.
 *
 * - A test case that uses DBs shall call setupTestDB() only in
 *   cut_setup() once. Dont' use it in the middle of the test.
 * - cut_setup() and each test can call loadTestDBxxxx() if needed.
 * - To simplify test code, it is recommended that loadTestDBxxxx() is
 *   aggregated into cut_setup().
 */
extern const char *TEST_DB_USER;
extern const char *TEST_DB_PASSWORD;
extern const char *TEST_DB_NAME;

void setupTestDB(void);

void loadTestDBTablesConfig(void);
void loadTestDBTablesUser(void);

void loadTestDBServerType(void);
void loadTestDBServer(void);
void loadTestDBUser(void);
void loadTestDBUserRole(void);
void loadTestDBAccessList(void);
void loadTestDBArmPlugin(void);

void loadTestDBTriggers(void);
void loadTestDBEvents(void);
void loadTestDBItems(void);
void loadTestDBServerStatus(void);

void loadTestDBAction(void);
void loadTestDBIncidents(void);
void loadTestDBIncidentTracker(void);

void loadTestDBServerHostDef(void);
void loadTestDBVMInfo(void);
void loadTestDBHostgroup(void);
void loadTestDBHostgroupMember(void);

#endif // DBClientTest_h
