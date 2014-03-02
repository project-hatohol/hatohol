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

static const char *TABLE_NAME_TRIGGERS             = "triggers";
static const char *TABLE_NAME_EVENTS               = "events";
static const char *TABLE_NAME_ITEMS                = "items";
static const char *TABLE_NAME_HOSTS                = "hosts";
static const char *TABLE_NAME_HOSTGROUPS           = "hostgroups";
static const char *TABLE_NAME_MAP_HOSTS_HOSTGROUPS = "map_hosts_hostgroups";

uint64_t DBClientHatohol::EVENT_NOT_FOUND = -1;
int DBClientHatohol::HATOHOL_DB_VERSION = 4;

const char *DBClientHatohol::DEFAULT_DB_NAME = "hatohol";

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
	TABLE_NAME_TRIGGERS,               // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
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
	TABLE_NAME_TRIGGERS,               // tableName
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
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
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
	TABLE_NAME_TRIGGERS,               // tableName
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
  TABLE_NAME_TRIGGERS, COLUMN_DEF_TRIGGERS,
  sizeof(COLUMN_DEF_TRIGGERS), NUM_IDX_TRIGGERS);

static const ColumnDef COLUMN_DEF_EVENTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
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
	TABLE_NAME_EVENTS,                 // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"time_sec",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
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
	TABLE_NAME_EVENTS,                 // tableName
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
	TABLE_NAME_EVENTS,                 // tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
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
	TABLE_NAME_TRIGGERS,               // tableName
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
  TABLE_NAME_EVENTS, COLUMN_DEF_EVENTS,
  sizeof(COLUMN_DEF_EVENTS), NUM_IDX_EVENTS);

static const ColumnDef COLUMN_DEF_ITEMS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS,                  // tableName
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
	TABLE_NAME_ITEMS,                  // tableName
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
	TABLE_NAME_ITEMS,                  // tableName
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
	TABLE_NAME_ITEMS,                  // tableName
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
	TABLE_NAME_ITEMS,                  // tableName
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
	TABLE_NAME_ITEMS,                  // tableName
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
  TABLE_NAME_ITEMS, COLUMN_DEF_ITEMS,
  sizeof(COLUMN_DEF_ITEMS), NUM_IDX_ITEMS);

static const ColumnDef COLUMN_DEF_HOSTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTS,                  // tableName
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
	TABLE_NAME_HOSTS,                  // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTS,                  // tableName
	"host_id",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTS,                  // tableName
	"host_name",                        // columnName
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
	IDX_HOSTS_SERVER_ID,
	IDX_HOSTS_ID,
	IDX_HOSTS_HOST_ID,
	IDX_HOSTS_HOST_NAME,
	NUM_IDX_HOSTS,
};

static const DBAgent::TableProfile tableProfileHosts(
  TABLE_NAME_HOSTS, COLUMN_DEF_HOSTS,
  sizeof(COLUMN_DEF_HOSTS), NUM_IDX_HOSTS);

static const ColumnDef COLUMN_DEF_HOSTGROUPS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTGROUPS,             // tableName
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
	TABLE_NAME_HOSTGROUPS,             // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTGROUPS,             // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTGROUPS,             // tableName
	"group_name",                      // columnName
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
	IDX_HOSTGROUPS_ID,
	IDX_HOSTGROUPS_SERVER_ID,
	IDX_HOSTGROUPS_GROUP_ID,
	IDX_HOSTGROUPS_GROUP_NAME,
	NUM_IDX_HOSTGROUPS,
};

static const DBAgent::TableProfile tableProfileHostgroups(
  TABLE_NAME_HOSTGROUPS, COLUMN_DEF_HOSTGROUPS,
  sizeof(COLUMN_DEF_HOSTGROUPS), NUM_IDX_HOSTGROUPS);

