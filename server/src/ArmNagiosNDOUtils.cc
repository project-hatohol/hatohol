/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <time.h>
#include "ArmNagiosNDOUtils.h"
#include "DBAgentMySQL.h"
#include "Utils.h"
#include "UnifiedDataStore.h"
#include "ItemGroupStream.h"
#include "DBClientJoinBuilder.h"
#include "SQLUtils.h"
#include "HatoholException.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_SERVICES      = "nagios_services";
static const char *TABLE_NAME_SERVICESTATUS = "nagios_servicestatus";
static const char *TABLE_NAME_HOSTS         = "nagios_hosts";
static const char *TABLE_NAME_STATEHISTORY  = "nagios_statehistory";
static const char *TABLE_NAME_HOSTGROUPS    = "nagios_hostgroups";
static const char *TABLE_NAME_HOSTGROUP_MEMBERS = "nagios_hostgroup_members";

enum
{
	STATE_OK       = 0,
	STATE_WARNING  = 1,
	STATE_CRITICAL = 2,
};

// The explanation of soft and hard state can be found in
// http://nagios.sourceforge.net/docs/3_0/statetypes.html
enum
{
	SOFT_STATE = 0,
	HARD_STATE = 1,
};

// [NOTE]
//   The following columns are listed only for used ones.
//   (i.e. some unused columns are omitted)
//   Different from DBClientXXXX, this class only reads the DB content.
//   So full definition is not necessary.

// Definitions: nagios_services
static const ColumnDef COLUMN_DEF_SERVICES[] = {
{
	"service_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_object_id",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"service_object_id",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
},
};

enum {
	IDX_SERVICES_SERVICE_ID,
	IDX_SERVICES_HOST_OBJECT_ID,
	IDX_SERVICES_SERVICE_OBJECT_ID,
	NUM_IDX_SERVICES,
};

static const DBAgent::TableProfile tableProfileServices =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SERVICES,
			    COLUMN_DEF_SERVICES,
			    NUM_IDX_SERVICES);

// Definitions: nagios_servicestatus
static const ColumnDef COLUMN_DEF_SERVICESTATUS[] = {
{
	"service_object_id",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"status_update_time",              // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"output",                          // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	"current_state",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	6,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"check_command",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
},
};

enum {
	IDX_SERVICESTATUS_SERVICE_OBJECT_ID,
	IDX_SERVICESTATUS_STATUS_UPDATE_TIME,
	IDX_SERVICESTATUS_OUTPUT,
	IDX_SERVICESTATUS_CURRENT_STATE,
	IDX_SERVICESTATUS_CHECK_COMMAND,
	NUM_IDX_SERVICESTATUS,
};

static const DBAgent::TableProfile tableProfileServiceStatus =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SERVICESTATUS,
			    COLUMN_DEF_SERVICESTATUS,
			    NUM_IDX_SERVICESTATUS);

// Definitions: nagios_hosts
static const ColumnDef COLUMN_DEF_HOSTS[] = {
{
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_object_id",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"display_name",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
},
};

enum {
	IDX_HOSTS_HOST_ID,
	IDX_HOSTS_HOST_OBJECT_ID,
	IDX_HOSTS_DISPLAY_NAME,
	NUM_IDX_HOSTS,
};

static const DBAgent::TableProfile tableProfileHosts =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTS,
			    COLUMN_DEF_HOSTS,
			    NUM_IDX_HOSTS);

// Definitions: nagios_hostgroups
static const ColumnDef COLUMN_DEF_HOSTGROUPS[] = {
{
	"hostgroup_id",                         // columnName
	SQL_COLUMN_TYPE_INT,                    // type
	11,                                     // columnLength
	0,                                      // decFracLength
	false,                                  // canBeNull
	SQL_KEY_PRI,                            // keyType
	0,                                      // flags
	NULL,                                   // defaultValue
}, {
	"alias",                                // columnName
	SQL_COLUMN_TYPE_VARCHAR,                // type
	255,                                    // columnLength
	0,                                      // decFracLength
	false,                                  // canBeNull
	SQL_KEY_NONE,                           // keyType
	0,                                      // flags
	NULL,                                   // defaultValue
},
};

