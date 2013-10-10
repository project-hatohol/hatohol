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

typedef uint32_t DBDomainId;

static const DBDomainId DB_DOMAIN_ID_CONFIG  = 0x0010;
static const DBDomainId DB_DOMAIN_ID_ACTION  = 0x0018;
static const DBDomainId DB_DOMAIN_ID_HATOHOL = 0x0020;
static const DBDomainId DB_DOMAIN_ID_USERS   = 0x0030;
static const DBDomainId DB_DOMAIN_ID_ZABBIX  = 0x1000;
static const size_t NUM_MAX_ZABBIX_SERVERS = 100;
// DBClintZabbix uses the number of domains by NUM_MAX_ZABBIX_SERVERS 
// So the domain ID is occupied
//   from DB_DOMAIN_ID_ZABBIX
//   to   DB_DOMAIN_ID_ZABBIX + NUM_MAX_ZABBIX_SERVERS - 1
static const DBDomainId DB_DOMAIN_ID_NONE    = -1;

typedef int UserIdType;
#define FMT_USER_ID "d"
static const UserIdType INVALID_USER_ID = -1;
static const UserIdType USER_ID_ADMIN   = 0;

#endif // Params_h


