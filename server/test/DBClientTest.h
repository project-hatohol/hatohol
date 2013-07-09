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

#include "DBClientConfig.h"
#include "DBClientHatohol.h"

typedef set<uint64_t>       HostIdSet;
typedef HostIdSet::iterator HostIdSetIterator;
typedef map<uint32_t, HostIdSet>    ServerIdHostIdMap;
typedef ServerIdHostIdMap::iterator ServerIdHostIdMapIterator;

typedef map<uint64_t, HostIdSet>             HostGroupHostIdMap;
typedef HostGroupHostIdMap::iterator         HostGroupHostIdMapIterator;
typedef map<uint32_t, HostGroupHostIdMap>    ServerIdHostGroupHostIdMap;
typedef ServerIdHostGroupHostIdMap::iterator ServerIdHostGroupHostIdMapIterator;

extern MonitoringServerInfo serverInfo[];
extern size_t NumServerInfo;

extern TriggerInfo testTriggerInfo[];
extern size_t NumTestTriggerInfo;

extern EventInfo testEventInfo[];
extern size_t NumTestEventInfo;
extern uint64_t findLastEventId(uint32_t serverId);

extern ItemInfo testItemInfo[];
extern size_t NumTestItemInfo;

extern size_t getNumberOfTestTriggers(uint32_t serverId);
extern size_t getNumberOfTestTriggers(uint32_t serverId, uint64_t hostGroupId,
                                      TriggerSeverityType severity);

extern size_t getNumberOfTestItems(uint32_t serverId);

extern void getTestHostInfoList(HostInfoList &hostInfoList,
                                uint32_t targetServerId,
                                ServerIdHostIdMap *serverIdHostIdMap = NULL);

extern size_t getNumberOfTestHostsWithStatus(uint32_t serverId,
                                          uint64_t hostGroupId, bool status);

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo);

#endif // DBClientTest_h

