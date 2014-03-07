/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <time.h>
#include "ArmNagiosNDOUtils.h"
#include "DBAgentMySQL.h"
#include "Utils.h"
#include "UnifiedDataStore.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_SERVICES      = "nagios_services";
static const char *TABLE_NAME_SERVICESTATUS = "nagios_servicestatus";
static const char *TABLE_NAME_HOSTS         = "nagios_hosts";
static const char *TABLE_NAME_STATEHISTORY  = "nagios_statehistory";

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
	ITEM_ID_NAGIOS_SERVICES_SERVICE_ID,// itemId
	TABLE_NAME_SERVICES,               // tableName
	"service_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICES_HOST_OBJECT_ID, // itemId
	TABLE_NAME_SERVICES,               // tableName
	"host_object_id",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICES_SERVICE_OBJECT_ID, // itemId
	TABLE_NAME_SERVICES,               // tableName
	"service_object_id",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
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

static const DBAgent::TableProfile tableProfileServices(
  TABLE_NAME_SERVICES, COLUMN_DEF_SERVICES,
  sizeof(COLUMN_DEF_SERVICES), NUM_IDX_SERVICES);

// Definitions: nagios_servicestatus
static const ColumnDef COLUMN_DEF_SERVICESTATUS[] = {
{
	ITEM_ID_NAGIOS_SERVICESTATUS_SERVICE_OBJECT_ID, // itemId
	TABLE_NAME_SERVICESTATUS,          // tableName
	"service_object_id",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICESTATUS_STATUS_UPDATE_TIME, // itemId
	TABLE_NAME_SERVICESTATUS,          // tableName
	"status_update_time",              // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICESTATUS_OUTPUT, // itemId
	TABLE_NAME_SERVICESTATUS,          // tableName
	"output",                          // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICESTATUS_CURRENT_STATE, // itemId
	TABLE_NAME_SERVICESTATUS,          // tableName
	"current_state",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	6,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_SERVICESTATUS_CHECK_COMMAND, // itemId
	TABLE_NAME_SERVICESTATUS,          // tableName
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

static const DBAgent::TableProfile tableProfileServiceStatus(
  TABLE_NAME_SERVICESTATUS, COLUMN_DEF_SERVICESTATUS,
  sizeof(COLUMN_DEF_SERVICESTATUS), NUM_IDX_SERVICESTATUS);

// Definitions: nagios_hosts
static const ColumnDef COLUMN_DEF_HOSTS[] = {
{
	ITEM_ID_NAGIOS_HOSTS_HOST_ID,      // itemId
	TABLE_NAME_HOSTS,                  // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NAGIOS_HOSTS_HOST_OBJECT_ID, // itemId
	TABLE_NAME_HOSTS,                  // tableName
	"host_object_id",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_HOSTS_DISPLAY_NAME,// itemId
	TABLE_NAME_HOSTS,                  // tableName
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

static const DBAgent::TableProfile tableProfileHosts(
  TABLE_NAME_HOSTS, COLUMN_DEF_HOSTS,
  sizeof(COLUMN_DEF_HOSTS), NUM_IDX_HOSTS);

// Definitions: nagios_statehistory
static const ColumnDef COLUMN_DEF_STATEHISTORY[] = {
{
	ITEM_ID_NAGIOS_STATEHISTORY_STATEHISTORY_ID, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
	"statehistory_id",                 // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_STATE_TIME, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
	"state_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_OBJECT_ID, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
	"object_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_STATE, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
	"state",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_STATE_TYPE, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
	"state_type",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_OUTPUT, // itemId
	TABLE_NAME_STATEHISTORY,           // tableName
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

static const DBAgent::TableProfile tableProfileStateHistory(
  TABLE_NAME_STATEHISTORY, COLUMN_DEF_STATEHISTORY,
  sizeof(COLUMN_DEF_STATEHISTORY), NUM_IDX_STATEHISTORY);

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
static const DBAgent::TableProfile *tableProfilesTrig[] = {
  &tableProfileServices,
  &tableProfileServiceStatus,
  &tableProfileHosts,
};
static const size_t numTableProfilesTrig =
  sizeof(tableProfilesTrig) / sizeof(DBAgent::TableProfile *);

static const DBAgent::TableProfile *tableProfilesEvent[] = {
  &tableProfileStateHistory,
  &tableProfileServices,
  &tableProfileHosts,
};
static const size_t numTableProfilesEvent =
  sizeof(tableProfilesEvent) / sizeof(DBAgent::TableProfile *);

static const DBAgent::TableProfile *tableProfilesItem[] = {
  &tableProfileServices,
  &tableProfileServiceStatus,
};
static const size_t numTableProfilesItem =
  sizeof(tableProfilesItem) / sizeof(DBAgent::TableProfile *);

struct ArmNagiosNDOUtils::PrivateContext
{
	DBAgentMySQL   *dbAgent;
	DBClientHatohol dbHatohol;
	DBAgent::SelectMultiTableArg selectTriggerArg;
	DBAgent::SelectMultiTableArg selectEventArg;
	DBAgent::SelectMultiTableArg selectItemArg;
	DBAgent::SelectExArg         selectHostArg;
	string               selectTriggerBaseCondition;
	string               selectEventBaseCondition;
	UnifiedDataStore    *dataStore;
	MonitoringServerInfo serverInfo;

	// methods
	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: dbAgent(NULL),
	  selectTriggerArg(tableProfilesTrig, numTableProfilesTrig),
	  selectEventArg(tableProfilesEvent, numTableProfilesEvent),
	  selectItemArg(tableProfilesItem, numTableProfilesItem),
	  selectHostArg(tableProfileHosts),
	  dataStore(NULL),
	  serverInfo(_serverInfo)
	{
		dataStore = UnifiedDataStore::getInstance();
	}

	virtual ~PrivateContext()
	{
		if (dbAgent)
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
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	makeSelectTriggerArg();
	makeSelectEventArg();
	makeSelectItemArg();
}

ArmNagiosNDOUtils::~ArmNagiosNDOUtils()
{
	requestExitAndWait();
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ArmNagiosNDOUtils::makeSelectTriggerArg(void)
{
	enum {
		TBLIDX_SERVICES,
		TBLIDX_STATUS,
		TBLIDX_HOSTS,
	};

	DBAgent::SelectMultiTableArg &arg = m_ctx->selectTriggerArg;
	arg.tableField = 
	  StringUtils::sprintf(
	    "%s "
	    "inner join %s on %s=%s "
	    "inner join %s on %s=%s",
	    TABLE_NAME_SERVICES,
	    TABLE_NAME_SERVICESTATUS,
	    arg.getFullName(TBLIDX_SERVICES,
                            IDX_SERVICES_SERVICE_OBJECT_ID).c_str(),
	    arg.getFullName(TBLIDX_STATUS,
                            IDX_SERVICESTATUS_SERVICE_OBJECT_ID).c_str(),
	    TABLE_NAME_HOSTS,
	    arg.getFullName(TBLIDX_SERVICES,
                            IDX_SERVICES_SERVICE_OBJECT_ID).c_str(),
	    arg.getFullName(TBLIDX_HOSTS,
                            IDX_HOSTS_HOST_OBJECT_ID).c_str());

	arg.setTable(TBLIDX_SERVICES);
	arg.add(IDX_SERVICES_SERVICE_ID);

	arg.setTable(TBLIDX_STATUS);
	arg.add(IDX_SERVICESTATUS_CURRENT_STATE);
	arg.add(IDX_SERVICESTATUS_STATUS_UPDATE_TIME);

	arg.setTable(TBLIDX_HOSTS);
	arg.add(IDX_HOSTS_HOST_ID);
	arg.add(IDX_HOSTS_DISPLAY_NAME);

	arg.setTable(TBLIDX_STATUS);
	arg.add(IDX_SERVICESTATUS_OUTPUT);

	// contiditon
	m_ctx->selectTriggerBaseCondition = StringUtils::sprintf(
	  "%s>=",
	  arg.getFullName(TBLIDX_STATUS,
	                  IDX_SERVICESTATUS_STATUS_UPDATE_TIME).c_str());
}

void ArmNagiosNDOUtils::makeSelectEventArg(void)
{
	enum {
		TBLIDX_HISTORY,
		TBLIDX_SERVICES,
		TBLIDX_HOSTS,
	};

	DBAgent::SelectMultiTableArg &arg = m_ctx->selectEventArg;
	arg.tableField = 
	  StringUtils::sprintf(
	    "%s "
	    "inner join %s on %s=%s "
	    "inner join %s on %s=%s",
	    TABLE_NAME_STATEHISTORY,
	    TABLE_NAME_SERVICES,
	    arg.getFullName(TBLIDX_HISTORY, IDX_STATEHISTORY_OBJECT_ID).c_str(),
	    arg.getFullName(TBLIDX_SERVICES,
	                    IDX_SERVICES_SERVICE_OBJECT_ID).c_str(),
	    TABLE_NAME_HOSTS,
	    arg.getFullName(TBLIDX_SERVICES,
	                    IDX_SERVICES_SERVICE_OBJECT_ID).c_str(),
	    arg.getFullName(TBLIDX_HOSTS, IDX_HOSTS_HOST_OBJECT_ID).c_str());

	arg.setTable(TBLIDX_HISTORY);
	arg.add(IDX_STATEHISTORY_STATEHISTORY_ID);
	arg.add(IDX_STATEHISTORY_STATE);
	arg.add(IDX_STATEHISTORY_STATE_TIME);

	arg.setTable(TBLIDX_SERVICES);
	arg.add(IDX_SERVICES_SERVICE_ID);

	arg.setTable(TBLIDX_HOSTS);
	arg.setTable(IDX_HOSTS_HOST_ID);
	arg.setTable(IDX_HOSTS_DISPLAY_NAME);

	arg.setTable(TBLIDX_HISTORY);
	arg.add(IDX_STATEHISTORY_OUTPUT);

	// contiditon
	m_ctx->selectEventBaseCondition = StringUtils::sprintf(
	  "%s=%d and %s>=",
	  arg.getFullName(TBLIDX_HISTORY, IDX_STATEHISTORY_STATE_TYPE).c_str(),
	  HARD_STATE,
	  arg.getFullName(TBLIDX_HISTORY,
	                  IDX_STATEHISTORY_STATEHISTORY_ID).c_str());
}

void ArmNagiosNDOUtils::makeSelectItemArg(void)
{
	enum {
		TBLIDX_SERVICES,
		TBLIDX_STATUS,
	};

	DBAgent::SelectMultiTableArg &arg = m_ctx->selectItemArg;
	arg.tableField =
	  StringUtils::sprintf(
	    "%s inner join %s on %s=%s",
	    TABLE_NAME_SERVICES,
	    TABLE_NAME_SERVICESTATUS,
	    arg.getFullName(TBLIDX_SERVICES,
	                    IDX_SERVICES_SERVICE_OBJECT_ID).c_str(),
	    arg.getFullName(TBLIDX_STATUS,
	                    IDX_SERVICESTATUS_SERVICE_OBJECT_ID).c_str());

	arg.setTable(TBLIDX_SERVICES);
	arg.add(IDX_SERVICES_SERVICE_ID);
	arg.add(IDX_SERVICES_HOST_OBJECT_ID);

	arg.setTable(TBLIDX_STATUS);
	arg.add(IDX_SERVICESTATUS_CHECK_COMMAND);
	arg.add(IDX_SERVICESTATUS_STATUS_UPDATE_TIME);
	arg.add(IDX_SERVICESTATUS_OUTPUT);
}

void ArmNagiosNDOUtils::makeSelectHostArg(void)
{
	DBAgent::SelectExArg &arg = m_ctx->selectHostArg;
	arg.add(IDX_HOSTS_HOST_ID);
	arg.add(IDX_HOSTS_DISPLAY_NAME);
}

void ArmNagiosNDOUtils::addConditionForTriggerQuery(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	time_t lastUpdateTime =
	   m_ctx->dbHatohol.getLastChangeTimeOfTrigger(svInfo.id);
	struct tm tm;
	localtime_r(&lastUpdateTime, &tm);

	m_ctx->selectTriggerArg.condition = m_ctx->selectTriggerBaseCondition;
	m_ctx->selectTriggerArg.condition +=
	   StringUtils::sprintf("'%04d-%02d-%02d %02d:%02d:%02d'",
	                        1900+tm.tm_year, tm.tm_mon+1, tm.tm_mday,
	                        tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void ArmNagiosNDOUtils::addConditionForEventQuery(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	uint64_t lastEventId = m_ctx->dbHatohol.getLastEventId(svInfo.id);
	string cond;
	m_ctx->selectEventArg.condition = m_ctx->selectEventBaseCondition;
	if (lastEventId == DBClientHatohol::EVENT_NOT_FOUND)
 		cond = "0";
	else
 		cond = StringUtils::sprintf("%"PRIu64, lastEventId+1);
	m_ctx->selectEventArg.condition += cond;
}

void ArmNagiosNDOUtils::getTrigger(void)
{
	addConditionForTriggerQuery();

	// TODO: should use transaction
	m_ctx->dbAgent->select(m_ctx->selectTriggerArg);
	size_t numTriggers =
	   m_ctx->selectTriggerArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of triggers: %zd\n", numTriggers);

	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfoList triggerInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectTriggerArg.dataTable->getItemGroupList();
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
		itemGroupStream >> trigInfo.hostId;   // host_id
		itemGroupStream >> trigInfo.hostName; // hosts.display_name
		itemGroupStream >> trigInfo.brief;    // output
		triggerInfoList.push_back(trigInfo);
	}
	m_ctx->dbHatohol.addTriggerInfoList(triggerInfoList);
}

void ArmNagiosNDOUtils::getEvent(void)
{
	addConditionForEventQuery();

	// TODO: should use transaction
	m_ctx->dbAgent->select(m_ctx->selectEventArg);
	size_t numEvents =
	   m_ctx->selectEventArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of events: %zd\n", numEvents);

	// TODO: use addEventInfoList with the newly added data.
	const MonitoringServerInfo &svInfo = getServerInfo();
	EventInfoList eventInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectEventArg.dataTable->getItemGroupList();
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
		itemGroupStream >> eventInfo.triggerId;   // service_id
		itemGroupStream >> eventInfo.hostId;      // host_id
		itemGroupStream >> eventInfo.hostName;    // hosts.display_name
		itemGroupStream >> eventInfo.brief;       // output
		eventInfoList.push_back(eventInfo);
	}
	m_ctx->dataStore->addEventList(eventInfoList);
}

void ArmNagiosNDOUtils::getItem(void)
{
	// TODO: should use transaction
	m_ctx->dbAgent->select(m_ctx->selectItemArg);
	size_t numItems =
	   m_ctx->selectItemArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of items: %zd\n", numItems);

	const MonitoringServerInfo &svInfo = getServerInfo();
	ItemInfoList itemInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectItemArg.dataTable->getItemGroupList();
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
	m_ctx->dbHatohol.addItemInfoList(itemInfoList);
}

void ArmNagiosNDOUtils::getHost(void)
{
	// TODO: should use transaction
	m_ctx->dbAgent->select(m_ctx->selectHostArg);
	size_t numHosts =
	  m_ctx->selectHostArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of hosts: %zd\n", numHosts);

	const MonitoringServerInfo &svInfo = getServerInfo();
	HostInfoList hostInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectHostArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		HostInfo hostInfo;
		hostInfo.serverId = svInfo.id;
		itemGroupStream >> hostInfo.id;
		itemGroupStream >> hostInfo.hostName;
		hostInfoList.push_back(hostInfo);
	}
	m_ctx->dbHatohol.addHostInfoList(hostInfoList);
}

void ArmNagiosNDOUtils::connect(void)
{
	m_ctx->connect();
}

gpointer ArmNagiosNDOUtils::mainThread(HatoholThreadArg *arg)
{
	connect();
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmNagiosNDOUtils (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

bool ArmNagiosNDOUtils::mainThreadOneProc(void)
{
	try {
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			getItem();
		} else {
			getTrigger();
			getEvent();
			if (!getCopyOnDemandEnabled())
				getItem();
		}
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s\n", e.what());
		return false;
	}
	return true;
}

