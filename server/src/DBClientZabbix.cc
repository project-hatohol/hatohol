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

#include <MutexLock.h>
#include "DBClientZabbix.h"
#include "ItemEnum.h"
#include "Params.h"
#include "HatoholException.h"
#include "ItemTableUtils.h"
#include "DBAgentFactory.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

struct BriefElem {
	string word;
	int    variableIndex;
};

const int DBClientZabbix::ZABBIX_DB_VERSION = 4;
const uint64_t DBClientZabbix::EVENT_ID_NOT_FOUND = -1;
const int DBClientZabbix::TRIGGER_CHANGE_TIME_NOT_FOUND = -1;

static const char *TABLE_NAME_TRIGGERS_RAW_2_0 = "triggers_raw_2_0";
static const char *TABLE_NAME_FUNCTIONS_RAW_2_0 = "functions_raw_2_0";
static const char *TABLE_NAME_ITEMS_RAW_2_0 = "items_raw_2_0";
static const char *TABLE_NAME_HOSTS_RAW_2_0 = "hosts_raw_2_0";
static const char *TABLE_NAME_EVENTS_RAW_2_0 = "events_raw_2_0";
static const char *TABLE_NAME_APPLICATIONS_RAW_2_0 = "applications_raw_2_0";
static const char *TABLE_NAME_GROUPS_RAW_2_0 = "groups_raw_2_0";
static const char *TABLE_NAME_HOSTS_GROUPS_RAW_2_0 = "hosts_groups_raw_2_0";

static const ColumnDef COLUMN_DEF_TRIGGERS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_TRIGGERS_TRIGGERID,    // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"triggerid",                       // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_EXPRESSION,   // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"expression",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_DESCRIPTION,  // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"description",                     // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_URL,          // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"url",                             // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_STATUS,       // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_VALUE,        // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"value",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_PRIORITY,     // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"priority",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_LASTCHANGE,   // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"lastchange",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_COMMENTS,     // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"comments",                        // columnName
	SQL_COLUMN_TYPE_TEXT,              // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_ERROR,        // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"error",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_TEMPLATEID,   // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"templateid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_TYPE,         // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_VALUE_FLAGS,  // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"value_flags",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_FLAGS,        // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"flags",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_HOSTID,       // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"hostid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}
};

