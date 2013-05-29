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

#include "ArmNagiosNDOUtils.h"
#include "DBAgentMySQL.h"

using namespace std;

static const char *TABLE_NAME_SERVICES      = "nagios_services";
static const char *TABLE_NAME_SERVICESTATUS = "nagios_servicestatus";
static const char *TABLE_NAME_HOSTS         = "nagios_hosts";

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
},
};

static const size_t NUM_COLUMNS_SERVICESTATUS =
  sizeof(COLUMN_DEF_SERVICESTATUS) / sizeof(ColumnDef);

enum {
	IDX_SERVICESTATUS_SERVICE_OBJECT_ID,
	IDX_SERVICESTATUS_STATUS_UPDATE_TIME,
	IDX_SERVICESTATUS_OUTPUT,
	IDX_SERVICESTATUS_CURRENT_STATE,
	NUM_IDX_SERVICESTATUS,
};

// Definitions: nagios_hosts
static const ColumnDef COLUMN_DEF_HOSTS[] = {
{
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
	"status_update_time",              // columnName
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
	IDX_HOSTS_HOST_OBJECT_ID,
	IDX_HOSTS_DISPLAY_NAME,
	NUM_IDX_HOSTS,
};

// ---------------------------------------------------------------------------
// Private context
// ---------------------------------------------------------------------------
struct ArmNagiosNDOUtils::PrivateContext
{
	DBAgentMySQL dbAgent;

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
gpointer ArmNagiosNDOUtils::mainThread(AsuraThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmNagiosNDOUtils (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

bool ArmNagiosNDOUtils::mainThreadOneProc(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}


