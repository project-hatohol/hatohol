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

#include <memory>

#include <SeparatorInjector.h>
#include <Mutex.h>
#include "DBAgentFactory.h"
#include "DBTablesConfig.h"
#include "ThreadLocalDBCache.h"
#include "ConfigManager.h"
#include "HatoholError.h"
#include "Params.h"
#include "ItemGroupStream.h"
#include "SQLUtils.h"
#include "DBClientJoinBuilder.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

static const char *TABLE_NAME_SYSTEM  = "system";
static const char *TABLE_NAME_SERVER_TYPES = "server_types";
static const char *TABLE_NAME_SERVERS = "servers";
static const char *TABLE_NAME_ARM_PLUGINS = "arm_plugins";
static const char *TABLE_NAME_INCIDENT_TRACKERS = "incident_trackers";
static const char *TABLE_NAME_SEVERITY_RANKS = "severity_ranks";
static const char *TABLE_NAME_CUSTOM_INCIDENT_STATUSES = "custom_incident_statuses";

int DBTablesConfig::CONFIG_DB_VERSION = 18;

const ServerIdSet EMPTY_SERVER_ID_SET;
const ServerIdSet EMPTY_INCIDENT_TRACKER_ID_SET;
static void operator>>(
  ItemGroupStream &itemGroupStream, MonitoringSystemType &monSysType)
{
	monSysType = itemGroupStream.read<int, MonitoringSystemType>();
}

static void operator>>(
  ItemGroupStream &itemGroupStream, IncidentTrackerType &incidentTrackerType)
{
       incidentTrackerType = itemGroupStream.read<int, IncidentTrackerType>();
}

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	"database_dir",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"''",                              // defaultValue
}, {
	"enable_face_mysql",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"face_rest_port",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	// obsolete
	"enable_copy_on_demand",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
},
};

enum {
	IDX_SYSTEM_DATABASE_DIR,
	IDX_SYSTEM_ENABLE_FACE_MYSQL,
	IDX_SYSTEM_FACE_REST_PORT,
	IDX_SYSTEM_ENABLE_COPY_ON_DEMAND, // obsolete
	NUM_IDX_SYSTEM,
};

static const DBAgent::TableProfile tableProfileSystem =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SYSTEM,
			    COLUMN_DEF_SYSTEM,
			    NUM_IDX_SYSTEM);

static const ColumnDef COLUMN_DEF_SERVER_TYPES[] = {
{
	// One of MonitoringSystemType shall be saved in this column.
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// This column contains a JSON string that represents
	// the specification of parameters for the monitoring server.
	"parameters",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"plugin_path",                     // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	2048,                              // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	// This column represent to plugin sql version
	"plugin_sql_version",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	"plugin_enabled",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	1,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	"uuid",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	36,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_SERVER_TYPES_TYPE,
	IDX_SERVER_TYPES_NAME,
	IDX_SERVER_TYPES_PARAMETERS,
	IDX_SERVER_TYPES_PLUGIN_PATH,
	IDX_SERVER_TYPES_PLUGIN_SQL_VERSION,
	IDX_SERVER_TYPES_PLUGIN_ENABLED,
	IDX_SERVER_TYPES_UUID,
	NUM_IDX_SERVER_TYPES,
};

static const int columnIndexesServerTypeUniqId[] = {
  IDX_SERVER_TYPES_TYPE, IDX_SERVER_TYPES_UUID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsServerTypes[] = {
  {"ServerTypeUniqId", (const int *)columnIndexesServerTypeUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileServerTypes =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SERVER_TYPES,
			    COLUMN_DEF_SERVER_TYPES,
			    NUM_IDX_SERVER_TYPES,
			    indexDefsServerTypes);

static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"ip_address",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"nickname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"port",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"polling_interval_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"retry_interval_sec",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"user_name",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"db_name",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"base_url",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	2047,                              // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"extended_info",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_SERVERS_ID,
	IDX_SERVERS_TYPE,
	IDX_SERVERS_HOSTNAME,
	IDX_SERVERS_IP_ADDRESS,
	IDX_SERVERS_NICKNAME,
	IDX_SERVERS_PORT,
	IDX_SERVERS_POLLING_INTERVAL_SEC,
	IDX_SERVERS_RETRY_INTERVAL_SEC,
	IDX_SERVERS_USER_NAME,
	IDX_SERVERS_PASSWORD,
	IDX_SERVERS_DB_NAME,
	IDX_SERVERS_BASE_URL,
	IDX_SERVERS_EXTENDED_INFO,
	NUM_IDX_SERVERS,
};

static const DBAgent::TableProfile tableProfileServers =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SERVERS,
			    COLUMN_DEF_SERVERS,
			    NUM_IDX_SERVERS);

static const ColumnDef COLUMN_DEF_ARM_PLUGINS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"type", // server.type             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"path",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"broker_url",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	4095,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"static_queue_addr",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"serverId",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"tls_certificate_path",            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	// The same as Linux's PATH_MAX without nul
	4095,                              // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"tls_key_path",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	// The same as Linux's PATH_MAX without nul
	4095,                              // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"tls_ca_certificate_path",         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	// The same as Linux's PATH_MAX without nul
	4095,                              // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"tls_enable_verify",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	1,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
}, {
	"uuid",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	36,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_ARM_PLUGINS_ID,
	IDX_ARM_PLUGINS_TYPE,
	IDX_ARM_PLUGINS_PATH,
	IDX_ARM_PLUGINS_BROKER_URL,
	IDX_ARM_PLUGINS_STATIC_QUEUE_ADDR,
	IDX_ARM_PLUGINS_SERVER_ID,
	IDX_ARM_PLUGINS_TLS_CERTIFICATE_PATH,
	IDX_ARM_PLUGINS_TLS_KEY_PATH,
	IDX_ARM_PLUGINS_TLS_CA_CERTIFICATE_PATH,
	IDX_ARM_PLUGINS_TLS_ENABLE_VERIFY,
	IDX_ARM_PLUGINS_UUID,
	NUM_IDX_ARM_PLUGINS,
};

static const DBAgent::TableProfile tableProfileArmPlugins =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_ARM_PLUGINS,
			    COLUMN_DEF_ARM_PLUGINS,
			    NUM_IDX_ARM_PLUGINS);

static const ColumnDef COLUMN_DEF_INCIDENT_TRACKERS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"nickname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"base_url",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"project_id",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"tracker_id",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"user_name",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_INCIDENT_TRACKERS_ID,
	IDX_INCIDENT_TRACKERS_TYPE,
	IDX_INCIDENT_TRACKERS_NICKNAME,
	IDX_INCIDENT_TRACKERS_BASE_URL,
	IDX_INCIDENT_TRACKERS_PROJECT_ID,
	IDX_INCIDENT_TRACKERS_TRACKER_ID,
	IDX_INCIDENT_TRACKERS_USER_NAME,
	IDX_INCIDENT_TRACKERS_PASSWORD,
	NUM_IDX_INCIDENT_TRACKERS,
};

static const DBAgent::TableProfile tableProfileIncidentTrackers =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_INCIDENT_TRACKERS,
			    COLUMN_DEF_INCIDENT_TRACKERS,
			    NUM_IDX_INCIDENT_TRACKERS);