enum {
	IDX_TRIGGERS_RAW_2_0_TRIGGERID,
	IDX_TRIGGERS_RAW_2_0_EXPRESSION,
	IDX_TRIGGERS_RAW_2_0_DESCRIPTION,
	IDX_TRIGGERS_RAW_2_0_URL,
	IDX_TRIGGERS_RAW_2_0_STATUS,
	IDX_TRIGGERS_RAW_2_0_VALUE,
	IDX_TRIGGERS_RAW_2_0_PRIORITY,
	IDX_TRIGGERS_RAW_2_0_LASTCHANGE,
	IDX_TRIGGERS_RAW_2_0_COMMENTS,
	IDX_TRIGGERS_RAW_2_0_ERROR,
	IDX_TRIGGERS_RAW_2_0_TEMPLATEID,
	IDX_TRIGGERS_RAW_2_0_TYPE,
	IDX_TRIGGERS_RAW_2_0_VALUE_FLAGS,
	IDX_TRIGGERS_RAW_2_0_FLAGS,
	IDX_TRIGGERS_RAW_2_0_HOSTID,
	NUM_IDX_TRIGGERS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileTriggersRaw_2_0(
  TABLE_NAME_TRIGGERS_RAW_2_0, COLUMN_DEF_TRIGGERS_RAW_2_0,
  sizeof(COLUMN_DEF_TRIGGERS_RAW_2_0), NUM_IDX_TRIGGERS_RAW_2_0);

static const ColumnDef COLUMN_DEF_FUNCTIONS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_FUNCTIONS_FUNCTIONID,  // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"functionid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_FUNCTIONS_ITEMID,      // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"itemid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_FUNCTIONS_TRIGGERID,   // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"triggerid",                       // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_FUNCTIONS_FUNCTION,    // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"function",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	12,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_FUNCTIONS_PARAMETER,   // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"parameter",                       // columnName
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
	IDX_FUNCTIONS_RAW_2_0_FUNCTIONSTIONID,
	IDX_FUNCTIONS_RAW_2_0_ITEMID,
	IDX_FUNCTIONS_RAW_2_0_TRIGGERID,
	IDX_FUNCTIONS_RAW_2_0_FUNCTIONSTION,
	IDX_FUNCTIONS_RAW_2_0_PARAMETER,
	NUM_IDX_FUNCTIONS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileFunctionsRaw_2_0(
  TABLE_NAME_FUNCTIONS_RAW_2_0, COLUMN_DEF_FUNCTIONS_RAW_2_0,
  sizeof(COLUMN_DEF_FUNCTIONS_RAW_2_0), NUM_IDX_FUNCTIONS_RAW_2_0);

static const ColumnDef COLUMN_DEF_ITEMS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_ITEMS_ITEMID,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"itemid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_TYPE,            // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMP_COMMUNITY,  // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmp_community",                  // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMP_OID,        // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmp_oid",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_HOSTID,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"hostid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_NAME,            // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_KEY_,            // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"key_",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_DELAY,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"delay",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_HISTORY,         // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"history",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"90",                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_TRENDS,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"trends",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"365",                             // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LASTVALUE,       // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"lastvalue",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LASTCLOCK,       // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"lastclock",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PREVVALUE,       // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"prevvalue",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_STATUS,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_VALUE_TYPE,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"value_type",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_TRAPPER_HOSTS,   // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"trapper_hosts",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_UNITS,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"units",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_MULTIPLIER,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"multiplier",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_DELTA,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"delta",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PREVORGVALUE,    // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"prevorgvalue",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYNAME, // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmpv3_securityname",             // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMPV3_SECURITYLEVEL, // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmpv3_securitylevel",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMPV3_AUTHPASSPHRASE, // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmpv3_authpassphrase",           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_SNMPV3_PRIVPASSPHRASE, // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"snmpv3_privpassphrase",           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_FORMULA,         // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"formula",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_ERROR,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"error",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LASTLOGSIZE,     // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"lastlogsize",                     // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LOGTIMEFMT,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"logtimefmt",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_TEMPLATEID,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"templateid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_VALUEMAPID,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"valuemapid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_DELAY_FLEX,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"delay_flex",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PARAMS,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"params",                          // columnName
	SQL_COLUMN_TYPE_TEXT,              // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_IPMI_SENSOR,     // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"ipmi_sensor",                     // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_DATA_TYPE,       // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"data_type",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_AUTHTYPE,        // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"authtype",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_USERNAME,        // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"username",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PASSWORD,        // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"password",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PUBLICKEY,       // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"publickey",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PRIVATEKEY,      // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"privatekey",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_MTIME,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"mtime",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LASTNS,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"lastns",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_FLAGS,           // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"flags",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_FILTER,          // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"filter",                          // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_INTERFACEID,     // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"interfaceid",                     // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_PORT,            // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"port",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_DESCRIPTION,     // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"description",                     // columnName
	SQL_COLUMN_TYPE_TEXT,              // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_INVENTORY_LINK,  // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"inventory_link",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_LIFETIME,        // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"lifetime",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"30",                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_APPLICATIONID,   // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"applicationid",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_ITEMS_RAW_2_0_ITEMID,
	IDX_ITEMS_RAW_2_0_TYPE,
	IDX_ITEMS_RAW_2_0_SNMP_COMMUNITY,
	IDX_ITEMS_RAW_2_0_SNMP_OID,
	IDX_ITEMS_RAW_2_0_HOSTID,
	IDX_ITEMS_RAW_2_0_NAME,
	IDX_ITEMS_RAW_2_0_KEY_,
	IDX_ITEMS_RAW_2_0_DELAY,
	IDX_ITEMS_RAW_2_0_HISTORY,
	IDX_ITEMS_RAW_2_0_TRENDS,
	IDX_ITEMS_RAW_2_0_LASTVALUE,
	IDX_ITEMS_RAW_2_0_LASTCLOCK,
	IDX_ITEMS_RAW_2_0_PREVVALUE,
	IDX_ITEMS_RAW_2_0_STATUS,
	IDX_ITEMS_RAW_2_0_VALUE_TYPE,
	IDX_ITEMS_RAW_2_0_TRAPPER_HOSTS,
	IDX_ITEMS_RAW_2_0_UNITS,
	IDX_ITEMS_RAW_2_0_MULTIPLIER,
	IDX_ITEMS_RAW_2_0_DELTA,
	IDX_ITEMS_RAW_2_0_PREVORGVALUE,
	IDX_ITEMS_RAW_2_0_SNMPV3_SECURITYNAME,
	IDX_ITEMS_RAW_2_0_SNMPV3_SECURITYLEVEL,
	IDX_ITEMS_RAW_2_0_SNMPV3_AUTHPASSPHRASE,
	IDX_ITEMS_RAW_2_0_SNMPV3_PRIVPASSPHRASE,
	IDX_ITEMS_RAW_2_0_FORMULA,
	IDX_ITEMS_RAW_2_0_ERROR,
	IDX_ITEMS_RAW_2_0_LASTLOGSIZE,
	IDX_ITEMS_RAW_2_0_LOGTIMEFMT,
	IDX_ITEMS_RAW_2_0_TEMPLATEID,
	IDX_ITEMS_RAW_2_0_VALUEMAPID,
	IDX_ITEMS_RAW_2_0_DELAY_FLEX,
	IDX_ITEMS_RAW_2_0_PARAMS,
	IDX_ITEMS_RAW_2_0_IPMI_SENSOR,
	IDX_ITEMS_RAW_2_0_DATA_TYPE,
	IDX_ITEMS_RAW_2_0_AUTHTYPE,
	IDX_ITEMS_RAW_2_0_USERNAME,
	IDX_ITEMS_RAW_2_0_PASSWORD,
	IDX_ITEMS_RAW_2_0_PUBLICKEY,
	IDX_ITEMS_RAW_2_0_PRIVATEKEY,
	IDX_ITEMS_RAW_2_0_MTIME,
	IDX_ITEMS_RAW_2_0_LASTNS,
	IDX_ITEMS_RAW_2_0_FLAGS,
	IDX_ITEMS_RAW_2_0_FILTER,
	IDX_ITEMS_RAW_2_0_INTERFACEID,
	IDX_ITEMS_RAW_2_0_PORT,
	IDX_ITEMS_RAW_2_0_DESCRIPTION,
	IDX_ITEMS_RAW_2_0_INVENTORY_LINK,
	IDX_ITEMS_RAW_2_0_LIFETIME,
	IDX_ITEMS_RAW_2_0_APPLICATIONID,
	NUM_IDX_ITEMS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileItemsRaw_2_0(
  TABLE_NAME_ITEMS_RAW_2_0, COLUMN_DEF_ITEMS_RAW_2_0,
  sizeof(COLUMN_DEF_ITEMS_RAW_2_0), NUM_IDX_ITEMS_RAW_2_0);

static const ColumnDef COLUMN_DEF_HOSTS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_HOSTS_HOSTID,          // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"hostid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_PROXY_HOSTID,    // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"proxy_hostid",                    // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_HOST,            // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"host",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_STATUS,          // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_DISABLE_UNTIL,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"disable_until",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_ERROR,           // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"error",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_AVAILABLE,       // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"available",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_ERRORS_FROM,     // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"errors_from",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_LASTACCESS,      // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"lastaccess",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_AUTHTYPE,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_authtype",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_PRIVILEGE,  // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_privilege",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"2",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_USERNAME,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_username",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	16,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_PASSWORD,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_password",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_DISABLE_UNTIL, // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_disable_until",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_AVAILABLE,  // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_available",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_SNMP_DISABLE_UNTIL, // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"snmp_disable_until",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_SNMP_AVAILABLE,  // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"snmp_available",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_MAINTENANCEID,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"maintenanceid",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_MAINTENANCE_STATUS, // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"maintenance_status",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_MAINTENANCE_TYPE,// itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"maintenace_type",                 // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_MAINTENANCE_FROM,// itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"maintenance_from",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_ERRORS_FROM,// itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_errors_from",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_SNMP_ERRORS_FROM,// itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"snmp_errors_from",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_IPMI_ERROR,      // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"ipmi_error",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_SNMP_ERROR,      // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"snmp_error",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_JMX_DISABLE_UNTIL,// itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"jmx_disable_until",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_JMX_AVAILABLE,   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"jmx_available",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_JMX_ERRORS_FROM, // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"jmx_errors_from",                 // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_JMX_ERROR,       // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"jmx_error",                       // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_NAME,            // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}
};