enum {
	IDX_HOSTGROUPS_HOSTGROUP_ID,
	IDX_HOSTGROUPS_ALIAS,
	NUM_IDX_HOSTGROUPS,
};

static const DBAgent::TableProfile tableProfileHostgroups =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTGROUPS,
			    COLUMN_DEF_HOSTGROUPS,
			    NUM_IDX_HOSTGROUPS);

// Definitions: nagios_hostgroup_members
static const ColumnDef COLUMN_DEF_HOSTGROUP_MEMBERS[] = {
{
	"host_object_id",                                // columnName
	SQL_COLUMN_TYPE_INT,                             // type
	11,                                              // columnLength
	0,                                               // decFracLength
	false,                                           // canBeNull
	SQL_KEY_NONE,                                    // keyType
	0,                                               // flags
	NULL,                                            // defaultValue
}, {
	"hostgroup_id",                                  // columnName
	SQL_COLUMN_TYPE_INT,                             // type
	11,                                              // columnLength
	0,                                               // decFracLength
	false,                                           // canBeNull
	SQL_KEY_NONE,                                    // keyType
	0,                                               // flags
	NULL,                                            // defaultValue
},
};

enum {
	IDX_HOSTGROUP_MEMBERS_HOST_OBJECT_ID,
	IDX_HOSTGROUP_MEMBERS_HOSTGROUP_ID,
	NUM_IDX_HOSTGROUP_MEMBERS,
};

static const DBAgent::TableProfile tableProfileHostgroupMembers =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_HOSTGROUP_MEMBERS,
			    COLUMN_DEF_HOSTGROUP_MEMBERS,
			    NUM_IDX_HOSTGROUP_MEMBERS);

// Definitions: nagios_statehistory
static const ColumnDef COLUMN_DEF_STATEHISTORY[] = {
{
	"statehistory_id",                 // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"state_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"object_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"state",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"state_type",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"output",                          // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                               // defaultValue
},
};

enum {
	IDX_STATEHISTORY_STATEHISTORY_ID,
	IDX_STATEHISTORY_STATE_TIME,
	IDX_STATEHISTORY_OBJECT_ID,
	IDX_STATEHISTORY_STATE,
	IDX_STATEHISTORY_STATE_TYPE,
	IDX_STATEHISTORY_OUTPUT,
	NUM_IDX_STATEHISTORY,
};