static const ColumnDef COLUMN_DEF_SEVERITY_RANKS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
},
{
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
{
	"color",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
{
	"label",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
{
	"as_important",                    // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
},
};

enum {
	IDX_SEVERITY_RANK_ID,
	IDX_SEVERITY_RANK_STATUS,
	IDX_SEVERITY_RANK_COLOR,
	IDX_SEVERITY_RANK_LABEL,
	IDX_SEVERITY_RANK_AS_IMPORTANT,
	NUM_IDX_SEVERITY_RANKS,
};

static const int columnIndexesSeverityRanksUniqId[] = {
  IDX_SEVERITY_RANK_STATUS, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsSeverityRanks[] = {
  {"SeverityRanksUniqId", (const int *)columnIndexesSeverityRanksUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileSeverityRanks =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_SEVERITY_RANKS,
			    COLUMN_DEF_SEVERITY_RANKS,
			    NUM_IDX_SEVERITY_RANKS,
			    indexDefsSeverityRanks);

static const ColumnDef COLUMN_DEF_CUSTOM_INCIDENT_STATUSES[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"code",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"label",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_CUSTOM_INCIDENT_STATUS_ID,
	IDX_CUSTOM_INCIDENT_STATUS_CODE,
	IDX_CUSTOM_INCIDENT_STATUS_LABEL,
	NUM_IDX_CUSTOM_INCIDENT_STATUSES,
};

static const int columnIndexesCustomIncidentStatusUniqId[] = {
  IDX_CUSTOM_INCIDENT_STATUS_CODE, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsCustomIncidentStatusCodes[] = {
  {"CustomIncidentStatusCodeUniqId", (const int *)columnIndexesCustomIncidentStatusUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileCustomIncidentStatus =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_CUSTOM_INCIDENT_STATUSES,
			    COLUMN_DEF_CUSTOM_INCIDENT_STATUSES,
			    NUM_IDX_CUSTOM_INCIDENT_STATUSES,
			    indexDefsCustomIncidentStatusCodes);

struct DBTablesConfig::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	const int &oldVer = oldPackedVer.minorVer;
	if (oldVer <= 5) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileSystem);
		addColumnsArg.columnIndexes.push_back(
		  IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
		dbAgent.addColumns(addColumnsArg);
	}
	if (oldVer <= 7) {
		// enable copy-on-demand by default
		DBAgent::UpdateArg arg(tableProfileSystem);
		arg.add(IDX_SYSTEM_ENABLE_COPY_ON_DEMAND, 1);
		dbAgent.update(arg);
	}
	if (oldVer == 9) {
		const string oldTableName = "issue_trackers";
		if (dbAgent.isTableExisting(oldTableName))
			dbAgent.renameTable(oldTableName,
			                    TABLE_NAME_INCIDENT_TRACKERS);
	}
	if (oldVer < 11) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileArmPlugins);
		addColumnsArg.columnIndexes.push_back(
			IDX_ARM_PLUGINS_TLS_CERTIFICATE_PATH);
		addColumnsArg.columnIndexes.push_back(
			IDX_ARM_PLUGINS_TLS_KEY_PATH);
		addColumnsArg.columnIndexes.push_back(
			IDX_ARM_PLUGINS_TLS_CA_CERTIFICATE_PATH);
		dbAgent.addColumns(addColumnsArg);
	}
	if (oldVer < 12) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileArmPlugins);
		addColumnsArg.columnIndexes.push_back(
			IDX_ARM_PLUGINS_TLS_ENABLE_VERIFY);
		dbAgent.addColumns(addColumnsArg);
	}
	if (oldVer < 13) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileServerTypes);
		addColumnsArg.columnIndexes.push_back(
			IDX_SERVER_TYPES_PLUGIN_SQL_VERSION);
		addColumnsArg.columnIndexes.push_back(
			IDX_SERVER_TYPES_PLUGIN_ENABLED);
		dbAgent.addColumns(addColumnsArg);
		// enable plugin_sql_version & plugin_enabled by default
		DBAgent::UpdateArg arg(tableProfileServerTypes);
		arg.add(IDX_SERVER_TYPES_PLUGIN_SQL_VERSION, 1);
		arg.add(IDX_SERVER_TYPES_PLUGIN_ENABLED, 1);
		dbAgent.update(arg);
	}
	if (oldVer < 14) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileServers);
		addColumnsArg.columnIndexes.push_back(IDX_SERVERS_BASE_URL);
		dbAgent.addColumns(addColumnsArg);
	}
	if (oldVer < 15) {
		DBAgent::AddColumnsArg addColumnsArg(tableProfileServers);
		addColumnsArg.columnIndexes.push_back(IDX_SERVERS_EXTENDED_INFO);
		dbAgent.addColumns(addColumnsArg);

		// fixup max length of servers.base_url
		dbAgent.changeColumnDef(tableProfileServers,
					"base_url",
					IDX_SERVERS_BASE_URL);
	}
	if (oldVer < 16) {
		// drop the primary key, use unique index instead
		dbAgent.dropPrimaryKey(tableProfileServerTypes.name);

		DBAgent::AddColumnsArg addArgForServerTypes(tableProfileServerTypes);
		addArgForServerTypes.columnIndexes.push_back(
			IDX_SERVER_TYPES_UUID);
		dbAgent.addColumns(addArgForServerTypes);
		DBAgent::AddColumnsArg addArgForArmPlugins(tableProfileArmPlugins);
		addArgForArmPlugins.columnIndexes.push_back(
			IDX_ARM_PLUGINS_UUID);
		dbAgent.addColumns(addArgForArmPlugins);
	}
	return true;
}

// ---------------------------------------------------------------------------
// ServerQueryOption
// ---------------------------------------------------------------------------
struct ServerQueryOption::Impl {
	ServerIdType targetServerId;

	Impl(void)
	: targetServerId(ALL_SERVERS)
	{
	}
};

ServerQueryOption::ServerQueryOption(const UserIdType &userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

ServerQueryOption::ServerQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

ServerQueryOption::~ServerQueryOption()
{
}

static string serverIdCondition(
  const ServerIdType &serverId, const bool &needTableName)
{
	string columnNameStr;
	if (needTableName) {
		columnNameStr =
			tableProfileServers.getFullColumnName(IDX_SERVERS_ID);
	}
	const char *columnName = needTableName ?
	                         columnNameStr.c_str() :
	                         COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName;
	return StringUtils::sprintf("%s=%" FMT_SERVER_ID, columnName, serverId);
}

bool ServerQueryOption::hasPrivilegeCondition(string &condition) const
{
	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_impl->targetServerId != ALL_SERVERS) {
			condition = serverIdCondition(m_impl->targetServerId,
			                              getTableNameAlways());
		}
		return true;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		condition = DBHatohol::getAlwaysFalseCondition();
		return true;
	}

	return false;
}

void ServerQueryOption::setTargetServerId(const ServerIdType &serverId)
{
	m_impl->targetServerId = serverId;
}

