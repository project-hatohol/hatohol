/*
 * Copyright (C) 2013-2016 Project Hatohol
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
#include <SeparatorInjector.h>
#include "UnifiedDataStore.h"
#include "DBAgentFactory.h"
#include "DBTablesMonitoring.h"
#include "DBTablesUser.h"
#include "ThreadLocalDBCache.h"
#include "SQLUtils.h"
#include "Params.h"
#include "ItemGroupStream.h"
#include "DBClientJoinBuilder.h"
#include "DBTermCStringProvider.h"
#include "StatisticsCounter.h"

// TODO: rmeove the followin two include files!
// This class should not be aware of it.
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"

using namespace std;
using namespace mlpl;

const char *DBTablesMonitoring::TABLE_NAME_TRIGGERS   = "triggers";
const char *DBTablesMonitoring::TABLE_NAME_EVENTS     = "events";
const char *DBTablesMonitoring::TABLE_NAME_ITEMS      = "items";
const char *DBTablesMonitoring::TABLE_NAME_ITEM_CATEGORIES = "item_categories";
const char *DBTablesMonitoring::TABLE_NAME_SERVER_STATUS = "server_status";
const char *DBTablesMonitoring::TABLE_NAME_INCIDENTS  = "incidents";
const char *DBTablesMonitoring::TABLE_NAME_INCIDENT_HISTORIES =
  "incident_histories";

// -> 1.0
//   * remove IDX_TRIGGERS_HOST_ID,
//   * add IDX_TRIGGERS_GLOBAL_HOST_ID and IDX_TRIGGERS_HOST_ID_IN_SERVER
//   * add IDX_ITEMS_GLOBAL_HOST_ID and IDX_ITEMS_HOST_ID_IN_SERVER
//   * triggers.id        -> VARCHAR
//   * events.trigger_id  -> VARCHAR
//   * events.id          -> VARCHAR
//   * incidents.event_id -> VARCHAR
//   * items.id           -> VARCHAR
// -> 2.0
//   * Add unique integer ID to COLUMN_DEF_ITEMS
//   * remove 'item_group_name' from COLUMN_DEF_ITEMS
//   * Add COLUMN_DEF_ITEM_GROUPS_NAME
// -> 2.2
//   *incident_histories.comment: 2048 -> 32767
const int DBTablesMonitoring::MONITORING_DB_VERSION =
  DBTables::Version::getPackedVer(0, 2, 2);

static StatisticsCounter *eventsCounters[] = {
  new StatisticsCounter(10),
  new StatisticsCounter(100),
  new StatisticsCounter(1000),
};
static size_t _check = HATOHOL_BUILD_EXPECT(
  ARRAY_SIZE(eventsCounters), DBTablesMonitoring::NUM_EVENTS_COUNTERS);

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

void operator>>(ItemGroupStream &itemGroupStream, HostValidity &rhs)
{
	rhs = itemGroupStream.read<int, HostValidity>();
}

extern void operator>>(ItemGroupStream &itemGroupStream, HostStatus &rhs);

void operator>>(ItemGroupStream &itemGroupStream, TriggerValidity &rhs)
{
	rhs = itemGroupStream.read<int, TriggerValidity>();
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
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
	"global_host_id",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id_in_server",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
}, {
	"extended_info",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"validity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"1",                               // defaultValue
},
};

enum {
	IDX_TRIGGERS_SERVER_ID,
	IDX_TRIGGERS_ID,
	IDX_TRIGGERS_STATUS,
	IDX_TRIGGERS_SEVERITY,
	IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
	IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
	IDX_TRIGGERS_GLOBAL_HOST_ID,
	IDX_TRIGGERS_HOST_ID_IN_SERVER,
	IDX_TRIGGERS_HOSTNAME,
	IDX_TRIGGERS_BRIEF,
	IDX_TRIGGERS_EXTENDED_INFO,
	IDX_TRIGGERS_VALIDITY,
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
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
	"global_host_id",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id_in_server",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
}, {
	"extended_info",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
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
	IDX_EVENTS_GLOBAL_HOST_ID,
	IDX_EVENTS_HOST_ID_IN_SERVER,
	IDX_EVENTS_HOST_NAME,
	IDX_EVENTS_BRIEF,
	IDX_EVENTS_EXTENDED_INFO,
	NUM_IDX_EVENTS,
};

static const int columnIndexesEventsUniqId[] = {
  IDX_EVENTS_SERVER_ID, IDX_EVENTS_ID, DBAgent::IndexDef::END,
};

static const int columnIndexesEventsTimeSequence[] = {
  IDX_EVENTS_TIME_SEC, IDX_EVENTS_TIME_NS,
  IDX_EVENTS_UNIFIED_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsEvents[] = {
  {"EventsId", (const int *)columnIndexesEventsUniqId, false},
  {"EventsTimeSequence", (const int *)columnIndexesEventsTimeSequence, false},
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
	"global_id",                       // columnName
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
	SQL_KEY_NONE, // indexDefsItems    // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"id",                              // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"global_host_id",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id_in_server",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
	"value_type",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"unit",                            // columnName
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
	IDX_ITEMS_GLOBAL_ID,
	IDX_ITEMS_SERVER_ID,
	IDX_ITEMS_ID,
	IDX_ITEMS_GLOBAL_HOST_ID,
	IDX_ITEMS_HOST_ID_IN_SERVER,
	IDX_ITEMS_BRIEF,
	IDX_ITEMS_LAST_VALUE_TIME_SEC,
	IDX_ITEMS_LAST_VALUE_TIME_NS,
	IDX_ITEMS_LAST_VALUE,
	IDX_ITEMS_PREV_VALUE,
	IDX_ITEMS_VALUE_TYPE,
	IDX_ITEMS_UNIT,
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

static const ColumnDef COLUMN_DEF_ITEM_CATEGORIES[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"global_item_id",                  // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_ITEM_CATEGORIES_ID,
	IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID,
	IDX_ITEM_CATEGORIES_NAME,
	NUM_IDX_ITEM_CATEGORIES,
};

static const DBAgent::TableProfile tableProfileItemCategories =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_ITEM_CATEGORIES,
			    COLUMN_DEF_ITEM_CATEGORIES,
			    NUM_IDX_ITEM_CATEGORIES);

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
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
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
}, {
	"unified_event_id",                // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"comment_count",                   // columnName
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
	IDX_INCIDENTS_UNIFIED_EVENT_ID,
	IDX_INCIDENTS_COMMENT_COUNT,
	NUM_IDX_INCIDENTS,
};

static const int columnIndexesIncidentsIdentifier[] = {
  IDX_INCIDENTS_TRACKER_ID, IDX_INCIDENTS_IDENTIFIER, DBAgent::IndexDef::END,
};

static const int columnIndexesIncidentsUnifiedEventId[] = {
  IDX_INCIDENTS_UNIFIED_EVENT_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsIncidents[] = {
  {
    "IncidentsIdentifier",
    (const int *)columnIndexesIncidentsIdentifier,
    true
  },
  {
    "IncidentsUnifiedEventId",
    (const int *)columnIndexesIncidentsUnifiedEventId,
    false
  },
  {NULL}
};

static const DBAgent::TableProfile tableProfileIncidents =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_INCIDENTS,
			    COLUMN_DEF_INCIDENTS,
			    NUM_IDX_INCIDENTS,
			    indexDefsIncidents);

// ----------------------------------------------------------------------------
// Table: incident_histories
// ----------------------------------------------------------------------------
static const ColumnDef COLUMN_DEF_INCIDENT_HISTORIES[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"unified_event_id",                // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"user_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
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
	"comment",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	32767,                             // columnLength
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
},
};

enum {
	IDX_INCIDENT_HISTORIES_ID,
	IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID,
	IDX_INCIDENT_HISTORIES_USER_ID,
	IDX_INCIDENT_HISTORIES_STATUS,
	IDX_INCIDENT_HISTORIES_COMMENT,
	IDX_INCIDENT_HISTORIES_CREATED_AT_SEC,
	IDX_INCIDENT_HISTORIES_CREATED_AT_NS,
	NUM_IDX_INCIDENT_HISTORIES,
};

static const int columnIndexesCreatedAtSequence[] = {
  IDX_INCIDENT_HISTORIES_CREATED_AT_SEC, IDX_INCIDENT_HISTORIES_CREATED_AT_NS,
  IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID, DBAgent::IndexDef::END,
};

static const DBAgent::IndexDef indexDefsIncidentHistories[] = {
  {"CreatedtAtSequence", (const int *)columnIndexesCreatedAtSequence, false},
  {NULL}
};

static const DBAgent::TableProfile tableProfileIncidentHistories =
  DBAGENT_TABLEPROFILE_INIT(DBTablesMonitoring::TABLE_NAME_INCIDENT_HISTORIES,
			    COLUMN_DEF_INCIDENT_HISTORIES,
			    NUM_IDX_INCIDENT_HISTORIES,
			    indexDefsIncidentHistories);

struct DBTablesMonitoring::Impl
{
	bool storedHostsChanged;

	Impl(void)
	: storedHostsChanged(true)
	{
	}

	virtual ~Impl()
	{
	}

	static void addEventStatistics(const uint64_t &number)
	{
		for (auto &counter : eventsCounters)
			counter->add(number);
	}
};

// ---------------------------------------------------------------------------
// EventInfo
// ---------------------------------------------------------------------------
void initEventInfo(EventInfo &eventInfo)
{
	eventInfo.unifiedId = 0;
	eventInfo.serverId = 0;
	eventInfo.id.clear();
	eventInfo.time.tv_sec = 0;
	eventInfo.time.tv_nsec = 0;
	eventInfo.type = EVENT_TYPE_UNKNOWN;
	eventInfo.triggerId.clear();
	eventInfo.status = TRIGGER_STATUS_UNKNOWN;
	eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
	eventInfo.globalHostId = INVALID_HOST_ID;
	eventInfo.hostIdInServer.clear();
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
  tableProfileEvents,
  IDX_EVENTS_HOST_ID_IN_SERVER, true,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID,
  IDX_EVENTS_GLOBAL_HOST_ID, IDX_HOSTGROUP_MEMBER_HOST_ID
);

struct EventsQueryOption::Impl {
	uint64_t limitOfUnifiedId;
	SortType sortType;
	SortDirection sortDirection;
	EventType type;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;
	TriggerIdType triggerId;
	timespec beginTime;
	timespec endTime;
	set<EventType> eventTypes;
	set<TriggerSeverityType> triggerSeverities;
	set<TriggerStatusType> triggerStatuses;
	set<string> incidentStatuses;
	vector<string> groupByColumns;
	list<string> hostnameList;
	list<EventIdType> eventIds;

	Impl()
	: limitOfUnifiedId(NO_LIMIT),
	  sortType(SORT_UNIFIED_ID),
	  sortDirection(SORT_DONT_CARE),
	  type(EVENT_TYPE_ALL),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL),
	  triggerId(ALL_TRIGGERS),
	  beginTime({0, 0}),
	  endTime({0, 0}),
	  hostnameList({}),
	  eventIds({})
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

	if (m_impl->type != EVENT_TYPE_ALL) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s=%d",
			getColumnName(IDX_EVENTS_EVENT_TYPE).c_str(),
			m_impl->type);
	}

	if (m_impl->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		if (!condition.empty())
			condition += " AND ";
		// Use events table because events tables contains severity.
		condition += StringUtils::sprintf(
			"%s>=%d",
			COLUMN_DEF_EVENTS[IDX_EVENTS_SEVERITY].columnName,
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
		DBTermCStringProvider rhs(*getDBTermCodec());
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s=%s",
			getColumnName(IDX_EVENTS_TRIGGER_ID).c_str(),
			rhs(m_impl->triggerId));
	}

	if (m_impl->beginTime.tv_sec != 0 || m_impl->beginTime.tv_nsec != 0) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"(%s>%ld OR (%s=%ld AND %s>=%ld))",
			getColumnName(IDX_EVENTS_TIME_SEC).c_str(),
			m_impl->beginTime.tv_sec,
			getColumnName(IDX_EVENTS_TIME_SEC).c_str(),
			m_impl->beginTime.tv_sec,
			getColumnName(IDX_EVENTS_TIME_NS).c_str(),
			m_impl->beginTime.tv_nsec);
	}

	if (m_impl->endTime.tv_sec != 0 || m_impl->endTime.tv_nsec != 0) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"(%s<%ld OR (%s=%ld AND %s<=%ld))",
			getColumnName(IDX_EVENTS_TIME_SEC).c_str(),
			m_impl->endTime.tv_sec,
			getColumnName(IDX_EVENTS_TIME_SEC).c_str(),
			m_impl->endTime.tv_sec,
			getColumnName(IDX_EVENTS_TIME_NS).c_str(),
			m_impl->endTime.tv_nsec);
	}

	if (!m_impl->hostnameList.empty()) {
		addCondition(condition,
			     makeHostnameListCondition(m_impl->hostnameList));
	}

	if (!m_impl->eventIds.empty()) {
		addCondition(condition,
		             makeEventIdListCondition(m_impl->eventIds));
	}

	string typeCondition;
	for (const auto &type: m_impl->eventTypes) {
		addCondition(
		  typeCondition,
		  StringUtils::sprintf(
		    "%s=%d",
		    getColumnName(IDX_EVENTS_EVENT_TYPE).c_str(),
		    type),
		  ADD_TYPE_OR);
	}
	if (m_impl->eventTypes.size() > 1)
		typeCondition = string("(") + typeCondition + string(")");
	addCondition(condition, typeCondition);

	string severityCondition;
	for (const auto &severity: m_impl->triggerSeverities) {
		addCondition(
		  severityCondition,
		  StringUtils::sprintf(
		    "%s=%d",
		    getColumnName(IDX_EVENTS_SEVERITY).c_str(),
		    severity),
		  ADD_TYPE_OR);
	}
	if (m_impl->triggerSeverities.size() > 1)
		severityCondition = string("(") + severityCondition + string(")");
	addCondition(condition, severityCondition);

	string statusCondition;
	for (const auto &status: m_impl->triggerStatuses) {
		addCondition(
		  statusCondition,
		  StringUtils::sprintf(
		    "%s=%d",
		    getColumnName(IDX_EVENTS_STATUS).c_str(),
		    status),
		  ADD_TYPE_OR);
	}
	if (m_impl->triggerStatuses.size() > 1)
		statusCondition = string("(") + statusCondition + string(")");
	addCondition(condition, statusCondition);

	string incidentStatusCondition;
	for (const auto &status: m_impl->incidentStatuses) {
		DBTermCStringProvider rhs(*getDBTermCodec());

		addCondition(
		  incidentStatusCondition,
		  StringUtils::sprintf(
		    "%s.%s=%s",
		    DBTablesMonitoring::TABLE_NAME_INCIDENTS,
		    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_STATUS].columnName,
		    rhs(status)),
		  ADD_TYPE_OR);

		if (status.empty()) {
			addCondition(
			  incidentStatusCondition,
			  StringUtils::sprintf(
			    "%s.%s is NULL",
			    DBTablesMonitoring::TABLE_NAME_INCIDENTS,
			    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_STATUS].columnName),
			  ADD_TYPE_OR);
		}
	}
	if (m_impl->incidentStatuses.size() > 1)
		incidentStatusCondition =
			string("(") +
			incidentStatusCondition +
			string(")");
	addCondition(condition, incidentStatusCondition);

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
		SortOrderVect sortOrderVect;
		SortOrder order1(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_SEC].columnName,
		  direction);
		SortOrder order2(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_NS].columnName,
		  direction);
		SortOrder order3(
		  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName,
		  direction);
		sortOrderVect.reserve(3);
		sortOrderVect.push_back(order1);
		sortOrderVect.push_back(order2);
		sortOrderVect.push_back(order3);
		setSortOrderVect(sortOrderVect);
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

void EventsQueryOption::setGroupByColumns(const std::vector<std::string> &columns)
{
	m_impl->groupByColumns = columns;

	vector<GroupBy> groupByVect;
	for (auto &column : columns) {
		GroupBy groupBy(column);
		groupByVect.push_back(groupBy);
	}
	setGroupByVect(groupByVect);
}

std::vector<std::string> EventsQueryOption::getGroupByColumns(void) const
{
	return m_impl->groupByColumns;
}

void EventsQueryOption::setMinimumSeverity(const TriggerSeverityType &severity)
{
	m_impl->minSeverity = severity;
}

TriggerSeverityType EventsQueryOption::getMinimumSeverity(void) const
{
	return m_impl->minSeverity;
}

void EventsQueryOption::setType(const EventType &type)
{
	m_impl->type = type;
}

EventType EventsQueryOption::getType(void) const
{
	return m_impl->type;
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

void EventsQueryOption::setBeginTime(const timespec &_beginTime)
{
	m_impl->beginTime = _beginTime;
}

const timespec &EventsQueryOption::getBeginTime(void)
{
	return m_impl->beginTime;
}

void EventsQueryOption::setEndTime(const timespec &_endTime)
{
	m_impl->endTime = _endTime;
}

void EventsQueryOption::setHostnameList(const list<string> &hostnameList)
{
	m_impl->hostnameList = hostnameList;
}

const list<string> EventsQueryOption::getHostnameList(void)
{
	return m_impl->hostnameList;
}

const timespec &EventsQueryOption::getEndTime(void)
{
	return m_impl->endTime;
}

void EventsQueryOption::setEventTypes(const std::set<EventType> &types)
{
	m_impl->eventTypes = types;
}

const std::set<EventType> &EventsQueryOption::getEventTypes(void) const
{
	return m_impl->eventTypes;
}

void EventsQueryOption::setEventIds(const list<EventIdType> &eventIds)
{
	m_impl->eventIds = eventIds;
}

void EventsQueryOption::setTriggerSeverities(
  const set<TriggerSeverityType> &severities)
{
	m_impl->triggerSeverities = severities;
}

const set<TriggerSeverityType> &EventsQueryOption::getTriggerSeverities(void) const
{
	return m_impl->triggerSeverities;
}

void EventsQueryOption::setTriggerStatuses(
  const std::set<TriggerStatusType> &statuses)
{
	m_impl->triggerStatuses = statuses;
}

const std::set<TriggerStatusType> &EventsQueryOption::getTriggerStatuses(void) const
{
	return m_impl->triggerStatuses;
}

void EventsQueryOption::setIncidentStatuses(const std::set<std::string> &statuses)
{
	m_impl->incidentStatuses = statuses;
}

const std::set<std::string> &EventsQueryOption::getIncidentStatuses(void) const
{
	return m_impl->incidentStatuses;
}

string EventsQueryOption::makeHostnameListCondition(
  const list<string> &hostnameList) const
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_EVENTS[IDX_EVENTS_HOST_NAME];
	SeparatorInjector commaInjector(",");
	DBTermCStringProvider rhs(*getDBTermCodec());
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto hostname : hostnameList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%s", rhs(hostname.c_str()));
	}

	condition += ")";
	return condition;
}

string EventsQueryOption::makeEventIdListCondition(
  const list<EventIdType> &eventIds) const
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_EVENTS[IDX_EVENTS_ID];
	SeparatorInjector commaInjector(",");
	DBTermCStringProvider rhs(*getDBTermCodec());
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (const auto &eventId : eventIds) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%s", rhs(eventId.c_str()));
	}
	condition += ")";
	return condition;
}

//
// TriggersQueryOption
//
static const HostResourceQueryOption::Synapse synapseTriggersQueryOption(
  tableProfileTriggers,
  IDX_TRIGGERS_ID, IDX_TRIGGERS_SERVER_ID,
  tableProfileTriggers,
  IDX_TRIGGERS_HOST_ID_IN_SERVER,
  true,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID,
  IDX_TRIGGERS_GLOBAL_HOST_ID, IDX_HOSTGROUP_MEMBER_HOST_ID);

struct TriggersQueryOption::Impl {
	TriggerIdType targetId;
	TriggerSeverityType minSeverity;
	TriggerStatusType triggerStatus;
	ExcludeFlags excludeFlags;
	timespec beginTime;
	timespec endTime;
	list<string> hostnameList;
	SortType sortType;
	SortDirection sortDirection;
	string triggerBrief;

	Impl()
	: targetId(ALL_TRIGGERS),
	  minSeverity(TRIGGER_SEVERITY_UNKNOWN),
	  triggerStatus(TRIGGER_STATUS_ALL),
	  excludeFlags(NO_EXCLUDE_HOST),
	  beginTime({0, 0}),
	  endTime({0, 0}),
	  hostnameList({}),
	  sortType(SORT_ID),
	  sortDirection(SORT_DONT_CARE)
	{
	}
	bool shouldExcludeSelfMonitoring() {
		return excludeFlags & EXCLUDE_SELF_MONITORING;
	}

	bool shouldExcludeInvalidHost() {
		return excludeFlags & EXCLUDE_INVALID_HOST;
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

void TriggersQueryOption::setExcludeFlags(const ExcludeFlags &flg)
{
	m_impl->excludeFlags = flg;
}


string TriggersQueryOption::getCondition(void) const
{
	string condition = HostResourceQueryOption::getCondition();

	if (DBHatohol::isAlwaysFalseCondition(condition))
		return condition;

	if (m_impl->shouldExcludeSelfMonitoring()) {
		addCondition(
		  condition,
		  StringUtils::sprintf(
		    "%s.%s!=%d",
		    DBTablesMonitoring::TABLE_NAME_TRIGGERS,
		    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_VALIDITY].columnName,
		    TRIGGER_VALID_SELF_MONITORING));
	}

	if (m_impl->shouldExcludeInvalidHost()) {
		addCondition(
		  condition,
		  StringUtils::sprintf(
		    "%s.%s!=%d",
		    DBTablesMonitoring::TABLE_NAME_TRIGGERS,
		    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_VALIDITY].columnName,
		    TRIGGER_INVALID));
	}

	if (m_impl->targetId != ALL_TRIGGERS) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		addCondition(condition, StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
			rhs(m_impl->targetId)));
	}

	if (m_impl->minSeverity != TRIGGER_SEVERITY_UNKNOWN) {
		addCondition(condition, StringUtils::sprintf(
			"%s.%s>=%d",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName,
			m_impl->minSeverity));
	}

	if (m_impl->triggerStatus != TRIGGER_STATUS_ALL) {
		addCondition(condition, StringUtils::sprintf(
			"%s.%s=%d",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS].columnName,
			m_impl->triggerStatus));
	}

	if (m_impl->beginTime.tv_sec != 0 || m_impl->beginTime.tv_nsec != 0) {
		addCondition(condition, StringUtils::sprintf(
			"(%s>%ld OR (%s=%ld AND %s>=%ld))",
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName,
			m_impl->beginTime.tv_sec,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName,
			m_impl->beginTime.tv_sec,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_NS].columnName,
			m_impl->beginTime.tv_nsec));
	}

	if (m_impl->endTime.tv_sec != 0 || m_impl->endTime.tv_nsec != 0) {
		addCondition(condition, StringUtils::sprintf(
			"(%s<%ld OR (%s=%ld AND %s<=%ld))",
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName,
			m_impl->endTime.tv_sec,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName,
			m_impl->endTime.tv_sec,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_NS].columnName,
			m_impl->endTime.tv_nsec));
	}

	if (!m_impl->hostnameList.empty()) {
		addCondition(condition,
			     makeHostnameListCondition(m_impl->hostnameList));
	}

	if (!m_impl->triggerBrief.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		addCondition(condition, StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_TRIGGERS,
			COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_BRIEF].columnName,
			rhs(m_impl->triggerBrief)));
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

void TriggersQueryOption::setBeginTime(const timespec &beginTime)
{
	m_impl->beginTime = beginTime;
}

const timespec &TriggersQueryOption::getBeginTime(void)
{
	return m_impl->beginTime;
}

void TriggersQueryOption::setEndTime(const timespec &endTime)
{
	m_impl->endTime = endTime;
}

const timespec &TriggersQueryOption::getEndTime(void)
{
	return m_impl->endTime;
}

void TriggersQueryOption::setHostnameList(const list<string> &hostnameList)
{
	m_impl->hostnameList = hostnameList;
}

const list<string> TriggersQueryOption::getHostnameList(void)
{
	return m_impl->hostnameList;
}

void TriggersQueryOption::setSortType(
  const SortType &type, const SortDirection &direction)
{
	m_impl->sortType = type;
	m_impl->sortDirection = direction;

	switch (type) {
	case SORT_ID:
	{
		SortOrder order(
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
		  direction);
		setSortOrder(order);
		break;
	}
	case SORT_TIME:
	{
		SortOrderVect sortOrderVect;
		SortOrder order1(
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName,
		  direction);
		SortOrder order2(
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_NS].columnName,
		  direction);
		SortOrder order3(
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName,
		  direction);
		sortOrderVect.reserve(3);
		sortOrderVect.push_back(order1);
		sortOrderVect.push_back(order2);
		sortOrderVect.push_back(order3);
		setSortOrderVect(sortOrderVect);
		break;
	}
	default:
		break;
	}
}

TriggersQueryOption::SortType TriggersQueryOption::getSortType(void) const
{
	return m_impl->sortType;
}

DataQueryOption::SortDirection TriggersQueryOption::getSortDirection(void) const
{
	return m_impl->sortDirection;
}

void TriggersQueryOption::setTriggerBrief(const string &triggerBrief) {
	m_impl->triggerBrief = triggerBrief;
}

string TriggersQueryOption::getTriggerBrief(void) const {
	return m_impl->triggerBrief;
}

string TriggersQueryOption::makeHostnameListCondition(
  const list<string> &hostnameList) const
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME];
	SeparatorInjector commaInjector(",");
	DBTermCStringProvider rhs(*getDBTermCodec());
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	for (auto hostname : hostnameList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%s", rhs(hostname.c_str()));
	}

	condition += ")";
	return condition;
}

//
// ItemsQueryOption
//
static const HostResourceQueryOption::Synapse synapseItemsQueryOption(
  tableProfileItems, IDX_ITEMS_ID, IDX_ITEMS_SERVER_ID,
  tableProfileItems, IDX_ITEMS_HOST_ID_IN_SERVER,
  true,
  tableProfileHostgroupMember,
  IDX_HOSTGROUP_MEMBER_SERVER_ID, IDX_HOSTGROUP_MEMBER_HOST_ID_IN_SERVER,
  IDX_HOSTGROUP_MEMBER_GROUP_ID,
  IDX_ITEMS_GLOBAL_HOST_ID, IDX_HOSTGROUP_MEMBER_HOST_ID);

struct ItemsQueryOption::Impl {
	ItemIdType targetId;
	string itemCategoryName;
	ExcludeFlags excludeFlags;

	Impl()
	: targetId(ALL_ITEMS),
	  excludeFlags(NO_EXCLUDE_HOST)
	{
	}
	bool shouldExcludeSelfMonitoring() {
		return excludeFlags & EXCLUDE_SELF_MONITORING;
	}

	bool shouldExcludeInvalidHost() {
		return excludeFlags & EXCLUDE_INVALID_HOST;
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

	if (m_impl->shouldExcludeInvalidHost()) {
		addCondition(
		  condition,
		  StringUtils::sprintf(
		    "%s!=%d",
		    tableProfileServerHostDef.getFullColumnName(
		      IDX_HOST_SERVER_HOST_DEF_HOST_STATUS).c_str(),
		    HOST_STAT_REMOVED));
	}

	if (m_impl->targetId != ALL_ITEMS) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_ITEMS,
			COLUMN_DEF_ITEMS[IDX_ITEMS_ID].columnName,
			rhs(m_impl->targetId));
	}

	if (!m_impl->itemCategoryName.empty()) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf(
			"%s.%s=%s",
			DBTablesMonitoring::TABLE_NAME_ITEM_CATEGORIES,
			COLUMN_DEF_ITEM_CATEGORIES[IDX_ITEM_CATEGORIES_NAME].columnName,
			rhs(m_impl->itemCategoryName));
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

void ItemsQueryOption::setTargetItemCategoryName(const string &categoryName)
{
	m_impl->itemCategoryName = categoryName;
}

const string &ItemsQueryOption::getTargetItemCategoryName(void)
{
	return m_impl->itemCategoryName;
}

void ItemsQueryOption::setExcludeFlags(const ExcludeFlags &flg)
{
	m_impl->excludeFlags = flg;
}

//
// IncidentsQueryOption
//
const UnifiedEventIdType IncidentsQueryOption::ALL_INCIDENTS = -1;

struct IncidentsQueryOption::Impl {
	UnifiedEventIdType unifiedEventId;

	Impl()
	: unifiedEventId(ALL_INCIDENTS)
	{
	}
};

IncidentsQueryOption::IncidentsQueryOption(const UserIdType &userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

IncidentsQueryOption::IncidentsQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

IncidentsQueryOption::IncidentsQueryOption(const IncidentsQueryOption &src)
: DataQueryOption(src),
  m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

IncidentsQueryOption::~IncidentsQueryOption()
{
}

void IncidentsQueryOption::setTargetUnifiedEventId(const UnifiedEventIdType &id)
{
	m_impl->unifiedEventId = id;
}

const UnifiedEventIdType IncidentsQueryOption::getTargetUnifiedEventId(void)
{
	return m_impl->unifiedEventId;
}

string IncidentsQueryOption::getCondition(void) const
{
	string condition = DataQueryOption::getCondition();
	if (m_impl->unifiedEventId != ALL_INCIDENTS) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string eventIdCondition =
		  StringUtils::sprintf(
		    "%s=%s",
		    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_UNIFIED_EVENT_ID].columnName,
		    rhs(m_impl->unifiedEventId));
		addCondition(condition, eventIdCondition);
	}
	return condition;
}


// ---------------------------------------------------------------------------
// IncidentHistoriesQueryOption
// ---------------------------------------------------------------------------
const UnifiedEventIdType IncidentHistoriesQueryOption::INVALID_ID = -1;

struct IncidentHistoriesQueryOption::Impl {
	IncidentHistoryIdType id;
	UnifiedEventIdType unifiedEventId;
	UserIdType         userId;
	SortType sortType;
	SortDirection sortDirection;

	Impl()
	: id(INVALID_ID),
	  unifiedEventId(INVALID_ID),
	  userId(INVALID_USER_ID),
	  sortType(SORT_UNIFIED_EVENT_ID),
	  sortDirection(SORT_DONT_CARE)
	{
	}
};

IncidentHistoriesQueryOption::IncidentHistoriesQueryOption(
  const UserIdType &userId)
: DataQueryOption(userId),
  m_impl(new Impl())
{
}

IncidentHistoriesQueryOption::IncidentHistoriesQueryOption(
  DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext),
  m_impl(new Impl())
{
}

IncidentHistoriesQueryOption::IncidentHistoriesQueryOption(
  const IncidentHistoriesQueryOption &src)
: DataQueryOption(src),
  m_impl(new Impl())
{
	*m_impl = *src.m_impl;
}

IncidentHistoriesQueryOption::~IncidentHistoriesQueryOption()
{
}

void IncidentHistoriesQueryOption::setTargetId(const IncidentHistoryIdType &id)
{
	m_impl->id = id;
}

const IncidentHistoryIdType &IncidentHistoriesQueryOption::getTargetId(void)
{
	return m_impl->id;
}

void IncidentHistoriesQueryOption::setTargetUnifiedEventId(const UnifiedEventIdType &id)
{
	m_impl->unifiedEventId = id;
}

const UnifiedEventIdType IncidentHistoriesQueryOption::getTargetUnifiedEventId(void)
{
	return m_impl->unifiedEventId;
}

void IncidentHistoriesQueryOption::setTargetUserId(const UserIdType &userId)
{
	m_impl->userId = userId;
}

const UserIdType IncidentHistoriesQueryOption::getTargetUserId(void)
{
	return m_impl->userId;
}

void IncidentHistoriesQueryOption::setSortType(
  const IncidentHistoriesQueryOption::SortType &type,
  const SortDirection &direction)
{
	m_impl->sortType = type;
	m_impl->sortDirection = direction;

	switch (type) {
	case SORT_UNIFIED_EVENT_ID:
	{
		SortOrder order(
		  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID].columnName,
		  direction);
		setSortOrder(order);
		break;
	}
	case SORT_TIME:
	{
		SortOrderVect sortOrderVect;
		SortOrder order1(
		  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_CREATED_AT_SEC].columnName,
		  direction);
		SortOrder order2(
		  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_CREATED_AT_NS].columnName,
		  direction);
		SortOrder order3(
		  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID].columnName,
		  direction);
		sortOrderVect.reserve(3);
		sortOrderVect.push_back(order1);
		sortOrderVect.push_back(order2);
		sortOrderVect.push_back(order3);
		setSortOrderVect(sortOrderVect);
		break;
	}
	default:
		break;
	}
}

IncidentHistoriesQueryOption::SortType
IncidentHistoriesQueryOption::getSortType(void) const
{
	return m_impl->sortType;
}

DataQueryOption::SortDirection
IncidentHistoriesQueryOption::getSortDirection(void) const
{
	return m_impl->sortDirection;
}

string IncidentHistoriesQueryOption::getCondition(void) const
{
	string condition = DataQueryOption::getCondition();
	if (m_impl->id != INVALID_ID) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string idCondition =
		  StringUtils::sprintf(
		    "%s=%s",
		    COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_ID].columnName,
		    rhs(m_impl->id));
		addCondition(condition, idCondition);
	}
	if (m_impl->unifiedEventId != INVALID_ID) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string unifiedIdCondition =
		  StringUtils::sprintf(
		    "%s=%s",
		    COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID].columnName,
		    rhs(m_impl->unifiedEventId));
		addCondition(condition, unifiedIdCondition);
	}
	if (m_impl->userId != INVALID_USER_ID) {
		DBTermCStringProvider rhs(*getDBTermCodec());
		string userIdCondition =
		  StringUtils::sprintf(
		    "%s=%s",
		    COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_USER_ID].columnName,
		    rhs(m_impl->userId));
		addCondition(condition, userIdCondition);
	}
	return condition;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBTablesMonitoring::reset(void)
{
	getSetupInfo().initialized = false;
}

const DBTables::SetupInfo &DBTablesMonitoring::getConstSetupInfo(void)
{
	return getSetupInfo();
}

DBTablesMonitoring::DBTablesMonitoring(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesMonitoring::~DBTablesMonitoring()
{
}

void DBTablesMonitoring::addTriggerInfo(const TriggerInfo *triggerInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const TriggerInfo *triggerInfo;

		TrxProc(const TriggerInfo *_triggerInfo)
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

void DBTablesMonitoring::addTriggerInfoList(
  const TriggerInfoList &triggerInfoList,
  DBAgent::TransactionHooks *hooks)
{
	struct : public SeqTransactionProc<TriggerInfo, TriggerInfoList> {
		void foreach(DBAgent &dbag, const TriggerInfo &trig) override
		{
			DBTablesMonitoring &dbMon = get<DBTablesMonitoring>();
			dbMon.addTriggerInfoWithoutTransaction(dbag, trig);
		}
	} trx;
	trx.init(this, &triggerInfoList);
	getDBAgent().runTransaction(trx, hooks);
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
	DBClientJoinBuilder builder(tableProfileTriggers, &option);
	builder.add(IDX_TRIGGERS_SERVER_ID);
	builder.add(IDX_TRIGGERS_ID);
	builder.add(IDX_TRIGGERS_STATUS);
	builder.add(IDX_TRIGGERS_SEVERITY);
	builder.add(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);
	builder.add(IDX_TRIGGERS_LAST_CHANGE_TIME_NS);
	builder.add(IDX_TRIGGERS_GLOBAL_HOST_ID);
	builder.add(IDX_TRIGGERS_HOST_ID_IN_SERVER);
	builder.add(IDX_TRIGGERS_HOSTNAME);
	builder.add(IDX_TRIGGERS_BRIEF);
	builder.add(IDX_TRIGGERS_EXTENDED_INFO);
	builder.add(IDX_TRIGGERS_VALIDITY);

	builder.addTable(
	 tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	 tableProfileTriggers, IDX_TRIGGERS_SERVER_ID,
	                       IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	 tableProfileTriggers, IDX_TRIGGERS_HOST_ID_IN_SERVER,
	                       IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);

	DBAgent::SelectExArg &arg = builder.build();
	arg.useDistinct = option.isHostgroupUsed();
	arg.useFullName = option.isHostgroupUsed();

	// condition check
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
		TriggerInfo trigInfo;

		itemGroupStream >> trigInfo.serverId;
		itemGroupStream >> trigInfo.id;
		itemGroupStream >> trigInfo.status;
		itemGroupStream >> trigInfo.severity;
		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		itemGroupStream >> trigInfo.lastChangeTime.tv_nsec;
		itemGroupStream >> trigInfo.globalHostId;
		itemGroupStream >> trigInfo.hostIdInServer;
		itemGroupStream >> trigInfo.hostName;
		itemGroupStream >> trigInfo.brief;
		itemGroupStream >> trigInfo.extendedInfo;
		itemGroupStream >> trigInfo.validity;

		triggerInfoList.push_back(trigInfo);
	}
}

// TODO: remove This method is not used
void DBTablesMonitoring::setTriggerInfoList(
  const TriggerInfoList &triggerInfoList, const ServerIdType &serverId)
{
	DBAgent::DeleteArg deleteArg(tableProfileTriggers);
	struct : public SeqTransactionProc<TriggerInfo, TriggerInfoList> {
		function<bool (DBAgent &)> _preproc;
		function<void (DBAgent &)> _funcTopHalf;

		virtual bool preproc(DBAgent &dbAgent) override
		{
			return _preproc(dbAgent);
		}

		void operator ()(DBAgent &dbAgent) override
		{
			_funcTopHalf(dbAgent);
			runBaseFunctor(dbAgent);
		}

		void foreach(DBAgent &dbag, const TriggerInfo &trig) override
		{
			DBTablesMonitoring &dbMon = get<DBTablesMonitoring>();
			dbMon.addTriggerInfoWithoutTransaction(dbag, trig);
		}
	} trx;
	trx._preproc = [&] (DBAgent &dbAgent) {
		// TODO: This way is too rough and inefficient.
		//       We should update only the changed triggers.
		DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
		deleteArg.condition = StringUtils::sprintf("%s=%s",
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
		  rhs(serverId));
		return true;
	};
	trx._funcTopHalf = [&] (DBAgent &dbag) { dbag.deleteRows(deleteArg); };
	trx.init(this, &triggerInfoList);
	getDBAgent().runTransaction(trx);
}

HatoholError DBTablesMonitoring::getTriggerBriefList(
  list<string> &triggerBriefList, const TriggersQueryOption &option)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileTriggers);
	arg.add(IDX_TRIGGERS_BRIEF);

	arg.useDistinct = true;

	arg.condition = option.getCondition();
	// Order By
	arg.orderBy = option.getOrderBy();
	if (DBHatohol::isAlwaysFalseCondition(arg.condition))
		return HatoholError(HTERR_OK);

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		string brief;

		itemGroupStream >> brief;
		triggerBriefList.push_back(brief);
	}

	return HatoholError(HTERR_OK);
}

int DBTablesMonitoring::getLastChangeTimeOfTrigger(const ServerIdType &serverId)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = StringUtils::sprintf("coalesce(max(%s), 0)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.condition = StringUtils::sprintf(
	    "%s=%s AND %s!=%d",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    rhs(serverId),
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_VALIDITY].columnName,
	    TRIGGER_VALID_SELF_MONITORING);

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

void DBTablesMonitoring::updateTrigger(const TriggerInfoList &triggerInfoList,
				       const ServerIdType &serverId)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setExcludeFlags(EXCLUDE_SELF_MONITORING);
	TriggerInfoList currTrigger;
	getTriggerInfoList(currTrigger, option);
	TriggerIdInfoMap triggerMap;
	TriggerInfoListIterator currTriggerItr = currTrigger.begin();
	for (; currTriggerItr != currTrigger.end(); ++currTriggerItr){
		TriggerInfo &triggerInfo = *currTriggerItr;
		triggerMap[triggerInfo.id] = &triggerInfo;
	}


	TriggerInfoList updateTriggerList;
	TriggerInfoListConstIterator newTriggerItr = triggerInfoList.begin();
	for (; newTriggerItr != triggerInfoList.end(); ++newTriggerItr){
		const TriggerInfo &newTriggerInfo = *newTriggerItr;
		TriggerIdInfoMapIterator currTriggerItr =
		  triggerMap.find(newTriggerInfo.id);
		if (currTriggerItr != triggerMap.end()) {
			const TriggerInfo *currTrigger = currTriggerItr->second;
			const TriggerValidity validity = currTrigger->validity;
			triggerMap.erase(currTriggerItr);
			if (validity == TRIGGER_VALID)
				continue;
		}
		updateTriggerList.push_back(newTriggerInfo);
	}


	TriggerIdInfoMapIterator invalidTriggerItr = triggerMap.begin();
	for (; invalidTriggerItr != triggerMap.end(); ++invalidTriggerItr) {
		TriggerInfo *invalidTrigger = invalidTriggerItr->second;
		invalidTrigger->validity = TRIGGER_INVALID;
		updateTriggerList.push_back(*invalidTrigger);
	}

	addTriggerInfoList(updateTriggerList);
}

static string makeTriggerIdListCondition(const TriggerIdList &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	DBTermCodec codec;
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_TRIGGER_ID,
						  codec.enc(id).c_str());
	}

	condition += ")";
	return condition;
}

static string makeConditionForDeleteTrigger(const TriggerIdList &idList,
                                            const ServerIdType &serverId)
{
	string condition = makeTriggerIdListCondition(idList);
	condition += " AND ";
	string columnName =
		tableProfileTriggers.columnDefs[IDX_TRIGGERS_SERVER_ID].columnName;
	condition += StringUtils::sprintf("%s=%" FMT_SERVER_ID,
	                                  columnName.c_str(), serverId);

	return condition;
}

HatoholError DBTablesMonitoring::deleteTriggerInfo(const TriggerIdList &idList,
                                                   const ServerIdType &serverId)
{
	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileTriggers),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDeleteTrigger(idList, serverId);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

static bool isTriggerDescriptionChanged(
  TriggerInfo trigger, map<TriggerIdType, const TriggerInfo *> currentTriggerMap)
{
	auto triggerItr = currentTriggerMap.find(trigger.id);
	if (triggerItr != currentTriggerMap.end()) {
		if (triggerItr->second->status != trigger.status) {
			return true;
		}
		if (triggerItr->second->severity != trigger.severity) {
			return true;
		}
		if (triggerItr->second->lastChangeTime.tv_sec !=
		    trigger.lastChangeTime.tv_sec ||
		    triggerItr->second->lastChangeTime.tv_nsec !=
		    trigger.lastChangeTime.tv_nsec) {
			return true;
		}
		if (triggerItr->second->globalHostId != trigger.globalHostId) {
			return true;
		}
		if (triggerItr->second->hostIdInServer != trigger.hostIdInServer) {
			return true;
		}
		if (triggerItr->second->hostName != trigger.hostName) {
			return true;
		}
		if (triggerItr->second->brief != trigger.brief) {
			return true;
		}
		if (triggerItr->second->extendedInfo != trigger.extendedInfo) {
			return true;
		}
		if (triggerItr->second->validity != trigger.validity) {
			return true;
		}
	}
	return false;
}

HatoholError DBTablesMonitoring::syncTriggers(
  const TriggerInfoList &incomingTriggerInfoList,
  const ServerIdType &serverId,
  DBAgent::TransactionHooks *hooks)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	TriggerInfoList _currTriggers;
	getTriggerInfoList(_currTriggers, option);

	const TriggerInfoList currTriggers = move(_currTriggers);

	map<TriggerIdType, const TriggerInfo *> currentTriggerMap;
	for (auto& trigger : currTriggers) {
		currentTriggerMap[trigger.id] = &trigger;
	}

	// Pick up triggers to be added
	TriggerInfoList serverTriggers;
	for (auto trigger : incomingTriggerInfoList) {
		if (!isTriggerDescriptionChanged(trigger, currentTriggerMap) &&
		    currentTriggerMap.erase(trigger.id) >= 1) {
			// If the hostgroup already exists of unmodified,
			// we have nothing to do.
			continue;
		}
		serverTriggers.push_back(move(trigger));
	}

	TriggerIdList invalidTriggerIdList;
	map<TriggerIdType, const TriggerInfo *> invalidTriggerMap =
		move(currentTriggerMap);
	for (auto invalidTriggerPair : invalidTriggerMap) {
		TriggerInfo invalidTrigger = *invalidTriggerPair.second;
		invalidTriggerIdList.push_back(invalidTrigger.id);
	}
	HatoholError err = HTERR_OK;
	if (invalidTriggerIdList.size() > 0)
		err = deleteTriggerInfo(invalidTriggerIdList, serverId);
	if (serverTriggers.size() > 0)
		addTriggerInfoList(serverTriggers, hooks);
	return err;
}

void DBTablesMonitoring::addEventInfo(EventInfo *eventInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		EventInfo *eventInfo;
		uint64_t numAdded;

		TrxProc(EventInfo *_eventInfo)
		: eventInfo(_eventInfo),
		  numAdded(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addEventInfoWithoutTransaction(dbAgent, *eventInfo);
			numAdded = 1;
		}
	} trx(eventInfo);
	getDBAgent().runTransaction(trx);
	m_impl->addEventStatistics(trx.numAdded);
}

void DBTablesMonitoring::addEventInfoList(EventInfoList &eventInfoList,
                                          DBAgent::TransactionHooks *hooks)
{
	struct : public MutableSeqTransactionProc<EventInfo, EventInfoList> {
		uint64_t numAdded;
		void foreach(DBAgent &dbag, EventInfo &eventInfo) override
		{
			DBTablesMonitoring &dbMon = get<DBTablesMonitoring>();
			dbMon.addEventInfoWithoutTransaction(dbag, eventInfo);
			numAdded++;
		}
	} trx;
	trx.numAdded = 0;
	trx.init(this, &eventInfoList);
	getDBAgent().runTransaction(trx, hooks);
	m_impl->addEventStatistics(trx.numAdded);
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
	builder.add(IDX_EVENTS_GLOBAL_HOST_ID);
	builder.add(IDX_EVENTS_HOST_ID_IN_SERVER);
	builder.add(IDX_EVENTS_HOST_NAME);
	builder.add(IDX_EVENTS_BRIEF);
	builder.add(IDX_EVENTS_EXTENDED_INFO);

	if (incidentInfoVect || !option.getIncidentStatuses().empty()) {
		builder.addTable(
		  tableProfileIncidents, DBClientJoinBuilder::LEFT_JOIN,
		  tableProfileEvents, IDX_EVENTS_UNIFIED_ID, IDX_INCIDENTS_UNIFIED_EVENT_ID);
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
		builder.add(IDX_INCIDENTS_UNIFIED_EVENT_ID);
		builder.add(IDX_INCIDENTS_COMMENT_COUNT);
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
		itemGroupStream >> eventInfo.status;
		itemGroupStream >> eventInfo.severity;
		itemGroupStream >> eventInfo.globalHostId;
		itemGroupStream >> eventInfo.hostIdInServer;
		itemGroupStream >> eventInfo.hostName;
		itemGroupStream >> eventInfo.brief;

		string triggerExtendedInfo;
		itemGroupStream >> triggerExtendedInfo;
		if (!triggerExtendedInfo.empty())
			eventInfo.extendedInfo = triggerExtendedInfo;

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
			itemGroupStream >> incidentInfo.unifiedEventId;
			itemGroupStream >> incidentInfo.commentCount;
			incidentInfo.statusCode
				= IncidentInfo::STATUS_UNKNOWN; // TODO: add column?
			incidentInfo.serverId  = eventInfo.serverId;
			incidentInfo.eventId   = eventInfo.id;
			incidentInfo.triggerId = eventInfo.triggerId;
			incidentInfo.unifiedEventId = eventInfo.unifiedId;
		}
	}
	return HatoholError(HTERR_OK);
}

EventIdType DBTablesMonitoring::getMaxEventId(const ServerIdType &serverId)
{
	using StringUtils::sprintf;
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileEvents);
	string stmt = sprintf("coalesce(max(%s), '%s')",
	                      COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName,
	                      EVENT_NOT_FOUND.c_str());
	arg.add(stmt, COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
	arg.condition =
	  sprintf("%s=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    rhs(serverId));

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
			const UnifiedEventIdType unifiedId =
			  itemGroupStream.read<UnifiedEventIdType>();

			// Get the time
			DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
			argTime.condition = sprintf("%s=%" FMT_UNIFIED_EVENT_ID,
			  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName,
			  unifiedId);
			dbAgent.select(argTime);
		}
	} trx;

	// First get the event ID.
	// TODO: Get it with the one statemetn by using sub query.
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	const ColumnDef &colDefUniId = COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID];
	const string stmt = sprintf("max(%s)", colDefUniId.columnName);
	trx.argId.add(stmt, colDefUniId.type);

	trx.argId.condition = sprintf("%s=%s AND %s!=%s",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    rhs(serverId),
	    COLUMN_DEF_EVENTS[IDX_EVENTS_HOST_ID_IN_SERVER].columnName,
	    rhs(MONITORING_SELF_LOCAL_HOST_ID));
	if (triggerId != ALL_TRIGGERS) {
		trx.argId.condition += sprintf(" AND %s=%s",
		    COLUMN_DEF_EVENTS[IDX_EVENTS_TRIGGER_ID].columnName,
		    rhs(triggerId));
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

void DBTablesMonitoring::addItemInfo(const ItemInfo *itemInfo)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const ItemInfo *itemInfo;

		TrxProc(const ItemInfo *_itemInfo)
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
	struct : public SeqTransactionProc<ItemInfo, ItemInfoList> {
		void foreach(DBAgent &dbag, const ItemInfo &itemInfo) override
		{
			DBTablesMonitoring &dbMon = get<DBTablesMonitoring>();
			dbMon.addItemInfoWithoutTransaction(dbag, itemInfo);
		}
	} trx;
	trx.init(this, &itemInfoList);
	getDBAgent().runTransaction(trx);
}

static string makeItemIdListCondition(const ItemIdList &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_ITEMS[IDX_ITEMS_ID];
	SeparatorInjector commaInjector(",");
	condition = StringUtils::sprintf("%s IN (", colId.columnName);
	DBTermCodec codec;
	for (auto id : idList) {
		commaInjector(condition);
		condition += StringUtils::sprintf("%" FMT_ITEM_ID,
						  codec.enc(id).c_str());
	}

	condition += ")";
	return condition;
}

static string makeConditionForDeleteItem(const ItemIdList &idList,
                                         const ServerIdType &serverId)
{
	string condition = makeItemIdListCondition(idList);
	condition += " AND ";
	string columnName =
		tableProfileItems.columnDefs[IDX_ITEMS_SERVER_ID].columnName;
	condition += StringUtils::sprintf("%s=%" FMT_SERVER_ID,
	                                  columnName.c_str(), serverId);

	return condition;
}

HatoholError DBTablesMonitoring::deleteItemInfo(const ItemIdList &idList,
                                                const ServerIdType &serverId)
{
	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileItems),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDeleteItem(idList, serverId);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}
	return HTERR_OK;
}

static bool isItemChanged(
  const ItemInfo item, map<ItemIdType, const ItemInfo *> currentItemMap)
{
	auto itemItr = currentItemMap.find(item.id);
	if (itemItr != currentItemMap.end()) {
		if (itemItr->second->brief != item.brief) {
			return true;
		}
		if (itemItr->second->lastValue != item.lastValue) {
			return true;
		}
		if (itemItr->second->categoryNames != item.categoryNames) {
			return true;
		}
		if (itemItr->second->lastValueTime.tv_sec !=
		    item.lastValueTime.tv_sec ||
		    itemItr->second->lastValueTime.tv_nsec !=
		    item.lastValueTime.tv_nsec) {
			return true;
		}
		if (itemItr->second->valueType != item.valueType) {
			return true;
		}
		if (itemItr->second->unit != item.unit) {
			return true;
		}
	}
	return false;
}

static LocalHostIdType getTargetHostId(const ItemInfoList itemInfoList)
{
	auto firstItem = *begin(itemInfoList);
	LocalHostIdType targetHostId = firstItem.hostIdInServer;
	for (const auto item : itemInfoList) {
		if (item.hostIdInServer != targetHostId)
			return ALL_LOCAL_HOSTS;
	}
	return targetHostId;
}

HatoholError DBTablesMonitoring::syncItems(const ItemInfoList &itemInfoList,
                                           const ServerIdType &serverId)
{
	HatoholError err = HTERR_OK;
	if (itemInfoList.size() == 0) {
		MLPL_WARN("The number of items: 0\n");
		return err;
	}

	ItemsQueryOption option(USER_ID_SYSTEM);
	ItemInfoList _currItems;

	option.setTargetServerId(serverId);
	LocalHostIdType targetHostId = getTargetHostId(itemInfoList);
	option.setTargetHostId(targetHostId);
	getItemInfoList(_currItems, option);
	const ItemInfoList currItems = move(_currItems);

	map<ItemIdType, const ItemInfo *> currentItemMap;
	for (const auto& item : currItems) {
		currentItemMap[item.id] = &item;
	}

	// Pick up items to be added
	ItemInfoList addItems;
	for (const auto item : itemInfoList) {
		if (!isItemChanged(item, currentItemMap) &&
		    currentItemMap.erase(item.id) >= 1) {
			continue;
		}
		addItems.push_back(move(item));
	}

	ItemIdList invalidItemIdList;
	map<ItemIdType, const ItemInfo *> invalidItemMap =
		move(currentItemMap);
	for (auto invalidItemPair : invalidItemMap) {
		ItemInfo invalidItem = *invalidItemPair.second;
		invalidItemIdList.push_back(invalidItem.id);
	}
	if (invalidItemIdList.size() > 0)
		err = deleteItemInfo(invalidItemIdList, serverId);
	if (addItems.size() > 0)
		addItemInfoList(addItems);
	return err;
}

void DBTablesMonitoring::getItemInfoList(ItemInfoList &itemInfoList,
				      const ItemsQueryOption &option)
{
	vector<GenericIdType> itemGlobalIds;
	auto getGlobalItemIds = [&] {
		DBClientJoinBuilder builder(tableProfileItems, &option);
		builder.add(IDX_ITEMS_GLOBAL_ID);
		builder.addTable(
		  tableProfileItemCategories, DBClientJoinBuilder::LEFT_JOIN,
		  IDX_ITEMS_GLOBAL_ID, IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID);
		builder.addTable(
		  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
		  tableProfileItems, IDX_ITEMS_SERVER_ID,
		                     IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
		  tableProfileItems, IDX_ITEMS_HOST_ID_IN_SERVER,
		                     IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);

		DBAgent::SelectExArg &arg = builder.build();
		arg.useDistinct = true;
		arg.useFullName = option.isHostgroupUsed();

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
		const auto &grpList = arg.dataTable->getItemGroupList();
		itemGlobalIds.reserve(grpList.size());
		for (const auto &grp: grpList) {
			const GenericIdType globalId = *(grp->getItemAt(0));
			itemGlobalIds.push_back(globalId);
		}
	};
	getGlobalItemIds();
	if (itemGlobalIds.empty())
		return;

	DBClientJoinBuilder builder(tableProfileItems, &option);
	builder.add(IDX_ITEMS_GLOBAL_ID);
	builder.add(IDX_ITEMS_SERVER_ID);
	builder.add(IDX_ITEMS_ID);
	builder.add(IDX_ITEMS_GLOBAL_HOST_ID);
	builder.add(IDX_ITEMS_HOST_ID_IN_SERVER);
	builder.add(IDX_ITEMS_BRIEF);
	builder.add(IDX_ITEMS_LAST_VALUE_TIME_SEC);
	builder.add(IDX_ITEMS_LAST_VALUE_TIME_NS);
	builder.add(IDX_ITEMS_LAST_VALUE);
	builder.add(IDX_ITEMS_PREV_VALUE);
	builder.add(IDX_ITEMS_VALUE_TYPE);
	builder.add(IDX_ITEMS_UNIT);
	builder.addTable(
	  tableProfileItemCategories, DBClientJoinBuilder::LEFT_JOIN,
	  IDX_ITEMS_GLOBAL_ID, IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID);
	builder.add(IDX_ITEM_CATEGORIES_NAME);

	DBAgent::SelectExArg &arg = builder.build();

	arg.condition =
	  tableProfileItems.getFullColumnName(IDX_ITEMS_GLOBAL_ID) + " IN (";
	SeparatorInjector commaInjector(",");
	for (const auto &globalId: itemGlobalIds) {
		commaInjector(arg.condition);
		arg.condition += StringUtils::sprintf("%" FMT_GEN_ID, globalId);
	}
	arg.condition += ")";

	getDBAgent().runTransaction(arg);

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	map<GenericIdType, ItemInfo *> globalItemIdMap;
	map<GenericIdType, ItemInfo *>::iterator itr;
	
	auto setCategoryName = [](ItemInfo &itemInfo, const string &name) {
		if (name.empty())
			return;
		itemInfo.categoryNames.push_back(name);
	};

	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		GenericIdType globalId;
		string category;
		ItemGroupStream itemGroupStream(*itemGrpItr);
		itemGroupStream >> globalId;
		itr = globalItemIdMap.find(globalId);
		if (itr != globalItemIdMap.end()) {
			ItemInfo &itemInfo = *itr->second;
			const ItemData *categoryData =
			  (*itemGrpItr)->getItemAt(NUM_IDX_ITEMS);
			setCategoryName(itemInfo, *categoryData);
			continue;
		}

		itemInfoList.push_back(ItemInfo());
		ItemInfo &itemInfo = itemInfoList.back();
		globalItemIdMap.insert(
		  pair<GenericIdType, ItemInfo *>(globalId, &itemInfo));
		itemInfo.globalId = globalId;
		itemGroupStream >> itemInfo.serverId;
		itemGroupStream >> itemInfo.id;
		itemGroupStream >> itemInfo.globalHostId;
		itemGroupStream >> itemInfo.hostIdInServer;
		itemGroupStream >> itemInfo.brief;
		itemGroupStream >> itemInfo.lastValueTime.tv_sec;
		itemGroupStream >> itemInfo.lastValueTime.tv_nsec;
		itemGroupStream >> itemInfo.lastValue;
		itemGroupStream >> itemInfo.prevValue;
		int valueType;
		itemGroupStream >> valueType;
		itemInfo.valueType = static_cast<ItemInfoValueType>(valueType);
		itemGroupStream >> itemInfo.unit;

		itemGroupStream >> category;
		setCategoryName(itemInfo, category);
	}
}

void DBTablesMonitoring::getItemCategoryNames(
  vector<string> &itemCategoryNames, const ItemsQueryOption &option)
{
	DBClientJoinBuilder builder(tableProfileItems, &option);
	builder.addTable(
	  tableProfileItemCategories, DBClientJoinBuilder::LEFT_JOIN,
	  IDX_ITEMS_GLOBAL_ID, IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID);
	builder.add(IDX_ITEM_CATEGORIES_NAME);
	builder.addTable(
	  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileItems, IDX_ITEMS_SERVER_ID,
	                     IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  tableProfileItems, IDX_ITEMS_HOST_ID_IN_SERVER,
	                     IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);

	DBAgent::SelectExArg &arg = builder.build();

	arg.useDistinct = true;
	arg.useFullName = option.isHostgroupUsed();

	// condition
	if (DBHatohol::isAlwaysFalseCondition(arg.condition))
		return;

	// Order by
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();
	if (!arg.limit && arg.offset)
		return;

	getDBAgent().runTransaction(arg);
	for (const auto &grp: arg.dataTable->getItemGroupList())
		itemCategoryNames.push_back(*grp->getItemAt(0));
}

void DBTablesMonitoring::addMonitoringServerStatus(
  const MonitoringServerStatus &serverStatus)
{
	struct TrxProc : public DBAgent::TransactionProc {
		const MonitoringServerStatus &serverStatus;

		TrxProc(const MonitoringServerStatus &_serverStatus)
		: serverStatus(_serverStatus)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			addMonitoringServerStatusWithoutTransaction(
			  dbAgent, serverStatus);
		}
	} trx(serverStatus);
	getDBAgent().runTransaction(trx);
}

size_t DBTablesMonitoring::getNumberOfTriggers(
  const TriggersQueryOption &option, const std::string &additionalCondition)
{
	DBClientJoinBuilder builder(tableProfileTriggers, &option);
	builder.addTable(
	  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileTriggers, IDX_TRIGGERS_SERVER_ID,
	                        IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  tableProfileTriggers, IDX_TRIGGERS_HOST_ID_IN_SERVER,
	                        IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
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
	DBAgent::SelectExArg  &arg = builder.build();

	arg.add(stmt, SQL_COLUMN_TYPE_INT);

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
	const string statusProblemCond = StringUtils::sprintf("%s=%d",
	  tableProfileTriggers.getFullColumnName(IDX_TRIGGERS_STATUS).c_str(),
	  TRIGGER_STATUS_PROBLEM);

	if (severity == TRIGGER_SEVERITY_ALL) {
		additionalCondition = statusProblemCond;
	} else {
		additionalCondition
		  = StringUtils::sprintf(
		      "%s=%d and %s",
		      tableProfileTriggers.getFullColumnName(
		        IDX_TRIGGERS_SEVERITY).c_str(),
		      severity,
		      statusProblemCond.c_str());
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
	DBClientJoinBuilder builder(tableProfileTriggers, &option);
	builder.addTable(
	  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileTriggers, IDX_TRIGGERS_SERVER_ID,
	                        IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  tableProfileTriggers, IDX_TRIGGERS_HOST_ID_IN_SERVER,
	                        IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_TRIGGERS_HOST_ID_IN_SERVER).c_str());
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

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
	DBClientJoinBuilder builder(tableProfileTriggers, &option);
	builder.addTable(
	  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileTriggers, IDX_TRIGGERS_SERVER_ID,
	                        IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  tableProfileTriggers, IDX_TRIGGERS_HOST_ID_IN_SERVER,
	                        IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_TRIGGERS_HOST_ID_IN_SERVER).c_str());
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

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
	DBClientJoinBuilder builder(tableProfileItems, &option);
	builder.addTable(
	  tableProfileItemCategories, DBClientJoinBuilder::LEFT_JOIN,
	  IDX_ITEMS_GLOBAL_ID, IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID);
	builder.addTable(
	  tableProfileServerHostDef, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileItems, IDX_ITEMS_SERVER_ID,
	                     IDX_HOST_SERVER_HOST_DEF_SERVER_ID,
	  tableProfileItems, IDX_ITEMS_HOST_ID_IN_SERVER,
	                     IDX_HOST_SERVER_HOST_DEF_HOST_ID_IN_SERVER);

	DBAgent::SelectExArg &arg = builder.build();

	const string stmt = StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_ITEMS_GLOBAL_ID).c_str());
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

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

size_t DBTablesMonitoring::getNumberOfEvents(const EventsQueryOption &option)
{
	DBClientJoinBuilder builder(tableProfileEvents, &option);
	builder.addTable(
	  tableProfileIncidents, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileEvents, IDX_EVENTS_UNIFIED_ID, IDX_INCIDENTS_UNIFIED_EVENT_ID);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_EVENTS_UNIFIED_ID).c_str());
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBTablesMonitoring::getNumberOfHostsWithSpecifiedEvents(
  const EventsQueryOption &option)
{
	DBClientJoinBuilder builder(tableProfileEvents, &option);
	builder.addTable(
	  tableProfileIncidents, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileEvents, IDX_EVENTS_UNIFIED_ID, IDX_INCIDENTS_UNIFIED_EVENT_ID);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    option.getColumnName(IDX_EVENTS_GLOBAL_HOST_ID).c_str());
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

HatoholError DBTablesMonitoring::getEventSeverityStatistics(
  std::vector<DBTablesMonitoring::EventSeverityStatistics> &severityStatisticsVect,
  const EventsQueryOption &option)
{
	DBClientJoinBuilder builder(tableProfileEvents, &option);
	builder.add(IDX_EVENTS_SEVERITY);
	builder.addTable(
	  tableProfileIncidents, DBClientJoinBuilder::LEFT_JOIN,
	  tableProfileEvents, IDX_EVENTS_UNIFIED_ID, IDX_INCIDENTS_UNIFIED_EVENT_ID);
	string severityColumnName =
	  option.getColumnName(IDX_EVENTS_SEVERITY).c_str();
	string stmt =
	  StringUtils::sprintf("count(%s)",
	    severityColumnName.c_str());
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// generate new non const EventsQueryOption to set target GroupBy columns
	EventsQueryOption groupByOption(option);
	groupByOption.setGroupByColumns({severityColumnName});
	arg.groupBy = groupByOption.getGroupBy();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		DBTablesMonitoring::EventSeverityStatistics statistics;

		uint64_t num;
		itemGroupStream >> statistics.severity;
		itemGroupStream >> num;
		statistics.num = num;

		severityStatisticsVect.push_back(statistics);
	}
	return HatoholError(HTERR_OK);
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

HatoholError DBTablesMonitoring::updateIncidentInfo(IncidentInfo &incidentInfo)
{

	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		DBAgent::UpdateArg arg;

		TrxProc(void)
		: arg(tableProfileIncidents)
		{
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(
				 TABLE_NAME_INCIDENTS,
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

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::UpdateArg &arg = trx.arg;
	arg.add(IDX_INCIDENTS_STATUS, incidentInfo.status);
	arg.add(IDX_INCIDENTS_ASSIGNEE, incidentInfo.assignee);
	arg.add(IDX_INCIDENTS_CREATED_AT_SEC, incidentInfo.createdAt.tv_sec);
	arg.add(IDX_INCIDENTS_CREATED_AT_NS, incidentInfo.createdAt.tv_nsec);
	arg.add(IDX_INCIDENTS_UPDATED_AT_SEC, incidentInfo.updatedAt.tv_sec);
	arg.add(IDX_INCIDENTS_UPDATED_AT_NS, incidentInfo.updatedAt.tv_nsec);
	arg.add(IDX_INCIDENTS_PRIORITY, incidentInfo.priority);
	arg.add(IDX_INCIDENTS_DONE_RATIO, incidentInfo.doneRatio);
	arg.add(IDX_INCIDENTS_COMMENT_COUNT, incidentInfo.commentCount);
	arg.condition = StringUtils::sprintf(
	  "%s=%s AND %s=%s",
	  COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].columnName,
	  rhs(incidentInfo.trackerId),
	  COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_IDENTIFIER].columnName,
	  rhs(incidentInfo.identifier));
	getDBAgent().runTransaction(trx);
	return trx.err;
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
		itemGroupStream >> incidentInfo.unifiedEventId;
		itemGroupStream >> incidentInfo.commentCount;
		incidentInfo.statusCode = IncidentInfo::STATUS_UNKNOWN; // TODO: add column?
	}

	return HatoholError(HTERR_OK);
}

uint64_t DBTablesMonitoring::getLastUpdateTimeOfIncidents(
  const IncidentTrackerIdType &trackerId)
{
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::SelectExArg arg(tableProfileIncidents);
	string stmt = StringUtils::sprintf("coalesce(max(%s), 0)",
	    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_UPDATED_AT_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].type);
	if (trackerId != ALL_INCIDENT_TRACKERS) {
		arg.condition = StringUtils::sprintf("%s=%s",
		    COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_TRACKER_ID].columnName,
		    rhs(trackerId));
	}

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<uint64_t>();
}

HatoholError DBTablesMonitoring::getSystemInfo(
  DBTablesMonitoring::SystemInfo &systemInfo, const DataQueryOption &option)
{
	if (!option.has(OPPRVLG_GET_SYSTEM_INFO))
		return HatoholError(HTERR_INVALID_USER);

	StatisticsCounter::Slot *prevSlot = NULL;
	StatisticsCounter::Slot *currSlot = NULL;
	for (size_t i = 0; i < NUM_EVENTS_COUNTERS; i++) {
		prevSlot = &systemInfo.eventsCounterPrevSlots[i];
		currSlot = &systemInfo.eventsCounterCurrSlots[i];
		eventsCounters[i]->get(prevSlot, currSlot);
	}
	return HatoholError(HTERR_OK);
}

void IncidentHistory::initialize(IncidentHistory &incidentHistory)
{
	incidentHistory.id = AUTO_INCREMENT_VALUE;
	incidentHistory.unifiedEventId = INVALID_EVENT_ID;
	incidentHistory.userId = INVALID_USER_ID;
	incidentHistory.status = "";
	incidentHistory.comment = "";
	timespec currTimespec = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	incidentHistory.createdAt.tv_sec = currTimespec.tv_sec;
	incidentHistory.createdAt.tv_nsec = currTimespec.tv_nsec;
}

HatoholError DBTablesMonitoring::addIncidentHistory(
  IncidentHistory &incidentHistory)
{
	IncidentHistoryIdType incidentHistoryId;

	DBAgent::InsertArg arg(tableProfileIncidentHistories);
	arg.add(incidentHistory.id);
	arg.add(incidentHistory.unifiedEventId);
	arg.add(incidentHistory.userId);
	arg.add(incidentHistory.status);
	arg.add(incidentHistory.comment);
	arg.add(incidentHistory.createdAt.tv_sec);
	arg.add(incidentHistory.createdAt.tv_nsec);

	DBAgent &dbAgent = getDBAgent();
	dbAgent.runTransaction(arg, incidentHistoryId);

	updateIncidentCommentCount(incidentHistory.unifiedEventId);

	return HatoholError(HTERR_OK);
}

HatoholError DBTablesMonitoring::updateIncidentHistory(
  IncidentHistory &incidentHistory)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		DBAgent::UpdateArg arg;

		TrxProc(void)
		: arg(tableProfileIncidentHistories)
		{
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(
				 TABLE_NAME_INCIDENT_HISTORIES,
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

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::UpdateArg &arg = trx.arg;
	arg.add(IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID,
		incidentHistory.unifiedEventId);
	arg.add(IDX_INCIDENT_HISTORIES_USER_ID,
		incidentHistory.userId);
	arg.add(IDX_INCIDENT_HISTORIES_STATUS,
		incidentHistory.status);
	arg.add(IDX_INCIDENT_HISTORIES_COMMENT,
		incidentHistory.comment);
	arg.add(IDX_INCIDENT_HISTORIES_CREATED_AT_SEC,
		incidentHistory.createdAt.tv_sec);
	arg.add(IDX_INCIDENT_HISTORIES_CREATED_AT_NS,
		incidentHistory.createdAt.tv_nsec);
	arg.condition = StringUtils::sprintf(
	  "%s=%s",
	  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_ID].columnName,
	  rhs(incidentHistory.id));
	getDBAgent().runTransaction(trx);

	updateIncidentCommentCount(incidentHistory.unifiedEventId);

	return HatoholError(HTERR_OK);
}

HatoholError DBTablesMonitoring::getIncidentHistory(
  list<IncidentHistory> &incidentHistoriesList,
  const IncidentHistoriesQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileIncidentHistories);
	arg.add(IDX_INCIDENT_HISTORIES_ID);
	arg.add(IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID);
	arg.add(IDX_INCIDENT_HISTORIES_USER_ID);
	arg.add(IDX_INCIDENT_HISTORIES_STATUS);
	arg.add(IDX_INCIDENT_HISTORIES_COMMENT);
	arg.add(IDX_INCIDENT_HISTORIES_CREATED_AT_SEC);
	arg.add(IDX_INCIDENT_HISTORIES_CREATED_AT_NS);

	arg.condition = option.getCondition();

	// Order By
	arg.orderBy = option.getOrderBy();

	getDBAgent().runTransaction(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	for (auto itemGrp : grpList) {
		ItemGroupStream itemGroupStream(itemGrp);

		IncidentHistory incidentHistory;
		itemGroupStream >> incidentHistory.id;
		itemGroupStream >> incidentHistory.unifiedEventId;
		itemGroupStream >> incidentHistory.userId;
		itemGroupStream >> incidentHistory.status;
		itemGroupStream >> incidentHistory.comment;
		itemGroupStream >> incidentHistory.createdAt.tv_sec;
		itemGroupStream >> incidentHistory.createdAt.tv_nsec;

		incidentHistoriesList.push_back(incidentHistory);
	}

	return HTERR_OK;
}

size_t DBTablesMonitoring::getIncidentCommentCount(
  UnifiedEventIdType unifiedEventId)
{
	const ColumnDef &unifiedEventIdColumnDef =
	  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_UNIFIED_EVENT_ID];
	const ColumnDef &commentColumnDef =
	  COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_COMMENT];
	DBClientJoinBuilder builder(tableProfileIncidentHistories);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    COLUMN_DEF_INCIDENT_HISTORIES[IDX_INCIDENT_HISTORIES_ID].columnName);
	DBAgent::SelectExArg &arg = builder.build();
	arg.add(stmt, SQL_COLUMN_TYPE_INT);
	arg.condition
	  = StringUtils::sprintf("%s=%" FMT_UNIFIED_EVENT_ID " and "
				 "%s is not null and %s<>''",
				 unifiedEventIdColumnDef.columnName,
				 unifiedEventId,
				 commentColumnDef.columnName,
				 commentColumnDef.columnName);

	getDBAgent().runTransaction(arg);

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

HatoholError DBTablesMonitoring::updateIncidentCommentCount(
  UnifiedEventIdType unifiedEventId)
{
	struct TrxProc : public DBAgent::TransactionProc {
		HatoholError err;
		DBAgent::UpdateArg arg;

		TrxProc(void)
		: arg(tableProfileIncidents)
		{
		}

		bool hasRecord(DBAgent &dbAgent)
		{
			return dbAgent.isRecordExisting(
				 TABLE_NAME_INCIDENTS,
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

	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	DBAgent::UpdateArg &arg = trx.arg;
	size_t commentCount = getIncidentCommentCount(unifiedEventId);
	arg.add(IDX_INCIDENTS_COMMENT_COUNT, commentCount);
	arg.condition = StringUtils::sprintf(
	  "%s=%s",
	  COLUMN_DEF_INCIDENTS[IDX_INCIDENTS_UNIFIED_EVENT_ID].columnName,
	  rhs(unifiedEventId));
	getDBAgent().runTransaction(trx);
	return trx.err;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data);

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
		&tableProfileItemCategories,
	}, {
		&tableProfileServerStatus,
	}, {
		&tableProfileIncidents,
	}, {
		&tableProfileIncidentHistories,
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
	arg.add(triggerInfo.globalHostId);
	arg.add(triggerInfo.hostIdInServer);
	arg.add(triggerInfo.hostName);
	arg.add(triggerInfo.brief);
	arg.add(triggerInfo.extendedInfo);
	arg.add(triggerInfo.validity);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

void DBTablesMonitoring::addEventInfoWithoutTransaction(
  DBAgent &dbAgent, EventInfo &eventInfo)
{
	mergeTriggerInfo(dbAgent, eventInfo);

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
	arg.add(eventInfo.globalHostId);
	arg.add(eventInfo.hostIdInServer);
	arg.add(eventInfo.hostName);
	arg.add(eventInfo.brief);
	arg.add(eventInfo.extendedInfo);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
	eventInfo.unifiedId = dbAgent.getLastInsertId();
}

void DBTablesMonitoring::addItemCategoryWithoutTransaction(
  DBAgent &dbAgent, const ItemCategory &category)
{
	DBAgent::InsertArg arg(tableProfileItemCategories);
	arg.upsertOnDuplicate = true;
	arg.add(category.id);
	arg.add(category.globalItemId);
	arg.add(category.name);
	dbAgent.insert(arg);
}

static void getItemCategoriesFromId(
  DBAgent &dbAgent,
  ItemCategoryVect &categories, const GenericIdType &globalItemId)
{
	DBAgent::SelectExArg arg(tableProfileItemCategories);
	arg.add(IDX_ITEM_CATEGORIES_ID);
	arg.add(IDX_ITEM_CATEGORIES_NAME);
	arg.condition = StringUtils::sprintf("%s=%" FMT_GEN_ID,
	  COLUMN_DEF_ITEM_CATEGORIES[IDX_ITEM_CATEGORIES_GLOBAL_ITEM_ID].columnName,
	  globalItemId);
	dbAgent.select(arg);

	for (const auto &itemGrp: arg.dataTable->getItemGroupList()) {
		ItemGroupStream itemGroupStream(itemGrp);
		categories.push_back(ItemCategory());
		ItemCategory &itemCategory = categories.back();
		itemCategory.globalItemId = globalItemId;
		itemGroupStream >> itemCategory.id;
		itemGroupStream >> itemCategory.name;
	}
}

static void deleteItemCateoryWithoutTransaction(
  DBAgent &dbAgent, const GenericIdType &id)
{
	DBAgent::DeleteArg arg(tableProfileItemCategories);
	arg.condition = StringUtils::sprintf("%s=%" FMT_GEN_ID,
	  COLUMN_DEF_ITEM_CATEGORIES[IDX_ITEM_CATEGORIES_ID].columnName,
	  id);
	dbAgent.deleteRows(arg);
}

static void calcDelta(
  const GenericIdType &globalItemId,
  const ItemCategoryVect &currCategories,
  const vector<string> latestCategoryNames,
  ItemCategoryVect &addCategories, vector<GenericIdType> &delCategories)
{
	ItemCategory category;
	category.id = AUTO_INCREMENT_VALUE_U64;
	category.globalItemId = globalItemId;

	map<string, const ItemCategory *> categoriesMap;
	for (const auto &cat: currCategories) {
		categoriesMap.insert(
		  pair<string, const ItemCategory *>(cat.name, &cat));
	}
	for (const auto &name: latestCategoryNames) {
		auto itr = categoriesMap.find(name);
		if (itr == categoriesMap.end()) { // Newly added one
			category.name = name;
			addCategories.push_back(category);
		} else {
			categoriesMap.erase(itr);
		}
	}
	for (const auto &pair: categoriesMap) {
		const auto  &delCategory = *pair.second;
		delCategories.push_back(delCategory.id);
	}
}

void DBTablesMonitoring::addItemInfoWithoutTransaction(
  DBAgent &dbAgent, const ItemInfo &itemInfo)
{
	auto getGlobalItemId = [&] (
	  const ServerIdType &serverId, const ItemIdType &itemId)
	{
		DBTermCStringProvider rhs(*dbAgent.getDBTermCodec());
		DBAgent::SelectExArg arg(tableProfileItems);
		arg.add(IDX_ITEMS_GLOBAL_ID);
		arg.condition = StringUtils::sprintf(
		  "%s=%s AND %s=%s",
		  COLUMN_DEF_ITEMS[IDX_ITEMS_SERVER_ID].columnName,
		  rhs(serverId),
		  COLUMN_DEF_ITEMS[IDX_ITEMS_ID].columnName,
		  rhs(itemId));
		dbAgent.select(arg);
		const ItemGroupList &grpList =
		  arg.dataTable->getItemGroupList();
		HATOHOL_ASSERT(grpList.size() == 1,
		               "Unexpected size: %zd", grpList.size());
		const auto &row = *grpList.begin();
		const GenericIdType &globalItemId = *row->getItemAt(0);
		return globalItemId;
	};

	DBAgent::InsertArg arg(tableProfileItems);
	arg.add(AUTO_INCREMENT_VALUE_U64);
	arg.add(itemInfo.serverId);
	arg.add(itemInfo.id);
	arg.add(itemInfo.globalHostId);
	arg.add(itemInfo.hostIdInServer);
	arg.add(itemInfo.brief);
	arg.add(itemInfo.lastValueTime.tv_sec);
	arg.add(itemInfo.lastValueTime.tv_nsec);
	arg.add(itemInfo.lastValue);
	arg.add(itemInfo.prevValue);
	arg.add(itemInfo.valueType);
	arg.add(itemInfo.unit);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);

	ItemCategoryVect      newItemCategories;
	vector<GenericIdType> delCatetegoryIds;
	if (dbAgent.lastUpsertDidInsert()) {
		ItemCategory itemCategory;
		itemCategory.id = AUTO_INCREMENT_VALUE_U64;
		itemCategory.globalItemId = dbAgent.getLastInsertId();
		for (const auto &name: itemInfo.categoryNames) {
			itemCategory.name = name;
			newItemCategories.push_back(itemCategory);
		}
	} else {
		ItemCategoryVect currCategories;
		const GenericIdType globalItemId =
		  getGlobalItemId(itemInfo.serverId, itemInfo.id);
		getItemCategoriesFromId(dbAgent, currCategories, globalItemId);
		calcDelta(globalItemId,
		          currCategories, itemInfo.categoryNames,
		          newItemCategories, delCatetegoryIds);
	}

	for (const auto &id: delCatetegoryIds)
		deleteItemCateoryWithoutTransaction(dbAgent, id);

	for (const auto &category: newItemCategories)
		addItemCategoryWithoutTransaction(dbAgent, category);
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
	arg.add(incidentInfo.unifiedEventId);
	arg.add(incidentInfo.commentCount);
	arg.upsertOnDuplicate = true;
	dbAgent.insert(arg);
}

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	const int &oldVer = oldPackedVer.getPackedVer();
	if (oldVer == DBTables::Version::getPackedVer(0, 2, 0)) {
		// add a new column "comment_count" to incidents
		// since (0, 2, 1)
		DBAgent::AddColumnsArg addColumnsArg(tableProfileIncidents);
		addColumnsArg.columnIndexes.push_back(
		  IDX_INCIDENTS_COMMENT_COUNT);
		dbAgent.addColumns(addColumnsArg);
	}
	if (oldVer < DBTables::Version::getPackedVer(0, 2, 2)) {
		dbAgent.changeColumnDef(
		  tableProfileIncidentHistories,
		  "comment",
		  IDX_INCIDENT_HISTORIES_COMMENT);
	}
	return true;
}

template <typename T>
static void substIfNeeded(T &lhs, const T &rhs, const T &initialValue)
{
	if (lhs == initialValue)
		lhs = rhs;
}

bool DBTablesMonitoring::mergeTriggerInfo(
  DBAgent &dbAgent, EventInfo &eventInfo)
{
	struct {
		void operator()(string &lhs, const string &rhs)
		{
			substIfNeeded<string>(lhs, rhs, "");
		}

		void operator()(HostIdType &lhs, const HostIdType &rhs)
		{
			substIfNeeded<HostIdType>(lhs, rhs, INVALID_HOST_ID);
		}

		void operator()(
		  TriggerSeverityType &lhs, const TriggerSeverityType &rhs)
		{
			substIfNeeded<TriggerSeverityType>(
			  lhs, rhs, TRIGGER_SEVERITY_UNKNOWN);
		}
	} setIfNeeded;

	// Get the corresponding trigger
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(eventInfo.serverId);
	option.setTargetId(eventInfo.triggerId);
	DBAgent::SelectExArg arg(tableProfileTriggers);
	arg.add(IDX_TRIGGERS_SEVERITY);
	arg.add(IDX_TRIGGERS_GLOBAL_HOST_ID);
	arg.add(IDX_TRIGGERS_HOST_ID_IN_SERVER);
	arg.add(IDX_TRIGGERS_HOSTNAME);
	arg.add(IDX_TRIGGERS_BRIEF);
	arg.add(IDX_TRIGGERS_EXTENDED_INFO);
	arg.condition = option.getCondition();
	dbAgent.select(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return false;
	ItemGroupStream itemGroupStream(*grpList.begin());
	TriggerInfo trigInfo;
	itemGroupStream >> trigInfo.severity;
	itemGroupStream >> trigInfo.globalHostId;
	itemGroupStream >> trigInfo.hostIdInServer;
	itemGroupStream >> trigInfo.hostName;
	itemGroupStream >> trigInfo.brief;
	itemGroupStream >> trigInfo.extendedInfo;

	setIfNeeded(eventInfo.severity,       trigInfo.severity);
	setIfNeeded(eventInfo.globalHostId,   trigInfo.globalHostId);
	setIfNeeded(eventInfo.hostIdInServer, trigInfo.hostIdInServer);
	setIfNeeded(eventInfo.hostName,       trigInfo.hostName);
	setIfNeeded(eventInfo.brief,          trigInfo.brief);
	setIfNeeded(eventInfo.extendedInfo,   trigInfo.extendedInfo);
	return true;
}