static const DBAgent::TableProfile tableProfileStateHistory =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_STATEHISTORY,
			    COLUMN_DEF_STATEHISTORY,
			    NUM_IDX_STATEHISTORY);

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmNagiosNDOUtils::Impl
{
	DBAgentMySQL        *dbAgent;
	DBClientJoinBuilder  selectTriggerBuilder;
	DBClientJoinBuilder  selectEventBuilder;
	DBClientJoinBuilder  selectItemBuilder;
	DBAgent::SelectExArg selectHostArg;
	DBAgent::SelectExArg selectHostgroupArg;
	DBAgent::SelectExArg selectHostgroupMembersArg;
	string               selectTriggerBaseCondition;
	string               selectEventBaseCondition;
	UnifiedDataStore    *dataStore;
	MonitoringServerInfo serverInfo;

	// methods
	Impl(const MonitoringServerInfo &_serverInfo)
	: dbAgent(NULL),
	  selectTriggerBuilder(tableProfileServices),
	  selectEventBuilder(tableProfileStateHistory),
	  selectItemBuilder(tableProfileServices),
	  selectHostArg(tableProfileHosts),
	  selectHostgroupArg(tableProfileHostgroups),
	  selectHostgroupMembersArg(tableProfileHostgroupMembers),
	  dataStore(NULL),
	  serverInfo(_serverInfo)
	{
		dataStore = UnifiedDataStore::getInstance();
	}

	virtual ~Impl()
	{
		delete dbAgent;
	}

	void connect(void)
	{
		HATOHOL_ASSERT(!dbAgent, "dbAgent is NOT NULL.");
		dbAgent = new DBAgentMySQL(
		  serverInfo.dbName.c_str(), serverInfo.userName.c_str(),
		  serverInfo.password.c_str(),
		  serverInfo.getHostAddress().c_str(), serverInfo.port);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmNagiosNDOUtils::ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo)
: ArmBase("ArmNagiosNDOUtils", serverInfo),
  m_impl(new Impl(serverInfo))
{
	makeSelectTriggerBuilder();
	makeSelectEventBuilder();
	makeSelectItemBuilder();
	makeSelectHostArg();
	makeSelectHostgroupArg();
	makeSelectHostgroupMembersArg();
}

ArmNagiosNDOUtils::~ArmNagiosNDOUtils()
{
	requestExitAndWait();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ArmNagiosNDOUtils::makeSelectTriggerBuilder(void)
{
	// TODO: Confirm what may be use using host_object_id.
	DBClientJoinBuilder &builder = m_impl->selectTriggerBuilder;
	builder.add(IDX_SERVICES_SERVICE_ID);

	builder.addTable(
	  tableProfileServiceStatus, DBClientJoinBuilder::INNER_JOIN,
	  IDX_SERVICES_SERVICE_OBJECT_ID, IDX_SERVICESTATUS_SERVICE_OBJECT_ID);
	builder.add(IDX_SERVICESTATUS_CURRENT_STATE);
	builder.add(IDX_SERVICESTATUS_STATUS_UPDATE_TIME);
	builder.add(IDX_SERVICESTATUS_OUTPUT);

	builder.addTable(
	  tableProfileHosts, DBClientJoinBuilder::INNER_JOIN,
	  tableProfileServices, IDX_SERVICES_HOST_OBJECT_ID,
	  IDX_HOSTS_HOST_OBJECT_ID);
	builder.add(IDX_HOSTS_HOST_OBJECT_ID);
	builder.add(IDX_HOSTS_DISPLAY_NAME);

	// contiditon
	m_impl->selectTriggerBaseCondition = StringUtils::sprintf(
	  "%s>=",
	  tableProfileServiceStatus.getFullColumnName(IDX_SERVICESTATUS_STATUS_UPDATE_TIME).c_str());
}

void ArmNagiosNDOUtils::makeSelectEventBuilder(void)
{
	// TODO: Confirm what may be use using host_object_id.
	DBClientJoinBuilder &builder = m_impl->selectEventBuilder;
	builder.add(IDX_STATEHISTORY_STATEHISTORY_ID);
	builder.add(IDX_STATEHISTORY_STATE);
	builder.add(IDX_STATEHISTORY_STATE_TIME);
	builder.add(IDX_STATEHISTORY_OUTPUT);

	builder.addTable(
	  tableProfileServices, DBClientJoinBuilder::INNER_JOIN,
	  IDX_STATEHISTORY_OBJECT_ID, IDX_SERVICES_SERVICE_OBJECT_ID);
	builder.add(IDX_SERVICES_SERVICE_ID);

	builder.addTable(
	  tableProfileHosts, DBClientJoinBuilder::INNER_JOIN,
	  IDX_SERVICES_HOST_OBJECT_ID, IDX_HOSTS_HOST_OBJECT_ID);
	builder.add(IDX_HOSTS_HOST_OBJECT_ID);
	builder.add(IDX_HOSTS_DISPLAY_NAME);

	// contiditon
	m_impl->selectEventBaseCondition = StringUtils::sprintf(
	  "%s=%d and %s>=",
	  tableProfileStateHistory.getFullColumnName(IDX_STATEHISTORY_STATE_TYPE).c_str(),
	  HARD_STATE,
	  tableProfileStateHistory.getFullColumnName(IDX_STATEHISTORY_STATEHISTORY_ID).c_str());
}

void ArmNagiosNDOUtils::makeSelectItemBuilder(void)
{
	DBClientJoinBuilder &builder = m_impl->selectItemBuilder;
	builder.add(IDX_SERVICES_SERVICE_ID);
	builder.add(IDX_SERVICES_HOST_OBJECT_ID);

	builder.addTable(
	  tableProfileServiceStatus, DBClientJoinBuilder::INNER_JOIN,
	  IDX_SERVICES_SERVICE_OBJECT_ID, IDX_SERVICESTATUS_SERVICE_OBJECT_ID);
	builder.add(IDX_SERVICESTATUS_CHECK_COMMAND);
	builder.add(IDX_SERVICESTATUS_STATUS_UPDATE_TIME);
	builder.add(IDX_SERVICESTATUS_OUTPUT);
}

void ArmNagiosNDOUtils::makeSelectHostArg(void)
{
	DBAgent::SelectExArg &arg = m_impl->selectHostArg;
	arg.add(IDX_HOSTS_HOST_OBJECT_ID);
	arg.add(IDX_HOSTS_DISPLAY_NAME);
}

void ArmNagiosNDOUtils::makeSelectHostgroupArg(void)
{
	DBAgent::SelectExArg &arg = m_impl->selectHostgroupArg;
	arg.add(IDX_HOSTGROUPS_HOSTGROUP_ID);
	arg.add(IDX_HOSTGROUPS_ALIAS);
}

void ArmNagiosNDOUtils::makeSelectHostgroupMembersArg(void)
{
	DBAgent::SelectExArg &arg = m_impl->selectHostgroupMembersArg;
	arg.add(IDX_HOSTGROUP_MEMBERS_HOSTGROUP_ID);
	arg.add(IDX_HOSTGROUP_MEMBERS_HOST_OBJECT_ID);
}

void ArmNagiosNDOUtils::addConditionForTriggerQuery(void)
{
	ThreadLocalDBCache cache;
	const MonitoringServerInfo &svInfo = getServerInfo();
	time_t lastUpdateTime =
	   cache.getMonitoring().getLastChangeTimeOfTrigger(svInfo.id);
	struct tm tm;
	localtime_r(&lastUpdateTime, &tm);

	DBAgent::SelectExArg &arg =
	  m_impl->selectTriggerBuilder.getSelectExArg();
	arg.condition = m_impl->selectTriggerBaseCondition;
	arg.condition +=
	   StringUtils::sprintf("'%04d-%02d-%02d %02d:%02d:%02d'",
	                        1900+tm.tm_year, tm.tm_mon+1, tm.tm_mday,
	                        tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void ArmNagiosNDOUtils::addConditionForEventQuery(void)
{
	ThreadLocalDBCache cache;
	const MonitoringServerInfo &svInfo = getServerInfo();
	uint64_t lastEventId = cache.getMonitoring().getLastEventId(svInfo.id);
	string cond;
	DBAgent::SelectExArg &arg = m_impl->selectEventBuilder.getSelectExArg();
	arg.condition = m_impl->selectEventBaseCondition;
	if (lastEventId == EVENT_NOT_FOUND)
		cond = "0";
	else
		cond = StringUtils::sprintf("%" PRIu64, lastEventId+1);
	arg.condition += cond;
}

void ArmNagiosNDOUtils::getTrigger(void)
{
	addConditionForTriggerQuery();

	// TODO: should use transaction
	DBAgent::SelectExArg &arg =
	  m_impl->selectTriggerBuilder.getSelectExArg();
	m_impl->dbAgent->select(arg);
	size_t numTriggers = arg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of triggers: %zd\n", numTriggers);

	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfoList triggerInfoList;
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		int currentStatus;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		TriggerInfo trigInfo;
		trigInfo.serverId = svInfo.id;
		trigInfo.lastChangeTime.tv_nsec = 0;

		itemGroupStream >> trigInfo.id;      // service_id

		// TODO: severity should not depend on the status.
		// status and severity (current_status)
		itemGroupStream >> currentStatus;
		trigInfo.status = TRIGGER_STATUS_OK;
		trigInfo.severity = TRIGGER_SEVERITY_INFO;
		if (currentStatus != STATE_OK) {
			trigInfo.status = TRIGGER_STATUS_PROBLEM;
			if (currentStatus == STATE_WARNING)
				trigInfo.severity = TRIGGER_SEVERITY_WARNING;
			else if (currentStatus == STATE_CRITICAL)
				trigInfo.severity = TRIGGER_SEVERITY_CRITICAL;
			else
				trigInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		}

		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		                                      //status_update_time
		itemGroupStream >> trigInfo.brief;    // output
		itemGroupStream >> trigInfo.hostId;   // host_id
		itemGroupStream >> trigInfo.hostName; // hosts.display_name
		triggerInfoList.push_back(trigInfo);
	}
	ThreadLocalDBCache cache;
	cache.getMonitoring().addTriggerInfoList(triggerInfoList);
}

void ArmNagiosNDOUtils::getEvent(void)
{
	addConditionForEventQuery();

	// TODO: should use transaction
	DBAgent::SelectExArg &arg = m_impl->selectEventBuilder.getSelectExArg();
	m_impl->dbAgent->select(arg);
	size_t numEvents = arg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of events: %zd\n", numEvents);

	// TODO: use addEventInfoList with the newly added data.
	const MonitoringServerInfo &svInfo = getServerInfo();
	EventInfoList eventInfoList;
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		int state;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		EventInfo eventInfo;
		eventInfo.serverId = svInfo.id;
		eventInfo.time.tv_nsec = 0;

		itemGroupStream >> eventInfo.id; // statehistory_id
		// type, status, and severity (state)
		itemGroupStream >> state;
		if (state == STATE_OK) {
			eventInfo.type = EVENT_TYPE_GOOD;
			eventInfo.status = TRIGGER_STATUS_OK,
			eventInfo.severity = TRIGGER_SEVERITY_INFO;
		} else {
			eventInfo.type = EVENT_TYPE_BAD;
			eventInfo.status = TRIGGER_STATUS_PROBLEM;
			if (state == STATE_WARNING)
				eventInfo.severity = TRIGGER_SEVERITY_WARNING;
			else if (state == STATE_CRITICAL)
				eventInfo.severity = TRIGGER_SEVERITY_CRITICAL;
			else
				eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		}
		itemGroupStream >> eventInfo.time.tv_sec; // state_time
		itemGroupStream >> eventInfo.brief;       // output
		itemGroupStream >> eventInfo.triggerId;   // service_id
		itemGroupStream >> eventInfo.hostId;      // host_id
		itemGroupStream >> eventInfo.hostName;    // hosts.display_name
		eventInfoList.push_back(eventInfo);
	}
	m_impl->dataStore->addEventList(eventInfoList);
}

void ArmNagiosNDOUtils::getItem(void)
{
	// TODO: should use transaction
	DBAgent::SelectExArg &arg = m_impl->selectItemBuilder.getSelectExArg();
	m_impl->dbAgent->select(arg);
	size_t numItems = arg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of items: %zd\n", numItems);

	const MonitoringServerInfo &svInfo = getServerInfo();
	ItemInfoList itemInfoList;
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);

		ItemInfo itemInfo;
		itemInfo.serverId = svInfo.id;
		itemInfo.lastValueTime.tv_nsec = 0;
		itemInfo.prevValue = "N/A";

		itemGroupStream >> itemInfo.id;        // service_id
		itemGroupStream >> itemInfo.hostId;    // host_id
		itemGroupStream >> itemInfo.brief;     // check_command
		itemGroupStream >> itemInfo.lastValueTime.tv_sec;
		                                       // status_update_time
		itemGroupStream >> itemInfo.lastValue; // last_value

		// TODO: We will take into account 'servicegroup' table.
		itemInfo.itemGroupName = "No group";

		itemInfoList.push_back(itemInfo);
	}
	ThreadLocalDBCache cache;
	cache.getMonitoring().addItemInfoList(itemInfoList);
}

