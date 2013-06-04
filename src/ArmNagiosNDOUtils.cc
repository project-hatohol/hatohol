/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <time.h>
#include "ArmNagiosNDOUtils.h"
#include "DBAgentMySQL.h"
#include "Utils.h"

using namespace std;

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

static const size_t NUM_COLUMNS_SERVICES =
  sizeof(COLUMN_DEF_SERVICES) / sizeof(ColumnDef);

enum {
	IDX_SERVICES_SERVICE_ID,
	IDX_SERVICES_HOST_OBJECT_ID,
	IDX_SERVICES_SERVICE_OBJECT_ID,
	NUM_IDX_SERVICES,
};

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
	11,                                // columnLength
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

static const size_t NUM_COLUMNS_SERVICESTATUS =
  sizeof(COLUMN_DEF_SERVICESTATUS) / sizeof(ColumnDef);

enum {
	IDX_SERVICESTATUS_SERVICE_OBJECT_ID,
	IDX_SERVICESTATUS_STATUS_UPDATE_TIME,
	IDX_SERVICESTATUS_OUTPUT,
	IDX_SERVICESTATUS_CURRENT_STATE,
	IDX_SERVICESTATUS_CHECK_COMMAND,
	NUM_IDX_SERVICESTATUS,
};

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

static const size_t NUM_COLUMNS_HOSTS =
  sizeof(COLUMN_DEF_HOSTS) / sizeof(ColumnDef);

enum {
	IDX_HOSTS_HOST_ID,
	IDX_HOSTS_HOST_OBJECT_ID,
	IDX_HOSTS_DISPLAY_NAME,
	NUM_IDX_HOSTS,
};

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
	TABLE_NAME_HOSTS,                  // tableName
	"state_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NAGIOS_STATEHISTORY_OBJECT_ID, // itemId
	TABLE_NAME_HOSTS,                  // tableName
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
	TABLE_NAME_HOSTS,                  // tableName
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
	TABLE_NAME_HOSTS,                  // tableName
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
	TABLE_NAME_HOSTS,                  // tableName
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

static const size_t NUM_COLUMNS_STATEHISTORY =
  sizeof(COLUMN_DEF_STATEHISTORY) / sizeof(ColumnDef);

enum {
	IDX_STATEHISTORY_STATEHISTORY_ID,
	IDX_STATEHISTORY_STATE_TIME,
	IDX_STATEHISTORY_OBJECT_ID,
	IDX_STATEHISTORY_STATE,
	IDX_STATEHISTORY_STATE_TYPE,
	IDX_STATEHISTORY_OUTPUT,
	NUM_IDX_STATEHISTORY,
};

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmNagiosNDOUtils::PrivateContext
{
	DBAgentMySQL dbAgent;
	DBClientAsura dbAsura;
	DBAgentSelectExArg selectTriggerArg;
	string             selectTriggerBaseCondition;
	DBAgentSelectExArg selectEventArg;
	string             selectEventBaseCondition;
	DBAgentSelectExArg selectItemArg;

	// methods
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: dbAgent(serverInfo.dbName.c_str(), serverInfo.userName.c_str(),
	          serverInfo.password.c_str(),
	          serverInfo.getHostAddress(), serverInfo.port)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmNagiosNDOUtils::ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo)
: ArmBase(serverInfo),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	makeSelectTriggerArg();
	makeSelectEventArg();
	makeSelectItemArg();
}

