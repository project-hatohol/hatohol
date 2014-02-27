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

#ifndef Params_h
#define Params_h

#include <cstdio>
#include <stdint.h>
#include <set>

typedef int DBDomainId;

static const DBDomainId DB_DOMAIN_ID_CONFIG  = 0x0010;
static const DBDomainId DB_DOMAIN_ID_ACTION  = 0x0018;
static const DBDomainId DB_DOMAIN_ID_HATOHOL = 0x0020;
static const DBDomainId DB_DOMAIN_ID_USERS   = 0x0030;
static const DBDomainId DB_DOMAIN_ID_ZABBIX  = 0x00100000;
// DBClintZabbix uses the number of domains by NUM_MAX_ZABBIX_SERVERS 
// So the domain ID is occupied
//   from DB_DOMAIN_ID_ZABBIX to 0xffffffff
static const DBDomainId DB_DOMAIN_ID_NONE    = -1;

typedef int ServerIdType;
#define FMT_SERVER_ID "d"

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

typedef uint64_t TriggerIdType;
#define FMT_TRIGGER_ID PRIu64

typedef uint64_t HostGroupIdType;
#define FMT_HOST_GROUP_ID PRIu64

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

static const UserIdType INVALID_USER_ID = -1;
static const UserIdType USER_ID_ANY     = -2;

// This ID is not used for actual users.
// This program and the tests use it internally.
static const UserIdType USER_ID_SYSTEM  = 0;

static const UserRoleIdType INVALID_USER_ROLE_ID = -1;

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
typedef ServerIdSet::const_iterator ServerIdSetIterator;
extern const ServerIdSet EMPTY_SERVER_ID_SET;

enum SyncType {
	SYNC,
	ASYNC,
};
#endif // Params_h