static const ColumnDef COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_MAP_HOSTS_HOSTGROUPS,   // tableName
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
	TABLE_NAME_MAP_HOSTS_HOSTGROUPS,   // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_MAP_HOSTS_HOSTGROUPS,   // tableName
	"host_id",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_MAP_HOSTS_HOSTGROUPS,   // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
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
  TABLE_NAME_MAP_HOSTS_HOSTGROUPS, COLUMN_DEF_MAP_HOSTS_HOSTGROUPS,
  sizeof(COLUMN_DEF_MAP_HOSTS_HOSTGROUPS), NUM_IDX_MAP_HOSTS_HOSTGROUPS);

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
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
// HostResourceQueryOption
// ---------------------------------------------------------------------------
struct HostResourceQueryOption::PrivateContext {
	string serverIdColumnName;
	string hostGroupIdColumnName;
	string hostIdColumnName;
	ServerIdType targetServerId;
	HostIdType targetHostId;
	HostGroupIdType targetHostgroupId;
	bool filterDataOfDefunctServers;

	PrivateContext()
	: serverIdColumnName("server_id"),
	  hostGroupIdColumnName("host_group_id"),
	  hostIdColumnName("host_id"),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_HOSTS),
	  targetHostgroupId(ALL_HOST_GROUPS),
	  filterDataOfDefunctServers(true)
	{
	}
};

HostResourceQueryOption::HostResourceQueryOption(const UserIdType &userId)
: DataQueryOption(userId)
{
	m_ctx = new PrivateContext();
}

HostResourceQueryOption::HostResourceQueryOption(const HostResourceQueryOption &src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

HostResourceQueryOption::~HostResourceQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

void HostResourceQueryOption::setServerIdColumnName(
  const std::string &name) const
{
	m_ctx->serverIdColumnName = name;
}

string HostResourceQueryOption::getServerIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->serverIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->serverIdColumnName.c_str());
}

void HostResourceQueryOption::setHostGroupIdColumnName(
  const std::string &name) const
{
	m_ctx->hostGroupIdColumnName = name;
}

string HostResourceQueryOption::getHostGroupIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->hostGroupIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->hostGroupIdColumnName.c_str());
}

void HostResourceQueryOption::setHostIdColumnName(
  const std::string &name) const
{
	m_ctx->hostIdColumnName = name;
}

string HostResourceQueryOption::getHostIdColumnName(
  const std::string &tableAlias) const
{
	if (tableAlias.empty())
		return m_ctx->hostIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    tableAlias.c_str(),
				    m_ctx->hostIdColumnName.c_str());
}

string HostResourceQueryOption::makeConditionHostGroup(
  const HostGroupSet &hostGroupSet, const string &hostGroupIdColumnName)
{
	string hostGrps;
	HostGroupSetConstIterator it = hostGroupSet.begin();
	size_t commaCnt = hostGroupSet.size() - 1;
	for (; it != hostGroupSet.end(); ++it, commaCnt--) {
		const uint64_t hostGroupId = *it;
		if (hostGroupId == ALL_HOST_GROUPS)
			return "";
		hostGrps += StringUtils::sprintf("%"PRIu64, hostGroupId);
		if (commaCnt)
			hostGrps += ",";
	}
	string cond = StringUtils::sprintf(
	  "%s IN (%s)", hostGroupIdColumnName.c_str(), hostGrps.c_str());
	return cond;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdSet &serverIdSet, const std::string &serverIdColumnName)
{
	if (serverIdSet.empty())
		return "";

	string condition = StringUtils::sprintf(
	  "%s IN (", serverIdColumnName.c_str());

	ServerIdSetConstIterator serverId = serverIdSet.begin();
	bool first = true;
	for (; serverId != serverIdSet.end(); ++serverId) {
		if (first)
			first = false;
		else
			condition += ",";
		condition += StringUtils::sprintf("%"FMT_SERVER_ID, *serverId);
	}
	condition += ")";
	return condition;
}

