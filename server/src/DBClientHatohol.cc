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

#include <memory>
#include <MutexLock.h>
#include "DBAgentFactory.h"
#include "DBClientHatohol.h"
#include "DBClientUser.h"
#include "CacheServiceDBClient.h"
#include "SQLUtils.h"
#include "Params.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

const char *DBClientHatohol::TABLE_NAME_TRIGGERS   = "triggers";
const char *DBClientHatohol::TABLE_NAME_EVENTS     = "events";
const char *DBClientHatohol::TABLE_NAME_ITEMS      = "items";
const char *DBClientHatohol::TABLE_NAME_HOSTS      = "hosts";
const char *DBClientHatohol::TABLE_NAME_HOSTGROUPS = "hostgroups";
const char *DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS
                                                   = "map_hosts_hostgroups";
static const char *TABLE_NAME_SERVERS              = "servers";

const EventIdType DBClientHatohol::EVENT_NOT_FOUND = -1;
const int         DBClientHatohol::HATOHOL_DB_VERSION = 4;
const char       *DBClientHatohol::DEFAULT_DB_NAME = "hatohol";

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

static const ColumnDef COLUMN_DEF_TRIGGERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsTriggers // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"last_change_time_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"last_change_time_ns",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
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

static const DBAgent::TableProfile tableProfileTriggers(
  DBClientHatohol::TABLE_NAME_TRIGGERS, COLUMN_DEF_TRIGGERS,
  sizeof(COLUMN_DEF_TRIGGERS), NUM_IDX_TRIGGERS);

static const ColumnDef COLUMN_DEF_EVENTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"unified_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsEvents   // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"time_sec",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"time_ns",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"event_value",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_EVENTS,// tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_TRIGGERS, // tableName
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

static const DBAgent::TableProfile tableProfileEvents(
  DBClientHatohol::TABLE_NAME_EVENTS, COLUMN_DEF_EVENTS,
  sizeof(COLUMN_DEF_EVENTS), NUM_IDX_EVENTS);

static const ColumnDef COLUMN_DEF_ITEMS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsItems    // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"last_value_time_sec",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"last_value_time_ns",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"last_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
	"prev_value",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_ITEMS, // tableName
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

static const DBAgent::TableProfile tableProfileItems(
  DBClientHatohol::TABLE_NAME_ITEMS, COLUMN_DEF_ITEMS,
  sizeof(COLUMN_DEF_ITEMS), NUM_IDX_ITEMS);

static const ColumnDef COLUMN_DEF_HOSTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTS, // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTS, // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHosts    // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTS, // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTS, // tableName
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

static const DBAgent::TableProfile tableProfileHosts(
  DBClientHatohol::TABLE_NAME_HOSTS, COLUMN_DEF_HOSTS,
  sizeof(COLUMN_DEF_HOSTS), NUM_IDX_HOSTS);

static const ColumnDef COLUMN_DEF_HOSTGROUPS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTGROUPS, // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTGROUPS, // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsHostgroups // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTGROUPS, // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_HOSTGROUPS, // tableName
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

static const DBAgent::TableProfile tableProfileHostgroups(
  DBClientHatohol::TABLE_NAME_HOSTGROUPS, COLUMN_DEF_HOSTGROUPS,
  sizeof(COLUMN_DEF_HOSTGROUPS), NUM_IDX_HOSTGROUPS);

static const ColumnDef COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS, // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS, // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE, // indexDefsMapHostsHostgroups // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS, // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS, // tableName
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

static const DBAgent::TableProfile tableProfileMapHostsHostgroups(
  DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS,
  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS,
  sizeof(COLUMN_DEF_MAP_HOSTS_HOSTGROUPS), NUM_IDX_MAP_HOSTS_HOSTGROUPS);