enum {
	IDX_HOSTS_RAW_2_0_HOSTID,
	IDX_HOSTS_RAW_2_0_PROXY_HOSTID,
	IDX_HOSTS_RAW_2_0_HOST,
	IDX_HOSTS_RAW_2_0_STATUS,
	IDX_HOSTS_RAW_2_0_DISABLE_UNTIL,
	IDX_HOSTS_RAW_2_0_ERROR,
	IDX_HOSTS_RAW_2_0_AVAILABLE,
	IDX_HOSTS_RAW_2_0_ERRORS_FROM,
	IDX_HOSTS_RAW_2_0_LASTACCESS,
	IDX_HOSTS_RAW_2_0_IPMI_AUTHTYPE,
	IDX_HOSTS_RAW_2_0_IPMI_PRIVILEGE,
	IDX_HOSTS_RAW_2_0_IPMI_USERNAME,
	IDX_HOSTS_RAW_2_0_IPMI_PASSWORD,
	IDX_HOSTS_RAW_2_0_IPMI_DISABLE_UNTIL,
	IDX_HOSTS_RAW_2_0_IPMI_AVAILABLE,
	IDX_HOSTS_RAW_2_0_SNMP_DISABLE_UNTIL,
	IDX_HOSTS_RAW_2_0_SNMP_AVAILABLE,
	IDX_HOSTS_RAW_2_0_MAINTENANCEID,
	IDX_HOSTS_RAW_2_0_MAINTENANCE_STATUS,
	IDX_HOSTS_RAW_2_0_MAINTENANCE_TYPE,
	IDX_HOSTS_RAW_2_0_MAINTENANCE_FROM,
	IDX_HOSTS_RAW_2_0_IPMI_ERRORS_FROM,
	IDX_HOSTS_RAW_2_0_SNMP_ERRORS_FROM,
	IDX_HOSTS_RAW_2_0_IPMI_ERROR,
	IDX_HOSTS_RAW_2_0_SNMP_ERROR,
	IDX_HOSTS_RAW_2_0_JMX_DISABLE_UNTIL,
	IDX_HOSTS_RAW_2_0_JMX_AVAILABLE,
	IDX_HOSTS_RAW_2_0_JMX_ERRORS_FROM,
	IDX_HOSTS_RAW_2_0_JMX_ERROR,
	IDX_HOSTS_RAW_2_0_NAME,
	NUM_IDX_HOSTS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileHostsRaw_2_0(
  TABLE_NAME_HOSTS_RAW_2_0, COLUMN_DEF_HOSTS_RAW_2_0,
  sizeof(COLUMN_DEF_HOSTS_RAW_2_0), NUM_IDX_HOSTS_RAW_2_0);

static const ColumnDef COLUMN_DEF_EVENTS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_EVENTS_EVENTID,        // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"eventid",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_SOURCE,         // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"source",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_OBJECT,         // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"object",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_OBJECTID,       // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"objectid",                        // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_CLOCK,          // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"clock",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_VALUE,          // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"value",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_ACKNOWLEDGED,   // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"acknowledged",                    // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_NS,             // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"ns",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
}, {
	ITEM_ID_ZBX_EVENTS_VALUE_CHANGED,  // itemId
	TABLE_NAME_EVENTS_RAW_2_0,         // tableName
	"value_changed",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	0,                                 // defaultValue
},
};

