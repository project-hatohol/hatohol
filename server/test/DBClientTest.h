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

#ifndef DBClientTest_h
#define DBClientTest_h

#include <map>
#include <set>
#include <SmartTime.h>
#include "DBClientConfig.h"
#include "DBClientHatohol.h"
#include "DBClientAction.h"
#include "DBClientUser.h"

typedef std::set<uint64_t>  HostIdSet;
typedef HostIdSet::iterator HostIdSetIterator;
typedef std::map<uint32_t, HostIdSet> ServerIdHostIdMap;
typedef ServerIdHostIdMap::iterator   ServerIdHostIdMapIterator;

typedef std::map<uint64_t, HostIdSet> HostgroupHostIdMap;
typedef HostgroupHostIdMap::iterator  HostgroupHostIdMapIterator;
typedef std::map<uint32_t, HostgroupHostIdMap> ServerIdHostgroupHostIdMap;
typedef ServerIdHostgroupHostIdMap::iterator ServerIdHostgroupHostIdMapIterator;

extern MonitoringServerInfo testServerInfo[];
extern size_t NumTestServerInfo;

extern MonitoringServerStatus testServerStatus[];
extern size_t NumTestServerStatus;

extern TriggerInfo testTriggerInfo[];
extern size_t NumTestTriggerInfo;

extern EventInfo testEventInfo[];
extern size_t NumTestEventInfo;
extern uint64_t findLastEventId(const ServerIdType &serverId);

extern ItemInfo testItemInfo[];
extern size_t NumTestItemInfo;

extern ActionDef testActionDef[];
extern const size_t NumTestActionDef;

extern UserInfo testUserInfo[];
extern const size_t NumTestUserInfo;
extern const UserIdType userIdWithMultipleAuthorizedHostgroups;

extern AccessInfo testAccessInfo[];
extern const size_t NumTestAccessInfo;

extern HostgroupInfo testHostgroupInfo[];
extern const size_t NumTestHostgroupInfo;

extern HostInfo testHostInfo[];
extern const size_t NumTestHostInfo;

extern HostgroupElement testHostgroupElement[];
extern const size_t NumTestHostgroupElement;

extern UserRoleInfo testUserRoleInfo[];
extern const size_t NumTestUserRoleInfo;

extern ArmPluginInfo testArmPluginInfo[];
extern const size_t NumTestArmPluginInfo;

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
 * @param hostId
 * A host ID. ALL_HOSTS can be specified.
 */
void getTestTriggersIndexes(
  std::map<ServerIdType, std::map<uint64_t, size_t> > &indexMap,
  const ServerIdType &serverId, uint64_t hostId);
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

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo);
mlpl::SmartTime getTimestampOfLastTestTrigger(const ServerIdType &serverId);

void getDBCTestHostInfo(HostInfoList &hostInfoList,
                        const ServerIdType &targetServerId = ALL_SERVERS);

typedef std::map<UserIdType, std::set<int> > UserIdIndexMap;
typedef UserIdIndexMap::iterator UserIdIndexMapIterator;
void makeTestUserIdIndexMap(UserIdIndexMap &userIdIndexMap);
void makeServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
			     UserIdType userId);
void makeServerHostGrpSetMap(ServerHostGrpSetMap &map,
                             const UserIdType &userId);
bool isAuthorized(ServerHostGrpSetMap &authMap,
                  const UserIdType &userId,
                  const ServerIdType &serverId,
                  const HostIdType &hostId = ALL_HOSTS);

size_t findIndexFromTestActionDef(const UserIdType &userId);

/**
 * Get a set of Hostgroup ID for the test material.
 */
const HostgroupIdSet &getTestHostgroupIdSet(void);

extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST;
extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
extern const MonitoringSystemType MONITORING_SYSTEM_HAPI_TEST_PASSIVE;

#endif // DBClientTest_h