// Trigger
static const int columnIndexesTrigUniqId[] = {
  IDX_TRIGGERS_SERVER_ID, IDX_TRIGGERS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsTriggers[] = {
  {"TrigUniqId", &tableProfileTriggers,
   (const int *)columnIndexesTrigUniqId, true},
  {NULL}
};

// Events
static const int columnIndexesEventsUniqId[] = {
  IDX_EVENTS_SERVER_ID, IDX_EVENTS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsEvents[] = {
  {"EventsUniqId", &tableProfileEvents,
   (const int *)columnIndexesEventsUniqId, true},
  {NULL}
};

// Items
static const int columnIndexesItemsUniqId[] = {
  IDX_ITEMS_SERVER_ID, IDX_ITEMS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsItems[] = {
  {"ItemsUniqId", &tableProfileItems,
   (const int *)columnIndexesItemsUniqId, true},
  {NULL}
};

// Hosts
static const int columnIndexesHostsUniqId[] = {
  IDX_HOSTS_SERVER_ID, IDX_HOSTS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHosts[] = {
  {"HostsUniqId", &tableProfileHosts,
   (const int *)columnIndexesHostsUniqId, true},
  {NULL}
};

// Hostgroups
static const int columnIndexesHostgroupsUniqId[] = {
  IDX_HOSTGROUPS_SERVER_ID, IDX_HOSTGROUPS_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsHostgroups[] = {
  {"HostgroupsUniqId", &tableProfileHostgroups,
   (const int *)columnIndexesHostgroupsUniqId, true},
  {NULL}
};

// MapHostsHostgroups
static const int columnIndexesMapHostsHostgroupsUniqId[] = {
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_ID,
  DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsMapHostsHostgroups[] = {
  {"MapHostsHostgroupsUniqId", &tableProfileMapHostsHostgroups,
   (const int *)columnIndexesMapHostsHostgroupsUniqId, true},
  {NULL}
};

static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
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

static const DBAgent::TableProfile tableProfileServers(
  TABLE_NAME_SERVERS, COLUMN_DEF_SERVERS,
  sizeof(COLUMN_DEF_SERVERS), NUM_IDX_SERVERS);

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
{
	&tableProfileTriggers,
	(const DBAgent::IndexDef *)&indexDefsTriggers,
}, {
	&tableProfileEvents,
	(const DBAgent::IndexDef *)&indexDefsEvents,
}, {
	&tableProfileItems,
	(const DBAgent::IndexDef *)&indexDefsItems,
}, {
	&tableProfileHosts,
	(const DBAgent::IndexDef *)&indexDefsHosts,
}, {
	&tableProfileHostgroups,
	(const DBAgent::IndexDef *)&indexDefsHostgroups,
}, {
	&tableProfileMapHostsHostgroups,
	(const DBAgent::IndexDef *)&indexDefsMapHostsHostgroups,
}, {
	&tableProfileServers,
}
};

static const size_t NUM_TABLE_INFO =
sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

static const DBClient::DBSetupFuncArg DB_SETUP_FUNC_ARG = {
	DBClientHatohol::HATOHOL_DB_VERSION,
	NUM_TABLE_INFO,
	DB_TABLE_INFO,
};

struct DBClientHatohol::PrivateContext
{
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
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
	eventInfo.hostId = 0;
}

// ---------------------------------------------------------------------------
// HostResourceQueryOption's subclasses
// ---------------------------------------------------------------------------

//
// EventQueryOption
//
static const HostResourceQueryOption::Synapse synapseEventsQueryOption(
  tableProfileEvents,
  IDX_EVENTS_UNIFIED_ID, IDX_EVENTS_SERVER_ID, IDX_EVENTS_HOST_ID,
  INVALID_COLUMN_IDX,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct EventsQueryOption::PrivateContext {
	uint64_t limitOfUnifiedId;
	SortType sortType;
	SortDirection sortDirection;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;

	PrivateContext()
	: limitOfUnifiedId(NO_LIMIT),
	  sortType(SORT_UNIFIED_ID),
	  sortDirection(SORT_DONT_CARE),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL)
	{
	}
};

EventsQueryOption::EventsQueryOption(const UserIdType &userId)
: HostResourceQueryOption(synapseEventsQueryOption, userId)
{
	m_ctx = new PrivateContext();
}

EventsQueryOption::EventsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseEventsQueryOption, dataQueryContext)
{
	m_ctx = new PrivateContext();
}

EventsQueryOption::EventsQueryOption(const EventsQueryOption &src)
: HostResourceQueryOption(src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

EventsQueryOption::~EventsQueryOption()
{
	delete m_ctx;
}

string EventsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBClient::isAlwaysFalseCondition(condition))
		return condition;

	if (m_ctx->limitOfUnifiedId) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s<=%"PRIu64,
			getColumnName(IDX_EVENTS_UNIFIED_ID).c_str(),
			m_ctx->limitOfUnifiedId);
	}

	if (m_ctx->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		if (!condition.empty())
			condition += " AND ";
		// Use triggers table because events tables doesn't contain
		// correct severity.
		condition += StringUtils::sprintf(
			"%s.%s>=%d",
			DBClientHatohol::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName,
			m_ctx->minSeverity);
	}

	if (m_ctx->triggerStatus != TRIGGER_STATUS_ALL) {
		if (!condition.empty())
			condition += " AND ";
		// Use events table because triggers table doesn't contain past
		// status.
		condition += StringUtils::sprintf(
			"%s=%d",
			getColumnName(IDX_EVENTS_STATUS).c_str(),
			m_ctx->triggerStatus);
	}

	return condition;
}

void EventsQueryOption::setLimitOfUnifiedId(const uint64_t &unifiedId)
{
	m_ctx->limitOfUnifiedId = unifiedId;
}

uint64_t EventsQueryOption::getLimitOfUnifiedId(void) const
{
	return m_ctx->limitOfUnifiedId;
}

void EventsQueryOption::setSortType(
  const SortType &type, const SortDirection &direction)
{
	m_ctx->sortType = type;
	m_ctx->sortDirection = direction;

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
	return m_ctx->sortType;
}

DataQueryOption::SortDirection EventsQueryOption::getSortDirection(void) const
{
	return m_ctx->sortDirection;
}

void EventsQueryOption::setMinimumSeverity(const TriggerSeverityType &severity)
{
	m_ctx->minSeverity = severity;
}

TriggerSeverityType EventsQueryOption::getMinimumSeverity(void) const
{
	return m_ctx->minSeverity;
}

void EventsQueryOption::setTriggerStatus(const TriggerStatusType &status)
{
	m_ctx->triggerStatus = status;
}

TriggerStatusType EventsQueryOption::getTriggerStatus(void) const
{
	return m_ctx->triggerStatus;
}

//
// TriggersQueryOption
//
static const HostResourceQueryOption::Synapse synapseTriggersQueryOption(
  tableProfileTriggers,
  IDX_TRIGGERS_ID, IDX_TRIGGERS_SERVER_ID, IDX_TRIGGERS_HOST_ID,
  INVALID_COLUMN_IDX,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct TriggersQueryOption::PrivateContext {
	TriggerIdType targetId;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;

	PrivateContext()
	: targetId(ALL_TRIGGERS),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL)
	{
	}
};

TriggersQueryOption::TriggersQueryOption(UserIdType userId)
: HostResourceQueryOption(synapseTriggersQueryOption, userId)
{
	m_ctx = new PrivateContext();
}

TriggersQueryOption::TriggersQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseTriggersQueryOption, dataQueryContext)
{
	m_ctx = new PrivateContext();
}