string ServerQueryOption::getCondition(void) const
{
	string condition;
	ServerIdType targetId = m_impl->targetServerId;

	if (hasPrivilegeCondition(condition))
		return condition;

	// check allowed servers
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	ServerHostGrpSetMap srvHostGrpSetMap;
	dbUser.getServerHostGrpSetMap(srvHostGrpSetMap, getUserId());

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	if (targetId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetId) != srvHostGrpSetMap.end())
	{
		return serverIdCondition(targetId, getTableNameAlways());
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const ServerIdType &serverId = it->first;

		if (serverId == ALL_SERVERS)
			return "";

		if (!condition.empty())
			condition += " OR ";
		condition += serverIdCondition(serverId, getTableNameAlways());
		++numServers;
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

// ---------------------------------------------------------------------------
// IncidentTrackerQueryOption
// ---------------------------------------------------------------------------
struct IncidentTrackerQueryOption::Impl {
	IncidentTrackerIdType targetId;
	Impl(void)
	: targetId(ALL_INCIDENT_TRACKERS)
	{
	}
};

IncidentTrackerQueryOption::IncidentTrackerQueryOption(const UserIdType &userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

IncidentTrackerQueryOption::IncidentTrackerQueryOption(
  DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

IncidentTrackerQueryOption::~IncidentTrackerQueryOption()
{
}

void IncidentTrackerQueryOption::setTargetId(
  const IncidentTrackerIdType &targetId)
{
	m_impl->targetId = targetId;
}

string IncidentTrackerQueryOption::getCondition(void) const
{
	UserIdType userId = getUserId();

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		return DBHatohol::getAlwaysFalseCondition();
	}

	if (userId != USER_ID_SYSTEM && !has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS))
		return DBHatohol::getAlwaysFalseCondition();

	if (m_impl->targetId != ALL_INCIDENT_TRACKERS) {
		const char *columnName
		  = COLUMN_DEF_INCIDENT_TRACKERS[IDX_INCIDENT_TRACKERS_ID].columnName;
	        return StringUtils::sprintf("%s=%" FMT_INCIDENT_TRACKER_ID,
					    columnName, m_impl->targetId);
	}

	return string();
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesConfig::reset(void)
{
	getSetupInfo().initialized = false;
}

// TODO: Remove this method after replaced our conventional Arm such
// ArmZabbixAPI and ArmNagiosNDOUtils are replaced with HAPI's ones.
bool DBTablesConfig::isHatoholArmPlugin(const MonitoringSystemType &type)
{
	if (type == MONITORING_SYSTEM_HAPI_JSON)
		return true;
	else if (type == MONITORING_SYSTEM_HAPI2)
		return true;
	return false;
}

const DBTables::SetupInfo &DBTablesConfig::getConstSetupInfo(void)
{
	return getSetupInfo();
}

DBTablesConfig::DBTablesConfig(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesConfig::~DBTablesConfig()
{
}

string DBTablesConfig::getDatabaseDir(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_DATABASE_DIR);
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<string>();
}

void DBTablesConfig::setDatabaseDir(const string &dir)
{
	DBAgent::UpdateArg arg(tableProfileSystem);
	arg.add(IDX_SYSTEM_DATABASE_DIR, dir);
	getDBAgent().runTransaction(arg);
}

int  DBTablesConfig::getFaceRestPort(void)
{
	DBAgent::SelectArg arg(tableProfileSystem);
	arg.columnIndexes.push_back(IDX_SYSTEM_FACE_REST_PORT);
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

void DBTablesConfig::setFaceRestPort(int port)
{
	DBAgent::UpdateArg arg(tableProfileSystem);
	arg.add(IDX_SYSTEM_FACE_REST_PORT, port);
	getDBAgent().runTransaction(arg);
}

void DBTablesConfig::registerServerType(const ServerTypeInfo &serverType)
{
	DBAgent::InsertArg arg(tableProfileServerTypes);
	arg.add(serverType.type);
	arg.add(serverType.name);
	arg.add(serverType.parameters);
	arg.add(serverType.pluginPath);
	arg.add(serverType.pluginSQLVersion);
	arg.add(serverType.pluginEnabled);
	arg.add(serverType.uuid);
	arg.upsertOnDuplicate = true;
	getDBAgent().runTransaction(arg);
}

string DBTablesConfig::getDefaultPluginPath(const MonitoringSystemType &type,
					    const string &uuid)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	ServerTypeInfo serverType;
	if (dbConfig.getServerType(serverType, type, uuid))
		return serverType.pluginPath;

	return "";
}

void DBTablesConfig::getServerTypes(ServerTypeInfoVect &serverTypes)
{
	DBAgent::SelectArg arg(tableProfileServerTypes);
	arg.add(IDX_SERVER_TYPES_TYPE);
	arg.add(IDX_SERVER_TYPES_NAME);
	arg.add(IDX_SERVER_TYPES_PARAMETERS);
	arg.add(IDX_SERVER_TYPES_PLUGIN_PATH);
	arg.add(IDX_SERVER_TYPES_UUID);
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	serverTypes.reserve(grpList.size());
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		serverTypes.push_back(ServerTypeInfo());
		ServerTypeInfo &svTypeInfo = serverTypes.back();

		itemGroupStream >> svTypeInfo.type;
		itemGroupStream >> svTypeInfo.name;
		itemGroupStream >> svTypeInfo.parameters;
		itemGroupStream >> svTypeInfo.pluginPath;
		itemGroupStream >> svTypeInfo.uuid;
	}
}

bool DBTablesConfig::getServerType(ServerTypeInfo &serverType,
				   const MonitoringSystemType &type,
				   const string &uuid)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileServerTypes);
	arg.condition = StringUtils::sprintf("%s=%d AND %s=%s",
	  COLUMN_DEF_SERVER_TYPES[IDX_SERVER_TYPES_TYPE].columnName,
	  type,
	  COLUMN_DEF_SERVER_TYPES[IDX_SERVER_TYPES_UUID].columnName,
	  rhs(uuid));
	arg.add(IDX_SERVER_TYPES_TYPE);
	arg.add(IDX_SERVER_TYPES_NAME);
	arg.add(IDX_SERVER_TYPES_PARAMETERS);
	arg.add(IDX_SERVER_TYPES_PLUGIN_PATH);
	arg.add(IDX_SERVER_TYPES_UUID);
	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return false;
	ItemGroupStream itemGroupStream(*grpList.begin());
	itemGroupStream >> serverType.type;
	itemGroupStream >> serverType.name;
	itemGroupStream >> serverType.parameters;
	itemGroupStream >> serverType.pluginPath;
	itemGroupStream >> serverType.uuid;
	return true;
}

bool validHostName(const string &hostName)
{
	// Currently do nothing to pass through IDN (Internationalized Domain
	// Name) to libsoup.
	return true;
}

HatoholError validServerInfo(const MonitoringServerInfo &serverInfo)
{
	if (serverInfo.type <= MONITORING_SYSTEM_UNKNOWN
	    || serverInfo.type >= NUM_MONITORING_SYSTEMS)
		return HTERR_INVALID_MONITORING_SYSTEM_TYPE;
	if (serverInfo.port < 0 || serverInfo.port > 65535)
		return HTERR_INVALID_PORT_NUMBER;
	if (serverInfo.ipAddress.empty() && serverInfo.hostName.empty())
		return HTERR_NO_IP_ADDRESS_AND_HOST_NAME;
	if (!serverInfo.hostName.empty() &&
	    !validHostName(serverInfo.hostName)) {
		return HTERR_INVALID_HOST_NAME;
	}
	if (!serverInfo.ipAddress.empty() &&
	    !Utils::isValidIPAddress(serverInfo.ipAddress)) {
		return HTERR_INVALID_IP_ADDRESS;
	}
	if (serverInfo.pollingIntervalSec < MonitoringServerInfo::MIN_POLLING_INTERVAL_SEC ||
	    serverInfo.pollingIntervalSec > MonitoringServerInfo::MAX_POLLING_INTERVAL_SEC) {
		return HTERR_INVALID_POLLING_INTERVAL;
	}
	if (serverInfo.retryIntervalSec < MonitoringServerInfo::MIN_RETRY_INTERVAL_SEC ||
	    serverInfo.retryIntervalSec > MonitoringServerInfo::MAX_RETRY_INTERVAL_SEC) {
		return HTERR_INVALID_RETRY_INTERVAL;
	}
	return HTERR_OK;
}

