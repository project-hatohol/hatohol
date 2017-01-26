/* Copyright (C) 2013-2016 Project Hatohol
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

#include <cutter.h>
#include <cppcutter.h>
#include <unistd.h>
#include "Helpers.h"
#include "DBTablesTest.h"
#include <ThreadLocalDBCache.h>
using namespace std;
using namespace mlpl;

static const ServerIdType defunctServerId1 = 0x7fff0001;

const ServerTypeInfo testServerTypeInfo[] =
{{
	MONITORING_SYSTEM_FAKE,  // type
	"Fake Monitorin",        // name
	"User name|password",    // paramters
	"fake-plugin",           // pluginPath
	1,                       // plugin_sql_version
	1,                       // plugin_enabled
	"",                      // uuid
},{
	MONITORING_SYSTEM_HAPI2,  // type
	"Zabbix",                 // name
	"["
	"{\"id\": \"nickname\", \"label\": \"Nickname\"}, "
	"{\"id\": \"hostName\", \"label\": \"Host name\"}, "
	"{\"id\": \"ipAddress\", \"label\": \"IP address\"}, "
	"{\"default\": \"80\", \"id\": \"port\", \"label\": \"Port\"}, "
	"{\"id\": \"userName\", \"label\": \"User name\"}, "
	"{\"inputStyle\": \"password\", \"id\": \"password\","
	" \"label\": \"Password\"}, "
	"{\"default\": \"30\", \"id\": \"pollingInterval\","
	" \"label\": \"Polling interval (sec)\"}, "
	"{\"default\": \"10\", \"id\": \"retryInterval\","
	" \"label\": \"Retry interval (sec)\"}"
	"]", // paramters
	"/usr/sbin/hatohol-arm-plugin-ver2",  // pluginPath
	1,                       // plugin_sql_version
	1,                       // plugin_enabled
	"",                      // uuid
},{
	MONITORING_SYSTEM_HAPI_JSON, // type
	"JSON",                      // name
	"["
	"{\"id\": \"nickname\", \"label\": \"Nickname\"}, "
	"{\"hint\": \"(empty: Default)\", \"allowEmpty\": true,"
	" \"id\": \"brokerUrl\", \"label\": \"Broker URL\"}, "
	"{\"hint\": \"(empty: Default)\", \"allowEmpty\": true,"
	" \"id\": \"staticQueueAddress\","
	" \"label\": \"Static queue address\"}, "
	"{\"allowEmpty\": true, \"id\": \"tlsCertificatePath\","
	" \"label\": \"TLS client certificate path\"}, "
	"{\"allowEmpty\": true, \"id\": \"tlsKeyPath\","
	" \"label\": \"TLS client key path\"}, "
	"{\"allowEmpty\": true, \"id\": \"tlsCACertificatePath\","
	" \"label\": \"TLS CA certificate path\"}, "
	"{\"inputStyle\": \"checkBox\", \"allowEmpty\": true,"
	" \"id\": \"tlsEnableVerify\", \"label\": \"TLS: Enable verify\"}"
	"]", // paramters
	"",  // pluginPath
	1,                       // plugin_sql_version
	1,                       // plugin_enabled
	"",                      // uuid
},{
	MONITORING_SYSTEM_HAPI2, // type
	"Zabbix",                // name
	"["
	"{\"id\": \"nickname\", \"label\": \"Nickname\"}, "
	"{\"id\": \"hostName\", \"label\": \"Host name\"}, "
	"{\"id\": \"ipAddress\", \"label\": \"IP address\"}, "
	"{\"default\": \"80\", \"id\": \"port\", \"label\": \"Port\"}, "
	"{\"id\": \"userName\", \"label\": \"User name\"}, "
	"{\"inputStyle\": \"password\", \"id\": \"password\","
	" \"label\": \"Password\"}, "
	"{\"default\": \"30\", \"id\": \"pollingInterval\","
	" \"label\": \"Polling interval (sec)\"}, "
	"{\"default\": \"10\", \"id\": \"retryInterval\","
	" \"label\": \"Retry interval (sec)\"}, "
	"{\"inputStyle\": \"checkBox\", \"id\": \"passiveMode\","
	" \"label\": \"Passive mode\"}, "
	"{\"hint\": \"(empty: Default)\", \"allowEmpty\": true,"
	" \"id\": \"brokerUrl\", \"label\": \"Broker URL\"}, "
	"{\"hint\": \"(empty: Default)\", \"allowEmpty\": true,"
	" \"id\": \"staticQueueAddress\","
	" \"label\": \"Static queue address\"}"
	"]",                     // paramters
	"hap2-zabbix",           // pluginPath
	1,                       // plugin_sql_version
	1,                       // plugin_enabled
	"8e632c14-d1f7-11e4-8350-d43d7e3146fb", // uuid
}};
const size_t NumTestServerTypeInfo = ARRAY_SIZE(testServerTypeInfo);

const MonitoringServerInfo testServerInfo[] =
{{
	1,                        // id
	MONITORING_SYSTEM_HAPI2,  // type
	"pochi.dog.com",          // hostname
	"192.168.0.5",            // ip_address
	"POCHI",                  // nickname
	80,                       // port
	10,                       // polling_interval_sec
	5,                        // retry_interval_sec
	"foo",                    // user_name
	"goo",                    // password
	"dbX",                    // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	2,                        // id
	MONITORING_SYSTEM_HAPI2,  // type
	"mike.dog.com",           // hostname
	"192.168.1.5",            // ip_address
	"MIKE",                   // nickname
	80,                       // port
	30,                       // polling_interval_sec
	15,                       // retry_interval_sec
	"Einstein",               // user_name
	"Albert",                 // password
	"gravity",                // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	3,                        // id
	MONITORING_SYSTEM_HAPI2,  // type
	"hachi.dog.com",          // hostname
	"192.168.10.1",           // ip_address
	"8",                      // nickname
	8080,                     // port
	60,                       // polling_interval_sec
	60,                       // retry_interval_sec
	"Fermi",                  // user_name
	"fermion",                // password
	"",                       // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	4,                        // id
	MONITORING_SYSTEM_HAPI2,  // type
	"mosquito.example.com",   // hostname
	"10.100.10.52",           // ip_address
	"KA",                     // nickname
	30000,                    // port
	3600,                     // polling_interval_sec
	600,                      // retry_interval_sec
	"Z",                      // user_name
	"OTSU",                   // password
	"zzz",                    // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	5,                        // id
	MONITORING_SYSTEM_HAPI2,  // type
	"overture.example.com",   // hostname
	"123.45.67.89",           // ip_address
	"OIOI",                   // nickname
	0,                        // port
	60,                       // polling_interval_sec
	600,                      // retry_interval_sec
	"earth",                  // user_name
	"ground",                 // password
	"magma",                  // db_name
	"",                       // base_url
	"{\"point\": 3}",         // exteneded_info
},{
	211,                      // id
	MONITORING_SYSTEM_HAPI2,  // type
	"x-men.example.com",      // hostname
	"172.16.32.51",           // ip_address
	"(^_^)",                  // nickname
	12345,                    // port
	10,                       // polling_interval_sec
	10,                       // retry_interval_sec
	"sake",                   // user_name
	"siranami",               // password
	"zabbix",                 // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	222,                      // id
	MONITORING_SYSTEM_HAPI2,  // type
	"zoo.example.com",        // hostname
	"10.0.0.48",              // ip_address
	"Akira",                  // nickname
	80,                       // port
	300,                      // polling_interval_sec
	60,                       // retry_interval_sec
	"ponta",                  // user_name
	"doradora",               // password
	"z@bb1x",                 // db_name
	"",                       // base_url
	"",                       // exteneded_info
},{
	301,                      // id
	MONITORING_SYSTEM_HAPI2,  // type
	"nagios.example.com",     // hostname
	"10.0.0.32",              // ip_address
	"Akira",                  // nickname
	3306,                     // port
	300,                      // polling_interval_sec
	60,                       // retry_interval_sec
	"nagios-operator",        // user_name
	"5t64k-f3-ui.l76n",       // password
	"nAgiOs_ndoutils",        // db_name
	"http://10.0.0.32/nagios3", // base_url
	"test extended info",     // exteneded_info
}};
const size_t NumTestServerInfo = ARRAY_SIZE(testServerInfo);

const MonitoringServerStatus testServerStatus[] =
{{
	1,                        // id
	1.1,                      // nvps
},{
	2,                        // id
	1.2,                      // nvps
},{
	3,                        // id
	1.3,                      // nvps
},{
	4,                        // id
	10.4,                     // nvps
},{
	5,                        // id
	0.0,                      // nvps
},{
	211,                      // id
	10.5,                     // nvps
},{
	222,                      // id
	0.00051234,               // nvps
}};
size_t const NumTestServerStatus = ARRAY_SIZE(testServerStatus);

const TriggerInfo testTriggerInfo[] =
{{
	1,                        // serverId
	"1",                      // id
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957197,0},           // lastChangeTime
	10,                       // globalHostId,
	"235012",                 // hostIdInServer,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	1,                        // serverId
	"2",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957198,0},           // lastChangeTime
	10,                       // globalHostId,
	"235012",                 // hostIdInServer,
	"hostX1",                 // hostName,
	"TEST Trigger 1a",        // brief,
	"{\"expandedDescription\":\"Test Trigger on hostX1\"}", // extendedInfo
	TRIGGER_VALID,          // validity
},{
	1,                        // serverId
	"3",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957117,0},           // lastChangeTime
	11,                       // globalHostId,
	"235013",                 // hostIdInServer,
	"hostX2",                 // hostName,
	"TEST Trigger 1b",        // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	1,                        // serverId
	"4",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957197,0},           // lastChangeTime
	10,                       // globalHostId,
	"235012",                 // hostIdInServer,
	"hostX1",                 // hostName,
	"TEST Trigger 1c",        // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	1,                        // serverId
	"5",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957198,0},           // lastChangeTime
	30,                       // globalHostId,
	"1129",                   // hostIdInServer,
	"hostX3",                 // hostName,
	"TEST Trigger 1d",        // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	3,                        // serverId
	"2",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
	{1362957200,0},           // lastChangeTime
	35,                       // globalHostId,
	"10001",                  // hostIdInServer,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
	"{\"expandedDescription\":\"Test Trigger on hostZ1\"}", // extendedInfo
	TRIGGER_VALID,          // validity
},{
	3,                        // serverId
	"3",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362951000,0},           // lastChangeTime
	41,                       // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	3,                        // serverId
	"4",                      // id
	TRIGGER_STATUS_UNKNOWN,   // status
	TRIGGER_SEVERITY_CRITICAL,// severity
	{1362951000,0},           // lastChangeTime
	41,                       // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"Status:Unknown, Severity:Critical", // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	2,                        // serverId
	"1147797409030816545", // 0xfedcba987654321 // id
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_WARNING, // severity
	{1362951000,0},           // lastChangeTime
	101,                      // globalHostId,
	"9920249034889494527",    // hostIdInServer,
	"hostQ1",                 // hostName,
	"TEST Trigger Action",    // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	2,                        // serverId
	"18364758544493064720", // 0xfedcba9876543210, // id
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_WARNING, // severity
	{1362951234,0},           // lastChangeTime
	101,                      // globalHostId,
	"9920249034889494527",    // hostIdInServer,
	"hostQ1",                 // hostName,
	"TEST Trigger Action 2",  // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	// This entry is used for testHatoholArmPluginGate.
	12345,                    // serverId
	"2468",                   // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957117,0},           // lastChangeTime
	12345,                    // globalHostId, (not in testServerHostDef)
	"10002",                  // hostIdInServer,
	"host12345",              // hostName,
	"Brief for host12345",    // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},{
	// This entry is for tests with a defunct server
	defunctServerId1,         // serverId
	"3",                      // id
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	{1362957117,0},           // lastChangeTime
	0xffeeddcc,               // globalHostId,(not in testServerHostDef)
	"10002",                  // hostIdInServer,
	"defunctSv1Host1",        // hostName,
	"defunctSv1Host1 material", // brief,
	"",                       // extendedInfo
	TRIGGER_VALID,          // validity
},
};
const size_t NumTestTriggerInfo = ARRAY_SIZE(testTriggerInfo);

static const TriggerInfo &trigInfoDefunctSv1 =
  testTriggerInfo[NumTestTriggerInfo-1];

const EventInfo testEventInfo[] = {
{
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	"1",                      // id
	{1362957200,0},           // time
	EVENT_TYPE_GOOD,          // type
	"2",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
	35,                       // globalHostId,
	"10001",                  // hostIdInServer,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
	"{\"expandedDescription\":\"Test Trigger on hostZ1\"}", // extendedInfo
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	"2",                      // id
	{1362958000,0},           // time
	EVENT_TYPE_GOOD,          // type
	"3",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	41,                       // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
	"",
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	"1",                      // id
	{1363123456,0},           // time
	EVENT_TYPE_GOOD,          // type
	"2",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	10,                       // globalHostId,
	"235012",                 // hostIdInServer,
	"hostX1",                 // hostName,
	"TEST Trigger 1a",        // brief,
	"{\"expandedDescription\":\"Test Trigger on hostX1\"}", // extendedInfo
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	"2",                      // id
	{1378900022,0},           // time
	EVENT_TYPE_GOOD,          // type
	"1",                      // triggerId
	TRIGGER_STATUS_OK,        // status
	TRIGGER_SEVERITY_INFO,    // severity
	10,                       // globalHostId,
	"235012",                 // hostIdInServer,
	"hostX1",                 // hostName,
	"TEST Trigger 1",         // brief,
	"",
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	1,                        // serverId
	"3",                      // id
	{1389123457,0},           // time
	EVENT_TYPE_GOOD,          // type
	"3",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	11,                       // globalHostId,
	"235013",                 // hostIdInServer,
	"hostX2",                 // hostName,
	"TEST Trigger 1b",        // brief,
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	"3",                      // id
	{1390000000,123456789},   // time
	EVENT_TYPE_BAD,           // type
	"2",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
	35,                       // globalHostId,
	"10001",                  // hostIdInServer,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
	"{\"expandedDescription\":\"Test Trigger on hostZ1\"}", // extendedInfo
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	"4",                      // id
	{1390000100,123456789},   // time
	EVENT_TYPE_UNKNOWN,       // type
	"4",                      // triggerId
	TRIGGER_STATUS_UNKNOWN,   // status
	TRIGGER_SEVERITY_CRITICAL, // severity
	41,                       // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"Status:Unknown, Severity:Critical", // brief,
	"", // extendedInfo
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,     // unifiedId
	trigInfoDefunctSv1.serverId, // serverId
	"1",                        // id
	trigInfoDefunctSv1.lastChangeTime, // time
	EVENT_TYPE_BAD,                    // type
	"3",                               // triggerId
	trigInfoDefunctSv1.status,         // status
	trigInfoDefunctSv1.severity,       // severity
	trigInfoDefunctSv1.globalHostId,   // globalHostId,
	trigInfoDefunctSv1.hostIdInServer, // hostIdInServer,
	trigInfoDefunctSv1.hostName,       // hostName,
	trigInfoDefunctSv1.brief,          // brief,
	"",
},
// We assumed the data of the default server's is at the tail in testEventInfo.
// See also the definition of trigInfoDefunctSv1 above. Anyway,
// ******* DON'T APPEND RECORDS AFTER HERE *******
};
const size_t NumTestEventInfo = ARRAY_SIZE(testEventInfo);

const EventInfo testDupEventInfo[] = {
{
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	DISCONNECT_SERVER_EVENT_ID, // id
	{1362957200,0},           // time
	EVENT_TYPE_GOOD,          // type
	"2",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_WARNING, // severity
	4,                        // globalHostId,
	"10001",                  // hostIdInServer,
	"hostZ1",                 // hostName,
	"TEST Trigger 2",         // brief,
	"{\"expandedDescription\":\"Test Trigger on hostZ1\"}", // extendedInfo
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	DISCONNECT_SERVER_EVENT_ID, // id
	{1362951000,0},           // time
	EVENT_TYPE_GOOD,          // type
	"3",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	6,                        // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
	"",
}, {
	AUTO_INCREMENT_VALUE,     // unifiedId
	3,                        // serverId
	DISCONNECT_SERVER_EVENT_ID, // id
	{1362951000,0},           // time
	EVENT_TYPE_GOOD,          // type
	"3",                      // triggerId
	TRIGGER_STATUS_PROBLEM,   // status
	TRIGGER_SEVERITY_INFO,    // severity
	6,                        // globalHostId,
	"10002",                  // hostIdInServer,
	"hostZ2",                 // hostName,
	"TEST Trigger 3",         // brief,
	"",
},
};
const size_t NumTestDupEventInfo = ARRAY_SIZE(testDupEventInfo);

const ItemInfo testItemInfo[] = {
{
	1,                        // globalId
	1,                        // serverId
	"2",                      // id
	30,                       // globalHostId
	"1129",                   // hostIdInServer
	"Rome wasn't built in a day",// brief
	{1362951129,0},           // lastValueTime
	"Fukuoka",                // lastValue
	"Sapporo",                // prevValue
	{"City"},                 // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_STRING, // valueType
	"",                       // unit
}, {
	2,                        // globalId
	3,                        // serverId
	"1",                      // id
	42,                       // globalHostId
	"5",                      // hostIdInServer
	"The age of the cat.",    // brief
	{1362957200,0},           // lastValueTime
	"1",                      // lastValue
	"5",                      // prevValue
	{"number"},               // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_INTEGER, // valueType
	"age",                    // unit
}, {
	3,                        // globalId
	3,                        // serverId
	"2",                      // id
	45,                       // globalHostId
	"100",                    // hostIdInServer
	"All roads lead to Rome.",// brief
	{1362951000,0},           // lastValueTime
	"Osaka",                  // lastValue
	"Ichikawa",               // prevValue
	{"City"},                 // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_STRING, // valueType
	"",                       // unit
}, {
	4,                        // globalId
	4,                        // serverId
	"1",                      // id
	100,                      // globalHostId
	"100",                    // hostIdInServer
	"All roads lead to Rome.",// brief
	{1362951000,0},           // lastValueTime
	"Osaka",                  // lastValue
	"Ichikawa",               // prevValue
	{"City"},                 // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_STRING, // valueType
	"",                       // unit
}, {
	5,                        // globalId
	211,                      // serverId
	"12345",                  // id
	1050,                     // globalHostId
	"200",                    // hostIdInServer
	"Multiple item group (category).", // brief
	{1362951444,123},         // lastValueTime
	"@n1m@l",                 // lastValue
	"animal",                 // prevValue
	{"DOG", "ABC", "I'm a perfect human."}, // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_STRING, // valueType
	"",                       // unit
}, {
	6,                        // globalId
	211,                      // serverId
	"abcde",                  // id
	2111,                     // globalHostId
	"12111",                  // hostIdInServer
	"No item group (category)", // brief
	{1362988899,456},         // lastValueTime
	"imo",                    // lastValue
	"arai",                   // prevValue
	{},                       // categoryNames;
	0,                        // delay
	ITEM_INFO_VALUE_TYPE_STRING, // valueType
	"",                       // unit
},
};
const size_t NumTestItemInfo = ARRAY_SIZE(testItemInfo);

static ActionDef bareTestActionDef[] = {
{
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID |
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  1,                        // serverId
	  "10",                     // hostIdInServer
	  "5",                      // hostgroupId
	  "3",                      // triggerId
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
	  ACTCOND_TRIGGER_STATUS |
	  ACTCOND_TRIGGER_SEVERITY, // enableBits
	  0,                        // serverId
	  "0",                      // hostIdInServer
	  "0",                      // hostgroupId
	  "74565", // 0x12345,      // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_CRITICAL,// triggerSeverity
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
	  100,                      // serverIdInServer
	  "9223372036854775807",    // hostIdInServer (0x7fffffffffffffff)
	  "9223372036854775808",  // 0x8000000000000000, // hostgroupId
	  "18364758544493064720", // 0xfedcba9876543210, // triggerId
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
	  "9920249034889494527",    // hostIdInServer
	  "9223372036854775808",  // 0x8000000000000000, // hostgroupId
	  "18364758544493064720", // 0xfedcba9876543210, // triggerId
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
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID |
	  ACTCOND_TRIGGER_ID |
	  ACTCOND_TRIGGER_STATUS | ACTCOND_TRIGGER_SEVERITY, // enableBits
	  100001,                   // serverId
	  "100001",                 // hostIdInServer
	  "100001",                 // hostgroupId
	  "74565", // 0x12345,      // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_WARNING, // triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,                      // type
	"/",                                 // working dir
	"/bin/rm -rf --no-preserve-root /",  // command
	0,                                   // timeout
	1,                                   // ownerUserId
}, {
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS,   // enableBits
	  101,                      // serverId
	  "9223372036854775807",    // hostIdInServer (0x7fffffffffffffff)
	  "9223372036854775808",  // 0x8000000000000000, // hostgroupId
	  "18364758544493064720", // 0xfedcba9876543210, // triggerId
	  TRIGGER_STATUS_OK,        // triggerStatus
	  TRIGGER_SEVERITY_CRITICAL,// triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"/home/foo",       // working dir
	"a-dog-meets-food",// command
	3939,              // timeout
	2,                 // ownerUserId
}, {
	0,                 // id (this field is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID |
	  ACTCOND_HOST_GROUP_ID,    // enableBits
	  1,                        // serverId
	  "10",                     // hostIdInServer
	  "5",                      // hostgroupId
	  "0",                      // triggerId
	  TRIGGER_STATUS_OK,        // triggerStatus
	  TRIGGER_SEVERITY_CRITICAL,// triggerSeverity
	  CMP_INVALID               // triggerSeverityCompType;
	), // condition
	ACTION_INCIDENT_SENDER, // type
	"",                     // working dir
	"3",                    // command
	0,                      // timeout
	0,                      // ownerUserId
},
};

const ActionDef *testActionDef = bareTestActionDef;
const size_t NumTestActionDef = ARRAY_SIZE(bareTestActionDef);

const ActionDef testUpdateActionDef = {
	2,                 // id (this field is needed when updating)
	ActionCondition(
		ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
		ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS |
		ACTCOND_TRIGGER_SEVERITY, // enableBits
		2,                        // serverId
		"1001",                   // hostIdInServer
		"2001",                   // hostGroupId
		"14000",                  // triggerId
		TRIGGER_STATUS_OK,        // triggerStatus
		TRIGGER_SEVERITY_WARNING, // triggerSeverity
		CMP_EQ_GT                 // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,          // type
	"/home/hatohol",         // working dir
	"/usr/lib/libupdate.so", // command
	0,                       // timeout
	2,                       // ownerUserId
};

static UserInfo bareTestUserInfo[] = {
{
	0,                 // id
	"cheesecake",      // name
	"CDEF~!@#$%^&*()", // password
	0,                 // flags
}, {
	0,                 // id
	"pineapple",       // name
	"Po+-\\|}{\":?><", // password
	OperationPrivilege::ALL_PRIVILEGES,    // flags
}, {
	0,                 // id
	"m1ffy@v@",        // name
	"S/N R@t10",       // password
	OperationPrivilege::makeFlag(OPPRVLG_DELETE_ACTION), // flags
}, {
	0,                 // id
	"higgs",           // name
	"gg (-> h*) -> ZZ",// password
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
}, {
	0,                         // id
	"IncidentSettingsAdmin",   // name
	"notActionsAdmin",         // password
	(1 << OPPRVLG_GET_ALL_INCIDENT_SETTINGS) |
	(1 << OPPRVLG_CREATE_INCIDENT_SETTING) |
	(1 << OPPRVLG_UPDATE_INCIDENT_SETTING) |
	(1 << OPPRVLG_DELETE_INCIDENT_SETTING), // flags
}, {
	0,                         // id
	"ActionsAdmin",            // name
	"notIncidentSettingsAdmin",// password
	(1 << OPPRVLG_GET_ALL_ACTION) |
	(1 << OPPRVLG_CREATE_ACTION) |
	(1 << OPPRVLG_UPDATE_ALL_ACTION) |
	(1 << OPPRVLG_UPDATE_ACTION) |
	(1 << OPPRVLG_DELETE_ALL_ACTION) |
	(1 << OPPRVLG_DELETE_ACTION), // flags
}, {
	0,                        // id
	"onlyMonitoring",         // name
	"onlyGetServerInfo",      // password
	OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER),
}
};
const UserInfo *testUserInfo = bareTestUserInfo;
const size_t NumTestUserInfo = ARRAY_SIZE(bareTestUserInfo);
const UserIdType userIdWithMultipleAuthorizedHostgroups = 7;

static AccessInfo bareTestAccessInfo[] = {
{
	0,                 // id
	1,                 // userId
	1,                 // serverId
	"0",               // hostgroupId
}, {
	0,                 // id
	1,                 // userId
	1,                 // serverId
	"1",               // hostgroupId
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
	"1",               // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	2,                 // serverId
	"2",               // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	4,                 // serverId
	"1",               // hostgroupId
}, {
	0,                 // id
	3,                 // userId
	211,               // serverId
	"123",             // hostgroupId
}, {
	0,                 // id
	5,                 // userId
	1,                 // serverId
	ALL_HOST_GROUPS,   // hostgroupId
}, {
	0,                 // id
	6,                 // userId
	211,               // serverId
	"124",             // hostgroupId
}, {
	0,                 // id
	6,                 // userId
	222,               // serverId
	"124",             // hostgroupId
}, {
	0,                 // id
	userIdWithMultipleAuthorizedHostgroups, // userId
	1,                 // serverId
	"1",               // hostgroupId
}, {
	0,                 // id
	userIdWithMultipleAuthorizedHostgroups, // userId
	1,                 // serverId
	"2",               // hostgroupId
}, {
	0,                 // id
	2,                 // userId
	211,               // serverId
	ALL_HOST_GROUPS,   // hostgroupId
}
};
const AccessInfo *testAccessInfo = bareTestAccessInfo;
const size_t NumTestAccessInfo = ARRAY_SIZE(bareTestAccessInfo);

const UserRoleInfo testUserRoleInfo[] = {
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
const size_t NumTestUserRoleInfo = ARRAY_SIZE(testUserRoleInfo);

const ArmPluginInfo testArmPluginInfo[] = {
{
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"start-stop-hap2-zabbix-api.sh", // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	1,                               // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"8e632c14-d1f7-11e4-8350-d43d7e3146fb", // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"start-stop-hap2-nagios-ndoutils.sh",  // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	2,                               // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"902d955c-d1f7-11e4-80f9-d43d7e3146fb", // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"./hapi-test-plugin",            // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	3,                               // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"hapi-test-non-existing-plugin", // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	4,                               // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"#PASSIVE_PLUGIN#",              // path
	"",                              // brokerUrl
	"testQueue",                     // staticQueueAddress
	5,                               // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"#PASSIVE_PLUGIN#",              // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	211,                             // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"#PASSIVE_PLUGIN#",              // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	222,                             // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}, {
	AUTO_INCREMENT_VALUE,            // id
	MONITORING_SYSTEM_HAPI2,         // type
	"#PASSIVE_PLUGIN#",              // path
	"",                              // brokerUrl
	"",                              // staticQueueAddress
	301,                             // serverId
	"",                              // tlsCertificatePath
	"",                              // tlsKeyPath
	"",                              // tlsCACertificatePath
	0,                               // tlsEnableVerify
	"",                              // uuid
}
};
const size_t NumTestArmPluginInfo = ARRAY_SIZE(testArmPluginInfo);

const IncidentTrackerInfo testIncidentTrackerInfo[] = {
{
	1,                        // id
	INCIDENT_TRACKER_REDMINE, // type
	"Numerical ID",           // nickname
	"http://localhost",       // baseURL
	"1",                      // projectId
	"3",                      // TrackerId
	"foo",                    // user_name
	"bar",                    // password
},{
	2,                        // id
	INCIDENT_TRACKER_REDMINE, // type
	"String project ID",      // nickname
	"http://localhost",       // baseURL
	"hatohol",                // projectId
	"3",                      // TrackerId
	"foo",                    // user_name
	"bar",                    // password
},{
	3,                        // id
	INCIDENT_TRACKER_REDMINE, // type
	"Redmine Emulator",       // nickname
	"http://localhost:44444", // baseURL
	"hatoholtestproject",     // projectId
	"1",                      // TrackerId
	"hatohol",                // user_name
	"o.o662L6q1V7E",          // password
},{
	4,                        // id
	INCIDENT_TRACKER_REDMINE, // type
	"Redmine Emulator",       // nickname
	"http://localhost:44444", // baseURL
	"hatoholtestproject",     // projectId
	"2",                      // TrackerId
	"hatohol",                // user_name
	"o.o662L6q1V7E",          // password
},{
	5,                        // id
	INCIDENT_TRACKER_HATOHOL, // type
	"Internal",               // nickname
	"",                       // baseURL
	"",                       // projectId
	"",                       // TrackerId
	"",                       // user_name
	"",                       // password
}
};
const size_t NumTestIncidentTrackerInfo = ARRAY_SIZE(testIncidentTrackerInfo);

const IncidentInfo testIncidentInfo[] = {
{
	3,                        // trackerId
	1,                        // serverId
	"1",                      // eventId
	"2",                      // triggerId
	"100",                    // identifier
	"http://localhost:44444/issues/100", // location
	"New",                    // status
	"Normal",                 // priority
	"foobar",                 // assignee
	0,                        // doneRatio
	{1412957260, 0},          // createdAt
	{1412957260, 0},          // updatedAt
	IncidentInfo::STATUS_OPENED,// statusCode
	3,                        // unifiedId
	0,                        // commentCount
},
{
	3,                        // trackerId
	1,                        // serverId
	"2",                      // eventId
	"1",                      // triggerId
	"101",                    // identifier
	"http://localhost:44444/issues/101", // location
	"New",                    // status
	"Normal",                 // priority
	"foobar",                 // assignee
	0,                        // doneRatio
	{1412957290, 0},          // createdAt
	{1412957290, 0},          // updatedAt
	IncidentInfo::STATUS_OPENED,// statusCode
	4,                        // unifiedId
	0,                        // commentCount
},
{
	5,                        // trackerId
	2,                        // serverId
	"2",                      // eventId
	"3",                      // triggerId
	"123",                    // identifier
	"",                       // location
	"NONE",                   // status
	"",                       // priority
	"",                       // assignee
	0,                        // doneRatio
	{1412957360, 0},          // createdAt
	{1412957360, 0},          // updatedAt
	IncidentInfo::STATUS_OPENED,// statusCode
	123,                      // unifiedId
	0,                        // commentCount
},
};
const size_t NumTestIncidentInfo = ARRAY_SIZE(testIncidentInfo);

const HistoryInfo testHistoryInfo[] = {
{
	3,              // serverId
	"1",            // itemId
	"0.0",          // value
	{1205277200,0}, // clock
},
{
	3,              // serverId
	"1",            // itemId
	"1.0",          // value
	{1236813200,0}, // clock
},
{
	3,              // serverId
	"1",            // itemId
	"2.0",          // value
	{1268349200,0}, // clock
},
{
	3,              // serverId
	"1",            // itemId
	"3.0",          // value
	{1299885200,0}, // clock
},
{
	3,              // serverId
	"1",            // itemId
	"4.0",          // value
	{1331421200,0}, // clock
},
{
	3,              // serverId
	"1",            // itemId
	"5.0",          // value
	{1362957200,0}, // clock
},
};
const size_t NumTestHistoryInfo = ARRAY_SIZE(testHistoryInfo);

const ServerHostDef testServerHostDef[] = {
{
	AUTO_INCREMENT_VALUE, // 1       // id
	10,                              // hostId
	1,                               // serverId
	"235012",                        // hostIdInServer
	"hostX1",                        // name
}, {
	AUTO_INCREMENT_VALUE, // 2       // id
	11,                              // hostId
	1,                               // serverId
	"235013",                        // hostIdInServer
	"hostX2",                        // name
}, {
	AUTO_INCREMENT_VALUE, // 3       // id
	30,                              // hostId
	1,                               // serverId
	"1129",                          // hostIdInServer
	"hostX3",                        // name
} ,{
	AUTO_INCREMENT_VALUE, // 4       // id
	35,                              // hostId
	3,                               // serverId
	"10001",                         // hostIdInServer
	"hostZ1",                        // name
} ,{
	AUTO_INCREMENT_VALUE, // 5       // id
	40,                              // hostId
	2,                               // serverId
	"512",                           // hostIdInServer
	"multi-host group",              // name
}, {
	AUTO_INCREMENT_VALUE, // 6       // id
	41,                              // hostId
	3,                               // serverId
	"10002",                         // hostIdInServer
	"hostZ2",                        // name
}, {
	AUTO_INCREMENT_VALUE, // 7       // id
	42,                              // hostId
	3,                               // serverId
	"5",                             // hostIdInServer
	"frog",                          // name
} ,{
	AUTO_INCREMENT_VALUE, // 8       // id
	45,                              // hostId
	3,                               // serverId
	"100",                           // hostIdInServer
	"dolphin",                       // name
}, {
	AUTO_INCREMENT_VALUE, // 9       // id
	100,                             // hostId
	4,                               // serverId
	"100",                           // hostIdInServer
	"squirrel",                      // name
}, {
	AUTO_INCREMENT_VALUE, // 10      // id
	101,                             // hostId
	2,                               // serverId
	//"0x89abcdefffffffff",          // hostIdInServer
	"9920249034889494527",           // getHostInfoList() handles HostID As decimal
	"hostQ1",                        // name
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE, // 11      // id
	494,                             // host_id
	trigInfoDefunctSv1.serverId,     // serverId
	//trigInfoDefunctSv1.hostId,       // hostIdInServer
	"10002", // TODO: use the above after host ID in trigger becomes string
	trigInfoDefunctSv1.hostName,     // name,
}, {
	AUTO_INCREMENT_VALUE,            // id
	1050,                            // hostId
	211,                             // serverId
	"200",                           // hostIdInServer
	"host 200",                      // name
}, {
	AUTO_INCREMENT_VALUE,            // id
	2111,                            // hostId
	211,                             // serverId
	"12111",                         // hostIdInServer
	"host 12111",                    // name
}, {
	AUTO_INCREMENT_VALUE,            // id
	2112,                            // hostId
	211,                             // serverId
	"12112",                         // hostIdInServer
	"host 12112",                    // name
}, {
	AUTO_INCREMENT_VALUE,            // id
	2113,                            // hostId
	211,                             // serverId
	"12113",                         // hostIdInServer
	"host 12113",                    // name
}, {
	AUTO_INCREMENT_VALUE,            // id
	10005,                           // hostId
	222,                             // serverId
	"110005",                        // hostIdInServer
	"host 110005",                   // name
}
};
const size_t NumTestServerHostDef = ARRAY_SIZE(testServerHostDef);

const VMInfo testVMInfo[] = {
{
	AUTO_INCREMENT_VALUE,            // id
	2111,                            // hostId
	1050,                            // hypervisorHostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	2112,                            // hostId
	1050,                            // hypervisorHostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	2113,                            // hostId
	1050,                            // hypervisorHostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	10005,                           // hostId
	1080,                            // hypervisorHostId
}
};
const size_t NumTestVMInfo = ARRAY_SIZE(testVMInfo);

const Hostgroup testHostgroup[] = {
{
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	"1",                   // idInServer
	"Monitor Servers"      // name
}, {
	AUTO_INCREMENT_VALUE,  // id
	1,                     // serverId
	"2",                   // idInServer
	"Monitored Servers"    // name
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	"1",                   // idInServer
	"Checking Servers"     // name
}, {
	AUTO_INCREMENT_VALUE,  // id
	3,                     // serverId
	"2",                   // idInServer
	"Checked Servers"      // name
}, {
	AUTO_INCREMENT_VALUE,  // id
	4,                     // serverId
	"1",                   // idInServer
	"Watching Servers"     // name
}, {
	AUTO_INCREMENT_VALUE,  // id
	4,                     // serverId
	"2",                   // idInServer
	"Watched Servers"      // name
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,  // id
	trigInfoDefunctSv1.serverId, // serverId
	"1",                   // idInServer
	"Hostgroup on a defunct servers" // name
}
};
const size_t NumTestHostgroup = ARRAY_SIZE(testHostgroup);

const HostgroupMember testHostgroupMember[] = {
{
	AUTO_INCREMENT_VALUE,            // id
	1,                               // serverId
	"235012",                        // hostIdInServer
	"1",                             // hostgroupIdInServer
	10,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	1,                               // serverId
	"235012",                        // hostIdInServer
	"2",                             // hostgroupIdInServer
	10,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	1,                               // serverId
	"235013",                        // hostIdInServer
	"2",                             // hostgroupIdInServer
	11,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	1,                               // serverId
	"1129",                          // hostIdInServer
	"1",                             // hostgroupIdInServer
	30,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	2,                               // serverId
	"512",                           // hostIdInServer
	"1",                             // hostgroupIdInServer
	40,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	2,                               // serverId
	"512",                           // hostIdInServer
	"2",                             // hostgroupIdInServer
	40,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	3,                               // serverId
	"10001",                         // hostIdInServer
	"2",                             // hostgroupIdInServer
	35,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	3,                               // serverId
	"10002",                         // hostIdInServer
	"1",                             // hostgroupIdInServer
	41,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	3,                               // serverId
	"5",                             // hostIdInServer
	"1",                             // hostgroupIdInServer
	42,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	3,                               // serverId
	"100",                           // hostIdInServer
	"2",                             // hostgroupIdInServer
	45,                              // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	4,                               // serverId
	"100",                           // hostIdInServer
	"1",                             // hostgroupIdInServer
	100,                             // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	2,                               // serverId
	"9920249034889494527", //"0x89abcdefffffffff",  // hostIdInServer
	"9223372036854775808", // "0x8000000000000000", // hostgroupIdInServer
	101,                             // hostId
}, {
	// This entry is for tests with a defunct server
	AUTO_INCREMENT_VALUE,            // id
	trigInfoDefunctSv1.serverId,     // serverId
	// trigInfoDefunctSv1.hostIdInServer, // hostId,
	"10002", // TODO: use the above after host ID in trigger becomes string
	"1",                             // hostgroupIdInServer
	494,                             // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	211,                             // serverId
	"200",                           // hostIdInServer
	"0",                             // hostgroupIdInServer
	1050,                            // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	211,                             // serverId
	"12111",                         // hostIdInServer
	"123",                           // hostgroupIdInServer
	2111,                            // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	211,                             // serverId
	"12112",                         // hostIdInServer
	"123",                           // hostgroupIdInServer
	2112,                            // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	211,                             // serverId
	"12113",                         // hostIdInServer
	"124",                           // hostgroupIdInServer
	2113,                            // hostId
}, {
	AUTO_INCREMENT_VALUE,            // id
	222,                             // serverId
	"110005",                        // hostIdInServer
	"124",                           // hostgroupIdInServer
	10005,                           // hostId
}
};
const size_t NumTestHostgroupMember = ARRAY_SIZE(testHostgroupMember);

const LastInfoDef testLastInfoDef[] = {
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_HOST,                  // dataType
	"1431228840",                    // value
	10,                              // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_HOST,                  // dataType
	"1431232440",                    // value
	11,                              // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_HOST_GROUP,            // dataType
	"1431221640",                    // value
	1001,                            // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_HOST_GROUP_MEMBERSHIP, // dataType
	"1431567240",                    // value
	1002,                            // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_TRIGGER,               // dataType
	"1431671640",                    // value
	1003,                            // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_EVENT,                 // dataType
	"1431585240",                    // value
	10001,                           // serverId
},
{
	AUTO_INCREMENT_VALUE,            // id
	LAST_INFO_HOST_PARENT,           // dataType
	"1431930840",                    // value
	10002,                           // serverId
},
};
const size_t NumTestLastInfoDef = ARRAY_SIZE(testLastInfoDef);

const SeverityRankInfo testSeverityRankInfoDef[] = {
{
	AUTO_INCREMENT_VALUE,      // id
	TRIGGER_SEVERITY_UNKNOWN,  // status
	"#BCBCBC",                 // color
	"Not classified",          // label
	false                      // asImportant
},
{
	AUTO_INCREMENT_VALUE,      // id
	TRIGGER_SEVERITY_INFO,     // status
	"#CCE2CC",                 // color
	"Information",             // label
	false                      // asImportant
},
{
	AUTO_INCREMENT_VALUE,      // id
	TRIGGER_SEVERITY_WARNING,  // status
	"#FDFD96",                 // color
	"Warning",                 // label
	false                      // asImportant
},
{
	AUTO_INCREMENT_VALUE,      // id
	TRIGGER_SEVERITY_ERROR,    // status
	"#DDAAAA",                 // color
	"Error",                   // label
	true                       // asImportant
},
{
	AUTO_INCREMENT_VALUE,      // id
	TRIGGER_SEVERITY_CRITICAL, // status
	"#FF8888",                 // color
	"Critical",                // label
	true                       // asImportant
},
{
	AUTO_INCREMENT_VALUE,       // id
	TRIGGER_SEVERITY_EMERGENCY, // status
	"#FF0000",                   // color
	"Emergency",                // label
	true                        // asImportant
},
};
const size_t NumTestSeverityRankInfoDef = ARRAY_SIZE(testSeverityRankInfoDef);

const CustomIncidentStatus testCustomIncidentStatus[] = {
{
	AUTO_INCREMENT_VALUE,     // id
	"DONE",                   // code
	"Done",                   // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"IN PROGRESS",            // code
	"In progress",            // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"HOLD",                   // code
	"Hold",                   // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"NONE",                   // code
	"None",                   // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER01",                 // code
	"User defined 01",        // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER02",                 // code
	"User defined 02",        // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER03",                 // code
	"User defined 03",        // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER04",                 // code
	"User defined 04",        // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER05",                 // code
	"User defined 05",        // label
},
{
	AUTO_INCREMENT_VALUE,     // id
	"USER06",                 // code
	"User defined 06",        // label
},
};
const size_t NumTestCustomIncidentStatus = ARRAY_SIZE(testCustomIncidentStatus);

const IncidentHistory testIncidentHistory[] = {
{
	AUTO_INCREMENT_VALUE,  // id
	3,                     // unifiedId
	1,                     // userId
	"NONE",                // status
	"",                    // comment
	{1412957260, 0},       // createdAt
},
{
	AUTO_INCREMENT_VALUE,  // id
	4,                     // unifiedId
	2,                     // userId
	"IN PROGRESS",         // status
	"This is a comment.",  // comment
	{1412957290, 0},       // createdAt
},
{
	AUTO_INCREMENT_VALUE,  // id
	123,                   // unifiedId
	3,                     // userId
	"HOLD",                // status
	"Hold to operate.",    // comment
	{1453451165, 0},       // createdAt
},
};
const size_t NumTestIncidentHistory = ARRAY_SIZE(testIncidentHistory);

const TriggerInfo &searchTestTriggerInfo(const EventInfo &eventInfo)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (trigInfo.serverId != eventInfo.serverId)
			continue;
		if (trigInfo.id != eventInfo.triggerId)
			continue;
		return trigInfo;
	}
	cut_fail("Not found: server ID: %u, trigger ID: %" FMT_TRIGGER_ID,
	         eventInfo.serverId, eventInfo.triggerId.c_str());
	return *(new TriggerInfo()); // never exectuted, just to pass build
}

SmartTime getTimestampOfLastTestTrigger(const ServerIdType &serverId)
{
	const TriggerInfo *lastTimeTrigInfo = NULL;
	SmartTime lastTimestamp;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (trigInfo.serverId != serverId)
			continue;
		SmartTime timestamp = SmartTime(trigInfo.lastChangeTime);
		if (!lastTimeTrigInfo || timestamp > lastTimestamp) {
			lastTimestamp = timestamp;
			lastTimeTrigInfo = &trigInfo;
		}
	}
	cppcut_assert_not_null(
	  lastTimeTrigInfo,
	  cut_message(
	    "No test triggers with ServerID: %" FMT_SERVER_ID, serverId));
	return lastTimestamp;
}

EventIdType findLastEventId(const ServerIdType &serverId)
{
	bool found = false;
	uint64_t maxId = 0;
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		const EventInfo &eventInfo = testEventInfo[i];
		if (eventInfo.serverId != serverId)
			continue;
		uint64_t eventId;
		Utils::conv(eventId, eventInfo.id);
		if (eventId >= maxId) {
			maxId = eventId;
			found = true;
		}
	}
	if (!found)
		return EVENT_NOT_FOUND;
	return to_string(maxId);
}

SmartTime findTimeOfLastEvent(
  const ServerIdType &serverId, const TriggerIdType &triggerId)
{
	uint64_t maxId = 0;
	const timespec *lastTime = NULL;
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		const EventInfo &eventInfo = testEventInfo[i];
		if (eventInfo.serverId != serverId)
			continue;
		if (triggerId != ALL_TRIGGERS &&
		    eventInfo.triggerId != triggerId)
			continue;
		uint64_t eventId;
		Utils::conv(eventId, eventInfo.id);
		if (eventId >= maxId) {
			maxId = eventId;
			lastTime = &eventInfo.time;
		}
	}
	if (!lastTime)
		return SmartTime();
	return SmartTime(*lastTime);
}

void getTestTriggersIndexes(
  ServerIdTriggerIdIdxMap &indexMap,
  const ServerIdType &serverId, const LocalHostIdType &hostIdInServer)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &trigInfo = testTriggerInfo[i];
		if (serverId != ALL_SERVERS) {
			if (trigInfo.serverId != serverId)
				continue;
		}
		if (hostIdInServer != ALL_LOCAL_HOSTS) {
			if (trigInfo.hostIdInServer != hostIdInServer)
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

const ItemInfo *findTestItem(
  const ServerIdItemInfoIdIndexMapMap &indexMap,
  const ServerIdType &serverId, const ItemIdType &itemId)
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
  const string &hostId, const string &hostgroupId)
{
	string s;
	s.append((char *)&serverId, sizeof(serverId));
	s += ":";
	s += hostId;
	s += ":";
	s += hostgroupId;
	return s;
}

/**
 * This functions return a kind of a perfect hash set that is made from
 * a server Id, a host Id, and a host group ID. We call it
 * 'HostGroupElementPack'.
 *
 * @return a set of HostGroupElementPack.
 */
