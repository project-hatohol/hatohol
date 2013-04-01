#ifndef DBAgentTest_h
#define DBAgentTest_h

static MonitoringServerInfo serverInfo[] = 
{{
	1,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"pochi.dog.com",          // hostname
	"192.168.0.5",            // ip_address
	"POCHI",                  // nickname
},{
	2,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"mike.dog.com",           // hostname
	"192.168.1.5",            // ip_address
	"MIKE",                   // nickname
},{
	3,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"hachi.dog.com",          // hostname
	"192.168.10.1",           // ip_address
	"8",                      // nickname
}};
static size_t NumServerInfo = sizeof(serverInfo) / sizeof(MonitoringServerInfo);

static TriggerInfo testTriggerInfo[] = 
{{
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957197,0},           // lastChangeTime
	1,                        // serverId
	"235012",                 // hostId,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
},{
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARN,    // severity
	{1362957200,0},           // lastChangeTime
	3,                        // serverId
	"10001",                  // hostId,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
},{
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362951000,0},           // lastChangeTime
	3,                        // serverId
	"10002",                  // hostId,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
}};
static size_t NumTestTriggerInfo =
   sizeof(testTriggerInfo) / sizeof(TriggerInfo);

#endif // DBAgentTest_h

