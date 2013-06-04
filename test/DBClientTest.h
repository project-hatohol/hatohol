#ifndef DBClientTest_h
#define DBClientTest_h

#include "DBClientConfig.h"
#include "DBClientAsura.h"

extern MonitoringServerInfo serverInfo[];
extern size_t NumServerInfo;

extern TriggerInfo testTriggerInfo[];
extern size_t NumTestTriggerInfo;

extern EventInfo testEventInfo[];
extern size_t NumTestEventInfo;
extern uint64_t findLastEventId(uint32_t serverId);

extern ItemInfo testItemInfo[];
extern size_t NumTestItemInfo;

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo);

#endif // DBClientTest_h