ArmNagiosNDOUtils::~ArmNagiosNDOUtils()
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	if (m_ctx)
		delete m_ctx;
	MLPL_INFO("ArmNagiosNDOUtils [%d:%s]: exit process completed.\n",
	          svInfo.id, svInfo.hostName.c_str());
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ArmNagiosNDOUtils::makeSelectTriggerArg(void)
{
	const static char *VAR_SERVICES = "sv";
	const static char *VAR_STATUS   = "st";
	const static char *VAR_HOSTS    = "h";

	// table name
	const ColumnDef &columnDefServicesServiceObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_OBJECT_ID];
	const ColumnDef &columnDefServicesHostObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_HOST_OBJECT_ID];
	const ColumnDef &columnDefStatusServiceObjectId = 
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_SERVICE_OBJECT_ID];
	const ColumnDef &columnDefHostsHostObjectId = 
	  COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_OBJECT_ID];
	m_ctx->selectTriggerArg.tableName =
	  StringUtils::sprintf(
	    "%s %s "
	    "inner join %s %s on %s.%s=%s.%s "
	    "inner join %s %s on %s.%s=%s.%s",
	    TABLE_NAME_SERVICES,      VAR_SERVICES,
	    TABLE_NAME_SERVICESTATUS, VAR_STATUS,
	    VAR_SERVICES, columnDefServicesServiceObjectId.columnName,
	    VAR_STATUS,   columnDefStatusServiceObjectId.columnName,
	    TABLE_NAME_HOSTS,         VAR_HOSTS,
	    VAR_SERVICES, columnDefServicesHostObjectId.columnName,
	    VAR_HOSTS,    columnDefHostsHostObjectId.columnName);

	// statements
	const ColumnDef &columnDefServicesServiceId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_ID];
	const ColumnDef &columnDefStatusCurrentState =
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_CURRENT_STATE];
	const ColumnDef &columnDefStatusUpdateTime =
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_STATUS_UPDATE_TIME];
	const ColumnDef &columnDefHostsHostId =
	  COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_ID];
	const ColumnDef &columnDefHostsDisplayName =
	  COLUMN_DEF_HOSTS[IDX_HOSTS_DISPLAY_NAME];
	const ColumnDef &columnDefStatusOutput = 
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_OUTPUT];

	// service_id
	m_ctx->selectTriggerArg.pushColumn(columnDefServicesServiceId,
	                                   VAR_SERVICES);
	// current_status
	m_ctx->selectTriggerArg.pushColumn(columnDefStatusCurrentState,
	                                   VAR_STATUS);
	// status_update_time
	m_ctx->selectTriggerArg.pushColumn(columnDefStatusUpdateTime,
	                                   VAR_STATUS);
	// host_id
	m_ctx->selectTriggerArg.pushColumn(columnDefHostsHostId,
	                                   VAR_HOSTS);
	// hosts.display_name 
	m_ctx->selectTriggerArg.pushColumn(columnDefHostsDisplayName,
	                                   VAR_HOSTS);
	// output
	m_ctx->selectTriggerArg.pushColumn(columnDefStatusOutput,
	                                   VAR_STATUS);

	// contiditon
	m_ctx->selectTriggerBaseCondition = StringUtils::sprintf(
	  "%s.%s>%d and %s.%s>=",
	  VAR_STATUS, columnDefStatusCurrentState.columnName, STATE_OK,
	  VAR_STATUS, columnDefStatusUpdateTime.columnName);
}

void ArmNagiosNDOUtils::makeSelectEventArg(void)
{
	const static char *VAR_STATEHISTORY = "sh";
	const static char *VAR_SERVICES     = "sv";
	const static char *VAR_HOSTS        = "h";

	// table name
	const ColumnDef &columnDefStateHistoryObjectId = 
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_OBJECT_ID];
	const ColumnDef &columnDefServicesServiceObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_OBJECT_ID];
	const ColumnDef &columnDefServicesHostObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_HOST_OBJECT_ID];
	const ColumnDef &columnDefHostsHostObjectId = 
	  COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_OBJECT_ID];
	m_ctx->selectEventArg.tableName =
	  StringUtils::sprintf(
	    "%s %s "
	    "inner join %s %s on %s.%s=%s.%s "
	    "inner join %s %s on %s.%s=%s.%s",
	    TABLE_NAME_STATEHISTORY,  VAR_STATEHISTORY,
	    TABLE_NAME_SERVICES,      VAR_SERVICES,
	    VAR_STATEHISTORY, columnDefStateHistoryObjectId.columnName,
	    VAR_SERVICES,     columnDefServicesServiceObjectId.columnName,
	    TABLE_NAME_HOSTS,         VAR_HOSTS,
	    VAR_SERVICES,     columnDefServicesHostObjectId.columnName,
	    VAR_HOSTS,        columnDefHostsHostObjectId.columnName);

	// statements
	const ColumnDef &columnDefStateHistoryStateHistoryId =
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_STATEHISTORY_ID];
	const ColumnDef &columnDefStateHistoryState =
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_STATE];
	const ColumnDef &columnDefStateHistoryStateTime =
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_STATE_TIME];

	const ColumnDef &columnDefServicesServiceId =
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_ID];
	const ColumnDef &columnDefHostsHostId =
	  COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_ID];
	const ColumnDef &columnDefHostsDisplayName =
	  COLUMN_DEF_HOSTS[IDX_HOSTS_DISPLAY_NAME];

	const ColumnDef &columnDefStateHistoryOutput = 
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_OUTPUT];
	const ColumnDef &columnDefStateHistoryStateType = 
	  COLUMN_DEF_STATEHISTORY[IDX_STATEHISTORY_STATE_TYPE];

	// statehistory_id
	m_ctx->selectEventArg.pushColumn(columnDefStateHistoryStateHistoryId,
	                                 VAR_STATEHISTORY);
	// state
	m_ctx->selectEventArg.pushColumn(columnDefStateHistoryState,
	                                 VAR_STATEHISTORY);
	// state time
	m_ctx->selectEventArg.pushColumn(columnDefStateHistoryStateTime,
	                                 VAR_STATEHISTORY);

	// sevice id
	m_ctx->selectEventArg.pushColumn(columnDefServicesServiceId,
	                                 VAR_SERVICES);
	// host_id
	m_ctx->selectEventArg.pushColumn(columnDefHostsHostId,
	                                 VAR_HOSTS);
	// hosts.display_name 
	m_ctx->selectEventArg.pushColumn(columnDefHostsDisplayName,
	                                 VAR_HOSTS);
	// output
	m_ctx->selectEventArg.pushColumn(columnDefStateHistoryOutput,
	                                 VAR_STATEHISTORY);

	// contiditon
	m_ctx->selectEventBaseCondition = StringUtils::sprintf(
	  "%s.%s=%d and %s.%s>=",
	  VAR_STATEHISTORY, columnDefStateHistoryStateType.columnName,
	  HARD_STATE,
	  VAR_STATEHISTORY, columnDefStateHistoryStateHistoryId.columnName);
}