string HostResourceQueryOption::makeConditionServer(
  const ServerIdType &serverId, const HostGroupSet &hostGroupSet,
  const string &serverIdColumnName, const string &hostGroupIdColumnName,
  const HostGroupIdType &hostgroupId)
{
	string condition;
	condition = StringUtils::sprintf(
	  "%s=%"FMT_SERVER_ID, serverIdColumnName.c_str(), serverId);

	string conditionHostGroup;
	if (hostgroupId == ALL_HOST_GROUPS) {
		conditionHostGroup =
		  makeConditionHostGroup(hostGroupSet, hostGroupIdColumnName);
	} else {
		conditionHostGroup = StringUtils::sprintf(
		  "%s=%"FMT_HOST_GROUP_ID, hostGroupIdColumnName.c_str(),
		  hostgroupId);
	}
	if (!conditionHostGroup.empty()) {
		return StringUtils::sprintf("(%s AND %s)",
					    condition.c_str(),
					    conditionHostGroup.c_str());
	} else {
		return condition;
	}
}

string HostResourceQueryOption::makeCondition(
  const ServerHostGrpSetMap &srvHostGrpSetMap,
  const string &serverIdColumnName,
  const string &hostGroupIdColumnName,
  const string &hostIdColumnName,
  ServerIdType targetServerId,
  HostGroupIdType targetHostgroupId,
  HostIdType targetHostId)
{
	string condition;

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	if (targetServerId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetServerId) == srvHostGrpSetMap.end())
	{
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const ServerIdType &serverId = it->first;

		if (targetServerId != ALL_SERVERS && targetServerId != serverId)
			continue;

		if (serverId == ALL_SERVERS)
			return "";

		string conditionServer = makeConditionServer(
					   serverId, it->second,
					   serverIdColumnName,
					   hostGroupIdColumnName,
					   targetHostgroupId);
		addCondition(condition, conditionServer, ADD_TYPE_OR);
		++numServers;
	}

	if (targetHostId != ALL_HOSTS) {
		return StringUtils::sprintf("((%s) AND %s=%"FMT_HOST_ID")",
					    condition.c_str(),
					    hostIdColumnName.c_str(),
					    targetHostId);
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::getCondition(const string &tableAlias) const
{
	string condition;
	if (getFilterForDataOfDefunctServers()) {
		addCondition(
		  condition,
		  makeConditionServer(
		    getDataQueryContext().getValidServerIdSet(),
		    getServerIdColumnName(tableAlias))
		);
	}

	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_ctx->targetServerId != ALL_SERVERS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_SERVER_ID,
				getServerIdColumnName(tableAlias).c_str(),
				m_ctx->targetServerId));
		}
		if (m_ctx->targetHostId != ALL_HOSTS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_HOST_ID,
				getHostIdColumnName(tableAlias).c_str(),
				m_ctx->targetHostId));
		}
		if (m_ctx->targetHostgroupId != ALL_HOST_GROUPS) {
			addCondition(condition,
			  StringUtils::sprintf(
				"%s=%"FMT_HOST_GROUP_ID,
				getHostGroupIdColumnName(tableAlias).c_str(),
				m_ctx->targetHostgroupId));
		}
		return condition;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	const ServerHostGrpSetMap &srvHostGrpSetMap =
	  getDataQueryContext().getServerHostGrpSetMap();
	condition = makeCondition(srvHostGrpSetMap,
	                          getServerIdColumnName(tableAlias),
	                          getHostGroupIdColumnName(tableAlias),
	                          getHostIdColumnName(tableAlias),
	                          m_ctx->targetServerId,
	                          m_ctx->targetHostgroupId,
	                          m_ctx->targetHostId);
	return condition;
}

ServerIdType HostResourceQueryOption::getTargetServerId(void) const
{
	return m_ctx->targetServerId;
}

void HostResourceQueryOption::setTargetServerId(const ServerIdType &targetServerId)
{
	m_ctx->targetServerId = targetServerId;
}

HostIdType HostResourceQueryOption::getTargetHostId(void) const
{
	return m_ctx->targetHostId;
}