HatoholError DBTablesConfig::addTargetServer(
  MonitoringServerInfo &monitoringServerInfo,
  ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_CREATE_SERVER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validServerInfo(monitoringServerInfo);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError              err;
		DBAgent::InsertArg        arg;
		string                    condForHap;
		DBTablesConfig           *obj;
		MonitoringServerInfo     &monitoringServerInfo;
		ArmPluginInfo            &armPluginInfo;

		TrxProc(DBTablesConfig       *_obj,
		        MonitoringServerInfo &_monitoringServerInfo,
		        ArmPluginInfo        &_armPluginInfo)
		: arg(tableProfileServers),
		  obj(_obj),
		  monitoringServerInfo(_monitoringServerInfo),
		  armPluginInfo(_armPluginInfo)
		{
			arg.add(monitoringServerInfo.id);
			arg.add(monitoringServerInfo.type);
			arg.add(monitoringServerInfo.hostName);
			arg.add(monitoringServerInfo.ipAddress);
			arg.add(monitoringServerInfo.nickname);
			arg.add(monitoringServerInfo.port);
			arg.add(monitoringServerInfo.pollingIntervalSec);
			arg.add(monitoringServerInfo.retryIntervalSec);
			arg.add(monitoringServerInfo.userName);
			arg.add(monitoringServerInfo.password);
			arg.add(monitoringServerInfo.dbName);
			arg.add(monitoringServerInfo.baseURL);
			arg.add(monitoringServerInfo.extendedInfo);
		}

		bool preproc(DBAgent &dbAgent) override
		{
			err = obj->preprocForSaveArmPlguinInfo(
			        monitoringServerInfo,
			        armPluginInfo, condForHap);
			return (err == HTERR_OK);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.insert(arg);
			if (monitoringServerInfo.id == AUTO_INCREMENT_VALUE) {
				monitoringServerInfo.id =
				  dbAgent.getLastInsertId();
			}
			// TODO: Add AccessInfo for the server to enable
			// the operator to access to it
			if (!condForHap.empty()) {
				armPluginInfo.serverId =
				  monitoringServerInfo.id;
			}
			err = obj->saveArmPluginInfoIfNeededWithoutTransaction(
			        armPluginInfo, condForHap);
			if (err != HTERR_OK)
				dbAgent.rollback();
		}
	} trx(this, monitoringServerInfo, armPluginInfo);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesConfig::updateTargetServer(
  MonitoringServerInfo &monitoringServerInfo,
  ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege)
{
	if (!canUpdateTargetServer(monitoringServerInfo, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validServerInfo(monitoringServerInfo);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError              err;
		DBAgent::UpdateArg        arg;
		string                    condForHap;
		DBTablesConfig           *obj;
		MonitoringServerInfo     &monitoringServerInfo;
		ArmPluginInfo            &armPluginInfo;

		TrxProc(DBTablesConfig       *_obj,
		        MonitoringServerInfo &_monitoringServerInfo,
		        ArmPluginInfo        &_armPluginInfo)
		: arg(tableProfileServers),
		  obj(_obj),
		  monitoringServerInfo(_monitoringServerInfo),
		  armPluginInfo(_armPluginInfo)
		{
		}

		bool preproc(DBAgent &dbAgent) override
		{
			err = obj->preprocForSaveArmPlguinInfo(
			        monitoringServerInfo,
			        armPluginInfo, condForHap);
			return (err == HTERR_OK);
		}

		bool hasRecord(DBAgent &dbAgent) {
			return dbAgent.isRecordExisting(TABLE_NAME_SERVERS,
							arg.condition);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (!hasRecord(dbAgent)) {
				err = HTERR_NOT_FOUND_TARGET_RECORD;
				return;
			}

			dbAgent.update(arg);
			err = obj->saveArmPluginInfoIfNeededWithoutTransaction(
			        armPluginInfo, condForHap);
			if (err != HTERR_OK)
				dbAgent.rollback();
		}
	} trx(this, monitoringServerInfo, armPluginInfo);

	trx.arg.add(IDX_SERVERS_TYPE,       monitoringServerInfo.type);
	trx.arg.add(IDX_SERVERS_HOSTNAME,   monitoringServerInfo.hostName);
	trx.arg.add(IDX_SERVERS_IP_ADDRESS, monitoringServerInfo.ipAddress);
	trx.arg.add(IDX_SERVERS_NICKNAME,   monitoringServerInfo.nickname);
	trx.arg.add(IDX_SERVERS_PORT,       monitoringServerInfo.port);
	trx.arg.add(IDX_SERVERS_POLLING_INTERVAL_SEC,
	            monitoringServerInfo.pollingIntervalSec);
	trx.arg.add(IDX_SERVERS_RETRY_INTERVAL_SEC,
	            monitoringServerInfo.retryIntervalSec);
	trx.arg.add(IDX_SERVERS_USER_NAME,  monitoringServerInfo.userName);
	trx.arg.add(IDX_SERVERS_PASSWORD,   monitoringServerInfo.password);
	trx.arg.add(IDX_SERVERS_DB_NAME,    monitoringServerInfo.dbName);
	trx.arg.add(IDX_SERVERS_BASE_URL,   monitoringServerInfo.baseURL);
	trx.arg.add(IDX_SERVERS_EXTENDED_INFO, monitoringServerInfo.extendedInfo);
	trx.arg.condition =
	   StringUtils::sprintf("id=%u", monitoringServerInfo.id);

	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError DBTablesConfig::deleteTargetServer(
  const ServerIdType &serverId,
  const OperationPrivilege &privilege)
{
	if (!canDeleteTargetServer(serverId, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg argArmPlugins;
		DBAgent::DeleteArg argServers;

		TrxProc(void)
		: argArmPlugins(tableProfileArmPlugins),
		  argServers(tableProfileServers)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(argArmPlugins);
			dbAgent.deleteRows(argServers);
		}
	} trx;
	trx.argServers.condition =
	   StringUtils::sprintf("%s=%" FMT_SERVER_ID,
	                        COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName,
	                        serverId);
	preprocForDeleteArmPluginInfo(serverId, trx.argArmPlugins.condition);
	getDBAgent().runTransaction(trx);
	return HTERR_OK;
}

void DBTablesConfig::getTargetServers(
  MonitoringServerInfoList &monitoringServers, ServerQueryOption &option,
  ArmPluginInfoVect *armPluginInfoVect)
{
	// TODO: We'd better consider if we should use a query like,
	//
	// select servers.id,servers.type,... from servers inner join
	// access_list on servers.id=access_list.server_id where user_id=5
	//
	// The current query statement uses a little complicated where clause,
	// for which the indexing mechanism may not be effective.

	DBClientJoinBuilder builder(tableProfileServers, &option);
	builder.add(IDX_SERVERS_ID);
	builder.add(IDX_SERVERS_TYPE);
	builder.add(IDX_SERVERS_HOSTNAME);
	builder.add(IDX_SERVERS_IP_ADDRESS);
	builder.add(IDX_SERVERS_NICKNAME);
	builder.add(IDX_SERVERS_PORT);
	builder.add(IDX_SERVERS_POLLING_INTERVAL_SEC);
	builder.add(IDX_SERVERS_RETRY_INTERVAL_SEC);
	builder.add(IDX_SERVERS_USER_NAME);
	builder.add(IDX_SERVERS_PASSWORD);
	builder.add(IDX_SERVERS_DB_NAME);
	builder.add(IDX_SERVERS_BASE_URL);
	builder.add(IDX_SERVERS_EXTENDED_INFO);

	builder.addTable(tableProfileArmPlugins, DBClientJoinBuilder::LEFT_JOIN,
	                 IDX_SERVERS_ID, IDX_ARM_PLUGINS_SERVER_ID);
	builder.add(IDX_ARM_PLUGINS_ID);
	builder.add(IDX_ARM_PLUGINS_TYPE);
	builder.add(IDX_ARM_PLUGINS_PATH);
	builder.add(IDX_ARM_PLUGINS_BROKER_URL);
	builder.add(IDX_ARM_PLUGINS_STATIC_QUEUE_ADDR);
	builder.add(IDX_ARM_PLUGINS_SERVER_ID);
	builder.add(IDX_ARM_PLUGINS_TLS_CERTIFICATE_PATH);
	builder.add(IDX_ARM_PLUGINS_TLS_KEY_PATH);
	builder.add(IDX_ARM_PLUGINS_TLS_CA_CERTIFICATE_PATH);
	builder.add(IDX_ARM_PLUGINS_TLS_ENABLE_VERIFY);
	builder.add(IDX_ARM_PLUGINS_UUID);

	getDBAgent().runTransaction(builder.getSelectExArg());

	// check the result and copy
	ItemTablePtr &dataTable = builder.getSelectExArg().dataTable;
	if (armPluginInfoVect) {
		const size_t reserveSize =
		  armPluginInfoVect->size() + dataTable->getNumberOfRows();
		armPluginInfoVect->reserve(reserveSize);
	}

	const ItemGroupList &grpList = dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		monitoringServers.push_back(MonitoringServerInfo());
		MonitoringServerInfo &svInfo = monitoringServers.back();

		itemGroupStream >> svInfo.id;
		itemGroupStream >> svInfo.type;
		itemGroupStream >> svInfo.hostName;
		itemGroupStream >> svInfo.ipAddress;
		itemGroupStream >> svInfo.nickname;
		itemGroupStream >> svInfo.port;
		itemGroupStream >> svInfo.pollingIntervalSec;
		itemGroupStream >> svInfo.retryIntervalSec;
		itemGroupStream >> svInfo.userName;
		itemGroupStream >> svInfo.password;
		itemGroupStream >> svInfo.dbName;
		itemGroupStream >> svInfo.baseURL;
		itemGroupStream >> svInfo.extendedInfo;

		// ArmPluginInfo
		if (!armPluginInfoVect)
			continue;

		ArmPluginInfo armPluginInfo;
		const ItemData *itemData = itemGroupStream.getItem();
		if (itemData->isNull())
			armPluginInfo.id = INVALID_ARM_PLUGIN_INFO_ID;
		else
			readArmPluginStream(itemGroupStream, armPluginInfo);
		armPluginInfoVect->push_back(armPluginInfo);
	}
}

void DBTablesConfig::getServerIdSet(ServerIdSet &serverIdSet,
                                    DataQueryContext *dataQueryContext)
{
	// TODO: We'd better use the access_list table with a query like
	// select distinct server_id from access_list where user_id=12
	ServerQueryOption option(dataQueryContext);

	DBAgent::SelectExArg arg(tableProfileServers);
	arg.add(IDX_SERVERS_ID);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ServerIdType id = *(*itemGrpItr)->getItemPtrAt(0);
		serverIdSet.insert(id);
	}
}

void DBTablesConfig::getArmPluginInfo(ArmPluginInfoVect &armPluginVect)
{
	DBAgent::SelectExArg arg(tableProfileArmPlugins);
	selectArmPluginInfo(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	armPluginVect.reserve(grpList.size());
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		ArmPluginInfo armPluginInfo;
		readArmPluginStream(itemGroupStream, armPluginInfo);
		armPluginVect.push_back(armPluginInfo);
	}
}