void ArmNagiosNDOUtils::makeSelectItemArg(void)
{
	const static char *VAR_SERVICES = "sv";
	const static char *VAR_STATUS   = "st";

	// table name
	const ColumnDef &columnDefServicesServiceObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_OBJECT_ID];
	const ColumnDef &columnDefStatusServiceObjectId = 
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_SERVICE_OBJECT_ID];
	m_ctx->selectItemArg.tableName =
	  StringUtils::sprintf(
	    "%s %s "
	    "inner join %s %s on %s.%s=%s.%s",
	    TABLE_NAME_SERVICES,      VAR_SERVICES,
	    TABLE_NAME_SERVICESTATUS, VAR_STATUS,
	    VAR_SERVICES, columnDefServicesServiceObjectId.columnName,
	    VAR_STATUS,   columnDefStatusServiceObjectId.columnName);

	// statements
	const ColumnDef &columnDefServicesServiceId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_SERVICE_ID];
	const ColumnDef &columnDefServicesHostObjectId = 
	  COLUMN_DEF_SERVICES[IDX_SERVICES_HOST_OBJECT_ID];
	const ColumnDef &columnDefStatusCheckCommand =
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_CHECK_COMMAND];
	const ColumnDef &columnDefStatusOutput = 
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_OUTPUT];
	const ColumnDef &columnDefStatusUpdateTime =
	  COLUMN_DEF_SERVICESTATUS[IDX_SERVICESTATUS_STATUS_UPDATE_TIME];

	// service_id
	m_ctx->selectItemArg.pushColumn(columnDefServicesServiceId,
	                                VAR_SERVICES);
	// host_id
	m_ctx->selectItemArg.pushColumn(columnDefServicesHostObjectId,
	                                VAR_SERVICES);
	// check_command
	m_ctx->selectItemArg.pushColumn(columnDefStatusCheckCommand,
	                                VAR_STATUS);
	// status_update_time
	m_ctx->selectItemArg.pushColumn(columnDefStatusUpdateTime,
	                                VAR_STATUS);
	// output
	m_ctx->selectItemArg.pushColumn(columnDefStatusOutput,
	                                VAR_STATUS);
}

void ArmNagiosNDOUtils::addConditionForTriggerQuery(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	time_t lastUpdateTime =
	   m_ctx->dbAsura.getLastChangeTimeOfTrigger(svInfo.id);
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
	uint64_t lastEventId = m_ctx->dbAsura.getLastEventId(svInfo.id);
	string cond;
	m_ctx->selectEventArg.condition = m_ctx->selectEventBaseCondition;
	if (lastEventId == DBClientAsura::EVENT_NOT_FOUND)
 		cond = "0";
	else
 		cond = StringUtils::sprintf("%"PRIu64, lastEventId+1);
	m_ctx->selectEventArg.condition += cond;
}

void ArmNagiosNDOUtils::getTrigger(void)
{
	addConditionForTriggerQuery();

	// TODO: should use transaction
	m_ctx->dbAgent.select(m_ctx->selectTriggerArg);
	size_t numTriggers =
	   m_ctx->selectTriggerArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of triggers: %zd\n", numTriggers);

	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfoList triggerInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectTriggerArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		int idx = 0;
		const ItemGroup *itemGroup = *it;
		TriggerInfo trigInfo;

		// serverId
		trigInfo.serverId = svInfo.id;

		// id (service_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemId);
		trigInfo.id = itemId->get();

		// status and severity (current_status)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemStatus);
		int currentStatus = itemStatus->get();
		trigInfo.status = TRIGGER_STATUS_OK;
		trigInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		if (currentStatus != STATE_OK) {
			trigInfo.status = TRIGGER_STATUS_PROBLEM;
			if (currentStatus == STATE_WARNING)
				trigInfo.severity = TRIGGER_SEVERITY_WARN;
			else if (currentStatus == STATE_CRITICAL)
				trigInfo.severity = TRIGGER_SEVERITY_CRITICAL;
			else
				MLPL_WARN("Unknown status: %d", currentStatus);
		}

		// lastChangeTime (status_update_time)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemLastchange);
		trigInfo.lastChangeTime.tv_sec = itemLastchange->get();
		trigInfo.lastChangeTime.tv_nsec = 0;

		// hostId (host_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemHostid);
		trigInfo.hostId = itemHostid->get();

		// hostName (hosts.display_name)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemHostName);
		trigInfo.hostName = itemHostName->get();

		// brief (output)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemDescription);
		trigInfo.brief = itemDescription->get();

		triggerInfoList.push_back(trigInfo);
	}
	m_ctx->dbAsura.setTriggerInfoList(triggerInfoList, svInfo.id);
}