void HostResourceQueryOption::setTargetHostId(HostIdType targetHostId)
{
	m_ctx->targetHostId = targetHostId;
}

HostGroupIdType HostResourceQueryOption::getTargetHostgroupId(void) const
{
	return m_ctx->targetHostgroupId;
}

void HostResourceQueryOption::setTargetHostgroupId(
  HostGroupIdType targetHostgroupId)
{
	m_ctx->targetHostgroupId = targetHostgroupId;
}

void HostResourceQueryOption::setFilterForDataOfDefunctServers(
  const bool &enable)
{
	m_ctx->filterDataOfDefunctServers = enable;
}

const bool &HostResourceQueryOption::getFilterForDataOfDefunctServers(void) const
{
	return m_ctx->filterDataOfDefunctServers;
}

struct EventsQueryOption::PrivateContext {
	uint64_t limitOfUnifiedId;
	SortType sortType;
	SortDirection sortDirection;

	PrivateContext()
	: limitOfUnifiedId(NO_LIMIT),
	  sortType(SORT_UNIFIED_ID),
	  sortDirection(SORT_DONT_CARE)
	{
	}
};

EventsQueryOption::EventsQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	m_ctx = new PrivateContext();
	setServerIdColumnName(
	  COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName);
	setHostIdColumnName(
	  COLUMN_DEF_EVENTS[IDX_EVENTS_HOST_ID].columnName);
}

EventsQueryOption::~EventsQueryOption()
{
	delete m_ctx;
}

EventsQueryOption::EventsQueryOption(const EventsQueryOption &src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

void EventsQueryOption::setLimitOfUnifiedId(uint64_t unifiedId)
{
	m_ctx->limitOfUnifiedId = unifiedId;
}

uint64_t EventsQueryOption::getLimitOfUnifiedId(void) const
{
	return m_ctx->limitOfUnifiedId;
}

void EventsQueryOption::setSortType(SortType type, SortDirection direction)
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

TriggersQueryOption::TriggersQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	setServerIdColumnName(
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName);
	setHostIdColumnName(
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
}

ItemsQueryOption::ItemsQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	setServerIdColumnName(
	  COLUMN_DEF_ITEMS[IDX_ITEMS_SERVER_ID].columnName);
	setHostIdColumnName(
	  COLUMN_DEF_ITEMS[IDX_ITEMS_HOST_ID].columnName);
}

HostsQueryOption::HostsQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	// Currently we don't have a DB table for hosts.
	// Fetch hosts information from triggers table instead.
	setServerIdColumnName(
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName);
	setHostIdColumnName(
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
}

HostgroupsQueryOption::HostgroupsQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	setServerIdColumnName(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_SERVER_ID].columnName);
	setHostGroupIdColumnName(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_GROUP_ID].columnName);
}