bool DBTablesConfig::getArmPluginInfo(ArmPluginInfo &armPluginInfo,
                                      const ServerIdType &serverId)
{
	DBAgent::SelectExArg arg(tableProfileArmPlugins);
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_SERVER_ID,
	  COLUMN_DEF_ARM_PLUGINS[IDX_ARM_PLUGINS_SERVER_ID].columnName, serverId);
	selectArmPluginInfo(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return false;
	ItemGroupStream itemGroupStream(*grpList.begin());
	readArmPluginStream(itemGroupStream, armPluginInfo);
	return true;
}

HatoholError DBTablesConfig::saveArmPluginInfo(ArmPluginInfo &armPluginInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError    err;
		string          condition;
		DBTablesConfig *obj;
		ArmPluginInfo  &armPluginInfo;

		TrxProc(DBTablesConfig *_obj,
		        ArmPluginInfo  &_armPluginInfo)
		: obj(_obj),
		  armPluginInfo(_armPluginInfo)
		{
		}

		bool preproc(DBAgent &dbAgent) override
		{
			err = obj->preprocForSaveArmPlguinInfo(armPluginInfo,
			                                       condition);
			return (err == HTERR_OK);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			err = obj->saveArmPluginInfoWithoutTransaction(
			        armPluginInfo, condition);
		}
	} trx(this, armPluginInfo);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

HatoholError validIncidentTrackerInfo(
  const IncidentTrackerInfo &incidentTrackerInfo)
{
	if (incidentTrackerInfo.type <= INCIDENT_TRACKER_UNKNOWN ||
	    incidentTrackerInfo.type >= NUM_INCIDENT_TRACKERS)
		return HTERR_INVALID_INCIDENT_TRACKER_TYPE;

	if (incidentTrackerInfo.type != INCIDENT_TRACKER_HATOHOL) {
		if (incidentTrackerInfo.baseURL.empty())
			return HTERR_NO_INCIDENT_TRACKER_LOCATION;
		if (!Utils::isValidURI(incidentTrackerInfo.baseURL))
			return HTERR_INVALID_URL;
	}
	return HTERR_OK;
}

HatoholError DBTablesConfig::addIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_CREATE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validIncidentTrackerInfo(incidentTrackerInfo);
	if (err != HTERR_OK)
		return err;

	DBAgent::InsertArg arg(tableProfileIncidentTrackers);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(incidentTrackerInfo.type);
	arg.add(incidentTrackerInfo.nickname);
	arg.add(incidentTrackerInfo.baseURL);
	arg.add(incidentTrackerInfo.projectId);
	arg.add(incidentTrackerInfo.trackerId);
	arg.add(incidentTrackerInfo.userName);
	arg.add(incidentTrackerInfo.password);

	getDBAgent().runTransaction(arg, incidentTrackerInfo.id);
	return HTERR_OK;
}

HatoholError DBTablesConfig::updateIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_UPDATE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	HatoholError err = validIncidentTrackerInfo(incidentTrackerInfo);
	if (err != HTERR_OK)
		return err;

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		DBAgent::UpdateArg arg;

		TrxProc(void)
		: arg(tableProfileIncidentTrackers)
		{
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(
				 TABLE_NAME_INCIDENT_TRACKERS,
				 arg.condition);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			if (!hasRecord(dbAgent)) {
				err = HTERR_NOT_FOUND_TARGET_RECORD;
				return;
			}
			dbAgent.update(arg);
			err = HTERR_OK;
		}
	} trx;

	DBAgent::UpdateArg &arg = trx.arg;
	arg.add(IDX_INCIDENT_TRACKERS_TYPE,       incidentTrackerInfo.type);
	arg.add(IDX_INCIDENT_TRACKERS_NICKNAME,   incidentTrackerInfo.nickname);
	arg.add(IDX_INCIDENT_TRACKERS_BASE_URL,   incidentTrackerInfo.baseURL);
	arg.add(IDX_INCIDENT_TRACKERS_PROJECT_ID, incidentTrackerInfo.projectId);
	arg.add(IDX_INCIDENT_TRACKERS_TRACKER_ID, incidentTrackerInfo.trackerId);
	arg.add(IDX_INCIDENT_TRACKERS_USER_NAME,  incidentTrackerInfo.userName);
	arg.add(IDX_INCIDENT_TRACKERS_PASSWORD,   incidentTrackerInfo.password);
	arg.condition = StringUtils::sprintf("id=%" FMT_INCIDENT_TRACKER_ID,
	                                     incidentTrackerInfo.id);
	getDBAgent().runTransaction(trx);
	return trx.err;
}

void DBTablesConfig::getIncidentTrackers(
  IncidentTrackerInfoVect &incidentTrackerInfoVect,
  IncidentTrackerQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileIncidentTrackers);
	arg.add(IDX_INCIDENT_TRACKERS_ID);
	arg.add(IDX_INCIDENT_TRACKERS_TYPE);
	arg.add(IDX_INCIDENT_TRACKERS_NICKNAME);
	arg.add(IDX_INCIDENT_TRACKERS_BASE_URL);
	arg.add(IDX_INCIDENT_TRACKERS_PROJECT_ID);
	arg.add(IDX_INCIDENT_TRACKERS_TRACKER_ID);
	arg.add(IDX_INCIDENT_TRACKERS_USER_NAME);
	arg.add(IDX_INCIDENT_TRACKERS_PASSWORD);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		incidentTrackerInfoVect.push_back(IncidentTrackerInfo());
		IncidentTrackerInfo &info = incidentTrackerInfoVect.back();

		itemGroupStream >> info.id;
		itemGroupStream >> info.type;
		itemGroupStream >> info.nickname;
		itemGroupStream >> info.baseURL;
		itemGroupStream >> info.projectId;
		itemGroupStream >> info.trackerId;
		itemGroupStream >> info.userName;
		itemGroupStream >> info.password;
	}
}

HatoholError DBTablesConfig::deleteIncidentTracker(
  const IncidentTrackerIdType &incidentTrackerId,
  const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_DELETE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgent::DeleteArg arg(tableProfileIncidentTrackers);
	const ColumnDef &colId
	  = COLUMN_DEF_INCIDENT_TRACKERS[IDX_INCIDENT_TRACKERS_ID];
	arg.condition = StringUtils::sprintf("%s=%" FMT_INCIDENT_TRACKER_ID,
	                                     colId.columnName, incidentTrackerId);

	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

void DBTablesConfig::getIncidentTrackerIdSet(
  IncidentTrackerIdSet &incidentTrackerIdSet)
{
	IncidentTrackerInfoVect incidentTrackersVect;
	IncidentTrackerQueryOption option(USER_ID_SYSTEM);

	getIncidentTrackers(incidentTrackersVect, option);
	if (incidentTrackersVect.empty())
		return;
	IncidentTrackerInfoVectIterator it = incidentTrackersVect.begin();
	for(; it != incidentTrackersVect.end();++it) {
		const IncidentTrackerInfo &incidentTrackerInfo = *it;
		incidentTrackerIdSet.insert(incidentTrackerInfo.id);
	}
}

void SeverityRankInfo::initialize(SeverityRankInfo &severityRankInfo)
{
	severityRankInfo.id = INVALID_SEVERITY_RANK_ID;
	severityRankInfo.status = INVALID_SEVERITY_STATUS_TYPE;
	severityRankInfo.color = "#000000";
	severityRankInfo.label = "";
	severityRankInfo.asImportant = false;
}

HatoholError DBTablesConfig::upsertSeverityRankInfo(
  SeverityRankInfo &severityRankInfo,
  const OperationPrivilege &privilege)
{
	HatoholError err =
		checkPrivilegeForSeverityRankAdd(privilege, severityRankInfo);
	if (err != HTERR_OK)
		return err;

	DBAgent::InsertArg arg(tableProfileSeverityRanks);

	arg.add(severityRankInfo.id);
	arg.add(severityRankInfo.status);
	arg.add(severityRankInfo.color);
	arg.add(severityRankInfo.label);
	arg.add(severityRankInfo.asImportant);
	arg.upsertOnDuplicate = true;

	getDBAgent().runTransaction(arg, severityRankInfo.id);
	return err;
}

HatoholError DBTablesConfig::updateSeverityRankInfo(
  SeverityRankInfo &severityRankInfo,
  const OperationPrivilege &privilege)
{
	HatoholError err =
		checkPrivilegeForSeverityRankUpdate(privilege, severityRankInfo);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileSeverityRanks);

	const char *severityRankInfoIdColumnName =
	  COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_SEVERITY_RANK_ID,
	                                     severityRankInfoIdColumnName,
	                                     severityRankInfo.id);
	arg.add(IDX_SEVERITY_RANK_STATUS, severityRankInfo.status);
	arg.add(IDX_SEVERITY_RANK_COLOR, severityRankInfo.color);
	arg.add(IDX_SEVERITY_RANK_LABEL, severityRankInfo.label);
	arg.add(IDX_SEVERITY_RANK_AS_IMPORTANT, severityRankInfo.asImportant);

	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