TriggersQueryOption::TriggersQueryOption(const TriggersQueryOption &src)
: HostResourceQueryOption(src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

TriggersQueryOption::~TriggersQueryOption()
{
	delete m_ctx;
}

string TriggersQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBClient::isAlwaysFalseCondition(condition))
		return condition;

	if (m_ctx->targetId != ALL_TRIGGERS) {
		const DBTermCodec *dbTermCodec = getDBTermCodec();
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBClientHatohol::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
			dbTermCodec->enc(m_ctx->targetId).c_str());
	}

	if (m_ctx->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s>=%d",
			DBClientHatohol::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName,
			m_ctx->minSeverity);
	}

	if (m_ctx->triggerStatus != TRIGGER_STATUS_ALL) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%d",
			DBClientHatohol::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS].columnName,
			m_ctx->triggerStatus);
	}

	return condition;
}

void TriggersQueryOption::setTargetId(const TriggerIdType &id)
{
	m_ctx->targetId = id;
}

TriggerIdType TriggersQueryOption::getTargetId(void) const
{
	return m_ctx->targetId;
}

void TriggersQueryOption::setMinimumSeverity(const TriggerSeverityType &severity)
{
	m_ctx->minSeverity = severity;
}

TriggerSeverityType TriggersQueryOption::getMinimumSeverity(void) const
{
	return m_ctx->minSeverity;
}

void TriggersQueryOption::setTriggerStatus(const TriggerStatusType &status)
{
	m_ctx->triggerStatus = status;
}

TriggerStatusType TriggersQueryOption::getTriggerStatus(void) const
{
	return m_ctx->triggerStatus;
}