HostgroupElementQueryOption::HostgroupElementQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	setServerIdColumnName(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID].columnName);
	setHostGroupIdColumnName(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID].columnName);
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
	// Now we don't have a DB table for hosts. So we get a host list from
	// the trigger table. In the future, we will add the table for hosts
	// and fix the following implementation to use it.
	DBAgent::SelectExArg arg(tableProfileTriggers);

	string stmt = StringUtils::sprintf("distinct %s", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName);
	arg.add(stmt, COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.add(IDX_TRIGGERS_HOST_ID);
	arg.add(IDX_TRIGGERS_HOSTNAME);

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
                                     const TriggersQueryOption &option,
                                     uint64_t triggerId)
{
	TriggerInfoList triggerInfoList;
	getTriggerInfoList(triggerInfoList, option, triggerId);
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
					 const TriggersQueryOption &option,
					 uint64_t targetTriggerId)
{
	// build a condition
	string optCond = option.getCondition(TABLE_NAME_TRIGGERS);
	if (isAlwaysFalseCondition(optCond))
		return;

	string condition;
	if (targetTriggerId != ALL_TRIGGERS) {
		const string colFullName =
		  SQLUtils::getFullName(COLUMN_DEF_TRIGGERS, IDX_TRIGGERS_ID);
		condition += StringUtils::sprintf(
		  "%s=%"PRIu64, colFullName.c_str(), targetTriggerId);
	}

	if (!optCond.empty()) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf("(%s)", optCond.c_str());
	}

	// select data
	static const DBAgent::TableProfile *tableProfiles[] = {
	  &tableProfileTriggers,
	  &tableProfileMapHostsHostgroups,
	  &tableProfileHostgroups,
	};
	enum {
		TBLIDX_TRIGGERS,
		TBLIDX_MAP_HOSTS_HOSTGROUPS,
		TBLIDX_HOSTGROUPS,
	};
	static const size_t numTableProfiles =
	  sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);
	DBAgent::SelectMultiTableArg arg(tableProfiles, numTableProfiles);

	arg.tableField = StringUtils::sprintf(
	  " %s inner join %s on %s=%s "
	  "inner join %s on ((%s=%s) and (%s=%s))",
	  TABLE_NAME_TRIGGERS,
	  TABLE_NAME_MAP_HOSTS_HOSTGROUPS,
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_HOST_ID).c_str(),
	  arg.getFullName(
	    TBLIDX_MAP_HOSTS_HOSTGROUPS, IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID).c_str(),
	  TABLE_NAME_HOSTGROUPS,
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_SERVER_ID).c_str(),
	  arg.getFullName(TBLIDX_HOSTGROUPS, IDX_HOSTGROUPS_SERVER_ID).c_str(),
	  arg.getFullName(TBLIDX_MAP_HOSTS_HOSTGROUPS, IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID).c_str(),
	  arg.getFullName(TBLIDX_HOSTGROUPS, IDX_HOSTGROUPS_GROUP_ID).c_str());

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

	arg.setTable(TBLIDX_MAP_HOSTS_HOSTGROUPS);
	arg.add(IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID);

	arg.setTable(TBLIDX_HOSTGROUPS);
	arg.add(IDX_HOSTGROUPS_GROUP_NAME);

	// condition
	arg.condition = condition;

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
		if (!option.isValidServer(trigInfo.serverId))
			continue;
		itemGroupStream >> trigInfo.id;
		itemGroupStream >> trigInfo.status;
		itemGroupStream >> trigInfo.severity;
		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		itemGroupStream >> trigInfo.lastChangeTime.tv_nsec;
		itemGroupStream >> trigInfo.hostId;
		itemGroupStream >> trigInfo.hostName;
		itemGroupStream >> trigInfo.brief;
		itemGroupStream >> trigInfo.hostgroupId;
		itemGroupStream >> trigInfo.hostgroupName;

		triggerInfoList.push_back(trigInfo);
	}
}

void DBClientHatohol::setTriggerInfoList(const TriggerInfoList &triggerInfoList,
                                         const ServerIdType &serverId)
{
	DBAgent::DeleteArg deleteArg(tableProfileTriggers);
	deleteArg.condition =
	  StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName, serverId);

	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoWithoutTransaction(*it);
	} DBCLIENT_TRANSACTION_END();
}