enum {
	IDX_EVENTS_RAW_2_0_EVENTID,
	IDX_EVENTS_RAW_2_0_SOURCE,
	IDX_EVENTS_RAW_2_0_OBJECT,
	IDX_EVENTS_RAW_2_0_OBJECTID,
	IDX_EVENTS_RAW_2_0_CLOCK,
	IDX_EVENTS_RAW_2_0_VALUE,
	IDX_EVENTS_RAW_2_0_ACKNOWLEDGED,
	IDX_EVENTS_RAW_2_0_NS,
	IDX_EVENTS_RAW_2_0_VALUE_CHANGED,
	NUM_IDX_EVENTS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileEventsRaw_2_0(
  TABLE_NAME_EVENTS_RAW_2_0, COLUMN_DEF_EVENTS_RAW_2_0,
  sizeof(COLUMN_DEF_EVENTS_RAW_2_0), NUM_IDX_EVENTS_RAW_2_0);

enum {
	EVENT_OBJECT_TRIGGER = 0,
	EVENT_OBJECT_DHOST,
	EVENT_OBJECT_DSERVICE,
	EVENT_OBJECT_ZABBIX_ACTIVE
};

static const ColumnDef COLUMN_DEF_APPLICATIONS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID, // itemId
	TABLE_NAME_APPLICATIONS_RAW_2_0,   // tableName
	"applicationid",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_APPLICATIONS_HOSTID,   // itemId
	TABLE_NAME_APPLICATIONS_RAW_2_0,   // tableName
	"hostid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_APPLICATIONS_NAME,     // itemId
	TABLE_NAME_APPLICATIONS_RAW_2_0,   // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_ZBX_APPLICATIONS_TEMPLATEID, // itemId
	TABLE_NAME_APPLICATIONS_RAW_2_0,   // tableName
	"templateid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_APPLICATIONS_RAW_2_0_APPLICATIONID,
	IDX_APPLICATIONS_RAW_2_0_HOSTID,
	IDX_APPLICATIONS_RAW_2_0_NAME,
	IDX_APPLICATIONS_RAW_2_0_TEMPLATEID,
	NUM_IDX_APPLICATIONS_RAW_2_0,
};

static const DBAgent::TableProfile tableProfileApplicationsRaw_2_0(
  TABLE_NAME_APPLICATIONS_RAW_2_0, COLUMN_DEF_APPLICATIONS_RAW_2_0,
  sizeof(COLUMN_DEF_APPLICATIONS_RAW_2_0), NUM_IDX_APPLICATIONS_RAW_2_0);

static const ColumnDef COLUMN_DEF_GROUPS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_GROUPS_GROUPID,        // itemId
	TABLE_NAME_GROUPS_RAW_2_0,         // tableName
	"groupid",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_GROUPS_NAME,           // itemId
	TABLE_NAME_GROUPS_RAW_2_0,         // tableName
	"name",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	128,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_GROUPS_INTERNAL,       // itemId
	TABLE_NAME_GROUPS_RAW_2_0,         // tableName
	"internal",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_GROUPS_RAW_2_0_GROUPID,
	IDX_GROUPS_RAW_2_0_NAME,
	IDX_GROUPS_RAW_2_0_INTERNAL,
	NUM_IDX_GROUPS_RAW_2_0
};

