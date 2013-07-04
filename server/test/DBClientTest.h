#ifndef DBClientTest_h
#define DBClientTest_h

#include "DBClientConfig.h"
#include "DBClientHatohol.h"

typedef set<uint64_t>       HostIdSet;
typedef HostIdSet::iterator HostIdSetIterator;
typedef map<uint32_t, HostIdSet>    ServerIdHostIdMap;
typedef ServerIdHostIdMap::iterator ServerIdHostIdMapIterator;

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
extern size_t getNumberOfTestItems(uint32_t serverId);

extern void getTestHostInfoList(HostInfoList &hostInfoList,
                                uint32_t targetServerId,
                                ServerIdHostIdMap *serverIdHostIdMap = NULL);

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo);

#endif // DBClientTest_h