void ArmNagiosNDOUtils::getHost(void)
{
	// TODO: should use transaction
	m_impl->dbAgent->select(m_impl->selectHostArg);
	size_t numHosts =
	  m_impl->selectHostArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of hosts: %zd\n", numHosts);

	const MonitoringServerInfo &svInfo = getServerInfo();
	HostInfoList hostInfoList;
	const ItemGroupList &grpList =
	  m_impl->selectHostArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		HostInfo hostInfo;
		hostInfo.serverId = svInfo.id;
		itemGroupStream >> hostInfo.id;
		itemGroupStream >> hostInfo.hostName;
		hostInfoList.push_back(hostInfo);
	}
	ThreadLocalDBCache cache;
	cache.getMonitoring().addHostInfoList(hostInfoList);
}

void ArmNagiosNDOUtils::getHostgroup(void)
{
	// TODO: should use transaction
	m_impl->dbAgent->select(m_impl->selectHostgroupArg);
	size_t numHostgroups =
	  m_impl->selectHostgroupArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of hostgroups: %zd\n", numHostgroups);

	const MonitoringServerInfo &svInfo = getServerInfo();
	HostgroupInfoList hostgroupInfoList;
	const ItemGroupList &grpList =
	  m_impl->selectHostgroupArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		HostgroupInfo hostgroupInfo;
		hostgroupInfo.id = AUTO_INCREMENT_VALUE;
		hostgroupInfo.serverId = svInfo.id;
		itemGroupStream >> hostgroupInfo.groupId;
		itemGroupStream >> hostgroupInfo.groupName;
		hostgroupInfoList.push_back(hostgroupInfo);
	}
	ThreadLocalDBCache cache;
	cache.getMonitoring().addHostgroupInfoList(hostgroupInfoList);
}