int DBClientHatohol::getLastChangeTimeOfTrigger(const ServerIdType &serverId)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.add(stmt, COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
	arg.condition = StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    serverId);

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

	// Tables
	arg.tableField = StringUtils::sprintf(
	  " %s inner join %s on %s=%s",
	  TABLE_NAME_EVENTS, TABLE_NAME_TRIGGERS,
	  arg.getFullName(TBLIDX_EVENTS, IDX_EVENTS_TRIGGER_ID).c_str(),
	  arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_ID).c_str());

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
	uint64_t limitOfUnifiedId = option.getLimitOfUnifiedId();
	if (limitOfUnifiedId) {
		string columnName
		  = arg.getFullName(TBLIDX_EVENTS, IDX_EVENTS_UNIFIED_ID);
		arg.condition += StringUtils::sprintf(
		  " AND %s<=%"PRIu64, columnName.c_str(), limitOfUnifiedId);
	}

	string optCond = option.getCondition(TABLE_NAME_EVENTS);
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
	DBAgent::DeleteArg deleteArg(tableProfileEvents);
	deleteArg.condition =
	  StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    serverId);

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
	DBAgent::SelectExArg arg(tableProfileEvents);
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName);
	arg.add(stmt, COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
	arg.condition = StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName, serverId);

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
				      const ItemsQueryOption &option,
				      uint64_t targetItemId)
{
	string optCond = option.getCondition();
	if (isAlwaysFalseCondition(optCond))
		return;

	string condition;
	if (targetItemId != ALL_ITEMS) {
		const char *colName = 
		  COLUMN_DEF_ITEMS[IDX_ITEMS_ID].columnName;
		condition += StringUtils::sprintf("%s=%"PRIu64, colName,
		                                  targetItemId);
	}

	if (!optCond.empty()) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf("(%s)", optCond.c_str());
	}

	getItemInfoList(itemInfoList, condition);
}

void DBClientHatohol::getItemInfoList(ItemInfoList &itemInfoList,
                                      const string &condition)
{
	DBAgent::SelectExArg arg(tableProfileItems);
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
	arg.condition = condition;

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

size_t DBClientHatohol::getNumberOfTriggers(const TriggersQueryOption &option,
                                            TriggerSeverityType severity)
{
	DBAgent::SelectExArg arg(tableProfileTriggers);
	arg.add("count (*)", SQL_COLUMN_TYPE_INT);

	// condition
	arg.condition = option.getCondition();
	if (!arg.condition.empty())
		arg.condition += " and ";
	arg.condition +=
	  StringUtils::sprintf("%s=%d and %s=%d",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY].columnName, severity,
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS].columnName,
	    TRIGGER_STATUS_PROBLEM);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBClientHatohol::getNumberOfHosts(const HostsQueryOption &option)
{
	// TODO: use hostGroupId after Hatohol supports it. 
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// condition
	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

size_t DBClientHatohol::getNumberOfGoodHosts(const HostsQueryOption &option)
{
	size_t numTotalHost = getNumberOfHosts(option);
	size_t numBadHosts = getNumberOfBadHosts(option);
	HATOHOL_ASSERT(numTotalHost >= numBadHosts,
	               "numTotalHost: %zd, numBadHosts: %zd",
	               numTotalHost, numBadHosts);
	return numTotalHost - numBadHosts;
}

size_t DBClientHatohol::getNumberOfBadHosts(const HostsQueryOption &option)
{
	// TODO: use hostGroupId after Hatohol supports it. 
	DBAgent::SelectExArg arg(tableProfileTriggers);
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
	arg.add(stmt, SQL_COLUMN_TYPE_INT);

	// condition
	arg.condition = option.getCondition();
	if (!arg.condition.empty())
		arg.condition += " AND ";

	arg.condition +=
	  StringUtils::sprintf("%s=%d",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS].columnName,
	    TRIGGER_STATUS_PROBLEM);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
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
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	  triggerInfo.serverId, triggerInfo.id);
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
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	   eventInfo.serverId, eventInfo.id);
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
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	  itemInfo.serverId, itemInfo.id);
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
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" and host_group_id=%"FMT_HOST_GROUP_ID,
	                                        groupInfo.serverId, groupInfo.groupId);
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
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" "
	                                        "and host_id=%"FMT_HOST_ID" "
	                                        "and host_group_id=%"FMT_HOST_GROUP_ID,
	                                        hostgroupElement.serverId,
	                                        hostgroupElement.hostId,
	                                        hostgroupElement.groupId);

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
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" "
	                                        "and host_id=%"FMT_HOST_ID,
	                                       hostInfo.serverId, hostInfo.id);

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