//
// ItemsQueryOption
//
static const HostResourceQueryOption::Synapse synapseItemsQueryOption(
  tableProfileItems, IDX_ITEMS_ID, IDX_ITEMS_SERVER_ID, IDX_ITEMS_HOST_ID,
  INVALID_COLUMN_IDX,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

struct ItemsQueryOption::PrivateContext {
	ItemIdType targetId;
	string itemGroupName;

	PrivateContext()
	: targetId(ALL_ITEMS)
	{
	}
};

ItemsQueryOption::ItemsQueryOption(UserIdType userId)
: HostResourceQueryOption(synapseItemsQueryOption, userId)
{
	m_ctx = new PrivateContext();
}

ItemsQueryOption::ItemsQueryOption(DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseItemsQueryOption, dataQueryContext)
{
	m_ctx = new PrivateContext();
}

ItemsQueryOption::ItemsQueryOption(const ItemsQueryOption &src)
: HostResourceQueryOption(src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

ItemsQueryOption::~ItemsQueryOption()
{
	delete m_ctx;
}

string ItemsQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBClient::isAlwaysFalseCondition(condition))
		return condition;

	if (m_ctx->targetId != ALL_ITEMS) {
		const DBTermCodec *dbTermCodec = getDBTermCodec();
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBClientHatohol::TABLE_NAME_ITEMS,
			COLUMN_DEF_ITEMS[IDX_ITEMS_ID].columnName,
			dbTermCodec->enc(m_ctx->targetId).c_str());
	}

	if (!m_ctx->itemGroupName.empty()) {
		if (!condition.empty())
			condition += " AND ";
		string escaped = StringUtils::replace(m_ctx->itemGroupName,
						      "'", "''");
		condition += StringUtils::sprintf(
			"%s.%s='%s'",
			DBClientHatohol::TABLE_NAME_ITEMS,
			COLUMN_DEF_ITEMS[IDX_ITEMS_ITEM_GROUP_NAME].columnName,
			escaped.c_str());
	}

	return condition;
}

void ItemsQueryOption::setTargetId(const ItemIdType &id)
{
	m_ctx->targetId = id;
}

ItemIdType ItemsQueryOption::getTargetId(void) const
{
	return m_ctx->targetId;
}

void ItemsQueryOption::setTargetItemGroupName(const string &itemGroupName)
{
	m_ctx->itemGroupName = itemGroupName;
}

const string &ItemsQueryOption::getTargetItemGroupName(void)
{
	return m_ctx->itemGroupName;
}