static const set<string> &getHostgroupMemberPackSet(void)
{
	static set<string> hostHGrpPackSet;
	if (!hostHGrpPackSet.empty())
		return hostHGrpPackSet;
	for (size_t i = 0; i < NumTestHostgroupMember; i++) {
		const HostgroupMember &hhgr = testHostgroupMember[i];
		const string mash =
		  makeHostgroupElementPack(
		    hhgr.serverId, hhgr.hostIdInServer,
		    hhgr.hostgroupIdInServer);
		pair<set<string>::iterator, bool> result =
		  hostHGrpPackSet.insert(mash);
		cppcut_assert_equal(true, result.second);
	}
	return hostHGrpPackSet;
}

static bool isInHostgroup(const TriggerInfo &trigInfo,
                          const HostgroupIdType &hostgroupId)
{
	if (hostgroupId == ALL_HOST_GROUPS)
		return true;

	const set<string> &hostgroupElementPackSet =
	  getHostgroupMemberPackSet();

	const string pack = makeHostgroupElementPack(trigInfo.serverId,
	                                             trigInfo.hostIdInServer,
	                                             hostgroupId.c_str());
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
			if (trigInfo.severity != severity &&
			    severity != TRIGGER_SEVERITY_ALL)
				continue;
			if (trigInfo.status == TRIGGER_STATUS_OK)
				continue;
			if (trigInfo.status == TRIGGER_STATUS_UNKNOWN)
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
                                 const HostgroupIdType &hostGrpIdForTrig,
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
	hostIdSet.erase(trigInfo.globalHostId);
}