void DBTablesConfig::getSeverityRanks(
  SeverityRankInfoVect &severityRankInfoVect,
  const SeverityRankQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileSeverityRanks);
	arg.add(IDX_SEVERITY_RANK_ID);
	arg.add(IDX_SEVERITY_RANK_STATUS);
	arg.add(IDX_SEVERITY_RANK_COLOR);
	arg.add(IDX_SEVERITY_RANK_LABEL);
	arg.add(IDX_SEVERITY_RANK_AS_IMPORTANT);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	severityRankInfoVect.reserve(grpList.size());
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		SeverityRankInfo severityRankInfo;
		int asImportant;

		itemGroupStream >> severityRankInfo.id;
		itemGroupStream >> severityRankInfo.status;
		itemGroupStream >> severityRankInfo.color;
		itemGroupStream >> severityRankInfo.label;
		itemGroupStream >> asImportant;
		severityRankInfo.asImportant = static_cast<bool>(asImportant);

		severityRankInfoVect.push_back(severityRankInfo);
	}
}

static string makeSeverityRankIdListCondition(
  const std::list<SeverityRankIdType> &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_SEVERITY_RANK_ID, id);
	}

	condition += ")";
	return condition;
}

static string makeConditionForSeverityRankDelete(
  const std::list<SeverityRankIdType> &idList,
  const OperationPrivilege &privilege)
{
	string condition = makeSeverityRankIdListCondition(idList);

	return condition;
}

HatoholError DBTablesConfig::deleteSeverityRanks(
  const std::list<SeverityRankIdType> &idList,
  const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForSeverityRankDelete(privilege, idList);
	if (err != HTERR_OK)
		return err;

	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileSeverityRanks),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForSeverityRankDelete(idList, privilege);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

void CustomIncidentStatus::initialize(CustomIncidentStatus &customIncidentStatus)
{
	customIncidentStatus.id = INVALID_CUSTOM_INCIDENT_STATUS_ID;
	customIncidentStatus.code = "__INITIAL_CODE";
	customIncidentStatus.label = "";
}

HatoholError DBTablesConfig::upsertCustomIncidentStatus(
  CustomIncidentStatus &customIncidentStatus,
  const OperationPrivilege &privilege)
{
	HatoholError err =
	  checkPrivilegeForCustomIncidentStatusAdd(privilege, customIncidentStatus);
	if (err != HTERR_OK)
		return err;

	DBAgent::InsertArg arg(tableProfileCustomIncidentStatus);

	arg.add(customIncidentStatus.id);
	arg.add(customIncidentStatus.code);
	arg.add(customIncidentStatus.label);
	arg.upsertOnDuplicate = true;

	getDBAgent().runTransaction(arg, customIncidentStatus.id);
	return err;
}

HatoholError DBTablesConfig::updateCustomIncidentStatus(
  CustomIncidentStatus &customIncidentStatus,
  const OperationPrivilege &privilege)
{
	HatoholError err =
	  checkPrivilegeForCustomIncidentStatusUpdate(privilege,
						      customIncidentStatus);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileCustomIncidentStatus);

	const char *customIncidentStatusIdColumnName =
	  COLUMN_DEF_CUSTOM_INCIDENT_STATUSES[IDX_CUSTOM_INCIDENT_STATUS_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_CUSTOM_INCIDENT_STATUS_ID,
	                                     customIncidentStatusIdColumnName,
	                                     customIncidentStatus.id);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_ID, customIncidentStatus.id);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_CODE, customIncidentStatus.code);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_LABEL, customIncidentStatus.label);

	getDBAgent().runTransaction(arg);
	return err;
}

void DBTablesConfig::getCustomIncidentStatuses(
  std::vector<CustomIncidentStatus> &customIncidentStatusVect,
  const CustomIncidentStatusesQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileCustomIncidentStatus);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_ID);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_CODE);
	arg.add(IDX_CUSTOM_INCIDENT_STATUS_LABEL);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	customIncidentStatusVect.reserve(grpList.size());
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		CustomIncidentStatus customIncidentStatus;

		itemGroupStream >> customIncidentStatus.id;
		itemGroupStream >> customIncidentStatus.code;
		itemGroupStream >> customIncidentStatus.label;

		customIncidentStatusVect.push_back(customIncidentStatus);
	}
}

static string makeCustomIncidentStatusIdListCondition(
  const std::list<CustomIncidentStatusIdType> &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_CUSTOM_INCIDENT_STATUSES[IDX_CUSTOM_INCIDENT_STATUS_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_CUSTOM_INCIDENT_STATUS_ID, id);
	}

	condition += ")";
	return condition;
}

static string makeConditionForCustomIncidentStatusDelete(
  const std::list<CustomIncidentStatusIdType> &idList,
  const OperationPrivilege &privilege)
{
	string condition = makeCustomIncidentStatusIdListCondition(idList);

	return condition;
}

HatoholError DBTablesConfig::deleteCustomIncidentStatus(
  const std::list<CustomIncidentStatusIdType> &idList,
  const OperationPrivilege &privilege)
{
	HatoholError err =
		checkPrivilegeForCustomIncidentStatusDelete(privilege, idList);
	if (err != HTERR_OK)
		return err;

	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileCustomIncidentStatus),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition =
		makeConditionForCustomIncidentStatusDelete(idList, privilege);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

// ---------------------------------------------------------------------------
// SeverityRankQueryOption
// ---------------------------------------------------------------------------

struct SeverityRankQueryOption::Impl {
	static const string conditionTemplate;

	SeverityRankQueryOption       *option;
	TriggerSeverityType           status;
	std::list<SeverityRankIdType> idList;
	string                        color;
	string                        label;
	int                           asImportant;

	Impl(SeverityRankQueryOption *_option)
	: option(_option), status(TRIGGER_SEVERITY_ALL),
	  asImportant(ALL_SEVERITY_RANK_AS_IMPORTANT)
	{
	}

	static string makeConditionTemplate(void);
	string getSeverityRankStatusCondition(void);
	string getSeverityRankAsImportantCondition(void);
};

const string SeverityRankQueryOption::Impl::conditionTemplate
  = makeConditionTemplate();

string SeverityRankQueryOption::Impl::makeConditionTemplate(void)
{
	string cond;

	// status;
	const ColumnDef &colDefStatus =
	  COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_STATUS];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%d))",
	  colDefStatus.columnName, colDefStatus.columnName);
	cond += " AND ";

	// color;
	const ColumnDef &colDefColor =
		COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_COLOR];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%s))",
	  colDefColor.columnName, colDefColor.columnName);
	cond += " AND ";

	// label;
	const ColumnDef &colDefLabel =
		COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_LABEL];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%s))",
	  colDefLabel.columnName, colDefLabel.columnName);
	cond += " AND ";

	// asImportant
	const ColumnDef &colDefAsImportant =
		COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_AS_IMPORTANT];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%s))",
	  colDefAsImportant.columnName, colDefAsImportant.columnName);
	cond += " AND ";
	return cond;
}

string SeverityRankQueryOption::Impl::getSeverityRankStatusCondition(void)
{
	string condition;
	switch(status) {
	case TRIGGER_SEVERITY_ALL:
		return condition;
	default:
		condition += StringUtils::sprintf("status=%d", static_cast<int>(status));
		return condition;
	}
}

string SeverityRankQueryOption::Impl::getSeverityRankAsImportantCondition(void)
{
	string condition;
	switch(asImportant) {
	case ALL_SEVERITY_RANK_AS_IMPORTANT :
		return condition;
	default:
		condition += StringUtils::sprintf(
		  "as_important=%d", static_cast<int>(asImportant));
		return condition;
	}
}

SeverityRankQueryOption::SeverityRankQueryOption(const UserIdType &userId)
: DataQueryOption(userId), m_impl(new Impl(this))
{
}