static const DBAgent::TableProfile tableProfileGroupsRaw_2_0(
  TABLE_NAME_GROUPS_RAW_2_0, COLUMN_DEF_GROUPS_RAW_2_0,
  sizeof(COLUMN_DEF_GROUPS_RAW_2_0), NUM_IDX_GROUPS_RAW_2_0);

static const ColumnDef COLUMN_DEF_HOSTS_GROUPS_RAW_2_0[] = {
{
	ITEM_ID_ZBX_HOSTS_GROUPS_HOSTGROUPID, // itemId
	TABLE_NAME_HOSTS_GROUPS_RAW_2_0,      // tableName
	"hostgroupid",                        // columnName
	SQL_COLUMN_TYPE_BIGUINT,              // type
	20,                                   // columnLength
	0,                                    // decFracLength
	false,                                // canBeNull
	SQL_KEY_PRI,                          // keyType
	SQL_COLUMN_FLAG_AUTO_INC,             // flags
	NULL,                                 // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_GROUPS_HOSTID,      // itemId
	TABLE_NAME_HOSTS_GROUPS_RAW_2_0,      // tableName
	"hostid",                             // columnName
	SQL_COLUMN_TYPE_BIGUINT,              // type
	20,                                   // columnLength
	0,                                    // decFracLength
	false,                                // canBeNull
	SQL_KEY_NONE,                         // keyType
	0,                                    // flags
	NULL,                                 // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_GROUPS_GROUPID,     // itemId
	TABLE_NAME_HOSTS_GROUPS_RAW_2_0,      // tableName
	"groupid",                            // columnName
	SQL_COLUMN_TYPE_BIGUINT,              // type
	20,                                   // columnLength
	0,                                    // decFracLength
	false,                                // canBeNull
	SQL_KEY_NONE,                         // keyType
	0,                                    // flags
	NULL,                                 // defaultValue
}
};

enum {
	IDX_HOSTS_GROUPS_RAW_2_0_HOSTGROUPID,
	IDX_HOSTS_GROUPS_RAW_2_0_HOSTID,
	IDX_HOSTS_GROUPS_RAW_2_0_GROUPID,
	NUM_IDX_HOSTS_GROUPS_RAW_2_0
};

static const DBAgent::TableProfile tableProfileHostsGroupsRaw_2_0(
  TABLE_NAME_HOSTS_GROUPS_RAW_2_0, COLUMN_DEF_HOSTS_GROUPS_RAW_2_0,
  sizeof(COLUMN_DEF_HOSTS_GROUPS_RAW_2_0), NUM_IDX_HOSTS_GROUPS_RAW_2_0);

static const DBAgent::TableProfile *tableProfiles[] = {
  &tableProfileTriggersRaw_2_0,
  &tableProfileHostsRaw_2_0,
};
static const size_t numTableProfiles =
   sizeof(tableProfiles) / sizeof(DBAgent::TableProfile *);

struct DBClientZabbix::PrivateContext
{
	size_t             serverId;
	DBAgent::SelectMultiTableArg selectExArgForTriggerAsHatoholFormat;

	// methods
	PrivateContext(size_t _serverId)
	: serverId(_serverId),
	  selectExArgForTriggerAsHatoholFormat(tableProfiles, numTableProfiles)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientZabbix::init(void)
{
}

DBDomainId DBClientZabbix::getDBDomainId(const ServerIdType zabbixServerId)
{
	return DB_DOMAIN_ID_ZABBIX + zabbixServerId;
}

DBClientZabbix *DBClientZabbix::create(const ServerIdType zabbixServerId)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		&tableProfileTriggersRaw_2_0,
	}, {
		&tableProfileFunctionsRaw_2_0,
	}, {
		&tableProfileItemsRaw_2_0,
	}, {
		&tableProfileHostsRaw_2_0,
	}, {
		&tableProfileEventsRaw_2_0,
	}, {
		&tableProfileApplicationsRaw_2_0,
	}, {
		&tableProfileGroupsRaw_2_0,
	}, {
		&tableProfileHostsGroupsRaw_2_0,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		ZABBIX_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	string dbName = getDBName(zabbixServerId);
	DBDomainId domainId = DB_DOMAIN_ID_ZABBIX + zabbixServerId;
	registerSetupInfo(domainId, dbName, &DB_SETUP_FUNC_ARG);

	return new DBClientZabbix(zabbixServerId);
}

