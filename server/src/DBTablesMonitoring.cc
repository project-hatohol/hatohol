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
#include <Mutex.h>
#include "DBAgentFactory.h"
#include "DBTablesMonitoring.h"
#include "DBTablesUser.h"
#include "ThreadLocalDBCache.h"
#include "SQLUtils.h"
#include "Params.h"
#include "ItemGroupStream.h"
#include "DBClientJoinBuilder.h"

// TODO: rmeove the followin two include files!
// This class should not be aware of it.
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"

using namespace std;
using namespace mlpl;

const char *DBTablesMonitoring::TABLE_NAME_TRIGGERS   = "triggers";
const char *DBTablesMonitoring::TABLE_NAME_EVENTS     = "events";
const char *DBTablesMonitoring::TABLE_NAME_ITEMS      = "items";
const char *DBTablesMonitoring::TABLE_NAME_HOSTS      = "hosts";
const char *DBTablesMonitoring::TABLE_NAME_HOSTGROUPS = "hostgroups";
const char *DBTablesMonitoring::TABLE_NAME_MAP_HOSTS_HOSTGROUPS
                                                   = "map_hosts_hostgroups";
const char *DBTablesMonitoring::TABLE_NAME_SERVER_STATUS = "server_status";
const char *DBTablesMonitoring::TABLE_NAME_INCIDENTS  = "incidents";

const int   DBTablesMonitoring::MONITORING_DB_VERSION = 6;

void operator>>(ItemGroupStream &itemGroupStream, TriggerStatusType &rhs)
{
	rhs = itemGroupStream.read<int, TriggerStatusType>();
}

void operator>>(ItemGroupStream &itemGroupStream, TriggerSeverityType &rhs)
{
	rhs = itemGroupStream.read<int, TriggerSeverityType>();
}

void operator>>(ItemGroupStream &itemGroupStream, EventType &rhs)
{
	rhs = itemGroupStream.read<int, EventType>();
}

