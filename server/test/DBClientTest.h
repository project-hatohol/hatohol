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