size_t getNumberOfTestHosts(
  const ServerIdType &serverId, const HostgroupIdType &hostgroupId)
{
	HATOHOL_ASSERT(hostgroupId == ALL_HOST_GROUPS,
	  "Not implemented the feature to take care host groups: "
	  "hostgroupID: %" FMT_HOST_GROUP_ID ".", hostgroupId.c_str());

	size_t numberOfTestHosts = 0;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &svHostDef = testServerHostDef[i];
		const ServerIdType &hostInfoServerId = svHostDef.serverId;
		if (hostInfoServerId == serverId)
			numberOfTestHosts++;
		// TODO: compare with hostgroup ID.
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
		if (!isAuthorized(authMap, userId, serverId,
		                  trigInfo.hostIdInServer))
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
			hostIdSet.insert(trigInfo.globalHostId);
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
			hostIdSet.insert(trigInfo.globalHostId);
			hostGrpIdMap[hostgroupId] = hostIdSet;
			continue;
		}

		HostIdSet &hostIdSet = hostIt->second;
		// Even when hostIdSet already has an element with
		// trigInfo.hostId, insert() just fails and doesn't
		// cause other side effects.
		// This behavior is no problem for this function and we
		// can skip the check of the existence in the set.
		hostIdSet.insert(trigInfo.globalHostId);
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