// ----------------------------------------------------------------------------
// Table: triggers
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_TRIGGERS[] = {
{
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsTriggers // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"last_change_time_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"last_change_time_ns",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
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
	"brief",                           // columnName
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
	IDX_TRIGGERS_SERVER_ID,
	IDX_TRIGGERS_ID,
	IDX_TRIGGERS_STATUS,
	IDX_TRIGGERS_SEVERITY,
	IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
	IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
	IDX_TRIGGERS_HOST_ID,
	IDX_TRIGGERS_HOSTNAME,
	IDX_TRIGGERS_BRIEF,
	NUM_IDX_TRIGGERS,
};

static const int columnIndexesTrigUniqId[] = {
  IDX_TRIGGERS_SERVER_ID, IDX_TRIGGERS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsTriggers[] = {
  {"TrigUniqId", (const int *)columnIndexesTrigUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileTriggers =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			    COLUMN_DEF_TRIGGERS,
			    NUM_IDX_TRIGGERS,
			    indexDefsTriggers);

// ----------------------------------------------------------------------------
// Table: events
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_EVENTS[] = {
{
	"unified_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsEvents   // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"time_sec",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"time_ns",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"event_value",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
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
	"brief",                           // columnName
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
	IDX_EVENTS_UNIFIED_ID,
	IDX_EVENTS_SERVER_ID,
	IDX_EVENTS_ID,
	IDX_EVENTS_TIME_SEC,
	IDX_EVENTS_TIME_NS,
	IDX_EVENTS_EVENT_TYPE,
	IDX_EVENTS_TRIGGER_ID,
	IDX_EVENTS_STATUS,
	IDX_EVENTS_SEVERITY,
	IDX_EVENTS_HOST_ID,
	IDX_EVENTS_HOST_NAME,
	IDX_EVENTS_BRIEF,
	NUM_IDX_EVENTS,
};

static const int columnIndexesEventsUniqId[] = {
  IDX_EVENTS_SERVER_ID, IDX_EVENTS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsEvents[] = {
  {"EventsId", (const int *)columnIndexesEventsUniqId, false},
  {NULL}
};

static const DBAgent::TableProfile tableProfileEvents =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_EVENTS,
			    COLUMN_DEF_EVENTS,
			    NUM_IDX_EVENTS,
			    indexDefsEvents);

// ----------------------------------------------------------------------------
// Table: items
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_ITEMS[] = {
{
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsItems    // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"last_value_time_sec",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"last_value_time_ns",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"last_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"prev_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"item_group_name",                 // columnName
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
	IDX_ITEMS_SERVER_ID,
	IDX_ITEMS_ID,
	IDX_ITEMS_HOST_ID,
	IDX_ITEMS_BRIEF,
	IDX_ITEMS_LAST_VALUE_TIME_SEC,
	IDX_ITEMS_LAST_VALUE_TIME_NS,
	IDX_ITEMS_LAST_VALUE,
	IDX_ITEMS_PREV_VALUE,
	IDX_ITEMS_ITEM_GROUP_NAME,
	NUM_IDX_ITEMS,
};

static const int columnIndexesItemsUniqId[] = {
  IDX_ITEMS_SERVER_ID, IDX_ITEMS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsItems[] = {
  {"ItemsUniqId", (const int *)columnIndexesItemsUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileItems =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_ITEMS,
			    COLUMN_DEF_ITEMS,
			    NUM_IDX_ITEMS,
			    indexDefsItems);

// ----------------------------------------------------------------------------
// Table: hosts
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_HOSTS[] = {
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
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHosts    // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_name",                       // columnName
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
	IDX_HOSTS_ID,
	IDX_HOSTS_SERVER_ID,
	IDX_HOSTS_HOST_ID,
	IDX_HOSTS_HOST_NAME,
	NUM_IDX_HOSTS,
};

static const int columnIndexesHostsUniqId[] = {
  IDX_HOSTS_SERVER_ID, IDX_HOSTS_HOST_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHosts[] = {
  {"HostsUniqId", (const int *)columnIndexesHostsUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileHosts =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_HOSTS,
			    COLUMN_DEF_HOSTS,
			    NUM_IDX_HOSTS,
			    indexDefsHosts);

// ----------------------------------------------------------------------------
// Table: hostgroups
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_HOSTGROUPS[] = {
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
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHostgroups // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"group_name",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_HOSTGROUPS_ID,
	IDX_HOSTGROUPS_SERVER_ID,
	IDX_HOSTGROUPS_GROUP_ID,
	IDX_HOSTGROUPS_GROUP_NAME,
	NUM_IDX_HOSTGROUPS,
};

static const int columnIndexesHostgroupsUniqId[] = {
  IDX_HOSTGROUPS_SERVER_ID, IDX_HOSTGROUPS_GROUP_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostgroups[] = {
  {"HostgroupsUniqId", (const int *)columnIndexesHostgroupsUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileHostgroups =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_HOSTGROUPS,
			    COLUMN_DEF_HOSTGROUPS,
			    NUM_IDX_HOSTGROUPS,
			    indexDefsHostgroups);

// ----------------------------------------------------------------------------
// Table: map_hosts_hostgroups
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[] = {
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
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsMapHostsHostgroups // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_MAP_HOSTS_HOSTGROUPS_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID,
	NUM_IDX_MAP_HOSTS_HOSTGROUPS,
};

static const int columnIndexesMapHostsHostgroupsUniqId[] = {
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_ID,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsMapHostsHostgroups[] = {
  {"MapHostsHostgroupsUniqId",
   (const int *)columnIndexesMapHostsHostgroupsUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileMapHostsHostgroups =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_MAP_HOSTS_HOSTGROUPS,
			    COLUMN_DEF_MAP_HOSTS_HOSTGROUPS,
			    NUM_IDX_MAP_HOSTS_HOSTGROUPS,
			    indexDefsMapHostsHostgroups);

// ----------------------------------------------------------------------------
// Table: server_status
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"nvps",                            // columnName
	SQL_COLUMN_TYPE_DOUBLE,            // type
	15,                                // columnLength
	2,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_SERVERS_ID,
	IDX_SERVERS_NVPS,
	NUM_IDX_SERVERS,
};

static const DBAgent::TableProfile tableProfileServerStatus =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_SERVER_STATUS,
			    COLUMN_DEF_SERVERS,
			    NUM_IDX_SERVERS);

// ----------------------------------------------------------------------------
// Table: incidents
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_INCIDENTS[] = {
{
	"tracker_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"event_id",                        // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"identifier",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"location",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"status",                          // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"assignee",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"created_at_sec",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"created_at_ns",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"updated_at_sec",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"updated_at_ns",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"priority",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"done_ratio",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_INCIDENTS_TRACKER_ID,
	IDX_INCIDENTS_SERVER_ID,
	IDX_INCIDENTS_EVENT_ID,
	IDX_INCIDENTS_TRIGGER_ID,
	IDX_INCIDENTS_IDENTIFIER,
	IDX_INCIDENTS_LOCATION,
	IDX_INCIDENTS_STATUS,
	IDX_INCIDENTS_ASSIGNEE,
	IDX_INCIDENTS_CREATED_AT_SEC,
	IDX_INCIDENTS_CREATED_AT_NS,
	IDX_INCIDENTS_UPDATED_AT_SEC,
	IDX_INCIDENTS_UPDATED_AT_NS,
	IDX_INCIDENTS_PRIORITY,
	IDX_INCIDENTS_DONE_RATIO,
	NUM_IDX_INCIDENTS,
};

static const int columnIndexesIncidentsUniqId[] = {
  IDX_INCIDENTS_SERVER_ID, IDX_INCIDENTS_EVENT_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsIncidents[] = {
  {"IncidentsEventId", (const int *)columnIndexesIncidentsUniqId, true},
  {NULL}
};

static const DBAgent::TableProfile tableProfileIncidents =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_INCIDENTS,
			    COLUMN_DEF_INCIDENTS,
			    NUM_IDX_INCIDENTS,
			    indexDefsIncidents);

struct DBTablesMonitoring::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

// ---------------------------------------------------------------------------
// EventInfo
// ---------------------------------------------------------------------------
void initEventInfo(EventInfo &eventInfo)
{
	eventInfo.unifiedId = 0;
	eventInfo.serverId = 0;
	eventInfo.id = 0;
	eventInfo.time.tv_sec = 0;
	eventInfo.time.tv_nsec = 0;
	eventInfo.type = EVENT_TYPE_UNKNOWN;
	eventInfo.triggerId = 0;
	eventInfo.status = TRIGGER_STATUS_UNKNOWN;
	eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
	eventInfo.hostId = INVALID_HOST_ID;
}

// ---------------------------------------------------------------------------
// HostResourceQueryOption's subclasses
// ---------------------------------------------------------------------------

//
// EventQueryOption
//
static const HostResourceQueryOption::Synapse synapseEventsQueryOption(
  tableProfileEvents,
  IDX_EVENTS_UNIFIED_ID, IDX_EVENTS_SERVER_ID,
  tableProfileTriggers,
  IDX_TRIGGERS_HOST_ID, true,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct EventsQueryOption::Impl {
	uint64_t limitOfUnifiedId;
	SortType sortType;
	SortDirection sortDirection;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;
	TriggerIdType triggerId;

	Impl()
	: limitOfUnifiedId(NO_LIMIT),
	  sortType(SORT_UNIFIED_ID),
	  sortDirection(SORT_DONT_CARE),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL),
	  triggerId(ALL_TRIGGERS)
	{
	}
};

EventsQueryOption::EventsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseEventsQueryOption, userId),
  m_impl(new Impl())
{
}

EventsQueryOption::EventsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseEventsQueryOption, dataQueryContext),
  m_impl(new Impl())
{
}

EventsQueryOption::EventsQueryOption(const EventsQueryOption &src)
: HostResourceQueryOption(src),
  m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

EventsQueryOption::~EventsQueryOption()
{
}

string EventsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBHatohol::isAlwaysFalseCondition(condition))
		return condition;

	if (m_impl->limitOfUnifiedId) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s<=%" PRIu64,
			getColumnName(IDX_EVENTS_UNIFIED_ID).c_str(),
			m_impl->limitOfUnifiedId);
	}

	if (m_impl->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		if (!condition.empty())
			condition += " AND ";
		// Use triggers table because events tables doesn't contain
		// correct severity.
		condition += StringUtils::sprintf(
			"%s.%s>=%d",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName,
			m_impl->minSeverity);
	}

	if (m_impl->triggerStatus != TRIGGER_STATUS_ALL) {
		if (!condition.empty())
			condition += " AND ";
		// Use events table because triggers table doesn't contain past
		// status.
		condition += StringUtils::sprintf(
			"%s=%d",
			getColumnName(IDX_EVENTS_STATUS).c_str(),
			m_impl->triggerStatus);
	}

	if (m_impl->triggerId != ALL_TRIGGERS) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s=%" FMT_TRIGGER_ID,
			getColumnName(IDX_EVENTS_TRIGGER_ID).c_str(),
			m_impl->triggerId);
	}

	return condition;
}

void EventsQueryOption::setLimitOfUnifiedId(const uint64_t &unifiedId)
{
	m_impl->limitOfUnifiedId = unifiedId;
}

uint64_t EventsQueryOption::getLimitOfUnifiedId(void) const
{
	return m_impl->limitOfUnifiedId;
}

void EventsQueryOption::setSortType(
  const SortType &type, const SortDirection &direction)
{
	m_impl->sortType = type;
	m_impl->sortDirection = direction;

	switch (type) {
	case SORT_UNIFIED_ID:
	{
		SortOrder order(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName,
		  direction);
		setSortOrder(order);
		break;
	}
	case SORT_TIME:
	{
		SortOrderList sortOrderList;
		SortOrder order1(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_SEC].columnName,
		  direction);
		SortOrder order2(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_NS].columnName,
		  direction);
		SortOrder order3(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName,
		  direction);
		sortOrderList.push_back(order1);
		sortOrderList.push_back(order2);
		sortOrderList.push_back(order3);
		setSortOrderList(sortOrderList);
		break;
	}
	default:
		break;
	}
}

EventsQueryOption::SortType EventsQueryOption::getSortType(void) const
{
	return m_impl->sortType;
}

DataQueryOption::SortDirection EventsQueryOption::getSortDirection(void) const
{
	return m_impl->sortDirection;
}

void EventsQueryOption::setMinimumSeverity(const TriggerSeverityType &severity)
{
	m_impl->minSeverity = severity;
}

TriggerSeverityType EventsQueryOption::getMinimumSeverity(void) const
{
	return m_impl->minSeverity;
}

void EventsQueryOption::setTriggerStatus(const TriggerStatusType &status)
{
	m_impl->triggerStatus = status;
}

TriggerStatusType EventsQueryOption::getTriggerStatus(void) const
{
	return m_impl->triggerStatus;
}

void EventsQueryOption::setTriggerId(const TriggerIdType &triggerId)
{
	m_impl->triggerId = triggerId;
}

TriggerIdType EventsQueryOption::getTriggerId(void) const
{
	return m_impl->triggerId;
}

//
// TriggersQueryOption
//
static const HostResourceQueryOption::Synapse synapseTriggersQueryOption(
  tableProfileTriggers,
  IDX_TRIGGERS_ID, IDX_TRIGGERS_SERVER_ID,
  tableProfileTriggers,
  IDX_TRIGGERS_HOST_ID,
  true,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct TriggersQueryOption::Impl {
	TriggerIdType targetId;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;

	Impl()
	: targetId(ALL_TRIGGERS),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL)
	{
	}
};

TriggersQueryOption::TriggersQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseTriggersQueryOption, userId),
  m_impl(new Impl())
{
}

TriggersQueryOption::TriggersQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseTriggersQueryOption, dataQueryContext),
  m_impl(new Impl())
{
}