void ArmNagiosNDOUtils::getEvent(void)
{
	addConditionForEventQuery();

	// TODO: should use transaction
	m_ctx->dbAgent.select(m_ctx->selectEventArg);
	size_t numEvents =
	   m_ctx->selectEventArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of events: %zd\n", numEvents);

	// TODO: use addEventInfoList with the newly added data.
	const MonitoringServerInfo &svInfo = getServerInfo();
	EventInfoList eventInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectEventArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		int idx = 0;
		const ItemGroup *itemGroup = *it;
		EventInfo eventInfo;

		// serverId
		eventInfo.serverId = svInfo.id;

		// id (statehistory_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemId);
		eventInfo.id = itemId->get();

		// type, status, and severity (state)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemState);
		int state = itemState->get();
		if (state == STATE_OK) {
			eventInfo.type = TRIGGER_DEACTIVATED;
			eventInfo.status = TRIGGER_STATUS_OK,
			eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		} else {
			eventInfo.type = TRIGGER_ACTIVATED;
			eventInfo.status = TRIGGER_STATUS_PROBLEM;
			if (state == STATE_WARNING)
				eventInfo.severity = TRIGGER_SEVERITY_WARN;
			if (state == STATE_CRITICAL)
				eventInfo.severity = TRIGGER_SEVERITY_CRITICAL;
			else
				eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		}

		// time (state_time)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemStateTime);
		eventInfo.time.tv_sec = itemStateTime->get();
		eventInfo.time.tv_nsec = 0;

		// trigger id (service_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemServiceId);
		eventInfo.triggerId = itemServiceId->get();

		// hostId (host_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemHostid);
		eventInfo.hostId = itemHostid->get();

		// hostName (hosts.display_name)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemHostName);
		eventInfo.hostName = itemHostName->get();

		// brief (output)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemDescription);
		eventInfo.brief = itemDescription->get();

		eventInfoList.push_back(eventInfo);
	}
	m_ctx->dbAsura.addEventInfoList(eventInfoList);
}

void ArmNagiosNDOUtils::getItem(void)
{
	// TODO: should use transaction
	m_ctx->dbAgent.select(m_ctx->selectItemArg);
	size_t numItems =
	   m_ctx->selectItemArg.dataTable->getNumberOfRows();
	MLPL_DBG("The number of items: %zd\n", numItems);

	const MonitoringServerInfo &svInfo = getServerInfo();
	ItemInfoList itemInfoList;
	const ItemGroupList &grpList =
	  m_ctx->selectItemArg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		int idx = 0;
		const ItemGroup *itemGroup = *it;
		ItemInfo itemInfo;

		// serverId
		itemInfo.serverId = svInfo.id;

		// id (service_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemId);
		itemInfo.id = itemId->get();

		// host Id (host_id)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemHostid);
		itemInfo.hostId = itemHostid->get();

		// brief (check_command)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemCheckCmd);
		itemInfo.brief = itemCheckCmd->get();

		// lastChangeTime (status_update_time)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemUpdateTime);
		itemInfo.lastValueTime.tv_sec = itemUpdateTime->get();
		itemInfo.lastValueTime.tv_nsec = 0;

		// last_value (last_value)
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemOutput);
		itemInfo.lastValue = itemOutput->get();

		// prev_value (Not available)
		itemInfo.prevValue = "N/A";

		// itemGroupName
		// TODO: We will take into account 'servicegroup' table.
		itemInfo.itemGroupName = "No group";

		itemInfoList.push_back(itemInfo);
	}
	m_ctx->dbAsura.addItemInfoList(itemInfoList);
}

gpointer ArmNagiosNDOUtils::mainThread(AsuraThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmNagiosNDOUtils (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

bool ArmNagiosNDOUtils::mainThreadOneProc(void)
{
	try {
		getTrigger();
		getEvent();
		getItem();
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s\n", e.what());
		return false;
	}
	return true;
}