bool filterOutAction(const ActionDef &actionDef, const ActionType &targetType)
{
	if (targetType == ACTION_ALL)
		return false;

	if (targetType == ACTION_USER_DEFINED) {
		if (actionDef.type == ACTION_INCIDENT_SENDER)
			return true;
		else
			return false;
	}

	if (targetType >= NUM_ACTION_TYPES)
		cut_fail("Invalid action type: %d\n", targetType);

	if (actionDef.type != targetType)
		return true;

	return false;
}

size_t getNumberOfTestActions(const ActionType &actionType)
{
	size_t num = 0;
	for (size_t i = 0; i < NumTestActionDef; ++i) {
		if (!filterOutAction(testActionDef[i], actionType))
			++num;
	}
	return num;
}

void makeTestUserIdIndexMap(UserIdIndexMap &userIdIndexMap)
{

	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		// ****
		//AccessInfo &accessInfo = testAccessInfo[i];
		const AccessInfo accessInfo = testAccessInfo[i];
		userIdIndexMap[accessInfo.userId].insert(i);
	}
}

void makeServerAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
			     UserIdType userId)
{
	for (size_t i = 0; i < NumTestAccessInfo; ++i) {
		const AccessInfo *accessInfo = &testAccessInfo[i];
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
  const ServerIdType &serverId, const LocalHostIdType &hostIdInServer,
  const set<string> *hgrpElementPackSet)
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

	if (hostIdInServer == ALL_LOCAL_HOSTS)
		return true;

	const HostgroupIdSet &hostgroupIds = serverIt->second;
	if (hostgroupIds.find(ALL_HOST_GROUPS) != hostgroupIds.end())
		return true;

	// check if the user is allowed to access to the host
	if (!hgrpElementPackSet)
		hgrpElementPackSet = &getHostgroupMemberPackSet();
	HostgroupIdSetConstIterator hostgroupIdItr = hostgroupIds.begin();
	for (; hostgroupIdItr != hostgroupIds.end(); ++hostgroupIdItr) {
		const string pack =
		  makeHostgroupElementPack(serverId,
		                           hostIdInServer, *hostgroupIdItr);
		if (hgrpElementPackSet->find(pack) != hgrpElementPackSet->end())
			return true;
	}

	return false;
}