TriggersQueryOption::TriggersQueryOption(const TriggersQueryOption &src)
: HostResourceQueryOption(src),
  m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

TriggersQueryOption::~TriggersQueryOption()
{
}

string TriggersQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBHatohol::isAlwaysFalseCondition(condition))
		return condition;

	if (m_impl->targetId != ALL_TRIGGERS) {
		const DBTermCodec *dbTermCodec = getDBTermCodec();
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
			dbTermCodec->enc(m_impl->targetId).c_str());
	}

	if (m_impl->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s>=%d",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName,
			m_impl->minSeverity);
	}

	if (m_impl->triggerStatus != TRIGGER_STATUS_ALL) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%d",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS].columnName,
			m_impl->triggerStatus);
	}

	return condition;
}

void TriggersQueryOption::setTargetId(const TriggerIdType &id)
{
	m_impl->targetId = id;
}

TriggerIdType TriggersQueryOption::getTargetId(void) const
{
	return m_impl->targetId;
}

void TriggersQueryOption::setMinimumSeverity(const TriggerSeverityType &severity)
{
	m_impl->minSeverity = severity;
}

TriggerSeverityType TriggersQueryOption::getMinimumSeverity(void) const
{
	return m_impl->minSeverity;
}

void TriggersQueryOption::setTriggerStatus(const TriggerStatusType &status)
{
	m_impl->triggerStatus = status;
}

TriggerStatusType TriggersQueryOption::getTriggerStatus(void) const
{
	return m_impl->triggerStatus;
}

//
// ItemsQueryOption
//
static const HostResourceQueryOption::Synapse synapseItemsQueryOption(
  tableProfileItems, IDX_ITEMS_ID, IDX_ITEMS_SERVER_ID,
  tableProfileItems, IDX_ITEMS_HOST_ID,
  true,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct ItemsQueryOption::Impl {
	ItemIdType targetId;
	string itemGroupName;

	Impl()
	: targetId(ALL_ITEMS)
	{
	}
};

ItemsQueryOption::ItemsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseItemsQueryOption, userId),
  m_impl(new Impl())
{
}

ItemsQueryOption::ItemsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseItemsQueryOption, dataQueryContext),
  m_impl(new Impl())
{
}

ItemsQueryOption::ItemsQueryOption(const ItemsQueryOption &src)
: HostResourceQueryOption(src),
  m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

ItemsQueryOption::~ItemsQueryOption()
{
}

string ItemsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBHatohol::isAlwaysFalseCondition(condition))
		return condition;

	if (m_impl->targetId != ALL_ITEMS) {
		const DBTermCodec *dbTermCodec = getDBTermCodec();
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_ITEMS,
			COLUMN_DEF_ITEMS[IDX_ITEMS_ID].columnName,
			dbTermCodec->enc(m_impl->targetId).c_str());
	}

	if (!m_impl->itemGroupName.empty()) {
		if (!condition.empty())
			condition += " AND ";
		string escaped = StringUtils::replace(m_impl->itemGroupName,
						      "'", "''");
		condition += StringUtils::sprintf(
			"%s.%s='%s'",
			DBTablesMonitoring::TABLE_NAME_ITEMS,
			COLUMN_DEF_ITEMS[IDX_ITEMS_ITEM_GROUP_NAME].columnName,
			escaped.c_str());
	}

	return condition;
}

void ItemsQueryOption::setTargetId(const ItemIdType &id)
{
	m_impl->targetId = id;
}

ItemIdType ItemsQueryOption::getTargetId(void) const
{
	return m_impl->targetId;
}

void ItemsQueryOption::setTargetItemGroupName(const string &itemGroupName)
{
	m_impl->itemGroupName = itemGroupName;
}

const string &ItemsQueryOption::getTargetItemGroupName(void)
{
	return m_impl->itemGroupName;
}

//
// HostsQueryOption
//
static const HostResourceQueryOption::Synapse synapseHostsQueryOption(
  tableProfileHosts, IDX_HOSTS_ID, IDX_HOSTS_SERVER_ID,
  tableProfileHosts, IDX_HOSTS_HOST_ID,
  true,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostsQueryOption::HostsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostsQueryOption, userId)
{
}

HostsQueryOption::HostsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostsQueryOption, dataQueryContext)
{
}

//
// HostgroupsQueryOption
//
static const HostResourceQueryOption::Synapse synapseHostgroupsQueryOption(
  tableProfileHostgroups,
  IDX_HOSTGROUPS_GROUP_ID, IDX_HOSTGROUPS_SERVER_ID,
  tableProfileHostgroups,
  INVALID_COLUMN_IDX,
  false,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostgroupsQueryOption::HostgroupsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostgroupsQueryOption, userId)
{
}

HostgroupsQueryOption::HostgroupsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostgroupsQueryOption, dataQueryContext)
{
}

//
// HostgroupElementQueryOption
//
static const HostResourceQueryOption::Synapse synapseHostgroupElementQueryOption(
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_ID, IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID, false,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostgroupElementQueryOption::HostgroupElementQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseHostgroupElementQueryOption, userId)
{
}

HostgroupElementQueryOption::HostgroupElementQueryOption(
  DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostgroupElementQueryOption, dataQueryContext)
{
}

//
// IncidentsQueryOption
//
IncidentsQueryOption::IncidentsQueryOption(const UserIdType &userId)
: DataQueryOption(userId)
{
}

IncidentsQueryOption::IncidentsQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesMonitoring::reset(void)
{
	getSetupInfo().initialized = false;
}

DBTablesMonitoring::DBTablesMonitoring(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesMonitoring::~DBTablesMonitoring()
{
}

void DBTablesMonitoring::getHostInfoList(HostInfoList &hostInfoList,
				      const HostsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileHosts);
	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();
	arg.useFullName = option.isHostgroupUsed();
	arg.add(IDX_HOSTS_SERVER_ID);
	arg.add(IDX_HOSTS_HOST_ID);
	arg.add(IDX_HOSTS_HOST_NAME);

	// condition
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		hostInfoList.push_back(HostInfo());
		HostInfo &hostInfo = hostInfoList.back();
		itemGroupStream >> hostInfo.serverId;
		itemGroupStream >> hostInfo.id;
		itemGroupStream >> hostInfo.hostName;
	}
}

