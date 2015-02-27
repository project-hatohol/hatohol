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
#include <vector>
#include <map>
#include <string>
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

typedef uint64_t GenericIdType;
#define FMT_GEN_ID PRIu64

typedef int ServerIdType;
#define FMT_SERVER_ID "d"

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

typedef std::string LocalHostIdType;
#define FMT_LOCAL_HOST_ID "s"

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

typedef uint64_t ItemCategoryIdType;
#define FMT_ITEM_CATEGORY_ID PRIu64

typedef uint64_t TriggerIdType;
#define FMT_TRIGGER_ID PRIu64

typedef std::string HostgroupIdType;
#define FMT_HOST_GROUP_ID "s"

typedef int IncidentTrackerIdType;
#define FMT_INCIDENT_TRACKER_ID "d"

// Special Server IDs =========================================================
static const ServerIdType ALL_SERVERS       = -1;
static const ServerIdType INVALID_SERVER_ID = -2;
static const ServerIdType SERVER_ID_INCIDENT_TRACKER = -3;

// Special Host IDs ===========================================================
static const HostIdType ALL_HOSTS                 = -1;
static const HostIdType INVALID_HOST_ID           = -2;
static const HostIdType INAPPLICABLE_HOST_ID      = -3;
static const HostIdType MONITORING_SERVER_SELF_ID = -4;
static const HostIdType AUTO_ASSIGNED_ID          = -5;

// TODO: We have to add code to escape real local host IDs that begin from
// SPECIAL_LOCAL_HOST_ID_PREFIX. Fortunately, such ID doesn't apper for
// ZABBIX, Nagios, and Ceilometer (OpenStack)
#define SPECIAL_LOCAL_HOST_ID_PREFIX "__"
static const LocalHostIdType ALL_LOCAL_HOSTS = "*";
static const LocalHostIdType INAPPLICABLE_LOCAL_HOST_ID =
  SPECIAL_LOCAL_HOST_ID_PREFIX "N/A";
static const LocalHostIdType MONITORING_SELF_LOCAL_HOST_ID =
  SPECIAL_LOCAL_HOST_ID_PREFIX "SELF_MONITOR";

// Special Hostgroup IDs ======================================================
static const HostgroupIdType ALL_HOST_GROUPS = "*";

// Special Indent Tracker IDs =================================================
static const IncidentTrackerIdType ALL_INCIDENT_TRACKERS = -1;

// Special Trigger IDs ========================================================
static const TriggerIdType ALL_TRIGGERS                    = -1;

static const TriggerIdType FAILED_CONNECT_ZABBIX_TRIGGER_ID     = -1000;
static const TriggerIdType FAILED_CONNECT_MYSQL_TRIGGER_ID      = -1001;
static const TriggerIdType FAILED_INTERNAL_ERROR_TRIGGER_ID     = -1002;
static const TriggerIdType FAILED_PARSER_JSON_DATA_TRIGGER_ID   = -1003;
static const TriggerIdType FAILED_CONNECT_BROKER_TRIGGER_ID     = -1004;
static const TriggerIdType FAILED_CONNECT_HAP_TRIGGER_ID        = -1005;
static const TriggerIdType FAILED_HAP_INTERNAL_ERROR_TRIGGER_ID = -1006;
static const TriggerIdType FAILED_SELF_TRIGGER_ID_TERMINATION   = -1007;

// Special Event IDs ==========================================================
static const EventIdType EVENT_NOT_FOUND = -1;

// Special Item IDs ===========================================================
static const ItemIdType ALL_ITEMS    = -1;

// Special ItemCategory IDs ===================================================
static const ItemCategoryIdType NO_ITEM_CATEGORY_ID = -1;

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
typedef UserIdSet::const_iterator UserIdSetConstIterator;
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

typedef std::vector<HostIdType>      HostIdVector;
typedef HostIdVector::iterator       HostIdVectorIterator;
typedef HostIdVector::const_iterator HostIdVectorConstIterator;

typedef std::set<HostIdType>      HostIdSet;
typedef HostIdSet::iterator       HostIdSetIterator;
typedef HostIdSet::const_iterator HostIdSetConstIterator;

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

typedef std::map<ItemCategoryIdType, std::string> ItemCategoryNameMap;
typedef ItemCategoryNameMap::iterator       ItemCategoryNameMapIterator;
typedef ItemCategoryNameMap::const_iterator ItemCategoryNameMapConstIterator;

enum SyncType {
	SYNC,
	ASYNC,
};

// Mainly used for DB access.
static const int      AUTO_INCREMENT_VALUE = 0;
static const uint64_t AUTO_INCREMENT_VALUE_U64 = 0;

#endif // Params_h