bool isAuthorized(
  const UserIdType &userId, const HostIdType &hostId)
{
	vector<ServerIdType> serverIds;
	vector<LocalHostIdType> hostIds;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef svHostDef = testServerHostDef[i];
		if (svHostDef.hostId != hostId)
			continue;
		serverIds.push_back(svHostDef.serverId);
		hostIds.push_back(svHostDef.hostIdInServer);
	}
	if (serverIds.empty())
		return false;

	ServerHostGrpSetMap authMap;
	makeServerHostGrpSetMap(authMap, userId);
	cppcut_assert_equal(serverIds.size(), hostIds.size());
	const set<string> &hostHGrpPackSet = getHostgroupMemberPackSet();
	for (size_t i = 0; i < serverIds.size(); i++) {
		if (isAuthorized(authMap, userId, serverIds[i], hostIds[i],
		                 &hostHGrpPackSet))
			return true;
	}

	return false;
}

bool isDefunctTestServer(const ServerIdType &serverId)
{
	static ServerIdSet svIdSet;
	if (svIdSet.empty()) {
		for (size_t i = 0; i < NumTestServerInfo; i++)
			svIdSet.insert(testServerInfo[i].id);
	}
	return svIdSet.find(serverId) == svIdSet.end();
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
	              "by user: %" FMT_USER_ID, userId));
	return idx;
}