void DBTablesMonitoring::addTriggerInfo(TriggerInfo *triggerInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		TriggerInfo *triggerInfo;

		TrxProc(TriggerInfo *_triggerInfo)
		: triggerInfo(_triggerInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addTriggerInfoWithoutTransaction(dbAgent, *triggerInfo);
		}
	} trx(triggerInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addTriggerInfoList(const TriggerInfoList &triggerInfoList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const TriggerInfoList &triggerInfoList;

		TrxProc(const TriggerInfoList &_triggerInfoList)
		: triggerInfoList(_triggerInfoList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			TriggerInfoListConstIterator it =
			  triggerInfoList.begin();
			for (; it != triggerInfoList.end(); ++it)
				addTriggerInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(triggerInfoList);
	getDBAgent().runTransaction(trx);
}

bool DBTablesMonitoring::getTriggerInfo(TriggerInfo &triggerInfo,
                                     const TriggersQueryOption &option)
{
	TriggerInfoList triggerInfoList;
	getTriggerInfoList(triggerInfoList, option);
	size_t numTriggers = triggerInfoList.size();
	HATOHOL_ASSERT(numTriggers <= 1,
	               "Number of triggers: %zd, condition: %s",
	               numTriggers, option.getCondition().c_str());
	if (numTriggers == 0)
		return false;

	triggerInfo = *triggerInfoList.begin();
	return true;
}

void DBTablesMonitoring::getTriggerInfoList(TriggerInfoList &triggerInfoList,
					 const TriggersQueryOption &option)
{
	// build a condition
	string condition = option.getCondition();
	if (DBHatohol::isAlwaysFalseCondition(condition))
		return;

	DBAgent::SelectExArg arg(tableProfileTriggers);
	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();
	arg.useFullName = option.isHostgroupUsed();
	arg.add(IDX_TRIGGERS_SERVER_ID);
	arg.add(IDX_TRIGGERS_ID);
	arg.add(IDX_TRIGGERS_STATUS);
	arg.add(IDX_TRIGGERS_SEVERITY);
	arg.add(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);
	arg.add(IDX_TRIGGERS_LAST_CHANGE_TIME_NS);
	arg.add(IDX_TRIGGERS_HOST_ID);
	arg.add(IDX_TRIGGERS_HOSTNAME);
	arg.add(IDX_TRIGGERS_BRIEF);

	// condition
	arg.condition = condition;

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();
	if (!arg.limit && arg.offset)
		return;

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		TriggerInfo trigInfo;

		itemGroupStream >> trigInfo.serverId;
		itemGroupStream >> trigInfo.id;
		itemGroupStream >> trigInfo.status;
		itemGroupStream >> trigInfo.severity;
		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		itemGroupStream >> trigInfo.lastChangeTime.tv_nsec;
		itemGroupStream >> trigInfo.hostId;
		itemGroupStream >> trigInfo.hostName;
		itemGroupStream >> trigInfo.brief;

		triggerInfoList.push_back(trigInfo);
	}
}

// TODO: remove This method is not used
void DBTablesMonitoring::setTriggerInfoList(
  const TriggerInfoList &triggerInfoList, const ServerIdType &serverId)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const TriggerInfoList &triggerInfoList;
		const ServerIdType &serverId;
		DBAgent::DeleteArg deleteArg;

		TrxProc(const TriggerInfoList &_triggerInfoList,
		        const ServerIdType &_serverId)
		: triggerInfoList(_triggerInfoList),
		  serverId(_serverId),
		  deleteArg(tableProfileTriggers)
		{
		}

		virtual bool preproc(DBAgent &dbAgent) override
		{
			// TODO: This way is too rough and inefficient.
			//       We should update only the changed triggers.
			const DBTermCodec *dbTermCodec =
			  dbAgent.getDBTermCodec();
			deleteArg.condition = StringUtils::sprintf("%s=%s",
			  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
			  dbTermCodec->enc(serverId).c_str());
			return true;
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(deleteArg);
			TriggerInfoListConstIterator it =
			  triggerInfoList.begin();
			for (; it != triggerInfoList.end(); ++it)
				addTriggerInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(triggerInfoList, serverId);
	getDBAgent().runTransaction(trx);
}

int DBTablesMonitoring::getLastChangeTimeOfTrigger(const ServerIdType &serverId)
{
	const DBTermCodec *dbTermCodec = getDBAgent().getDBTermCodec();
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = StringUtils::sprintf("coalesce(max(%s), 0)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.condition = StringUtils::sprintf("%s=%s AND %s < %lu",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    dbTermCodec->enc(serverId).c_str(),
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
	    FAILED_SELF_TRIGGERID_TERMINATION);

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	// TODO: we want to select the template parameter automatically.
	//       Since the above code pushes
	//       COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type, so the
	//       template parameter is decided at the compile time in principle.
	//       However, I don't have a good idea. Propably constexpr,
	//       feature of C++11, may solve this problem.
	return itemGroupStream.read<int>();
}

void DBTablesMonitoring::addEventInfo(EventInfo *eventInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		EventInfo *eventInfo;

		TrxProc(EventInfo *_eventInfo)
		: eventInfo(_eventInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addEventInfoWithoutTransaction(dbAgent, *eventInfo);
		}
	} trx(eventInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addEventInfoList(const EventInfoList &eventInfoList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const EventInfoList &eventInfoList;

		TrxProc(const EventInfoList &_eventInfoList)
		: eventInfoList(_eventInfoList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			EventInfoListConstIterator it = eventInfoList.begin();
			for (; it != eventInfoList.end(); ++it)
				addEventInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(eventInfoList);
	getDBAgent().runTransaction(trx);
}

HatoholError DBTablesMonitoring::getEventInfoList(
  EventInfoList &eventInfoList, const EventsQueryOption &option,
  IncidentInfoVect *incidentInfoVect)
{
	DBClientJoinBuilder builder(tableProfileEvents, &option);
	builder.add(IDX_EVENTS_UNIFIED_ID);
	builder.add(IDX_EVENTS_SERVER_ID);
	builder.add(IDX_EVENTS_ID);
	builder.add(IDX_EVENTS_TIME_SEC);
	builder.add(IDX_EVENTS_TIME_NS);
	builder.add(IDX_EVENTS_EVENT_TYPE);
	builder.add(IDX_EVENTS_TRIGGER_ID);
	builder.add(IDX_EVENTS_STATUS);
	builder.add(IDX_EVENTS_SEVERITY);
	builder.add(IDX_EVENTS_HOST_ID);
	builder.add(IDX_EVENTS_HOST_NAME);
	builder.add(IDX_EVENTS_BRIEF);

	builder.addTable(
	  tableProfileTriggers, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileEvents, IDX_EVENTS_SERVER_ID, IDX_TRIGGERS_SERVER_ID,
	  tableProfileEvents, IDX_EVENTS_TRIGGER_ID, IDX_TRIGGERS_ID);
	builder.add(IDX_TRIGGERS_STATUS);
	builder.add(IDX_TRIGGERS_SEVERITY);
	builder.add(IDX_TRIGGERS_HOST_ID);
	builder.add(IDX_TRIGGERS_HOSTNAME);
	builder.add(IDX_TRIGGERS_BRIEF);

	if (incidentInfoVect) {
		builder.addTable(
		  tableProfileIncidents, DBClientJoinBuilder::LEFT_JOIN,
		  tableProfileEvents, IDX_EVENTS_SERVER_ID,
		  IDX_INCIDENTS_SERVER_ID,
		  tableProfileEvents, IDX_EVENTS_ID, IDX_INCIDENTS_EVENT_ID);
		builder.add(IDX_INCIDENTS_TRACKER_ID);
		builder.add(IDX_INCIDENTS_IDENTIFIER);
		builder.add(IDX_INCIDENTS_LOCATION);
		builder.add(IDX_INCIDENTS_STATUS);
		builder.add(IDX_INCIDENTS_ASSIGNEE);
		builder.add(IDX_INCIDENTS_CREATED_AT_SEC);
		builder.add(IDX_INCIDENTS_CREATED_AT_NS);
		builder.add(IDX_INCIDENTS_UPDATED_AT_SEC);
		builder.add(IDX_INCIDENTS_UPDATED_AT_NS);
		builder.add(IDX_INCIDENTS_PRIORITY);
		builder.add(IDX_INCIDENTS_DONE_RATIO);
	}

	// Condition
	DBAgent::SelectExArg &arg = builder.build();

	string optCond;
	arg.condition.swap(optCond); // option.getCondition() must be set.
	if (DBHatohol::isAlwaysFalseCondition(optCond))
		return HatoholError(HTERR_OK);

	arg.condition = optCond;

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();

	if (!arg.limit && arg.offset)
		return HTERR_OFFSET_WITHOUT_LIMIT;

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		eventInfoList.push_back(EventInfo());
		EventInfo &eventInfo = eventInfoList.back();

		itemGroupStream >> eventInfo.unifiedId;
		itemGroupStream >> eventInfo.serverId;
		itemGroupStream >> eventInfo.id;
		itemGroupStream >> eventInfo.time.tv_sec;
		itemGroupStream >> eventInfo.time.tv_nsec;
		itemGroupStream >> eventInfo.type;
		itemGroupStream >> eventInfo.triggerId;

		TriggerStatusType   eventStatus;
		TriggerSeverityType eventSeverity;
		HostIdType          eventHostId;
		string              eventHostName;
		string              eventBrief;
		itemGroupStream >> eventStatus;
		itemGroupStream >> eventSeverity;
		itemGroupStream >> eventHostId;
		itemGroupStream >> eventHostName;
		itemGroupStream >> eventBrief;

		TriggerStatusType   triggerStatus;
		TriggerSeverityType triggerSeverity;
		HostIdType          triggerHostId;
		string              triggerHostName;
		string              triggerBrief;
		itemGroupStream >> triggerStatus;
		itemGroupStream >> triggerSeverity;
		itemGroupStream >> triggerHostId;
		itemGroupStream >> triggerHostName;
		itemGroupStream >> triggerBrief;

		if (eventStatus != TRIGGER_STATUS_UNKNOWN) {
			eventInfo.status = eventStatus;
		} else {
			eventInfo.status = triggerStatus;
		}
		if (eventSeverity != TRIGGER_SEVERITY_UNKNOWN) {
			eventInfo.severity = eventSeverity;
		} else {
			eventInfo.severity = triggerSeverity;
		}
		if (eventHostId != 0 && eventHostId != INVALID_HOST_ID) {
			eventInfo.hostId = eventHostId;
			eventInfo.hostName = eventHostName;
		} else {
			eventInfo.hostId = triggerHostId;
			eventInfo.hostName = triggerHostName;
		}
		if (!eventBrief.empty()) {
			eventInfo.brief = eventBrief;
		} else {
			eventInfo.brief = triggerBrief;
		}

		if (incidentInfoVect) {
			incidentInfoVect->push_back(IncidentInfo());
			IncidentInfo &incidentInfo = incidentInfoVect->back();
			itemGroupStream >> incidentInfo.trackerId;
			itemGroupStream >> incidentInfo.identifier;
			itemGroupStream >> incidentInfo.location;
			itemGroupStream >> incidentInfo.status;
			itemGroupStream >> incidentInfo.assignee;
			itemGroupStream >> incidentInfo.createdAt.tv_sec;
			itemGroupStream >> incidentInfo.createdAt.tv_nsec;
			itemGroupStream >> incidentInfo.updatedAt.tv_sec;
			itemGroupStream >> incidentInfo.updatedAt.tv_nsec;
			itemGroupStream >> incidentInfo.priority;
			itemGroupStream >> incidentInfo.doneRatio;
			incidentInfo.statusCode
				= IncidentInfo::STATUS_UNKNOWN; // TODO: add column?
			incidentInfo.serverId  = eventInfo.serverId;
			incidentInfo.eventId   = eventInfo.id;
			incidentInfo.triggerId = eventInfo.triggerId;
		}
	}
	return HatoholError(HTERR_OK);
}

// TODO: remove this fucntion (no longer used)
void DBTablesMonitoring::setEventInfoList(
  const EventInfoList &eventInfoList, const ServerIdType &serverId)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const EventInfoList &eventInfoList;
		const ServerIdType &serverId;
		DBAgent::DeleteArg deleteArg;

		TrxProc(const EventInfoList &_eventInfoList,
		        const ServerIdType &_serverId)
		: eventInfoList(_eventInfoList),
		  serverId(_serverId),
		  deleteArg(tableProfileEvents)
		{
		}

		virtual bool preproc(DBAgent &dbAgent) override
		{
			deleteArg.condition =
			  StringUtils::sprintf("%s=%s",
			    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
			    dbAgent.getDBTermCodec()->enc(serverId).c_str());
			return true;
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(deleteArg);
			EventInfoListConstIterator it = eventInfoList.begin();
			for (; it != eventInfoList.end(); ++it)
				addEventInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(eventInfoList, serverId);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addHostgroupInfo(HostgroupInfo *groupInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HostgroupInfo *groupInfo;
		
		TrxProc(HostgroupInfo *_groupInfo)
		: groupInfo(_groupInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addHostgroupInfoWithoutTransaction(dbAgent, *groupInfo);
		}
	} trx(groupInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addHostgroupInfoList(
  const HostgroupInfoList &groupInfoList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const HostgroupInfoList &groupInfoList;
		
		TrxProc(const HostgroupInfoList &_groupInfoList)
		: groupInfoList(_groupInfoList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			HostgroupInfoListConstIterator it =
			  groupInfoList.begin();
			for (; it != groupInfoList.end(); ++it)
				addHostgroupInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(groupInfoList);
	getDBAgent().runTransaction(trx);

}

void DBTablesMonitoring::addHostgroupElement(
  HostgroupElement *hostgroupElement)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HostgroupElement *hostgroupElement;
		
		TrxProc(HostgroupElement *_hostgroupElement)
		: hostgroupElement(_hostgroupElement)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addHostgroupElementWithoutTransaction(
			  dbAgent, *hostgroupElement);
		}
	} trx(hostgroupElement);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addHostgroupElementList(
  const HostgroupElementList &hostgroupElementList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const HostgroupElementList &hostgroupElementList;
		
		TrxProc(const HostgroupElementList &_hostgroupElementList)
		: hostgroupElementList(_hostgroupElementList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			HostgroupElementListConstIterator it =
			  hostgroupElementList.begin();
			for (; it != hostgroupElementList.end(); ++it) {
				addHostgroupElementWithoutTransaction(dbAgent,
				                                      *it);
			}
		}
	} trx(hostgroupElementList);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addHostInfo(HostInfo *hostInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HostInfo *hostInfo;
		
		TrxProc(HostInfo *_hostInfo)
		: hostInfo(_hostInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addHostInfoWithoutTransaction(dbAgent, *hostInfo);
		}
	} trx(hostInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addHostInfoList(const HostInfoList &hostInfoList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const HostInfoList &hostInfoList;
		
		TrxProc(const HostInfoList &_hostInfoList)
		: hostInfoList(_hostInfoList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			HostInfoListConstIterator it = hostInfoList.begin();
			for(; it != hostInfoList.end(); ++it)
				addHostInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(hostInfoList);
	getDBAgent().runTransaction(trx);
}

EventIdType DBTablesMonitoring::getLastEventId(const ServerIdType &serverId)
{
	const DBTermCodec *dbTermCodec = getDBAgent().getDBTermCodec();
	DBAgent::SelectExArg arg(tableProfileEvents);
	string stmt = StringUtils::sprintf("coalesce(max(%s), -1)",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName);
	arg.add(stmt, COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
	arg.condition = StringUtils::sprintf("%s=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName, 
	    dbTermCodec->enc(serverId).c_str());

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<EventIdType>();
}

SmartTime DBTablesMonitoring::getTimeOfLastEvent(
  const ServerIdType &serverId, const TriggerIdType &triggerId)
{
	using StringUtils::sprintf;

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::SelectExArg argId;
		DBAgent::SelectExArg argTime;
		bool                 foundEvent;

		TrxProc(void)
		: argId(tableProfileEvents),
		  argTime(tableProfileEvents),
		  foundEvent(false)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.select(argId);
			const ItemGroupList &grpList =
			   argId.dataTable->getItemGroupList();
			if (grpList.empty())
				return;
			foundEvent = true;

			ItemGroupStream itemGroupStream(*grpList.begin());
			const EventIdType eventId =
			  itemGroupStream.read<EventIdType>();

			// Get the time
			argTime.condition = sprintf("%s=%" FMT_EVENT_ID,
			  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName,
			  eventId);
			dbAgent.select(argTime);
		}
	} trx;

	// First get the event ID.
	// TODO: Get it with the one statemetn by using sub query.
	const DBTermCodec *dbTermCodec = getDBAgent().getDBTermCodec();
	const ColumnDef &colDefUniId = COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID];
	const string stmt = sprintf("max(%s)", colDefUniId.columnName);
	trx.argId.add(stmt, colDefUniId.type);

	trx.argId.condition = sprintf("%s=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    dbTermCodec->enc(serverId).c_str());
	if (triggerId != ALL_TRIGGERS) {
		trx.argId.condition += sprintf(" AND %s=%s",
		    COLUMN_DEF_EVENTS[IDX_EVENTS_TRIGGER_ID].columnName,
		    dbTermCodec->enc(triggerId).c_str());
	}

	// Then get the time of the event ID.
	trx.argTime.add(IDX_EVENTS_TIME_SEC);
	trx.argTime.add(IDX_EVENTS_TIME_NS);

	getDBAgent().runTransaction(trx);
	if (!trx.foundEvent)
		return SmartTime();

	const ItemGroupList &grpList =
	  trx.argTime.dataTable->getItemGroupList();
	if (grpList.empty())
		return SmartTime();
	ItemGroupStream itemGroupStream(*grpList.begin());

	timespec ts;
	itemGroupStream >> ts.tv_sec;
	itemGroupStream >> ts.tv_nsec;
	return SmartTime(ts);
}

void DBTablesMonitoring::addItemInfo(ItemInfo *itemInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		ItemInfo *itemInfo;
		
		TrxProc(ItemInfo *_itemInfo)
		: itemInfo(_itemInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addItemInfoWithoutTransaction(dbAgent, *itemInfo);
		}
	} trx(itemInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addItemInfoList(const ItemInfoList &itemInfoList)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const ItemInfoList &itemInfoList;
		
		TrxProc(const ItemInfoList &_itemInfoList)
		: itemInfoList(_itemInfoList)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			ItemInfoListConstIterator it = itemInfoList.begin();
			for(; it != itemInfoList.end(); ++it)
				addItemInfoWithoutTransaction(dbAgent, *it);
		}
	} trx(itemInfoList);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::getItemInfoList(ItemInfoList &itemInfoList,
				      const ItemsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileItems);
	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();
	arg.useFullName = option.isHostgroupUsed();
	arg.add(IDX_ITEMS_SERVER_ID);
	arg.add(IDX_ITEMS_ID);
	arg.add(IDX_ITEMS_HOST_ID);
	arg.add(IDX_ITEMS_BRIEF);
	arg.add(IDX_ITEMS_LAST_VALUE_TIME_SEC);
	arg.add(IDX_ITEMS_LAST_VALUE_TIME_NS);
	arg.add(IDX_ITEMS_LAST_VALUE);
	arg.add(IDX_ITEMS_PREV_VALUE);
	arg.add(IDX_ITEMS_ITEM_GROUP_NAME);

	// condition
	arg.condition = option.getCondition();
	if (DBHatohol::isAlwaysFalseCondition(arg.condition))
		return;

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();
	if (!arg.limit && arg.offset)
		return;

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemInfoList.push_back(ItemInfo());
		ItemInfo &itemInfo = itemInfoList.back();

		itemGroupStream >> itemInfo.serverId;
		itemGroupStream >> itemInfo.id;
		itemGroupStream >> itemInfo.hostId;
		itemGroupStream >> itemInfo.brief;
		itemGroupStream >> itemInfo.lastValueTime.tv_sec;
		itemGroupStream >> itemInfo.lastValueTime.tv_nsec;
		itemGroupStream >> itemInfo.lastValue;
		itemGroupStream >> itemInfo.prevValue;
		itemGroupStream >> itemInfo.itemGroupName;
	}
}

void DBTablesMonitoring::addMonitoringServerStatus(
  MonitoringServerStatus *serverStatus)
{
	struct TrxProc : public DBAgent::TransactionProc {
		MonitoringServerStatus *serverStatus;
		
		TrxProc(MonitoringServerStatus *_serverStatus)
		: serverStatus(_serverStatus)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addMonitoringServerStatusWithoutTransaction(
			  dbAgent, *serverStatus);
		}
	} trx(serverStatus);
	getDBAgent().runTransaction(trx);
}