DBClientZabbix::~DBClientZabbix()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientZabbix::addTriggersRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, tableProfileTriggersRaw_2_0,
		         IDX_TRIGGERS_RAW_2_0_TRIGGERID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addFunctionsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgent::DeleteArg arg(tableProfileFunctionsRaw_2_0);
		deleteRows(arg);
		addItems(tablePtr, tableProfileFunctionsRaw_2_0,
		         IDX_FUNCTIONS_RAW_2_0_FUNCTIONSTIONID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addItemsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgent::DeleteArg arg(tableProfileItemsRaw_2_0);
		deleteRows(arg);
		addItems(tablePtr, tableProfileItemsRaw_2_0,
		         IDX_ITEMS_RAW_2_0_ITEMID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addHostsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, tableProfileHostsRaw_2_0,
		         IDX_HOSTS_RAW_2_0_HOSTID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addEventsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// Presently, the synchronization algorithm is not assumed to
		// get the duplicated events. So we don't specify 5th argument
		// for a update check.
		addItems(tablePtr, tableProfileEventsRaw_2_0,
		         IDX_EVENTS_RAW_2_0_EVENTID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addApplicationsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, tableProfileApplicationsRaw_2_0,
		         IDX_APPLICATIONS_RAW_2_0_APPLICATIONID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addGroupsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		DBAgent::DeleteArg arg(tableProfileHostsGroupsRaw_2_0);
		deleteRows(arg);
		addItems(tablePtr, tableProfileGroupsRaw_2_0,
		         IDX_GROUPS_RAW_2_0_GROUPID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addHostsGroupsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, tableProfileHostsGroupsRaw_2_0,
			 IDX_HOSTS_GROUPS_RAW_2_0_HOSTGROUPID);
	}DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::getTriggersAsHatoholFormat(TriggerInfoList &triggerInfoList)
{
	// get data from data base
	DBAgent::SelectMultiTableArg &arg =
	  m_ctx->selectExArgForTriggerAsHatoholFormat;
	if (arg.tableField.empty())
		makeSelectExArgForTriggerAsHatoholFormat();
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// copy obtained data to triggerInfoList
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		TriggerInfo trigInfo;
		trigInfo.serverId = m_ctx->serverId;
		trigInfo.lastChangeTime.tv_nsec = 0;

		itemGroupStream >> trigInfo.id;
		itemGroupStream >> trigInfo.status;
		itemGroupStream >> trigInfo.severity;
		itemGroupStream >> trigInfo.lastChangeTime.tv_sec;
		itemGroupStream >> trigInfo.brief;
		itemGroupStream >> trigInfo.hostId;
		itemGroupStream >> trigInfo.hostName;
		triggerInfoList.push_back(trigInfo);
	}
}

void DBClientZabbix::getEventsAsHatoholFormat(EventInfoList &eventInfoList)
{
	// get data from data base
	DBAgent::SelectArg arg(tableProfileEventsRaw_2_0);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_EVENTID);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_OBJECT);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_OBJECTID);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_CLOCK);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_VALUE);
	arg.columnIndexes.push_back(IDX_EVENTS_RAW_2_0_NS);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// copy obtained data to eventInfoList
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		EventInfo eventInfo;
		eventInfo.serverId = m_ctx->serverId;

		itemGroupStream >> eventInfo.id;
		if (itemGroupStream.read<int>() != EVENT_OBJECT_TRIGGER)
			continue;
		itemGroupStream >> eventInfo.triggerId;
		itemGroupStream >> eventInfo.time.tv_sec;
		itemGroupStream >> eventInfo.type;
		itemGroupStream >> eventInfo.time.tv_nsec;
		eventInfoList.push_back(eventInfo);
	}
}

bool DBClientZabbix::transformItemItemGroupToItemInfo
  (ItemInfo &itemInfo, const ItemGroup *itemItemGroup, DBClientZabbix &dbZabbix)
{
	itemInfo.lastValueTime.tv_nsec = 0;
	itemInfo.brief = makeItemBrief(itemItemGroup);

	ItemGroupStream itemGroupStream(itemItemGroup);

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_ITEMID);
	itemGroupStream >> itemInfo.id;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_HOSTID);
	itemGroupStream >> itemInfo.hostId;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_LASTCLOCK),
	itemGroupStream >> itemInfo.lastValueTime.tv_sec;
	if (itemInfo.lastValueTime.tv_sec == 0) {
		// We assume that the item in this case is a kind of
		// template such as 'Incoming network traffic on {#IFNAME}'.
		return false;
	}

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_LASTVALUE);
	itemGroupStream >> itemInfo.lastValue;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_PREVVALUE);
	itemGroupStream >> itemInfo.prevValue;

	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_DELAY);
	itemGroupStream >> itemInfo.delay;

	uint64_t applicationId;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_APPLICATIONID);
	itemGroupStream >> applicationId;
	itemInfo.itemGroupName = dbZabbix.getApplicationName(applicationId);

	return true;
}