SeverityRankQueryOption::SeverityRankQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext), m_impl(new Impl(this))
{
}

SeverityRankQueryOption::~SeverityRankQueryOption()
{
}

void SeverityRankQueryOption::setTargetStatus(const TriggerSeverityType &status)
{
	m_impl->status = status;
}

const TriggerSeverityType SeverityRankQueryOption::getTargetStatus(void)
{
	return m_impl->status;
}

void SeverityRankQueryOption::setTargetColor(const string &color)
{
	m_impl->color = color;
}

const string SeverityRankQueryOption::getTargetColor(void)
{
	return m_impl->color;
}

void SeverityRankQueryOption::setTargetIdList(const list<SeverityRankIdType> &idList)
{
	m_impl->idList = idList;
}

const list<SeverityRankIdType> SeverityRankQueryOption::getTargetIdList(void) {
	return m_impl->idList;
}

void SeverityRankQueryOption::setTargetLabel(const string &label)
{
	m_impl->label = label;
}

const string SeverityRankQueryOption::getTargetLabel(void)
{
	return m_impl->label;
}

void SeverityRankQueryOption::setTargetAsImportant(const int &asImportant)
{
	m_impl->asImportant = asImportant;
}

const int SeverityRankQueryOption::getTargetAsImportant(void)
{
	return m_impl->asImportant;
}

string SeverityRankQueryOption::getCondition(void) const
{
	string condition = DataQueryOption::getCondition();

	// filter by status
	string severityRankStatusCondition = m_impl->getSeverityRankStatusCondition();
	if (!severityRankStatusCondition.empty()) {
		addCondition(condition, severityRankStatusCondition);
	}

	// filter by ID list
	if (!m_impl->idList.empty()) {
		addCondition(condition,
		             makeSeverityRankIdListCondition(m_impl->idList));
	}

	HATOHOL_ASSERT(!m_impl->conditionTemplate.empty(),
	               "SeverityRank condition template is empty.");

	if (!m_impl->color.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string colorCondition = StringUtils::sprintf(
		  "%s=%s",
		  COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_COLOR].columnName,
		  rhs(m_impl->color.c_str()));
		addCondition(condition, colorCondition);
	}

	if (!m_impl->label.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string labelCondition = StringUtils::sprintf(
		  "%s=%s",
		  COLUMN_DEF_SEVERITY_RANKS[IDX_SEVERITY_RANK_LABEL].columnName,
		  rhs(m_impl->label.c_str()));
		addCondition(condition, labelCondition);
	}

	// filter by asImportant
	string severityRankAsImportantCondition =
		m_impl->getSeverityRankAsImportantCondition();
	if (!severityRankAsImportantCondition.empty()) {
		addCondition(condition, severityRankAsImportantCondition);
	}


	return condition;
}

// ---------------------------------------------------------------------------
// CustomIncidentStatusesQueryOption
// ---------------------------------------------------------------------------

struct CustomIncidentStatusesQueryOption::Impl {
	static const string conditionTemplate;

	CustomIncidentStatusesQueryOption *option;
	string                            code;
	string                            label;
	std::list<CustomIncidentStatusIdType> idList;

	Impl(CustomIncidentStatusesQueryOption *_option)
	: option(_option)
	{
	}
};

CustomIncidentStatusesQueryOption::CustomIncidentStatusesQueryOption(
  const UserIdType &userId)
: DataQueryOption(userId), m_impl(new Impl(this))
{
}

CustomIncidentStatusesQueryOption::CustomIncidentStatusesQueryOption(
  DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext), m_impl(new Impl(this))
{
}

CustomIncidentStatusesQueryOption::~CustomIncidentStatusesQueryOption()
{
}

void CustomIncidentStatusesQueryOption::setTargetCode(const string &code)
{
	m_impl->code = code;
}

const string CustomIncidentStatusesQueryOption::getTargetCode(void)
{
	return m_impl->code;
}

void CustomIncidentStatusesQueryOption::setTargetLabel(const string &label)
{
	m_impl->label = label;
}

const string CustomIncidentStatusesQueryOption::getTargetLabel(void)
{
	return m_impl->label;
}

void CustomIncidentStatusesQueryOption::setTargetIdList(
  const list<CustomIncidentStatusIdType> &idList)
{
	m_impl->idList = idList;
}

const list<CustomIncidentStatusIdType>
&CustomIncidentStatusesQueryOption::getTargetIdList(void) {
	return m_impl->idList;
}

string CustomIncidentStatusesQueryOption::getCondition(void) const
{
	string condition = DataQueryOption::getCondition();

	// filter by ID list
	if (!m_impl->idList.empty()) {
		addCondition(condition,
		             makeCustomIncidentStatusIdListCondition(m_impl->idList));
	}

	if (!m_impl->code.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string codeCondition = StringUtils::sprintf(
		  "%s=%s",
		  COLUMN_DEF_CUSTOM_INCIDENT_STATUSES[IDX_CUSTOM_INCIDENT_STATUS_CODE].columnName,
		  rhs(m_impl->code.c_str()));
		addCondition(condition, codeCondition);
	}

	if (!m_impl->label.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string labelCondition = StringUtils::sprintf(
		  "%s=%s",
		  COLUMN_DEF_CUSTOM_INCIDENT_STATUSES[IDX_CUSTOM_INCIDENT_STATUS_LABEL].columnName,
		  rhs(m_impl->label.c_str()));
		addCondition(condition, labelCondition);
	}

	return condition;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBTables::SetupInfo &DBTablesConfig::getSetupInfo(void)
{
	static const TableSetupInfo TABLE_INFO[] = {
	{
		&tableProfileSystem,
		tableInitializerSystem,
	}, {
		&tableProfileServerTypes,
	}, {
		&tableProfileServers,
	}, {
		&tableProfileArmPlugins,
	}, {
		&tableProfileIncidentTrackers,
	}, {
		&tableProfileSeverityRanks,
	}, {
		&tableProfileCustomIncidentStatus,
	}
	};

	static SetupInfo setupInfo = {
		DBTablesId::CONFIG,
		CONFIG_DB_VERSION,
		ARRAY_SIZE(TABLE_INFO),
		TABLE_INFO,
		&updateDB,
	};
	return setupInfo;
}

void DBTablesConfig::tableInitializerSystem(DBAgent &dbAgent, void *data)
{
	const ColumnDef &columnDefDatabaseDir =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_DATABASE_DIR];
	const ColumnDef &columnDefFaceRestPort =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_FACE_REST_PORT];
	const ColumnDef &columnDefEnableCopyOnDemand =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_ENABLE_COPY_ON_DEMAND];

	DBAgent::InsertArg arg(tableProfileSystem);
	arg.add(columnDefDatabaseDir.defaultValue);
	arg.add(0); // enable_face_mysql
	arg.add(atoi(columnDefFaceRestPort.defaultValue));
	arg.add(atoi(columnDefEnableCopyOnDemand.defaultValue));
	dbAgent.insert(arg);
}

bool DBTablesConfig::canUpdateTargetServer(
  MonitoringServerInfo &monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_UPDATE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_UPDATE_SERVER))
		return false;

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.isAccessible(monitoringServerInfo.id, privilege, false);
}

bool DBTablesConfig::canDeleteTargetServer(
  const ServerIdType &serverId, const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_DELETE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_DELETE_SERVER))
		return false;

	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.isAccessible(serverId, privilege);
}

void DBTablesConfig::selectArmPluginInfo(DBAgent::SelectExArg &arg)
{
	arg.add(IDX_ARM_PLUGINS_ID);
	arg.add(IDX_ARM_PLUGINS_TYPE);
	arg.add(IDX_ARM_PLUGINS_PATH);
	arg.add(IDX_ARM_PLUGINS_BROKER_URL);
	arg.add(IDX_ARM_PLUGINS_STATIC_QUEUE_ADDR);
	arg.add(IDX_ARM_PLUGINS_SERVER_ID);
	arg.add(IDX_ARM_PLUGINS_TLS_CERTIFICATE_PATH);
	arg.add(IDX_ARM_PLUGINS_TLS_KEY_PATH);
	arg.add(IDX_ARM_PLUGINS_TLS_CA_CERTIFICATE_PATH);
	arg.add(IDX_ARM_PLUGINS_TLS_ENABLE_VERIFY);
	arg.add(IDX_ARM_PLUGINS_UUID);

	getDBAgent().runTransaction(arg);
}