size_t DBTablesMonitoring::getNumberOfTriggers(
  const TriggersQueryOption &option, const std::string &additionalCondition)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = "count(*)";
	if (option.isHostgroupUsed()) {
		// Because a same trigger can be counted multiple times in
		// this case, we should distinguish duplicated records. Althogh
		// we have to use 2 columns (server ID and trigger ID) to do
		// it, count() function doesn't accept multiple arguments.
		// To avoid this issue we concat server ID and trigger ID.

		// TODO: The statement depends on SQL implementations.
		// We should remove this code after we improve the hostgroups
		// issue by using sub query (github issue #168).
		const char *fmt = NULL;
		const type_info &dbAgentType = typeid(getDBAgent());
		if (dbAgentType == typeid(DBAgentSQLite3))
			fmt = "count(distinct %s || ',' || %s)";
		else if (dbAgentType == typeid(DBAgentMySQL))
			fmt = "count(distinct %s,%s)";
		HATOHOL_ASSERT(fmt, "Unknown DBAgent type.");

		stmt = StringUtils::sprintf(fmt,
		  option.getColumnName(IDX_TRIGGERS_SERVER_ID).c_str(),
		  option.getColumnName(IDX_TRIGGERS_ID).c_str());
	}
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// from
	arg.tableField = option.getFromClause();

	// condition
	arg.condition = option.getCondition();
	if (!additionalCondition.empty()) {
		if (!arg.condition.empty())
			arg.condition += " and ";
		arg.condition += additionalCondition;
	}

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBTablesMonitoring::getNumberOfBadTriggers(
  const TriggersQueryOption &option, TriggerSeverityType severity)
{
	string additionalCondition;

	if (severity == TRIGGER_SEVERITY_ALL) {
		additionalCondition
		  = StringUtils::sprintf(
		      "%s=%d",
		      option.getColumnName(IDX_TRIGGERS_STATUS).c_str(),
		      TRIGGER_STATUS_PROBLEM);
	} else {
		additionalCondition
		  = StringUtils::sprintf(
		      "%s=%d and %s=%d",
		      option.getColumnName(IDX_TRIGGERS_SEVERITY).c_str(),
		      severity,
		      option.getColumnName(IDX_TRIGGERS_STATUS).c_str(),
		      TRIGGER_STATUS_PROBLEM);
	}
	return getNumberOfTriggers(option, additionalCondition);
}