//
// HostsQueryOption
//
static const HostResourceQueryOption::Synapse synapseHostsQueryOption(
  tableProfileHosts, IDX_HOSTS_ID, IDX_HOSTS_SERVER_ID, IDX_HOSTS_HOST_ID,
  INVALID_COLUMN_IDX,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostsQueryOption::HostsQueryOption(UserIdType userId)
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
  IDX_HOSTGROUPS_GROUP_ID, IDX_HOSTGROUPS_SERVER_ID, INVALID_COLUMN_IDX,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostgroupsQueryOption::HostgroupsQueryOption(UserIdType userId)
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
  IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  tableProfileMapHostsHostgroups,
  IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
  IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

HostgroupElementQueryOption::HostgroupElementQueryOption(UserIdType userId)
: HostResourceQueryOption(synapseHostgroupElementQueryOption, userId)
{
}

HostgroupElementQueryOption::HostgroupElementQueryOption(
  DataQueryContext *dataQueryContext)
: HostResourceQueryOption(synapseHostgroupElementQueryOption, dataQueryContext)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientHatohol::init(void)
{
	registerSetupInfo(
	  DB_DOMAIN_ID_HATOHOL, DEFAULT_DB_NAME, &DB_SETUP_FUNC_ARG);
}

DBClientHatohol::DBClientHatohol(void)
: DBClient(DB_DOMAIN_ID_HATOHOL),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientHatohol::~DBClientHatohol()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientHatohol::getHostInfoList(HostInfoList &hostInfoList,
				      const HostsQueryOption &option)
{
	static const DBAgent::TableProfile *tableProfiles[] = {
	  &tableProfileHosts,
	  &tableProfileMapHostsHostgroups,
	};
	enum {
		TBLIDX_HOSTS,
		TBLIDX_MAP_HOSTS_HOSTGROUPS,
	};
	static const size_t numTableProfiles =
	  sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);
	DBAgent::SelectMultiTableArg arg(tableProfiles, numTableProfiles);

	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();

	arg.setTable(TBLIDX_HOSTS);
	arg.add(IDX_HOSTS_SERVER_ID);
	arg.add(IDX_HOSTS_HOST_ID);
	arg.add(IDX_HOSTS_HOST_NAME);

	// condition
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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

void DBClientHatohol::addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addTriggerInfoWithoutTransaction(*triggerInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addTriggerInfoList(const TriggerInfoList &triggerInfoList)
{
	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientHatohol::getTriggerInfo(TriggerInfo &triggerInfo,
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

void DBClientHatohol::getTriggerInfoList(TriggerInfoList &triggerInfoList,
					 const TriggersQueryOption &option)
{
	// build a condition
	string condition = option.getCondition();
	if (isAlwaysFalseCondition(condition))
		return;

	// select data
	static const DBAgent::TableProfile *tableProfiles[] = {
	  &tableProfileTriggers,
	  &tableProfileMapHostsHostgroups,
	};
	enum {
		TBLIDX_TRIGGERS,
		TBLIDX_MAP_HOSTS_HOSTGROUPS,
	};
	static const size_t numTableProfiles =
	  sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);
	DBAgent::SelectMultiTableArg arg(tableProfiles, numTableProfiles);

	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();

	arg.setTable(TBLIDX_TRIGGERS);
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

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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

void DBClientHatohol::setTriggerInfoList(const TriggerInfoList &triggerInfoList,
                                         const ServerIdType &serverId)
{
	// TODO: This way is too rough and inefficient.
	//       We should update only the changed triggers.
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	DBAgent::DeleteArg deleteArg(tableProfileTriggers);
	deleteArg.condition =
	  StringUtils::sprintf("%s=%s",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    dbTermCodec->enc(serverId).c_str());

	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

int DBClientHatohol::getLastChangeTimeOfTrigger(const ServerIdType &serverId)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.condition = StringUtils::sprintf("%s=%s",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    dbTermCodec->enc(serverId).c_str());

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	if (arg.dataTable->getNumberOfRows() == 0)
		return 0;

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

void DBClientHatohol::addEventInfo(EventInfo *eventInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addEventInfoWithoutTransaction(*eventInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addEventInfoList(const EventInfoList &eventInfoList)
{
	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != eventInfoList.end(); ++it)
			addEventInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

HatoholError DBClientHatohol::getEventInfoList(EventInfoList &eventInfoList,
                                               const EventsQueryOption &option)
{
	static const DBAgent::TableProfile *tableProfiles[] = {
	  &tableProfileEvents,
	  &tableProfileTriggers,
	};
	enum {
		TBLIDX_EVENTS,
		TBLIDX_TRIGGERS,
	};
	static const size_t numTableProfiles =
	  sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);
	DBAgent::SelectMultiTableArg arg(tableProfiles, numTableProfiles);

	option.useTableNameAlways();
	arg.useDistinct = option.isHostgroupUsed();

	// Tables
	arg.tableField = option.getFromClause();
	arg.tableField += StringUtils::sprintf(
	  " INNER JOIN %s ON (%s=%s AND %s=%s)",
	  TABLE_NAME_TRIGGERS,

	  option.getColumnName(IDX_EVENTS_SERVER_ID).c_str(),
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_SERVER_ID).c_str(),

	  option.getColumnName(IDX_EVENTS_TRIGGER_ID).c_str(),
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_ID).c_str()),

	// Columns
	arg.setTable(TBLIDX_EVENTS);
	arg.add(IDX_EVENTS_UNIFIED_ID);
	arg.add(IDX_EVENTS_SERVER_ID);
	arg.add(IDX_EVENTS_ID);
	arg.add(IDX_EVENTS_TIME_SEC);
	arg.add(IDX_EVENTS_TIME_NS);
	arg.add(IDX_EVENTS_EVENT_TYPE);
	arg.add(IDX_EVENTS_TRIGGER_ID);

	arg.setTable(TBLIDX_TRIGGERS);
	arg.add(IDX_TRIGGERS_STATUS);
	arg.add(IDX_TRIGGERS_SEVERITY);
	arg.add(IDX_TRIGGERS_HOST_ID);
	arg.add(IDX_TRIGGERS_HOSTNAME);
	arg.add(IDX_TRIGGERS_BRIEF);

	// Condition
	arg.condition = StringUtils::sprintf(
	  "%s=%s", 
	  arg.getFullName(TBLIDX_EVENTS, IDX_EVENTS_SERVER_ID).c_str(),
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_SERVER_ID).c_str());

	string optCond = option.getCondition();

	if (isAlwaysFalseCondition(optCond))
		return HatoholError(HTERR_OK);
	if (!optCond.empty()) {
		arg.condition += " AND ";
		arg.condition += optCond;
	}

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();

	if (!arg.limit && arg.offset)
		return HTERR_OFFSET_WITHOUT_LIMIT;

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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
		itemGroupStream >> eventInfo.status;
		itemGroupStream >> eventInfo.severity;
		itemGroupStream >> eventInfo.hostId;
		itemGroupStream >> eventInfo.hostName;
		itemGroupStream >> eventInfo.brief;
	}
	return HatoholError(HTERR_OK);
}

void DBClientHatohol::setEventInfoList(const EventInfoList &eventInfoList,
                                       const ServerIdType &serverId)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	DBAgent::DeleteArg deleteArg(tableProfileEvents);
	deleteArg.condition =
	  StringUtils::sprintf("%s=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    dbTermCodec->enc(serverId).c_str());

	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != eventInfoList.end(); ++it)
			addEventInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupInfo(HostgroupInfo *groupInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostgroupInfoWithoutTransaction(*groupInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupInfoList(const HostgroupInfoList &groupInfoList)
{
	HostgroupInfoListConstIterator it = groupInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != groupInfoList.end(); ++it)
			addHostgroupInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupElement
  (HostgroupElement *hostgroupElement)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostgroupElementWithoutTransaction(*hostgroupElement);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupElementList
  (const HostgroupElementList &hostgroupElementList)
{
	HostgroupElementListConstIterator it = hostgroupElementList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != hostgroupElementList.end(); ++it)
			addHostgroupElementWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostInfo(HostInfo *hostInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostInfoWithoutTransaction(*hostInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostInfoList(const HostInfoList &hostInfoList)
{
	HostInfoListConstIterator it = hostInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for(; it != hostInfoList.end(); ++it)
			addHostInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

uint64_t DBClientHatohol::getLastEventId(const ServerIdType &serverId)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	DBAgent::SelectExArg arg(tableProfileEvents);
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName);
	arg.add(stmt, COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
	arg.condition = StringUtils::sprintf("%s=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName, 
	    dbTermCodec->enc(serverId).c_str());

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	if (arg.dataTable->getNumberOfRows() == 0)
		return EVENT_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<uint64_t>();
}

void DBClientHatohol::addItemInfo(ItemInfo *itemInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItemInfoWithoutTransaction(*itemInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addItemInfoList(const ItemInfoList &itemInfoList)
{
	ItemInfoListConstIterator it = itemInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != itemInfoList.end(); ++it)
			addItemInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::getItemInfoList(ItemInfoList &itemInfoList,
				      const ItemsQueryOption &option)
{
	static const DBAgent::TableProfile *tableProfiles[] = {
	  &tableProfileItems,
	  &tableProfileMapHostsHostgroups,
	};
	enum {
		TBLIDX_ITEMS,
		TBLIDX_MAP_HOSTS_HOSTGROUPS,
	};
	static const size_t numTableProfiles =
	  sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);
	DBAgent::SelectMultiTableArg arg(tableProfiles, numTableProfiles);

	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();

	arg.setTable(TBLIDX_ITEMS);
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
	if (isAlwaysFalseCondition(arg.condition))
		return;

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();
	if (!arg.limit && arg.offset)
		return;

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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

void DBClientHatohol::addMonitoringServerStatus(
  MonitoringServerStatus *serverStatus)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addMonitoringServerStatusWithoutTransaction(*serverStatus);
	} DBCLIENT_TRANSACTION_END();
}

size_t DBClientHatohol::getNumberOfBadTriggers(
  const TriggersQueryOption &option, TriggerSeverityType severity)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = "count(*)";
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// from
	arg.tableField = option.getFromClause();
	arg.useDistinct = option.isHostgroupUsed();

	// condition
	arg.condition = option.getCondition();
	if (!arg.condition.empty())
		arg.condition += " and ";
	arg.condition +=
	  StringUtils::sprintf("%s=%d and %s=%d",
	    option.getColumnName(IDX_TRIGGERS_SEVERITY).c_str(), severity,
	    option.getColumnName(IDX_TRIGGERS_STATUS).c_str(), 
	    TRIGGER_STATUS_PROBLEM);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBClientHatohol::getNumberOfHosts(const TriggersQueryOption &option)
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

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBClientHatohol::getNumberOfGoodHosts(const TriggersQueryOption &option)
{
	size_t numTotalHost = getNumberOfHosts(option);
	size_t numBadHosts = getNumberOfBadHosts(option);
	HATOHOL_ASSERT(numTotalHost >= numBadHosts,
	               "numTotalHost: %zd, numBadHosts: %zd",
	               numTotalHost, numBadHosts);
	return numTotalHost - numBadHosts;
}

size_t DBClientHatohol::getNumberOfBadHosts(const TriggersQueryOption &option)
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

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

HatoholError DBClientHatohol::getNumberOfMonitoredItemsPerSecond
  (const DataQueryOption &option, MonitoringServerStatus &serverStatus)
{
	serverStatus.nvps = 0.0;
	if (!option.isValidServer(serverStatus.serverId))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgent::SelectExArg arg(tableProfileServers);
	string stmt = COLUMN_DEF_SERVERS[IDX_SERVERS_NVPS].columnName;
	arg.add(stmt, SQL_COLUMN_TYPE_DOUBLE);

	arg.condition =
	  StringUtils::sprintf("%s=%d",
	    COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName,
	    serverStatus.serverId);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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

void DBClientHatohol::pickupAbsentHostIds(vector<uint64_t> &absentHostIdVector,
                         const vector<uint64_t> &hostIdVector)
{
	string condition;
	static const string tableName = TABLE_NAME_HOSTS;
	static const string hostIdName =
	  COLUMN_DEF_HOSTS[IDX_HOSTS_HOST_ID].columnName;
	DBCLIENT_TRANSACTION_BEGIN() {
		for (size_t i = 0; i < hostIdVector.size(); i++) {
			uint64_t id = hostIdVector[i];
			condition = hostIdName;
			condition += StringUtils::sprintf("=%"PRIu64, id);
			if (isRecordExisting(tableName, condition))
				continue;
			absentHostIdVector.push_back(id);
		}
	} DBCLIENT_TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
// TODO: Use DBAgent::updateIfExistElseInsert() for these methods.
void DBClientHatohol::addTriggerInfoWithoutTransaction(
  const TriggerInfo &triggerInfo)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND id=%s",
	    dbTermCodec->enc(triggerInfo.serverId).c_str(),
	    dbTermCodec->enc(triggerInfo.id).c_str());
	if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
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
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileTriggers);
		arg.add(IDX_TRIGGERS_SERVER_ID, triggerInfo.serverId);
		arg.add(IDX_TRIGGERS_STATUS,    triggerInfo.status);
		arg.add(IDX_TRIGGERS_SEVERITY,  triggerInfo.severity);
		arg.add(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
		        triggerInfo.lastChangeTime.tv_sec);
		arg.add(IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
		        triggerInfo.lastChangeTime.tv_nsec);
		arg.add(IDX_TRIGGERS_HOST_ID,   triggerInfo.hostId);
		arg.add(IDX_TRIGGERS_HOSTNAME,  triggerInfo.hostName);
		arg.add(IDX_TRIGGERS_BRIEF,     triggerInfo.brief);
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addEventInfoWithoutTransaction(const EventInfo &eventInfo)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND id=%s",
	  dbTermCodec->enc(eventInfo.serverId).c_str(),
	  dbTermCodec->enc(eventInfo.id).c_str());
	if (!isRecordExisting(TABLE_NAME_EVENTS, condition)) {
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
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileEvents);
		arg.add(IDX_EVENTS_SERVER_ID,  eventInfo.serverId);
		arg.add(IDX_EVENTS_TIME_SEC,   eventInfo.time.tv_sec);
		arg.add(IDX_EVENTS_TIME_NS,    eventInfo.time.tv_nsec);
		arg.add(IDX_EVENTS_EVENT_TYPE, eventInfo.type);
		arg.add(IDX_EVENTS_TRIGGER_ID, eventInfo.triggerId);
		arg.add(IDX_EVENTS_STATUS,     eventInfo.status);
		arg.add(IDX_EVENTS_SEVERITY,   eventInfo.severity);
		arg.add(IDX_EVENTS_HOST_ID,    eventInfo.hostId);
		arg.add(IDX_EVENTS_HOST_NAME,  eventInfo.hostName);
		arg.add(IDX_EVENTS_BRIEF,      eventInfo.brief);
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addItemInfoWithoutTransaction(const ItemInfo &itemInfo)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND id=%s",
	  dbTermCodec->enc(itemInfo.serverId).c_str(),
	  dbTermCodec->enc(itemInfo.id).c_str());
	if (!isRecordExisting(TABLE_NAME_ITEMS, condition)) {
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
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileItems);
		arg.add(IDX_ITEMS_SERVER_ID,  itemInfo.serverId);
		arg.add(IDX_ITEMS_ID,         itemInfo.id);
		arg.add(IDX_ITEMS_HOST_ID,    itemInfo.hostId);
		arg.add(IDX_ITEMS_BRIEF,      itemInfo.brief);
		arg.add(IDX_ITEMS_LAST_VALUE_TIME_SEC,
		        itemInfo.lastValueTime.tv_sec);
		arg.add(IDX_ITEMS_LAST_VALUE_TIME_NS,
		        itemInfo.lastValueTime.tv_nsec); 
		arg.add(IDX_ITEMS_LAST_VALUE, itemInfo.lastValue);
		arg.add(IDX_ITEMS_PREV_VALUE, itemInfo.prevValue);
		arg.add(IDX_ITEMS_ITEM_GROUP_NAME, itemInfo.itemGroupName);
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addHostgroupInfoWithoutTransaction(
  const HostgroupInfo &groupInfo)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND host_group_id=%s",
	  dbTermCodec->enc(groupInfo.serverId).c_str(),
	  dbTermCodec->enc(groupInfo.groupId).c_str());
	if (!isRecordExisting(TABLE_NAME_HOSTGROUPS, condition)) {
		DBAgent::InsertArg arg(tableProfileHostgroups);
		arg.add(groupInfo.id);
		arg.add(groupInfo.serverId);
		arg.add(groupInfo.groupId);
		arg.add(groupInfo.groupName);
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileHostgroups);
		arg.add(IDX_HOSTGROUPS_SERVER_ID,  groupInfo.serverId);
		arg.add(IDX_HOSTGROUPS_GROUP_ID,   groupInfo.groupId);
		arg.add(IDX_HOSTGROUPS_GROUP_NAME, groupInfo.groupName);
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addHostgroupElementWithoutTransaction(
  const HostgroupElement &hostgroupElement)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND host_id=%s AND host_group_id=%s",
	  dbTermCodec->enc(hostgroupElement.serverId).c_str(),
	  dbTermCodec->enc(hostgroupElement.hostId).c_str(),
	  dbTermCodec->enc(hostgroupElement.groupId).c_str());

	if (!isRecordExisting(TABLE_NAME_MAP_HOSTS_HOSTGROUPS, condition)) {
		DBAgent::InsertArg arg(tableProfileMapHostsHostgroups);
		arg.add(hostgroupElement.id);
		arg.add(hostgroupElement.serverId);
		arg.add(hostgroupElement.hostId);
		arg.add(hostgroupElement.groupId);
		insert(arg);
	}
}

void DBClientHatohol::addHostInfoWithoutTransaction(const HostInfo &hostInfo)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "server_id=%s AND host_id=%s",
	  dbTermCodec->enc(hostInfo.serverId).c_str(),
	  dbTermCodec->enc(hostInfo.id).c_str());

	if (!isRecordExisting(TABLE_NAME_HOSTS, condition)) {
		DBAgent::InsertArg arg(tableProfileHosts);
		arg.add(AUTO_INCREMENT_VALUE);
		arg.add(hostInfo.serverId);
		arg.add(hostInfo.id);
		arg.add(hostInfo.hostName);
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileHosts);
		arg.add(IDX_HOSTS_SERVER_ID, hostInfo.serverId);
		arg.add(IDX_HOSTS_HOST_ID,   hostInfo.id);
		arg.add(IDX_HOSTS_HOST_NAME, hostInfo.hostName);
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addMonitoringServerStatusWithoutTransaction(
  const MonitoringServerStatus &serverStatus)
{
	const DBTermCodec *dbTermCodec = getDBAgent()->getDBTermCodec();
	string condition = StringUtils::sprintf(
	  "id=%s", dbTermCodec->enc(serverStatus.serverId).c_str());
	if (!isRecordExisting(TABLE_NAME_SERVERS, condition)) {
		DBAgent::InsertArg arg(tableProfileServers);
		arg.add(serverStatus.serverId);
		arg.add(serverStatus.nvps);
		insert(arg);
	} else {
		DBAgent::UpdateArg arg(tableProfileServers);
		arg.add(IDX_SERVERS_ID,   serverStatus.serverId);
		arg.add(IDX_SERVERS_NVPS, serverStatus.nvps);
		arg.condition = condition;
		update(arg);
	}
}

HatoholError DBClientHatohol::getHostgroupInfoList
  (HostgroupInfoList &hostgroupInfoList, const HostgroupsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileHostgroups);
	arg.add(IDX_HOSTGROUPS_ID);
	arg.add(IDX_HOSTGROUPS_SERVER_ID);
	arg.add(IDX_HOSTGROUPS_GROUP_ID);
	arg.add(IDX_HOSTGROUPS_GROUP_NAME);
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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
HatoholError DBClientHatohol::getHostgroupElementList
  (HostgroupElementList &hostgroupElementList,
   const HostgroupElementQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileMapHostsHostgroups);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

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