void ArmNagiosNDOUtils::getHostgroupMembers(void)
{
	// TODO: should use transaction
	m_impl->dbAgent->select(m_impl->selectHostgroupMembersArg);
	size_t numHostgroupMembers =
	  m_impl->selectHostgroupMembersArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of hostgroupMembers: %zd\n", numHostgroupMembers);

	const MonitoringServerInfo &svInfo = getServerInfo();
	HostgroupElementList hostgroupElementList;
	const ItemGroupList &grpList =
	  m_impl->selectHostgroupMembersArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		HostgroupElement hostgroupElement;
		hostgroupElement.id = AUTO_INCREMENT_VALUE;
		hostgroupElement.serverId = svInfo.id;
		itemGroupStream >> hostgroupElement.groupId;
		itemGroupStream >> hostgroupElement.hostId;
		hostgroupElementList.push_back(hostgroupElement);
	}
	ThreadLocalDBCache cache;
	cache.getMonitoring().addHostgroupElementList(hostgroupElementList);
}

void ArmNagiosNDOUtils::connect(void)
{
	m_impl->connect();
}

gpointer ArmNagiosNDOUtils::mainThread(HatoholThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmNagiosNDOUtils (server: %s)\n",
	          svInfo.hostName.c_str());
	ArmBase::registerAvailableTrigger(COLLECT_NG_DISCONNECT_NAGIOS,
					  FAILED_CONNECT_MYSQL_TRIGGERID,
					  HTERR_FAILED_CONNECT_MYSQL);
	ArmBase::registerAvailableTrigger(COLLECT_NG_INTERNAL_ERROR,
					  FAILED_INTERNAL_ERROR_TRIGGERID,
					  HTERR_INTERNAL_ERROR);
	return ArmBase::mainThread(arg);
}

ArmBase::ArmPollingResult ArmNagiosNDOUtils::mainThreadOneProc(void)
{
	try {
		if (!m_impl->dbAgent)
			connect();
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			getItem();
		} else {
			getTrigger();
			getEvent();
			getHost();
			getHostgroup();
			getHostgroupMembers();
			if (!getCopyOnDemandEnabled())
				getItem();
		}
	} catch (const HatoholException &he) {
		if (he.getErrCode() == HTERR_FAILED_CONNECT_MYSQL) {
			MLPL_ERR("Error Connection: %s %d\n", he.what(), he.getErrCode());
			delete m_impl->dbAgent;
			m_impl->dbAgent = NULL;
			return COLLECT_NG_DISCONNECT_NAGIOS;
		} else {
			MLPL_ERR("Got exception: %s\n", he.what());
			return COLLECT_NG_INTERNAL_ERROR;
		}
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s\n", e.what());
		return COLLECT_NG_INTERNAL_ERROR;
	}
	return COLLECT_OK;
}