size_t DBTablesMonitoring::getNumberOfTriggers(const TriggersQueryOption &option)
{
	return getNumberOfTriggers(option, string());
}

size_t DBTablesMonitoring::getNumberOfHosts(const TriggersQueryOption &option)
{
	// TODO: consider if we can use hosts table.
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_TRIGGERS_HOST_ID).c_str());
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// from
	arg.tableField = option.getFromClause();

	// condition
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBTablesMonitoring::getNumberOfGoodHosts(const TriggersQueryOption &option)
{
	size_t numTotalHost = getNumberOfHosts(option);
	size_t numBadHosts = getNumberOfBadHosts(option);
	HATOHOL_ASSERT(numTotalHost >= numBadHosts,
	               "numTotalHost: %zd, numBadHosts: %zd",
	               numTotalHost, numBadHosts);
	return numTotalHost - numBadHosts;
}

size_t DBTablesMonitoring::getNumberOfBadHosts(const TriggersQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_TRIGGERS_HOST_ID).c_str());
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// from
	arg.tableField = option.getFromClause();

	// condition
	arg.condition = option.getCondition();
	if (!arg.condition.empty())
		arg.condition += " AND ";

	arg.condition +=
	  StringUtils::sprintf("%s=%d",
	    option.getColumnName(IDX_TRIGGERS_STATUS).c_str(),
	    TRIGGER_STATUS_PROBLEM);

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBTablesMonitoring::getNumberOfItems(
  const ItemsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = "count(*)";
	if (option.isHostgroupUsed()) {
		// TODO: It has a same issue with getNumberOfTriggers();
		const char *fmt = NULL;
		const type_info &dbAgentType = typeid(getDBAgent());
		if (dbAgentType == typeid(DBAgentSQLite3))
			fmt = "count(distinct %s || ',' || %s)";
		else if (dbAgentType == typeid(DBAgentMySQL))
			fmt = "count(distinct %s,%s)";
		HATOHOL_ASSERT(fmt, "Unknown DBAgent type.");

		stmt = StringUtils::sprintf(fmt,
		  option.getColumnName(IDX_ITEMS_SERVER_ID).c_str(),
		  option.getColumnName(IDX_ITEMS_ID).c_str());
	}
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// from
	arg.tableField = option.getFromClause();

	// condition
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

HatoholError DBTablesMonitoring::getNumberOfMonitoredItemsPerSecond
  (const DataQueryOption &option, MonitoringServerStatus &serverStatus)
{
	serverStatus.nvps = 0.0;
	if (!option.isValidServer(serverStatus.serverId))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgent::SelectExArg arg(tableProfileServerStatus);
	string stmt = COLUMN_DEF_SERVERS[IDX_SERVERS_NVPS].columnName;
	arg.add(stmt, SQL_COLUMN_TYPE_DOUBLE);

	arg.condition =
	  StringUtils::sprintf("%s=%d",
	    COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName,
	    serverStatus.serverId);

	getDBAgent().runTransaction(arg);

	// TODO: currently the folowing return 0 for Nagios servers
	// We just return 0 instead of an error.
	if (arg.dataTable->getNumberOfRows() == 0) {
		serverStatus.nvps = 0;
		//return HatoholError(HTERR_NOT_FOUND_TARGET_RECORD);
	} else {
		const ItemGroupList &grpList =
		  arg.dataTable->getItemGroupList();
		ItemGroupStream itemGroupStream(*grpList.begin());
		serverStatus.nvps = itemGroupStream.read<double>();
	}

	return HatoholError(HTERR_OK);
}

void DBTablesMonitoring::pickupAbsentHostIds(
  vector<uint64_t> &absentHostIdVector, const vector<uint64_t> &hostIdVector)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const string tableName;
		const string hostIdName;
		vector<uint64_t> &absentHostIdVector;
		const vector<uint64_t> &hostIdVector;
		
		TrxProc(vector<uint64_t> &_absentHostIdVector,
		        const vector<uint64_t> &_hostIdVector)
		: tableName(TABLE_NAME_HOSTS),
		  hostIdName(COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_ID].columnName),
		  absentHostIdVector(_absentHostIdVector),
		  hostIdVector(_hostIdVector)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			for (size_t i = 0; i < hostIdVector.size(); i++)
				addHostId(dbAgent, hostIdVector[i]);
		}

		void addHostId(DBAgent &dbAgent, const uint64_t &id)
		{
			string condition = hostIdName;
			condition += StringUtils::sprintf("=%" PRIu64, id);
			if (dbAgent.isRecordExisting(tableName, condition))
				return;
			absentHostIdVector.push_back(id);
		}
	} trx(absentHostIdVector, hostIdVector);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::addIncidentInfo(IncidentInfo *incidentInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		IncidentInfo *incidentInfo;
		
		TrxProc(IncidentInfo *_incidentInfo)
		: incidentInfo(_incidentInfo)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addIncidentInfoWithoutTransaction(
			  dbAgent, *incidentInfo);
		}
	} trx(incidentInfo);
	getDBAgent().runTransaction(trx);
}

