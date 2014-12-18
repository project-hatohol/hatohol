/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef Params_h
#define Params_h

#include <cstdio>
#include <stdint.h>
#include <set>
#include <map>
#include "config.h"

#ifndef USE_CPP11
#define override
#define unique_ptr auto_ptr
#endif

typedef int DBTablesId;

static const DBTablesId DB_TABLES_ID_CONFIG     = 0x0010;
static const DBTablesId DB_TABLES_ID_ACTION     = 0x0018;
static const DBTablesId DB_TABLES_ID_MONITORING = 0x0020;
static const DBTablesId DB_TABLES_ID_USER       = 0x0030;
static const DBTablesId DB_TABLES_ID_HOST       = 0x0040;

typedef int ServerIdType;
#define FMT_SERVER_ID "d"

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

typedef int ActionIdType;
#define FMT_ACTION_ID "d"

typedef int UserIdType;
#define FMT_USER_ID "d"

typedef int AccessInfoIdType;
#define FMT_ACCESS_INFO_ID "d"

typedef int UserRoleIdType;
#define FMT_USER_ROLE_ID "d"

typedef uint64_t EventIdType;
#define FMT_EVENT_ID PRIu64

typedef uint64_t ItemIdType;
#define FMT_ITEM_ID PRIu64

typedef uint64_t TriggerIdType;
#define FMT_TRIGGER_ID PRIu64

typedef uint64_t HostgroupIdType;
#define FMT_HOST_GROUP_ID PRIu64

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

typedef int IncidentTrackerIdType;
#define FMT_INCIDENT_TRACKER_ID "d"

// Special Server IDs =========================================================
static const ServerIdType    INVALID_SERVER_ID = -2;
static const ServerIdType    ALL_SERVERS       = -1;

// Special Host IDs ===========================================================
static const HostIdType ALL_HOSTS                 = -1;
static const HostIdType INVALID_HOST_ID           = -2;
static const HostIdType INAPPLICABLE_HOST_ID      = -3;
static const HostIdType MONITORING_SERVER_SELF_ID = -4;

// Special Hostgroup IDs ======================================================
static const HostgroupIdType ALL_HOST_GROUPS = -1;

// Special Indent Tracker IDs =================================================
static const IncidentTrackerIdType ALL_INCIDENT_TRACKERS = -1;

// Special Trigger IDs ========================================================
static const TriggerIdType ALL_TRIGGERS                    = -1;

static const TriggerIdType FAILED_CONNECT_ZABBIX_TRIGGERID      = -1000;
static const TriggerIdType FAILED_CONNECT_MYSQL_TRIGGERID       = -1001;
static const TriggerIdType FAILED_INTERNAL_ERROR_TRIGGERID      = -1002;
static const TriggerIdType FAILED_PARSER_JSON_DATA_TRIGGERID    = -1003;
static const TriggerIdType FAILED_CONNECT_BROKER_TRIGGERID      = -1004;
static const TriggerIdType FAILED_CONNECT_HAP_TRIGGERID         = -1005;
static const TriggerIdType FAILED_HAP_INTERNAL_ERROR_TRIGGERID  = -1006;
static const TriggerIdType FAILED_SELF_TRIGGERID_TERMINATION    = -1007;

// Special Event IDs ==========================================================
static const EventIdType EVENT_NOT_FOUND = -1;

// Special Item IDs ===========================================================
static const TriggerIdType ALL_ITEMS    = -1;

// Special User IDs ===========================================================
static const UserIdType INVALID_USER_ID = -1;
static const UserIdType USER_ID_ANY     = -2;

// This ID is not used for actual users.
// This program and the tests use it internally.
static const UserIdType USER_ID_SYSTEM  = 0;

// Special User Role IDs ======================================================
static const UserRoleIdType INVALID_USER_ROLE_ID = -1;

// Special Arm Plugin IDs =====================================================
static const int INVALID_ARM_PLUGIN_INFO_ID = -1;

// Special index for ColumnDef ================================================
static const size_t INVALID_COLUMN_IDX = -1;

typedef std::set<UserIdType>      UserIdSet;
typedef UserIdSet::iterator       UserIdSetIterator;
typedef UserIdSet::const_iterator UserIdSetIterator;
extern const UserIdSet EMPTY_USER_ID_SET;

typedef std::set<AccessInfoIdType>      AccessInfoIdSet;
typedef AccessInfoIdSet::iterator       AccessInfoIdSetIterator;
typedef AccessInfoIdSet::const_iterator AccessInfoIdSetIterator;
extern const AccessInfoIdSet EMPTY_ACCESS_INFO_ID_SET;

typedef std::set<UserRoleIdType>      UserRoleIdSet;
typedef UserRoleIdSet::iterator       UserRoleIdSetIterator;
typedef UserRoleIdSet::const_iterator UserRoleIdSetIterator;
extern const UserRoleIdSet EMPTY_USER_ROLE_ID_SET;

typedef std::set<ServerIdType>      ServerIdSet;
typedef ServerIdSet::iterator       ServerIdSetIterator;
typedef ServerIdSet::const_iterator ServerIdSetConstIterator;
extern const ServerIdSet EMPTY_SERVER_ID_SET;

typedef std::set<IncidentTrackerIdType>      IncidentTrackerIdSet;
typedef IncidentTrackerIdSet::iterator       IncidentTrackerIdSetIterator;
typedef IncidentTrackerIdSet::const_iterator IncidentTrackerIdSetConstIterator;
extern const IncidentTrackerIdSet EMPTY_INCIDENT_TRACKER_ID_SET;

typedef std::set<HostgroupIdType>           HostgroupIdSet;
typedef HostgroupIdSet::iterator            HostgroupIdSetIterator;
typedef HostgroupIdSet::const_iterator      HostgroupIdSetConstIterator;

typedef std::map<ServerIdType, HostgroupIdSet> ServerHostGrpSetMap;
typedef ServerHostGrpSetMap::iterator       ServerHostGrpSetMapIterator;
typedef ServerHostGrpSetMap::const_iterator ServerHostGrpSetMapConstIterator;

enum SyncType {
	SYNC,
	ASYNC,
};
#endif // Params_h