size_t findIndexFromTestActionDef(const ActionType &type)
{
	size_t idx = 0;
	for (; idx < NumTestActionDef; idx++) {
		const ActionDef &actDef = testActionDef[idx];
		if (actDef.type == type)
			break;
	}
	cppcut_assert_not_equal(
	  NumTestActionDef, idx,
	  cut_message("Not found a testActionDef entry with type: %d",
	              type));
	return idx;
}

const HostgroupIdSet &getTestHostgroupIdSet(void)
{
	static HostgroupIdSet testHostgroupIdSet;
	if (!testHostgroupIdSet.empty())
		return testHostgroupIdSet;

	for (size_t i = 0; i < NumTestHostgroupMember; i++) {
		testHostgroupIdSet.insert(
		  testHostgroupMember[i].hostgroupIdInServer);
	}
	return testHostgroupIdSet;
}

ArmPluginInfo *getTestArmPluginInfo(void)
{
	static ArmPluginInfo *data = NULL;

	if (data)
		return data;
	data = new ArmPluginInfo[NumTestArmPluginInfo];

	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		data[i] = testArmPluginInfo[i];
		if (data[i].path.empty())
			continue;
		if (data[i].path[0] == '/')
			continue;
		data[i].path = cut_build_path(getBaseDir().c_str(),
					      data[i].path.c_str(),
					      NULL);
	}
	return data;
}