void DBTablesMonitoring::updateIncidentInfo(IncidentInfo &incidentInfo)
{
	DBAgent::UpdateArg arg(tableProfileIncidents);
	arg.add(IDX_INCIDENTS_STATUS, incidentInfo.status);
	arg.add(IDX_INCIDENTS_ASSIGNEE, incidentInfo.assignee);
	arg.add(IDX_INCIDENTS_CREATED_AT_SEC, incidentInfo.createdAt.tv_sec);
	arg.add(IDX_INCIDENTS_CREATED_AT_NS, incidentInfo.createdAt.tv_nsec);
	arg.add(IDX_INCIDENTS_UPDATED_AT_SEC, incidentInfo.updatedAt.tv_sec);
	arg.add(IDX_INCIDENTS_UPDATED_AT_NS, incidentInfo.updatedAt.tv_nsec);
	arg.add(IDX_INCIDENTS_PRIORITY, incidentInfo.priority);
	arg.add(IDX_INCIDENTS_DONE_RATIO, incidentInfo.doneRatio);
	arg.condition = StringUtils::sprintf(
	  "%s=%" FMT_INCIDENT_TRACKER_ID " AND %s=%s",
	  COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].columnName,
	  incidentInfo.trackerId,
	  COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_IDENTIFIER].columnName,
	  incidentInfo.identifier.c_str());
	getDBAgent().runTransaction(arg);
}

HatoholError DBTablesMonitoring::getIncidentInfoVect(
  IncidentInfoVect &incidentInfoVect, const IncidentsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileIncidents);
	for (int i = 0; i < NUM_IDX_INCIDENTS; i++)
		arg.add(i);

	// condition
	arg.condition = option.getCondition();
	if (DBHatohol::isAlwaysFalseCondition(arg.condition))
		return HatoholError(HTERR_OK);

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();
	if (!arg.limit && arg.offset)
		return HatoholError(HTERR_OFFSET_WITHOUT_LIMIT);

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		incidentInfoVect.push_back(IncidentInfo());
		IncidentInfo &incidentInfo = incidentInfoVect.back();
		itemGroupStream >> incidentInfo.trackerId;
		itemGroupStream >> incidentInfo.serverId;
		itemGroupStream >> incidentInfo.eventId;
		itemGroupStream >> incidentInfo.triggerId;
		itemGroupStream >> incidentInfo.identifier;
		itemGroupStream >> incidentInfo.location;
		itemGroupStream >> incidentInfo.status;
		itemGroupStream >> incidentInfo.assignee;
		itemGroupStream >> incidentInfo.createdAt.tv_sec;
		itemGroupStream >> incidentInfo.createdAt.tv_nsec;
		itemGroupStream >> incidentInfo.updatedAt.tv_sec;
		itemGroupStream >> incidentInfo.updatedAt.tv_nsec;
		itemGroupStream >> incidentInfo.priority;
		itemGroupStream >> incidentInfo.doneRatio;
		incidentInfo.statusCode = IncidentInfo::STATUS_UNKNOWN; // TODO: add column?
	}

	return HatoholError(HTERR_OK);
}

uint64_t DBTablesMonitoring::getLastUpdateTimeOfIncidents(
  const IncidentTrackerIdType &trackerId)
{
	const DBTermCodec *dbTermCodec = getDBAgent().getDBTermCodec();
	DBAgent::SelectExArg arg(tableProfileIncidents);
	string stmt = StringUtils::sprintf("coalesce(max(%s), 0)",
	    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_UPDATED_AT_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].type);
	if (trackerId != ALL_INCIDENT_TRACKERS) {
		arg.condition = StringUtils::sprintf("%s=%s",
		    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].columnName,
		    dbTermCodec->enc(trackerId).c_str());
	}

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<uint64_t>();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
static bool updateDB(DBAgent &dbAgent, const int &oldVer, void *data);