void DBTablesConfig::readArmPluginStream(
  ItemGroupStream &itemGroupStream, ArmPluginInfo &armPluginInfo)
{
	itemGroupStream >> armPluginInfo.id;
	itemGroupStream >> armPluginInfo.type;
	itemGroupStream >> armPluginInfo.path;
	itemGroupStream >> armPluginInfo.brokerUrl;
	itemGroupStream >> armPluginInfo.staticQueueAddress;
	itemGroupStream >> armPluginInfo.serverId;
	itemGroupStream >> armPluginInfo.tlsCertificatePath;
	itemGroupStream >> armPluginInfo.tlsKeyPath;
	itemGroupStream >> armPluginInfo.tlsCACertificatePath;
	itemGroupStream >> armPluginInfo.tlsEnableVerify;
	itemGroupStream >> armPluginInfo.uuid;
}

HatoholError DBTablesConfig::preprocForSaveArmPlguinInfo(
  const MonitoringServerInfo &serverInfo,
  const ArmPluginInfo &armPluginInfo, string &condition)
{
	if (!isHatoholArmPlugin(serverInfo.type))
		return HTERR_OK;

	if (armPluginInfo.type != serverInfo.type)
		return HTERR_DIFFER_TYPE_SERVER_AND_ARM_PLUGIN;
	return preprocForSaveArmPlguinInfo(armPluginInfo, condition);
}

HatoholError DBTablesConfig::preprocForSaveArmPlguinInfo(
  const ArmPluginInfo &armPluginInfo, string &condition)
{
	if (armPluginInfo.type != MONITORING_SYSTEM_HAPI_JSON &&
	    armPluginInfo.type != MONITORING_SYSTEM_HAPI2 &&
	    armPluginInfo.path.empty())
		return HTERR_INVALID_ARM_PLUGIN_PATH;

	if (armPluginInfo.type != MONITORING_SYSTEM_HAPI_JSON &&
	    armPluginInfo.type != MONITORING_SYSTEM_HAPI2) {
		MLPL_ERR("Invalid type: %d\n", armPluginInfo.type);
		return HTERR_INVALID_ARM_PLUGIN_TYPE;
	}
	if (StringUtils::hasPrefix(armPluginInfo.staticQueueAddress, "hap2."))
		return HTERR_RESERVED_QUEUE_NAME;
	if (StringUtils::casecmp(armPluginInfo.staticQueueAddress, "")){
	} else if (isDuplicateQueueName(armPluginInfo)){
			return HTERR_DUPLICATE_QUEUE_NAME;
	}

	condition = StringUtils::sprintf(
	  "%s=%d",
	  COLUMN_DEF_ARM_PLUGINS[IDX_ARM_PLUGINS_ID].columnName,
	  armPluginInfo.id);
	return HTERR_OK;
}

HatoholError DBTablesConfig::saveArmPluginInfoWithoutTransaction(
  ArmPluginInfo &armPluginInfo, const string &condition)
{
	if (armPluginInfo.id != AUTO_INCREMENT_VALUE &&
	    !isRecordExisting(TABLE_NAME_ARM_PLUGINS, condition)) {
		return HTERR_INVALID_ARM_PLUGIN_ID;
	} else if (armPluginInfo.id != AUTO_INCREMENT_VALUE) {
		DBAgent::UpdateArg arg(tableProfileArmPlugins);
		arg.add(IDX_ARM_PLUGINS_PATH, armPluginInfo.path);
		arg.add(IDX_ARM_PLUGINS_BROKER_URL,
		        armPluginInfo.brokerUrl);
		arg.add(IDX_ARM_PLUGINS_STATIC_QUEUE_ADDR,
		        armPluginInfo.staticQueueAddress);
		arg.add(IDX_ARM_PLUGINS_SERVER_ID,
		        armPluginInfo.serverId);
		arg.add(IDX_ARM_PLUGINS_TLS_CERTIFICATE_PATH,
		        armPluginInfo.tlsCertificatePath);
		arg.add(IDX_ARM_PLUGINS_TLS_KEY_PATH,
		        armPluginInfo.tlsKeyPath);
		arg.add(IDX_ARM_PLUGINS_TLS_CA_CERTIFICATE_PATH,
		        armPluginInfo.tlsCACertificatePath);
		arg.add(IDX_ARM_PLUGINS_TLS_ENABLE_VERIFY,
		        armPluginInfo.tlsEnableVerify);
		arg.add(IDX_ARM_PLUGINS_UUID,
		        armPluginInfo.uuid);
		arg.condition = condition;
		update(arg);
	} else {
		DBAgent::InsertArg arg(tableProfileArmPlugins);
		arg.add(AUTO_INCREMENT_VALUE); // armPluginInfo.id
		arg.add(armPluginInfo.type);
		arg.add(armPluginInfo.path);
		arg.add(armPluginInfo.brokerUrl);
		arg.add(armPluginInfo.staticQueueAddress);
		arg.add(armPluginInfo.serverId);
		arg.add(armPluginInfo.tlsCertificatePath);
		arg.add(armPluginInfo.tlsKeyPath);
		arg.add(armPluginInfo.tlsCACertificatePath);
		arg.add(armPluginInfo.tlsEnableVerify);
		arg.add(armPluginInfo.uuid);
		insert(arg);
		armPluginInfo.id = getLastInsertId();
	}
	return HTERR_OK;
}

void DBTablesConfig::preprocForDeleteArmPluginInfo(
  const ServerIdType &serverId, std::string &condition)
{
	condition = StringUtils::sprintf(
	  "%s=%" FMT_SERVER_ID,
	  COLUMN_DEF_ARM_PLUGINS[IDX_ARM_PLUGINS_SERVER_ID].columnName,
	  serverId);
}

HatoholError DBTablesConfig::saveArmPluginInfoIfNeededWithoutTransaction(
  ArmPluginInfo &armPluginInfo, const std::string &condition)
{
	if (condition.empty())
		return HTERR_OK;
	return saveArmPluginInfoWithoutTransaction(armPluginInfo, condition);
}

void DBTablesConfig::deleteArmPluginInfoWithoutTransaction(
  const std::string &condition)
{
	DBAgent::DeleteArg arg(tableProfileArmPlugins);
	arg.condition = condition;
	deleteRows(arg);
}

HatoholError DBTablesConfig::checkPrivilegeForSeverityRankAdd(
  const OperationPrivilege &privilege,
  const SeverityRankInfo &severityRankInfo)
{
	if (!privilege.has(OPPRVLG_CREATE_SEVERITY_RANK))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesConfig::checkPrivilegeForSeverityRankDelete(
  const OperationPrivilege &privilege, const std::list<SeverityRankIdType> &idList)
{
	if (!privilege.has(OPPRVLG_DELETE_SEVERITY_RANK))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesConfig::checkPrivilegeForSeverityRankUpdate(
  const OperationPrivilege &privilege,
  const SeverityRankInfo &severityRankInfo)
{
	if (!privilege.has(OPPRVLG_UPDATE_SEVERITY_RANK))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesConfig::checkPrivilegeForCustomIncidentStatusAdd(
  const OperationPrivilege &privilege,
  const CustomIncidentStatus &customIncidentStatus)
{
	if (!privilege.has(OPPRVLG_CREATE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesConfig::checkPrivilegeForCustomIncidentStatusUpdate(
  const OperationPrivilege &privilege,
  const CustomIncidentStatus &customIncidentStatus)
{
	if (!privilege.has(OPPRVLG_UPDATE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

HatoholError DBTablesConfig::checkPrivilegeForCustomIncidentStatusDelete(
  const OperationPrivilege &privilege,
  const std::list<CustomIncidentStatusIdType> &idList)
{
	if (!privilege.has(OPPRVLG_DELETE_INCIDENT_SETTING))
		return HatoholError(HTERR_NO_PRIVILEGE);

	const UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	return HTERR_OK;
}

bool DBTablesConfig::isDuplicateQueueName(
  const ArmPluginInfo &armPluginInfo)
{
	return isRecordExisting(TABLE_NAME_ARM_PLUGINS,
	        StringUtils::sprintf("%s='%s' and %s!=%d",
	        COLUMN_DEF_ARM_PLUGINS[IDX_ARM_PLUGINS_STATIC_QUEUE_ADDR].columnName,
	        armPluginInfo.staticQueueAddress.c_str(),
	        COLUMN_DEF_ARM_PLUGINS[IDX_ARM_PLUGINS_ID].columnName,
	        armPluginInfo.id));
}