ArmPluginInfo createCompanionArmPluginInfo(const MonitoringServerInfo &svInfo)
{
	const int armPluginIdx = findIndexOfTestArmPluginInfo(svInfo.id);
	cppcut_assert_not_equal(-1, armPluginIdx);

	ArmPluginInfo armInfo = testArmPluginInfo[0];
	armInfo.id = armPluginIdx + 1;
	armInfo.serverId = testArmPluginInfo[armPluginIdx].serverId;
	return armInfo;
}

const ArmPluginInfo *findTestArmPluginInfo(const ServerIdType &serverId)
{
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		const ArmPluginInfo *pluginInfo = &testArmPluginInfo[i];
		if (pluginInfo->serverId == serverId)
			return pluginInfo;
	}
	return NULL;
}

const ArmPluginInfo *findTestArmPluginInfoWithStaticQueueAddress(
  const string &queueAddress)
{
	for (const auto &pluginInfo: testArmPluginInfo) {
		if (pluginInfo.staticQueueAddress == queueAddress)
			return &pluginInfo;
	}
	return NULL;
}

int findIndexOfTestArmPluginInfo(const MonitoringSystemType &type)
{
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		if (getTestArmPluginInfo()[i].type == type)
			return i;
	}
	return -1;
}

int findIndexOfTestArmPluginInfo(const ServerIdType &serverId)
{
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		if (getTestArmPluginInfo()[i].serverId == serverId)
			return i;
	}
	return -1;
}