void DBClientZabbix::transformItemsToHatoholFormat(
  ItemInfoList &itemInfoList, MonitoringServerStatus &serverStatus,
  const ItemTablePtr items)
{
	DBClientZabbix dbZabbix(serverStatus.serverId);
	const ItemGroupList &itemGroupList = items->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		ItemInfo itemInfo;
		itemInfo.serverId = serverStatus.serverId;
		if (!transformItemItemGroupToItemInfo(itemInfo, *it, dbZabbix))
			continue;
		itemInfoList.push_back(itemInfo);
	}

	ItemInfoListConstIterator item_it = itemInfoList.begin();
	serverStatus.nvps = 0.0;
	for (; item_it != itemInfoList.end(); ++item_it) {
		if ((*item_it).delay != 0)
			serverStatus.nvps += 1.0/(*item_it).delay;
	}
}

uint64_t DBClientZabbix::getLastEventId(void)
{
	const ColumnDef &columnDefEventId = 
	  COLUMN_DEF_EVENTS_RAW_2_0[IDX_EVENTS_RAW_2_0_EVENTID];

	DBAgent::SelectExArg arg(tableProfileEventsRaw_2_0);
	arg.add(StringUtils::sprintf("max(%s)", columnDefEventId.columnName),
	        columnDefEventId.type);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return EVENT_ID_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());

	if (itemGroupStream.getItem()->isNull())
		return EVENT_ID_NOT_FOUND;

	return itemGroupStream.read<uint64_t>();
}

int DBClientZabbix::getTriggerLastChange(void)
{
	const ColumnDef &columnDefTriggerLastChange =
	  COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_LASTCHANGE];

	DBAgent::SelectExArg arg(tableProfileTriggersRaw_2_0);
	string stmt = StringUtils::sprintf(
	                "max(%s)", columnDefTriggerLastChange.columnName);
	arg.add(stmt, columnDefTriggerLastChange.type);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return TRIGGER_CHANGE_TIME_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<int>();
}

string DBClientZabbix::getApplicationName(uint64_t applicationId)
{
	DBAgent::SelectExArg arg(tableProfileApplicationsRaw_2_0);
	arg.add(IDX_APPLICATIONS_RAW_2_0_NAME);
	arg.condition = StringUtils::sprintf("applicationid=%" PRIu64,
	                                     applicationId);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return "";

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupStream itemGroupStream(*grpList.begin());
	return itemGroupStream.read<string>();
}