DBTables::SetupInfo &DBTablesMonitoring::getSetupInfo(void)
{
	static const TableSetupInfo DB_TABLE_INFO[] = {
	{
		&tableProfileTriggers,
	}, {
		&tableProfileEvents,
	}, {
		&tableProfileItems,
	}, {
		&tableProfileHosts,
	}, {
		&tableProfileHostgroups,
	}, {
		&tableProfileMapHostsHostgroups,
	}, {
		&tableProfileServerStatus,
	}, {
		&tableProfileIncidents,
	}
	};

	static SetupInfo setupInfo = {
		DB_TABLES_ID_MONITORING,
		MONITORING_DB_VERSION,
		ARRAY_SIZE(DB_TABLE_INFO),
		DB_TABLE_INFO,
		&updateDB,
	};
	return setupInfo;
}

void DBTablesMonitoring::addTriggerInfoWithoutTransaction(
  DBAgent &dbAgent, const TriggerInfo &triggerInfo)
{
	DBAgent::InsertArg arg(tableProfileTriggers);
	arg.add(triggerInfo.serverId);
	arg.add(triggerInfo.id);
	arg.add(triggerInfo.status);
	arg.add(triggerInfo.severity),
	arg.add(triggerInfo.lastChangeTime.tv_sec);
	arg.add(triggerInfo.lastChangeTime.tv_nsec);
	arg.add(triggerInfo.hostId);
	arg.add(triggerInfo.hostName);
	arg.add(triggerInfo.brief);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addEventInfoWithoutTransaction(
  DBAgent &dbAgent, const EventInfo &eventInfo)
{
	DBAgent::InsertArg arg(tableProfileEvents);
	arg.add(AUTO_INCREMENT_VALUE_U64);
	arg.add(eventInfo.serverId);
	arg.add(eventInfo.id);
	arg.add(eventInfo.time.tv_sec);
	arg.add(eventInfo.time.tv_nsec);
	arg.add(eventInfo.type);
	arg.add(eventInfo.triggerId);
	arg.add(eventInfo.status);
	arg.add(eventInfo.severity);
	arg.add(eventInfo.hostId);
	arg.add(eventInfo.hostName);
	arg.add(eventInfo.brief);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addItemInfoWithoutTransaction(
  DBAgent &dbAgent, const ItemInfo &itemInfo)
{
	DBAgent::InsertArg arg(tableProfileItems);
	arg.add(itemInfo.serverId);
	arg.add(itemInfo.id);
	arg.add(itemInfo.hostId);
	arg.add(itemInfo.brief);
	arg.add(itemInfo.lastValueTime.tv_sec);
	arg.add(itemInfo.lastValueTime.tv_nsec);
	arg.add(itemInfo.lastValue);
	arg.add(itemInfo.prevValue);
	arg.add(itemInfo.itemGroupName);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addHostgroupInfoWithoutTransaction(
  DBAgent &dbAgent, const HostgroupInfo &groupInfo)
{
	DBAgent::InsertArg arg(tableProfileHostgroups);
	arg.add(groupInfo.id);
	arg.add(groupInfo.serverId);
	arg.add(groupInfo.groupId);
	arg.add(groupInfo.groupName);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addHostgroupElementWithoutTransaction(
  DBAgent &dbAgent, const HostgroupElement &hostgroupElement)
{
	// TODO: create the condition outside of the transaction.
	const DBTermCodec *dbTermCodec = dbAgent.getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND host_id=%s AND host_group_id=%s",
	  dbTermCodec->enc(hostgroupElement.serverId).c_str(),
	  dbTermCodec->enc(hostgroupElement.hostId).c_str(),
	  dbTermCodec->enc(hostgroupElement.groupId).c_str());

	if (!dbAgent.isRecordExisting(TABLE_NAME_MAP_HOSTS_HOSTGROUPS,
	                              condition)) {
		DBAgent::InsertArg arg(tableProfileMapHostsHostgroups);
		arg.add(hostgroupElement.id);
		arg.add(hostgroupElement.serverId);
		arg.add(hostgroupElement.hostId);
		arg.add(hostgroupElement.groupId);
		dbAgent.insert(arg);
	}
}

void DBTablesMonitoring::addHostInfoWithoutTransaction(
  DBAgent &dbAgent, const HostInfo &hostInfo)
{
	DBAgent::InsertArg arg(tableProfileHosts);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(hostInfo.serverId);
	arg.add(hostInfo.id);
	arg.add(hostInfo.hostName);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addMonitoringServerStatusWithoutTransaction(
  DBAgent &dbAgent, const MonitoringServerStatus &serverStatus)
{
	DBAgent::InsertArg arg(tableProfileServerStatus);
	arg.add(serverStatus.serverId);
	arg.add(serverStatus.nvps);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addIncidentInfoWithoutTransaction(
  DBAgent &dbAgent, const IncidentInfo &incidentInfo)
{
	DBAgent::InsertArg arg(tableProfileIncidents);
	arg.add(incidentInfo.trackerId);
	arg.add(incidentInfo.serverId);
	arg.add(incidentInfo.eventId);
	arg.add(incidentInfo.triggerId);
	arg.add(incidentInfo.identifier);
	arg.add(incidentInfo.location);
	arg.add(incidentInfo.status);
	arg.add(incidentInfo.assignee);
	arg.add(incidentInfo.createdAt.tv_sec);
	arg.add(incidentInfo.createdAt.tv_nsec);
	arg.add(incidentInfo.updatedAt.tv_sec);
	arg.add(incidentInfo.updatedAt.tv_nsec);
	arg.add(incidentInfo.priority);
	arg.add(incidentInfo.doneRatio);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

HatoholError DBTablesMonitoring::getHostgroupInfoList
  (HostgroupInfoList &hostgroupInfoList, const HostgroupsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileHostgroups);
	arg.add(IDX_HOSTGROUPS_ID);
	arg.add(IDX_HOSTGROUPS_SERVER_ID);
	arg.add(IDX_HOSTGROUPS_GROUP_ID);
	arg.add(IDX_HOSTGROUPS_GROUP_NAME);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		hostgroupInfoList.push_back(HostgroupInfo());
		HostgroupInfo &hostgroupInfo = hostgroupInfoList.back();

		itemGroupStream >> hostgroupInfo.id;
		itemGroupStream >> hostgroupInfo.serverId;
		itemGroupStream >> hostgroupInfo.groupId;
		itemGroupStream >> hostgroupInfo.groupName;
	}

	return HTERR_OK;
}

// TODO: In this implementation, behavior of this function is inefficient.
//       Because this function returns unnecessary informations.
//       Add a new function which returns only hostgroupId.
HatoholError DBTablesMonitoring::getHostgroupElementList
  (HostgroupElementList &hostgroupElementList,
   const HostgroupElementQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileMapHostsHostgroups);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);
	arg.condition = option.getCondition();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		hostgroupElementList.push_back(HostgroupElement());
		HostgroupElement &hostgroupElement = hostgroupElementList.back();
		itemGroupStream >> hostgroupElement.id;
		itemGroupStream >> hostgroupElement.serverId;
		itemGroupStream >> hostgroupElement.hostId;
		itemGroupStream >> hostgroupElement.groupId;
	}

	return HTERR_OK;
}

static bool updateDB(DBAgent &dbAgent, const int &oldVer, void *data)
{
	if (oldVer == 4) {
		const string oldTableName = "issues";
		if (dbAgent.isTableExisting(oldTableName)) {
			dbAgent.renameTable(
			  oldTableName,
			  DBTablesMonitoring::TABLE_NAME_INCIDENTS);
		}
	}
	if (oldVer <= 5) {
		// add new columns to incidents
		DBAgent::AddColumnsArg addColumnsArg(tableProfileIncidents);
		addColumnsArg.columnIndexes.push_back(IDX_INCIDENTS_PRIORITY);
		addColumnsArg.columnIndexes.push_back(IDX_INCIDENTS_DONE_RATIO);
		dbAgent.addColumns(addColumnsArg);
	}
	return true;
}