const ArmPluginInfo &getTestArmPluginInfo(const MonitoringSystemType &type)
{
	const int testArmPluginIndex = findIndexOfTestArmPluginInfo(type);
	cppcut_assert_not_equal(-1, testArmPluginIndex);
	return getTestArmPluginInfo()[testArmPluginIndex];
}

string makeEventIncidentMapKey(const EventInfo &eventInfo)
{
	return StringUtils::sprintf("%" FMT_SERVER_ID ":%" FMT_EVENT_ID,
				    eventInfo.serverId, eventInfo.id.c_str());
}

void makeEventIncidentMap(map<string, const IncidentInfo*> &eventIncidentMap)
{
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		string key = StringUtils::sprintf(
			       "%" FMT_SERVER_ID ":%" FMT_EVENT_ID,
			       testIncidentInfo[i].serverId,
			       testIncidentInfo[i].eventId.c_str());
		eventIncidentMap[key] = &testIncidentInfo[i];
	}
}

void loadHostInfoCache(
  HostInfoCache &hostInfoCache, const ServerIdType &serverId)
{
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &svHostDef = testServerHostDef[i];
		if (svHostDef.serverId != serverId)
			continue;
		hostInfoCache.update(svHostDef);
	}
}

IncidentTrackerIdType findIncidentTrackerIdByType(IncidentTrackerType type)
{
	for (auto &tracker : testIncidentTrackerInfo) {
		if (tracker.type == type)
			return tracker.id;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Setup methods
// ---------------------------------------------------------------------------
const char *TEST_DB_USER = "hatohol_test_user";
const char *TEST_DB_PASSWORD = ""; // empty: No password is used
const char *TEST_DB_NAME = "test_db_hatohol";

static void deleteFileAndCheck(const string &path)
{
	unlink(path.c_str());
	cut_assert_not_exist_path(path.c_str());
}

static string deleteDBClientHatoholDB(void)
{
	string dbPath = getDBPathForDBClientHatohol();
	deleteFileAndCheck(dbPath);
	return dbPath;
}

void setupTestDB(void)
{
	DBHatohol::setDefaultDBParams(TEST_DB_NAME,
	                              TEST_DB_USER, TEST_DB_PASSWORD);
	const bool dbRecreate = true;
	makeTestMySQLDBIfNeeded(TEST_DB_NAME, dbRecreate);

	// Only when we use SQLite3 for DBTablesHatoho,
	// the following line should be enabled.
	const bool dbHatoholUseSQLite3 = false;
	if (dbHatoholUseSQLite3)
		deleteDBClientHatoholDB();
}

void loadTestDBTablesConfig(void)
{
	loadTestDBServer();
	loadTestDBIncidentTracker();
}

void loadTestDBTablesUser(void)
{
	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBUserRole();
}

void loadTestDBServerType(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	for (size_t i = 0; i < NumTestServerTypeInfo; i++)
		dbConfig.registerServerType(testServerTypeInfo[i]);
}

void loadTestDBServer(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	cppcut_assert_equal(NumTestServerInfo, NumTestArmPluginInfo);
	for (size_t i = 0; i < NumTestServerInfo; i++) {
		// We have to make a copy since addTargetServer() changes
		// a member of MonitoringServerInfo.
		MonitoringServerInfo svInfo = testServerInfo[i];
		ArmPluginInfo armInfo = testArmPluginInfo[i];
		dbConfig.addTargetServer(svInfo, armInfo, privilege);
	}
}

void loadTestDBUser(void)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	HatoholError err;
	OperationPrivilege opePrivilege(OperationPrivilege::ALL_PRIVILEGES);
	for (auto &userInfo : bareTestUserInfo) {
		err = dbUser.addUserInfo(userInfo, opePrivilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBUserRole(void)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	HatoholError err;
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (auto userRoleInfo : testUserRoleInfo) {
		err = dbUser.addUserRoleInfo(userRoleInfo, privilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBAccessList(void)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	HatoholError err;
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (auto &accessInfo : bareTestAccessInfo) {
		err = dbUser.addAccessInfo(accessInfo, privilege);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBArmPlugin(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	for (size_t i = 0; i < NumTestArmPluginInfo; i++) {
		// Make a copy since armPluginInfo.id will be set.
		ArmPluginInfo armPluginInfo = getTestArmPluginInfo()[i];
		HatoholError err = dbConfig.saveArmPluginInfo(armPluginInfo);
		assertHatoholError(HTERR_OK, err);
	}
}

void loadTestDBTriggers(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		dbMonitoring.addTriggerInfo(&testTriggerInfo[i]);
}

void loadTestDBEvents(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		// Make a copy since EventInfo.id will be changed.
		EventInfo evtInfo = testEventInfo[i];
		dbMonitoring.addEventInfo(&evtInfo);
	}
}

void loadTestDBItems(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestItemInfo; i++)
		dbMonitoring.addItemInfo(&testItemInfo[i]);
}

void loadTestDBAction(void)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (size_t i = 0; i < NumTestActionDef; i++)
		dbAction.addAction(bareTestActionDef[i], privilege);
}

void loadTestDBServerStatus(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	for (size_t i = 0; i < NumTestServerStatus; i++)
		dbMonitoring.addMonitoringServerStatus(testServerStatus[i]);
}

void loadTestDBIncidents(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		IncidentInfo incidentInfo = testIncidentInfo[i];
		dbMonitoring.addIncidentInfo(&incidentInfo);
	}
}

void loadTestDBIncidentTracker(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (auto incidentTrackerInfo : testIncidentTrackerInfo)
		dbConfig.addIncidentTracker(incidentTrackerInfo, privilege);
}

void getTestHistory(HistoryInfoVect &historyInfoVect,
		    const ServerIdType &serverId,
		    const ItemIdType itemId,
		    const time_t &beginTime, const time_t &endTime)
{
	for (size_t i = 0; i < NumTestHistoryInfo; i++) {
		const HistoryInfo &historyInfo = testHistoryInfo[i];
		if (serverId != ALL_SERVERS &&
		    historyInfo.serverId != serverId) {
			continue;
		}
		if (historyInfo.itemId != itemId)
			continue;
		if (historyInfo.clock.tv_sec < beginTime)
			continue;
		if (historyInfo.clock.tv_sec > endTime ||
		    (historyInfo.clock.tv_sec == endTime &&
		     historyInfo.clock.tv_nsec > 0)) {
			continue;
		}
		historyInfoVect.push_back(historyInfo);
	}
}

void loadTestDBServerHostDef(void)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestServerHostDef; i++)
		dbHost.upsertServerHostDef(testServerHostDef[i]);
}

void loadTestDBVMInfo(void)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestVMInfo; i++)
		dbHost.upsertVMInfo(testVMInfo[i]);
}

void loadTestDBHostgroup(void)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestHostgroup; i++)
		dbHost.upsertHostgroup(testHostgroup[i]);
}

void loadTestDBHostgroupMember(void)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	OperationPrivilege privilege(OperationPrivilege::ALL_PRIVILEGES);
	for (size_t i = 0; i < NumTestHostgroupMember; i++)
		dbHost.upsertHostgroupMember(testHostgroupMember[i]);
}

void loadTestDBLastInfo(void)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (auto lastInfoDef : testLastInfoDef)
		dbLastInfo.upsertLastInfo(lastInfoDef, privilege);
}

void loadTestDBSeverityRankInfo(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (auto severityRankInfo : testSeverityRankInfoDef)
		dbConfig.upsertSeverityRankInfo(severityRankInfo, privilege);
}

void loadTestDBCustomIncidentStatusInfo(void)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (auto customIncidentStatus : testCustomIncidentStatus)
		dbConfig.upsertCustomIncidentStatus(customIncidentStatus, privilege);
}

void loadTestDBIncidentHistory(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (auto incidentHistory : testIncidentHistory)
		dbMonitoring.addIncidentHistory(incidentHistory);
}