void DBClientZabbix::pickupAbsentHostIds(vector<uint64_t> &absentHostIdVector,
                                         const vector<uint64_t> &hostIdVector)
{
	string condition;
	static const string tableName = TABLE_NAME_HOSTS_RAW_2_0;
	static const string hostidName =
	  COLUMN_DEF_HOSTS_RAW_2_0[IDX_HOSTS_RAW_2_0_HOSTID].columnName;
	DBCLIENT_TRANSACTION_BEGIN() {
		for (size_t i = 0; i < hostIdVector.size(); i++) {
			uint64_t id = hostIdVector[i];
			condition = hostidName;
			condition += StringUtils::sprintf("=%" PRIu64, id);
			if (isRecordExisting(tableName, condition))
				continue;
			absentHostIdVector.push_back(id);
		}
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::pickupAbsentApplcationIds
  (vector<uint64_t> &absentAppIdVector, const vector<uint64_t> &appIdVector)
{
	// Merge pickupAbsentHostIds() by making a commonly used method.
	string condition;
	static const string tableName = TABLE_NAME_APPLICATIONS_RAW_2_0;
	static const string appidName =
	  COLUMN_DEF_APPLICATIONS_RAW_2_0[
	    IDX_APPLICATIONS_RAW_2_0_APPLICATIONID].columnName;
	DBCLIENT_TRANSACTION_BEGIN() {
		for (size_t i = 0; i < appIdVector.size(); i++) {
			uint64_t id = appIdVector[i];
			condition = appidName;
			condition += StringUtils::sprintf("=%" PRIu64, id);
			if (isRecordExisting(tableName, condition))
				continue;
			absentAppIdVector.push_back(id);
		}
	} DBCLIENT_TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string DBClientZabbix::getDBName(const ServerIdType zabbixServerId)
{
	return StringUtils::sprintf("hatohol_cache_zabbix_%" FMT_SERVER_ID,
	                            zabbixServerId);
}

void DBClientZabbix::updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data)
{
	THROW_HATOHOL_EXCEPTION(
	  "Not implemented: %s, oldVer: %d, curr: %d, data: %p",
	  __PRETTY_FUNCTION__, oldVer, DBClientZabbix::ZABBIX_DB_VERSION, data);
}

string DBClientZabbix::makeItemBrief(const ItemGroup *itemItemGroup)
{
	ItemGroupStream itemGroupStream(itemItemGroup);

	// get items.name
	string name;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_NAME);
	itemGroupStream >> name;
	StringVector vect;
	StringUtils::split(vect, name, ' ');

	// summarize word and variables ($n : n = 1,2,3,..)
	vector<BriefElem> briefElemVect;
	for (size_t i = 0; i < vect.size(); i++) {
		briefElemVect.push_back(BriefElem());
		BriefElem &briefElem = briefElemVect.back();
		briefElem.variableIndex = getItemVariable(vect[i]);
		if (briefElem.variableIndex <= 0)
			briefElem.word = vect[i];
	}

	// extract words to be replace
	string itemKey;
	itemGroupStream.seek(ITEM_ID_ZBX_ITEMS_KEY_);
	itemGroupStream >> itemKey;

	StringVector params;
	extractItemKeys(params, itemKey);

	// make a brief
	string brief;
	for (size_t i = 0; i < briefElemVect.size(); i++) {
		BriefElem &briefElem = briefElemVect[i];
		if (briefElem.variableIndex <= 0) {
			brief += briefElem.word;
		} else if (params.size() >= (size_t)briefElem.variableIndex) {
			// normal case
			brief += params[briefElem.variableIndex-1];
		} else {
			// error case
			MLPL_WARN("Not found index: %d, %s, %s\n",
			          briefElem.variableIndex,
			          name.c_str(), itemKey.c_str());
			brief += "<INTERNAL ERROR>";
		}
		if (i < briefElemVect.size()-1)
			brief += " ";
	}

	return brief;
}

int DBClientZabbix::getItemVariable(const string &word)
{
	if (word.empty())
		return -1;

	// check the first '$'
	const char *wordPtr = word.c_str();
	if (wordPtr[0] != '$')
		return -1;

	// check if the remaining characters are all numbers.
	int val = 0;
	for (size_t idx = 1; wordPtr[idx]; idx++) {
		val *= 10;
		if (wordPtr[idx] < '0')
			return -1;
		if (wordPtr[idx] > '9')
			return -1;
		val += wordPtr[idx] - '0';
	}
	return val;
}

void DBClientZabbix::extractItemKeys(StringVector &params, const string &key)
{
	// find '['
	size_t pos = key.find_first_of('[');
	if (pos == string::npos)
		return;

	// we assume the last character is ']'
	if (key[key.size()-1] != ']') {
		MLPL_WARN("The last charater is not ']': %s", key.c_str());
		return;
	}

	// split parameters
	string paramString(key, pos+1, key.size()-pos-2);
	if (paramString.empty()) {
		params.push_back("");
		return;
	}
	const bool doMerge = false;
	StringUtils::split(params, paramString, ',', doMerge);
}

//
// Non-static methods
//
DBClientZabbix::DBClientZabbix(const ServerIdType zabbixServerId)
: DBClient(getDBDomainId(zabbixServerId)),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext(zabbixServerId);
}

void DBClientZabbix::addItems(
  ItemTablePtr tablePtr, const DBAgent::TableProfile &tableProfile,
  int updateCheckIndex)
{
	//
	// We assumed that this function is called in the transaction.
	//
	const ItemGroupList &itemGroupList = tablePtr->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		const ItemGroup *itemGroup = *it;
		updateIfExistElseInsert(itemGroup, tableProfile,
		                        updateCheckIndex);
	}
}

void DBClientZabbix::makeSelectExArgForTriggerAsHatoholFormat(void)
{
	enum
	{
		TBLIDX_TRIGGERS,
		TBLIDX_HOSTS,
	};

	DBAgent::SelectMultiTableArg &arg =
	  m_ctx->selectExArgForTriggerAsHatoholFormat;

	arg.tableField = StringUtils::sprintf(
	   "%s left join %s on %s=%s",
	   TABLE_NAME_TRIGGERS_RAW_2_0,
	   TABLE_NAME_HOSTS_RAW_2_0,
	   arg.getFullName(TBLIDX_TRIGGERS, IDX_TRIGGERS_RAW_2_0_HOSTID).c_str(),
	   arg.getFullName(TBLIDX_HOSTS, IDX_HOSTS_RAW_2_0_HOSTID).c_str());

	arg.setTable(TBLIDX_TRIGGERS);
	arg.add(IDX_TRIGGERS_RAW_2_0_TRIGGERID);
	arg.add(IDX_TRIGGERS_RAW_2_0_VALUE);
	arg.add(IDX_TRIGGERS_RAW_2_0_PRIORITY);
	arg.add(IDX_TRIGGERS_RAW_2_0_LASTCHANGE);
	arg.add(IDX_TRIGGERS_RAW_2_0_DESCRIPTION);

	arg.setTable(TBLIDX_HOSTS);
	arg.add(IDX_HOSTS_RAW_2_0_HOSTID);
	arg.add(IDX_HOSTS_RAW_2_0_NAME);
}
