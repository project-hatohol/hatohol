/*
 * Copyright (C) 2013 Project Hatohol
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

static const size_t NUM_COLUMNS_TRIGGERS =
  sizeof(COLUMN_DEF_TRIGGERS) / sizeof(ColumnDef);

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

static const size_t NUM_COLUMNS_EVENTS =
  sizeof(COLUMN_DEF_EVENTS) / sizeof(ColumnDef);

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

static const size_t NUM_COLUMNS_ITEMS =
  sizeof(COLUMN_DEF_ITEMS) / sizeof(ColumnDef);

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

static const size_t NUM_COLUMNS_HOSTS =
  sizeof(COLUMN_DEF_HOSTS) / sizeof(ColumnDef);

enum {
	IDX_HOSTS_SERVER_ID,
	IDX_HOSTS_ID,
	IDX_HOSTS_HOST_ID,
	IDX_HOSTS_HOST_NAME,
	NUM_IDX_HOSTS,
};

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

static const size_t NUM_COLUMNS_HOSTGROUPS =
  sizeof(COLUMN_DEF_HOSTGROUPS) / sizeof(ColumnDef);

enum {
	IDX_HOSTGROUPS_ID,
	IDX_HOSTGROUPS_SERVER_ID,
	IDX_HOSTGROUPS_GROUP_ID,
	IDX_HOSTGROUPS_GROUP_NAME,
	NUM_IDX_HOSTGROUPS,
};

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

static const size_t NUM_COLUMNS_MAP_HOSTS_HOSTGROUPS =
  sizeof(COLUMN_DEF_MAP_HOSTS_HOSTGROUPS) / sizeof(ColumnDef);

enum {
	IDX_MAP_HOSTS_HOSTGROUPS_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID,
	IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID,
	NUM_IDX_MAP_HOSTS_HOSTGROUPS,
};

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
{
	TABLE_NAME_TRIGGERS,
	NUM_COLUMNS_TRIGGERS,
	COLUMN_DEF_TRIGGERS,
}, {
	TABLE_NAME_EVENTS,
	NUM_COLUMNS_EVENTS,
	COLUMN_DEF_EVENTS,
}, {
	TABLE_NAME_ITEMS,
	NUM_COLUMNS_ITEMS,
	COLUMN_DEF_ITEMS,
}, {
	TABLE_NAME_HOSTS,
	NUM_COLUMNS_HOSTS,
	COLUMN_DEF_HOSTS,
}, {
	TABLE_NAME_HOSTGROUPS,
	NUM_COLUMNS_HOSTGROUPS,
	COLUMN_DEF_HOSTGROUPS,
}, {
	TABLE_NAME_MAP_HOSTS_HOSTGROUPS,
	NUM_COLUMNS_MAP_HOSTS_HOSTGROUPS,
	COLUMN_DEF_MAP_HOSTS_HOSTGROUPS,
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
	string tableNameForServerId;
	string serverIdColumnName;
	string hostGroupIdColumnName;
	string hostIdColumnName;
	ServerIdType targetServerId;
	uint64_t targetHostId;
	uint64_t targetHostgroupId;

	PrivateContext()
	: serverIdColumnName("server_id"),
	  hostGroupIdColumnName("host_group_id"),
	  hostIdColumnName("host_id"),
	  targetServerId(ALL_SERVERS),
	  targetHostId(ALL_HOSTS),
	  targetHostgroupId(ALL_HOST_GROUPS)
	{
	}
};

HostResourceQueryOption::HostResourceQueryOption(UserIdType userId)
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

string HostResourceQueryOption::getServerIdColumnName(void) const
{
	if (m_ctx->tableNameForServerId.empty())
		return m_ctx->serverIdColumnName;

	return StringUtils::sprintf("%s.%s",
				    m_ctx->tableNameForServerId.c_str(),
				    m_ctx->serverIdColumnName.c_str());
}

void HostResourceQueryOption::setHostGroupIdColumnName(
  const std::string &name) const
{
	m_ctx->hostGroupIdColumnName = name;
}

string HostResourceQueryOption::getHostGroupIdColumnName(void) const
{
	return m_ctx->hostGroupIdColumnName;
}

void HostResourceQueryOption::setHostIdColumnName(
  const std::string &name) const
{
	m_ctx->hostIdColumnName = name;
}

string HostResourceQueryOption::getHostIdColumnName(void) const
{
	return m_ctx->hostIdColumnName;
}

void HostResourceQueryOption::appendCondition(string &cond, const string &newCond)
{
	if (cond.empty()) {
		cond = newCond;
		return;
	}
	cond += " OR ";
	cond += newCond;
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
  const ServerIdType &serverId, const HostGroupSet &hostGroupSet,
  const string &serverIdColumnName, const string &hostGroupIdColumnName,
  const uint64_t &hostgroupId)
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
  ServerIdType targetServerId, uint64_t targetHostId,
  uint64_t targetHostgroupId)
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
		appendCondition(condition, conditionServer);
		++numServers;
	}

	if (targetHostId != ALL_HOSTS) {
		return StringUtils::sprintf("((%s) AND %s=%"PRIu64")",
					    condition.c_str(),
					    hostIdColumnName.c_str(),
					    targetHostId);
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

string HostResourceQueryOption::getCondition(void) const
{
	string condition;
	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_ctx->targetServerId != ALL_SERVERS) {
			condition = StringUtils::sprintf(
				"%s=%"FMT_SERVER_ID,
				getServerIdColumnName().c_str(),
				m_ctx->targetServerId);
		}
		if (m_ctx->targetHostId != ALL_HOSTS) {
			if (!condition.empty())
				condition += " AND ";
			condition += StringUtils::sprintf(
				"%s=%"PRIu64,
				getHostIdColumnName().c_str(),
				m_ctx->targetHostId);
		}
		if (m_ctx->targetHostgroupId != ALL_HOST_GROUPS) {
			if (!condition.empty())
				condition += " AND ";
			condition += StringUtils::sprintf(
				"%s=%"FMT_HOST_GROUP_ID,
				getHostGroupIdColumnName().c_str(),
				m_ctx->targetHostgroupId);
		}
		return condition;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	ServerHostGrpSetMap srvHostGrpSetMap;
	dbUser->getServerHostGrpSetMap(srvHostGrpSetMap, userId);
	condition = makeCondition(srvHostGrpSetMap,
	                          getServerIdColumnName(),
	                          getHostGroupIdColumnName(),
	                          getHostIdColumnName(),
	                          m_ctx->targetServerId,
	                          m_ctx->targetHostId,
	                          m_ctx->targetHostgroupId);
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

uint64_t HostResourceQueryOption::getTargetHostId(void) const
{
	return m_ctx->targetHostId;
}

void HostResourceQueryOption::setTargetHostId(uint64_t targetHostId)
{
	m_ctx->targetHostId = targetHostId;
}

uint64_t HostResourceQueryOption::getTargetHostgroupId(void) const
{
	return m_ctx->targetHostgroupId;
}

void HostResourceQueryOption::setTargetHostgroupId(uint64_t targetHostgroupId)
{
	m_ctx->targetHostgroupId = targetHostgroupId;
}

string HostResourceQueryOption::getTableNameForServerId(void) const
{
	return m_ctx->tableNameForServerId;
}

void HostResourceQueryOption::setTableNameForServerId(const std::string &name)
{
	m_ctx->tableNameForServerId = name;
}

EventsQueryOption::EventsQueryOption(UserIdType userId)
: HostResourceQueryOption(userId)
{
	setServerIdColumnName(
	  COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName);
	setHostIdColumnName(
	  COLUMN_DEF_EVENTS[IDX_EVENTS_HOST_ID].columnName);
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
	HATOHOL_ASSERT(NUM_COLUMNS_TRIGGERS == NUM_IDX_TRIGGERS,
	  "NUM_COLUMNS_TRIGGERS: %zd, NUM_IDX_TRIGGERS: %d",
	  NUM_COLUMNS_TRIGGERS, NUM_IDX_TRIGGERS);

	HATOHOL_ASSERT(NUM_COLUMNS_EVENTS == NUM_IDX_EVENTS,
	  "NUM_COLUMNS_EVENTS: %zd, NUM_IDX_EVENTS: %d",
	  NUM_COLUMNS_EVENTS, NUM_IDX_EVENTS);

	HATOHOL_ASSERT(NUM_COLUMNS_ITEMS == NUM_IDX_ITEMS,
	  "NUM_COLUMNS_ITEMS: %zd, NUM_IDX_ITEMS: %d",
	  NUM_COLUMNS_ITEMS, NUM_IDX_ITEMS);

	HATOHOL_ASSERT(NUM_COLUMNS_HOSTS == NUM_IDX_HOSTS,
	  "NUM_COLUMNS_HOSTS: %zd, NUM_IDX_ITEMS: %d",
	  NUM_COLUMNS_HOSTS, NUM_IDX_HOSTS);

	HATOHOL_ASSERT(NUM_COLUMNS_HOSTGROUPS == NUM_IDX_HOSTGROUPS,
	  "NUM_COLUMN_GROUPS: %zd, NUM_IDX_ITEMS: %d",
	  NUM_COLUMNS_HOSTGROUPS, NUM_IDX_HOSTGROUPS);

	HATOHOL_ASSERT(NUM_COLUMNS_MAP_HOSTS_HOSTGROUPS == NUM_IDX_MAP_HOSTS_HOSTGROUPS,
	  "NUM_COLUMN_HOSTSGROUPS: %zd, NUM_IDX_MAP_HOSTS_HOSTGROUPS: %d",
	  NUM_COLUMNS_MAP_HOSTS_HOSTGROUPS, NUM_IDX_MAP_HOSTS_HOSTGROUPS);

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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;

	// server ID
	string stmt = StringUtils::sprintf("distinct %s", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);

	// host ID
	arg.statements.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
	arg.columnTypes.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].type);

	// host name
	arg.statements.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME].columnName);
	arg.columnTypes.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME].type);

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
		addTriggerInfoBare(*triggerInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addTriggerInfoList(const TriggerInfoList &triggerInfoList)
{
	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientHatohol::getTriggerInfo(TriggerInfo &triggerInfo,
                                     const ServerIdType &serverId,
                                     uint64_t triggerId)
{
	string condition;
	const char *colNameServerId = 
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName;
	const char *colNameId = 
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName;
	condition = StringUtils::sprintf("%s=%"FMT_SERVER_ID" and %s=%"PRIu64,
	                                  colNameServerId, serverId,
	                                  colNameId, triggerId);

	TriggerInfoList triggerInfoList;
	getTriggerInfoList(triggerInfoList, condition);
	size_t numTriggers = triggerInfoList.size();
	HATOHOL_ASSERT(numTriggers <= 1,
	               "Number of triggers: %zd", numTriggers);
	if (numTriggers == 0)
		return false;

	triggerInfo = *triggerInfoList.begin();
	return true;
}

void DBClientHatohol::getTriggerInfoList(TriggerInfoList &triggerInfoList,
					 const TriggersQueryOption &option,
					 uint64_t targetTriggerId)
{
	string optCond = option.getCondition();
	if (isAlwaysFalseCondition(optCond))
		return;

	string condition;
	if (targetTriggerId != ALL_TRIGGERS) {
		const char *colName = 
		  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID].columnName;
		condition += StringUtils::sprintf("%s=%"PRIu64, colName,
		                                  targetTriggerId);
	}

	if (!optCond.empty()) {
		if (!condition.empty())
			condition += " AND ";
		condition += StringUtils::sprintf("(%s)", optCond.c_str());
	}

	getTriggerInfoList(triggerInfoList, condition);
}

void DBClientHatohol::setTriggerInfoList(const TriggerInfoList &triggerInfoList,
                                         const ServerIdType &serverId)
{
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_TRIGGERS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName, serverId);

	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

int DBClientHatohol::getLastChangeTimeOfTrigger(const ServerIdType &serverId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].type);
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
		addEventInfoBare(*eventInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addEventInfoList(const EventInfoList &eventInfoList)
{
	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

HatoholError DBClientHatohol::getEventInfoList(EventInfoList &eventInfoList,
                                               EventsQueryOption &option)
{
	const ColumnDef &eventsUnifiedId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID];
	const ColumnDef &eventsServerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID];
	const ColumnDef &eventsId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_ID];
	const ColumnDef &eventsTimeSec =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_SEC];
	const ColumnDef &eventsTimeNs =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_NS];
	const ColumnDef &eventsEventValue = 
	  COLUMN_DEF_EVENTS[IDX_EVENTS_EVENT_TYPE];
	const ColumnDef &eventsTriggerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TRIGGER_ID];

	const ColumnDef &triggersServerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID];
	const ColumnDef &triggersTriggerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID];
	const ColumnDef &triggersStatus =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS];
	const ColumnDef &triggersSeverity =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY];
	const ColumnDef &triggersHostId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID];
	const ColumnDef &triggersHostName =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME];
	const ColumnDef &triggersBrief =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_BRIEF];

	DBAgentSelectExArg arg;

	// Tables
	const static char *VAR_EVENTS = "e";
	const static char *VAR_TRIGGERS = "t";
	option.setTableNameForServerId(VAR_EVENTS);
	arg.tableName = StringUtils::sprintf(
	  " %s %s inner join %s %s on %s.%s=%s.%s",
	  TABLE_NAME_EVENTS, VAR_EVENTS,
	  TABLE_NAME_TRIGGERS, VAR_TRIGGERS,
	  VAR_EVENTS, eventsTriggerId.columnName,
	  VAR_TRIGGERS, triggersTriggerId.columnName);

	// Columns
	arg.pushColumn(eventsUnifiedId,  VAR_EVENTS);
	arg.pushColumn(eventsServerId,   VAR_EVENTS);
	arg.pushColumn(eventsId,         VAR_EVENTS);
	arg.pushColumn(eventsTimeSec,    VAR_EVENTS);
	arg.pushColumn(eventsTimeNs,     VAR_EVENTS);
	arg.pushColumn(eventsEventValue, VAR_EVENTS);
	arg.pushColumn(eventsTriggerId,  VAR_EVENTS);

	arg.pushColumn(triggersStatus,   VAR_TRIGGERS);
	arg.pushColumn(triggersSeverity, VAR_TRIGGERS);
	arg.pushColumn(triggersHostId,   VAR_TRIGGERS);
	arg.pushColumn(triggersHostName, VAR_TRIGGERS);
	arg.pushColumn(triggersBrief,    VAR_TRIGGERS);

	// Condition
	DataQueryOption::SortOrder sortOrder = option.getSortOrder();
	arg.condition = StringUtils::sprintf(
	  "%s.%s=%s.%s", 
	  VAR_EVENTS, eventsServerId.columnName,
	  VAR_TRIGGERS, triggersServerId.columnName);
	uint64_t startId = option.getStartId();
	if (startId) {
		if (sortOrder != DataQueryOption::SORT_ASCENDING &&
		    sortOrder != DataQueryOption::SORT_DESCENDING) {
			return HatoholError(HTERR_NOT_FOUND_SORT_ORDER);
		}
		arg.condition += StringUtils::sprintf(
		  " AND %s.%s%s%"PRIu64,
		  VAR_EVENTS, eventsUnifiedId.columnName,
		  sortOrder == DataQueryOption::SORT_ASCENDING ? ">=" : "<=",
		  startId);
	}

	string optCond = option.getCondition();
	if (isAlwaysFalseCondition(optCond))
		return HatoholError(HTERR_OK);
	if (!optCond.empty()) {
		arg.condition += " AND ";
		arg.condition += optCond;
	}

	// Order By
	if (sortOrder != DataQueryOption::SORT_DONT_CARE) {
		arg.orderBy +=
		  COLUMN_DEF_EVENTS[IDX_EVENTS_UNIFIED_ID].columnName;
		if (sortOrder == DataQueryOption::SORT_ASCENDING) {
			arg.orderBy += " ASC";
		} else if (sortOrder == DataQueryOption::SORT_DESCENDING) {
			arg.orderBy += " DESC";
		} else {
			HATOHOL_ASSERT(false, "Unknown sort order: %d\n",
			               sortOrder);
		}
	}

	// Limit and Offset
	arg.limit = option.getMaximumNumber();

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
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_EVENTS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    serverId);

	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupInfo(HostgroupInfo *groupInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostgroupInfoBare(*groupInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupInfoList(const HostgroupInfoList &groupInfoList)
{
	HostgroupInfoListConstIterator it = groupInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != groupInfoList.end(); ++it)
			addHostgroupInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupElement
  (HostgroupElement *hostgroupElement)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostgroupElementBare(*hostgroupElement);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostgroupElementList
  (const HostgroupElementList &hostgroupElementList)
{
	HostgroupElementListConstIterator it = hostgroupElementList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != hostgroupElementList.end(); ++it)
			addHostgroupElementBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostInfo(HostInfo *hostInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addHostInfoBare(*hostInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addHostInfoList(const HostInfoList &hostInfoList)
{
	HostInfoListConstIterator it = hostInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for(; it != hostInfoList.end(); ++it)
			addHostInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

uint64_t DBClientHatohol::getLastEventId(const ServerIdType &serverId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_EVENTS;
	string stmt = StringUtils::sprintf("max(%s)", 
	    COLUMN_DEF_EVENTS[IDX_EVENTS_ID].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(COLUMN_DEF_EVENTS[IDX_EVENTS_ID].type);
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
		addItemInfoBare(*itemInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientHatohol::addItemInfoList(const ItemInfoList &itemInfoList)
{
	ItemInfoListConstIterator it = itemInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != itemInfoList.end(); ++it)
			addItemInfoBare(*it);
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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ITEMS;
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_ID]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_HOST_ID]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_BRIEF]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_LAST_VALUE_TIME_SEC]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_LAST_VALUE_TIME_NS]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_LAST_VALUE]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_PREV_VALUE]);
	arg.pushColumn(COLUMN_DEF_ITEMS[IDX_ITEMS_ITEM_GROUP_NAME]);

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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	arg.statements.push_back("count (*)");
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_INT);

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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_INT);

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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	string stmt =
	  StringUtils::sprintf("count(distinct %s)",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID].columnName);
	arg.statements.push_back(stmt);
	arg.columnTypes.push_back(SQL_COLUMN_TYPE_INT);

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
void DBClientHatohol::addTriggerInfoBare(const TriggerInfo &triggerInfo)
{
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	  triggerInfo.serverId, triggerInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.numColumns = NUM_COLUMNS_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;
		row->addNewItem(triggerInfo.serverId);
		row->addNewItem(triggerInfo.id);
		row->addNewItem(triggerInfo.status);
		row->addNewItem(triggerInfo.severity),
		row->addNewItem(triggerInfo.lastChangeTime.tv_sec); 
		row->addNewItem(triggerInfo.lastChangeTime.tv_nsec); 
		row->addNewItem(triggerInfo.hostId);
		row->addNewItem(triggerInfo.hostName);
		row->addNewItem(triggerInfo.brief);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;

		row->addNewItem(triggerInfo.serverId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);

		row->addNewItem(triggerInfo.status);
		arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);

		row->addNewItem(triggerInfo.severity);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);

		row->addNewItem(triggerInfo.lastChangeTime.tv_sec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);

		row->addNewItem(triggerInfo.lastChangeTime.tv_nsec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_NS);

		row->addNewItem(triggerInfo.hostId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);

		row->addNewItem(triggerInfo.hostName);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);

		row->addNewItem(triggerInfo.brief);
		arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);
		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addEventInfoBare(const EventInfo &eventInfo)
{
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	   eventInfo.serverId, eventInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_EVENTS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.numColumns = NUM_COLUMNS_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;
		row->addNewItem(AUTO_INCREMENT_VALUE_U64);
		row->addNewItem(eventInfo.serverId);
		row->addNewItem(eventInfo.id);
		row->addNewItem(eventInfo.time.tv_sec); 
		row->addNewItem(eventInfo.time.tv_nsec); 
		row->addNewItem(eventInfo.type);
		row->addNewItem(eventInfo.triggerId);
		row->addNewItem(eventInfo.status);
		row->addNewItem(eventInfo.severity);
		row->addNewItem(eventInfo.hostId);
		row->addNewItem(eventInfo.hostName);
		row->addNewItem(eventInfo.brief);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;

		row->addNewItem(eventInfo.serverId);
		arg.columnIndexes.push_back(IDX_EVENTS_SERVER_ID);

		row->addNewItem(eventInfo.time.tv_sec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_SEC);

		row->addNewItem(eventInfo.time.tv_nsec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_NS);

		row->addNewItem(eventInfo.type);
		arg.columnIndexes.push_back(IDX_EVENTS_EVENT_TYPE);

		row->addNewItem(eventInfo.triggerId);
		arg.columnIndexes.push_back(IDX_EVENTS_TRIGGER_ID);

		row->addNewItem(eventInfo.status);
		arg.columnIndexes.push_back(IDX_EVENTS_STATUS);

		row->addNewItem(eventInfo.severity);
		arg.columnIndexes.push_back(IDX_EVENTS_SEVERITY);

		row->addNewItem(eventInfo.hostId);
		arg.columnIndexes.push_back(IDX_EVENTS_HOST_ID);

		row->addNewItem(eventInfo.hostName);
		arg.columnIndexes.push_back(IDX_EVENTS_HOST_NAME);

		row->addNewItem(eventInfo.brief);
		arg.columnIndexes.push_back(IDX_EVENTS_BRIEF);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addItemInfoBare(const ItemInfo &itemInfo)
{
	string condition = StringUtils::sprintf(
	  "server_id=%"FMT_SERVER_ID" and id=%"PRIu64,
	  itemInfo.serverId, itemInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_ITEMS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_ITEMS;
		arg.numColumns = NUM_COLUMNS_ITEMS;
		arg.columnDefs = COLUMN_DEF_ITEMS;
		row->addNewItem(itemInfo.serverId);
		row->addNewItem(itemInfo.id);
		row->addNewItem(itemInfo.hostId);
		row->addNewItem(itemInfo.brief);
		row->addNewItem(itemInfo.lastValueTime.tv_sec); 
		row->addNewItem(itemInfo.lastValueTime.tv_nsec); 
		row->addNewItem(itemInfo.lastValue);
		row->addNewItem(itemInfo.prevValue);
		row->addNewItem(itemInfo.itemGroupName);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_ITEMS;
		arg.columnDefs = COLUMN_DEF_ITEMS;

		row->addNewItem(itemInfo.serverId);
		arg.columnIndexes.push_back(IDX_ITEMS_SERVER_ID);

		row->addNewItem(itemInfo.id);
		arg.columnIndexes.push_back(IDX_ITEMS_ID);

		row->addNewItem(itemInfo.hostId);
		arg.columnIndexes.push_back(IDX_ITEMS_HOST_ID);

		row->addNewItem(itemInfo.brief);
		arg.columnIndexes.push_back(IDX_ITEMS_BRIEF);

		row->addNewItem(itemInfo.lastValueTime.tv_sec); 
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_SEC);

		row->addNewItem(itemInfo.lastValueTime.tv_nsec); 
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE_TIME_NS);

		row->addNewItem(itemInfo.lastValue);
		arg.columnIndexes.push_back(IDX_ITEMS_LAST_VALUE);

		row->addNewItem(itemInfo.prevValue);
		arg.columnIndexes.push_back(IDX_ITEMS_PREV_VALUE);

		row->addNewItem(itemInfo.itemGroupName);
		arg.columnIndexes.push_back(IDX_ITEMS_ITEM_GROUP_NAME);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addHostgroupInfoBare(const HostgroupInfo &groupInfo)
{
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" and host_group_id=%"FMT_HOST_GROUP_ID,
	                                        groupInfo.serverId, groupInfo.groupId);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_HOSTGROUPS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_HOSTGROUPS;
		arg.numColumns = NUM_COLUMNS_HOSTGROUPS;
		arg.columnDefs = COLUMN_DEF_HOSTGROUPS;
		row->addNewItem(groupInfo.id);
		row->addNewItem(groupInfo.serverId);
		row->addNewItem(groupInfo.groupId);
		row->addNewItem(groupInfo.groupName);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_HOSTGROUPS;
		arg.columnDefs = COLUMN_DEF_HOSTGROUPS;

		row->addNewItem(groupInfo.serverId);
		arg.columnIndexes.push_back(IDX_HOSTGROUPS_SERVER_ID);

		row->addNewItem(groupInfo.groupId);
		arg.columnIndexes.push_back(IDX_HOSTGROUPS_GROUP_ID);

		row->addNewItem(groupInfo.groupName);
		arg.columnIndexes.push_back(IDX_HOSTGROUPS_GROUP_NAME);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::addHostgroupElementBare(const HostgroupElement &hostgroupElement)
{
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" "
	                                        "and host_id=%"FMT_HOST_ID" "
	                                        "and host_group_id=%"FMT_HOST_GROUP_ID,
	                                        hostgroupElement.serverId,
	                                        hostgroupElement.hostId,
	                                        hostgroupElement.groupId);

	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_MAP_HOSTS_HOSTGROUPS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_MAP_HOSTS_HOSTGROUPS;
		arg.numColumns = NUM_COLUMNS_MAP_HOSTS_HOSTGROUPS;
		arg.columnDefs = COLUMN_DEF_MAP_HOSTS_HOSTGROUPS;
		row->addNewItem(hostgroupElement.id);
		row->addNewItem(hostgroupElement.serverId);
		row->addNewItem(hostgroupElement.hostId);
		row->addNewItem(hostgroupElement.groupId);
		arg.row = row;
		insert(arg);
	}
}

void DBClientHatohol::addHostInfoBare(const HostInfo &hostInfo)
{
	string condition = StringUtils::sprintf("server_id=%"FMT_SERVER_ID" "
	                                        "and host_id=%"FMT_HOST_ID,
	                                       hostInfo.serverId, hostInfo.id);

	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_HOSTS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_HOSTS;
		arg.numColumns = NUM_COLUMNS_HOSTS;
		arg.columnDefs = COLUMN_DEF_HOSTS;
		row->addNewItem(AUTO_INCREMENT_VALUE);
		row->addNewItem(hostInfo.serverId);
		row->addNewItem(hostInfo.id);
		row->addNewItem(hostInfo.hostName);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_HOSTS;
		arg.columnDefs = COLUMN_DEF_HOSTS;

		row->addNewItem(hostInfo.serverId);
		arg.columnIndexes.push_back(IDX_HOSTS_SERVER_ID);

		row->addNewItem(hostInfo.id);
		arg.columnIndexes.push_back(IDX_HOSTS_HOST_ID);

		row->addNewItem(hostInfo.hostName);
		arg.columnIndexes.push_back(IDX_HOSTS_HOST_NAME);

		arg.row = row;
		arg.condition = condition;
		update(arg);
	}
}

void DBClientHatohol::getTriggerInfoList(TriggerInfoList &triggerInfoList,
                                         const string &condition)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_NS]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME]);
	arg.pushColumn(COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_BRIEF]);

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
		triggerInfoList.push_back(TriggerInfo());
		TriggerInfo &trigInfo = triggerInfoList.back();

		itemGroupStream >> trigInfo.serverId;
		itemGroupStream >> trigInfo.id;
		itemGroupStream >> trigInfo.status;
		itemGroupStream >> trigInfo.severity;
		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		itemGroupStream >> trigInfo.lastChangeTime.tv_nsec;
		itemGroupStream >> trigInfo.hostId;
		itemGroupStream >> trigInfo.hostName;
		itemGroupStream >> trigInfo.brief;
	}
}

HatoholError DBClientHatohol::getHostgroupInfoList
  (HostgroupInfoList &hostgroupInfoList, const HostgroupsQueryOption &option)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_HOSTGROUPS;

	arg.statements.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_SERVER_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_SERVER_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_GROUP_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_GROUP_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_GROUP_NAME].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_HOSTGROUPS[IDX_HOSTGROUPS_GROUP_NAME].type);

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
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_MAP_HOSTS_HOSTGROUPS;

	arg.statements.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_SERVER_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_HOST_ID].type);

	arg.statements.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID].columnName);
	arg.columnTypes.push_back(
	  COLUMN_DEF_MAP_HOSTS_HOSTGROUPS[IDX_MAP_HOSTS_HOSTGROUPS_GROUP_ID].type);

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
