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

#include <MutexLock.h>
using namespace mlpl;

#include "DBClientZabbix.h"
#include "ItemEnum.h"
#include "Params.h"
#include "HatoholException.h"
#include "ItemTableUtils.h"
#include "DBAgentFactory.h"

struct BriefElem {
	string word;
	int    variableIndex;
};

const int DBClientZabbix::ZABBIX_DB_VERSION = 4;
const uint64_t DBClientZabbix::EVENT_ID_NOT_FOUND = -1;
const int DBClientZabbix::TRIGGER_CHANGE_TIME_NOT_FOUND = -1;

static const char *TABLE_NAME_SYSTEM = "system";
static const char *TABLE_NAME_TRIGGERS_RAW_2_0 = "triggers_raw_2_0";
static const char *TABLE_NAME_FUNCTIONS_RAW_2_0 = "functions_raw_2_0";
static const char *TABLE_NAME_ITEMS_RAW_2_0 = "items_raw_2_0";
static const char *TABLE_NAME_HOSTS_RAW_2_0 = "hosts_raw_2_0";
static const char *TABLE_NAME_EVENTS_RAW_2_0 = "events_raw_2_0";
static const char *TABLE_NAME_APPLICATIONS_RAW_2_0 = "applications_raw_2_0";

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"dummy",                           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_SYSTEM =
  sizeof(COLUMN_DEF_SYSTEM) / sizeof(ColumnDef);

enum {
	IDX_SYSTEM_DUMMY,
	NUM_IDX_SYSTEM,
};

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}
};
static const size_t NUM_COLUMNS_TRIGGERS_RAW_2_0 =
   sizeof(COLUMN_DEF_TRIGGERS_RAW_2_0) / sizeof(ColumnDef);

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
static const size_t NUM_COLUMNS_FUNCTIONS_RAW_2_0 =
   sizeof(COLUMN_DEF_FUNCTIONS_RAW_2_0) / sizeof(ColumnDef);

enum {
	IDX_FUNCTIONS_RAW_2_0_FUNCTIONSTIONID,
	IDX_FUNCTIONS_RAW_2_0_ITEMID,
	IDX_FUNCTIONS_RAW_2_0_TRIGGERID,
	IDX_FUNCTIONS_RAW_2_0_FUNCTIONSTION,
	IDX_FUNCTIONS_RAW_2_0_PARAMETER,
	NUM_IDX_FUNCTIONS_RAW_2_0,
};

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
static const size_t NUM_COLUMNS_ITEMS_RAW_2_0 =
  sizeof(COLUMN_DEF_ITEMS_RAW_2_0) / sizeof(ColumnDef);

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"",                                // defaultValue
}
};
static const size_t NUM_COLUMNS_HOSTS_RAW_2_0 =
   sizeof(COLUMN_DEF_HOSTS_RAW_2_0) / sizeof(ColumnDef);

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
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
static const size_t NUM_COLUMNS_EVENTS_RAW_2_0 =
   sizeof(COLUMN_DEF_EVENTS_RAW_2_0) / sizeof(ColumnDef);

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
	SQL_KEY_MUL,                       // keyType
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
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_APPLICATIONS_RAW_2_0 =
  sizeof(COLUMN_DEF_APPLICATIONS_RAW_2_0) / sizeof(ColumnDef);

enum {
	IDX_APPLICATIONS_RAW_2_0_APPLICATIONID,
	IDX_APPLICATIONS_RAW_2_0_HOSTID,
	IDX_APPLICATIONS_RAW_2_0_NAME,
	IDX_APPLICATIONS_RAW_2_0_TEMPLATEID,
	NUM_IDX_APPLICATIONS_RAW_2_0,
};

struct DBClientZabbix::PrivateContext
{
	size_t             serverId;
	DBAgentSelectExArg selectExArgForTriggerAsHatoholFormat;

	// methods
	PrivateContext(size_t _serverId)
	: serverId(_serverId)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientZabbix::init(void)
{
	HATOHOL_ASSERT(
	  NUM_COLUMNS_TRIGGERS_RAW_2_0 == NUM_IDX_TRIGGERS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_TRIGGERS_RAW_2_0 (%zd), "
	  "NUM_IDX_TRIGGERS_RAW_2_0 (%d)",
	  NUM_COLUMNS_TRIGGERS_RAW_2_0, NUM_IDX_TRIGGERS_RAW_2_0);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_FUNCTIONS_RAW_2_0 == NUM_IDX_FUNCTIONS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_FUNCTIONS_RAW_2_0 (%zd), "
	  "NUM_IDX_FUNCTIONS_RAW_2_0 (%d)",
	  NUM_COLUMNS_FUNCTIONS_RAW_2_0, NUM_IDX_FUNCTIONS_RAW_2_0);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_ITEMS_RAW_2_0 == NUM_IDX_ITEMS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_ITEMS_RAW_2_0 (%zd), "
	  "NUM_IDX_ITEMS_RAW_2_0 (%d)",
	  NUM_COLUMNS_ITEMS_RAW_2_0, NUM_IDX_ITEMS_RAW_2_0);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_HOSTS_RAW_2_0 == NUM_IDX_HOSTS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_HOSTS_RAW_2_0 (%zd), "
	  "NUM_IDX_HOSTS_RAW_2_0 (%d)",
	  NUM_COLUMNS_HOSTS_RAW_2_0, NUM_IDX_HOSTS_RAW_2_0);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_EVENTS_RAW_2_0 == NUM_IDX_EVENTS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_EVENTS_RAW_2_0 (%zd), "
	  "NUM_IDX_EVENTS_RAW_2_0 (%d)",
	  NUM_COLUMNS_EVENTS_RAW_2_0, NUM_IDX_EVENTS_RAW_2_0);

	HATOHOL_ASSERT(
	  NUM_COLUMNS_APPLICATIONS_RAW_2_0 == NUM_IDX_APPLICATIONS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_APPLICATIONS_RAW_2_0 (%zd), "
	  "NUM_IDX_APPLICATIONS_RAW_2_0 (%d)",
	  NUM_COLUMNS_APPLICATIONS_RAW_2_0, NUM_IDX_APPLICATIONS_RAW_2_0);

	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_SYSTEM,
		NUM_COLUMNS_SYSTEM,
		COLUMN_DEF_SYSTEM,
		tableInitializerSystem,
	}, {
		TABLE_NAME_TRIGGERS_RAW_2_0,
		NUM_COLUMNS_TRIGGERS_RAW_2_0,
		COLUMN_DEF_TRIGGERS_RAW_2_0,
	}, {
		TABLE_NAME_FUNCTIONS_RAW_2_0,
		NUM_COLUMNS_FUNCTIONS_RAW_2_0,
		COLUMN_DEF_FUNCTIONS_RAW_2_0,
	}, {
		TABLE_NAME_ITEMS_RAW_2_0,
		NUM_COLUMNS_ITEMS_RAW_2_0,
		COLUMN_DEF_ITEMS_RAW_2_0,
	}, {
		TABLE_NAME_HOSTS_RAW_2_0,
		NUM_COLUMNS_HOSTS_RAW_2_0,
		COLUMN_DEF_HOSTS_RAW_2_0,
	}, {
		TABLE_NAME_EVENTS_RAW_2_0,
		NUM_COLUMNS_EVENTS_RAW_2_0,
		COLUMN_DEF_EVENTS_RAW_2_0,
	}, {
		TABLE_NAME_APPLICATIONS_RAW_2_0,
		NUM_COLUMNS_APPLICATIONS_RAW_2_0,
		COLUMN_DEF_APPLICATIONS_RAW_2_0,
	},
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

	static DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		ZABBIX_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	for (size_t i = 0; i < NUM_MAX_ZABBIX_SERVERS; i++) {
		string dbName =
		  StringUtils::sprintf("hatohol_cache_zabbix_%zd", i);
		DBDomainId domainId = DB_DOMAIN_ID_ZABBIX + i;
		registerSetupInfo(domainId, dbName, &DB_SETUP_FUNC_ARG);
	}
}

DBDomainId DBClientZabbix::getDBDomainId(int zabbixServerId)
{
	return DB_DOMAIN_ID_ZABBIX + zabbixServerId;
}

DBClientZabbix::DBClientZabbix(size_t zabbixServerId)
: DBClient(getDBDomainId(zabbixServerId)),
  m_ctx(NULL)
{
	HATOHOL_ASSERT(zabbixServerId < NUM_MAX_ZABBIX_SERVERS,
	   "The specified zabbix server ID is larger than max: %zd",
	   zabbixServerId); 
	m_ctx = new PrivateContext(zabbixServerId);
}

DBClientZabbix::~DBClientZabbix()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientZabbix::addTriggersRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, TABLE_NAME_TRIGGERS_RAW_2_0,
		         NUM_COLUMNS_TRIGGERS_RAW_2_0,
		         COLUMN_DEF_TRIGGERS_RAW_2_0,
		         IDX_TRIGGERS_RAW_2_0_TRIGGERID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addFunctionsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgentDeleteArg arg;
		arg.tableName = TABLE_NAME_FUNCTIONS_RAW_2_0;
		deleteRows(arg);
		addItems(tablePtr, TABLE_NAME_FUNCTIONS_RAW_2_0,
		         NUM_COLUMNS_FUNCTIONS_RAW_2_0,
		         COLUMN_DEF_FUNCTIONS_RAW_2_0);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addItemsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgentDeleteArg arg;
		arg.tableName = TABLE_NAME_ITEMS_RAW_2_0;
		deleteRows(arg);
		addItems(tablePtr, TABLE_NAME_ITEMS_RAW_2_0,
		         NUM_COLUMNS_ITEMS_RAW_2_0,
		         COLUMN_DEF_ITEMS_RAW_2_0);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addHostsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, TABLE_NAME_HOSTS_RAW_2_0,
		         NUM_COLUMNS_HOSTS_RAW_2_0,
		         COLUMN_DEF_HOSTS_RAW_2_0,
		         IDX_HOSTS_RAW_2_0_HOSTID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addEventsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// Presently, the synchronization algorithm is not assumed to
		// get the duplicated events. So we don't specify 5th argument
		// for a update check.
		addItems(tablePtr, TABLE_NAME_EVENTS_RAW_2_0,
		         NUM_COLUMNS_EVENTS_RAW_2_0, COLUMN_DEF_EVENTS_RAW_2_0);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addApplicationsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, TABLE_NAME_APPLICATIONS_RAW_2_0,
		         NUM_COLUMNS_APPLICATIONS_RAW_2_0,
		         COLUMN_DEF_APPLICATIONS_RAW_2_0,
		         IDX_APPLICATIONS_RAW_2_0_APPLICATIONID);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::getTriggersAsHatoholFormat(TriggerInfoList &triggerInfoList)
{
	// get data from data base
	DBAgentSelectExArg &arg = m_ctx->selectExArgForTriggerAsHatoholFormat;
	if (arg.tableName.empty())
		makeSelectExArgForTriggerAsHatoholFormat();
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// copy obtained data to triggerInfoList
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		int idx = 0;
		const ItemGroup *itemGroup = *it;
		TriggerInfo trigInfo;

		// serverId
		trigInfo.serverId = m_ctx->serverId;

		// id
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemUint64, itemId);
		trigInfo.id = itemId->get();

		// value
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemValue);
		trigInfo.status = (TriggerStatusType)itemValue->get();

		// severity
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemSeverity);
		trigInfo.severity = (TriggerSeverityType)itemSeverity->get();

		// lastChangeTime
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemLastchange);
		trigInfo.lastChangeTime.tv_sec = itemLastchange->get();
		trigInfo.lastChangeTime.tv_nsec = 0;

		// brief
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemDescription);
		trigInfo.brief = itemDescription->get();

		// hostId
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemUint64, itemHostid);
		trigInfo.hostId = itemHostid->get();

		// hostName
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemString, itemHostName);
		trigInfo.hostName = itemHostName->get();

		triggerInfoList.push_back(trigInfo);
	}
}

void DBClientZabbix::getEventsAsHatoholFormat(EventInfoList &eventInfoList)
{
	// get data from data base
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_EVENTS_RAW_2_0;
	arg.columnDefs = COLUMN_DEF_EVENTS_RAW_2_0;
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
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		int idx = 0;
		const ItemGroup *itemGroup = *it;
		EventInfo eventInfo;

		// serverId
		eventInfo.serverId = m_ctx->serverId;

		// event id
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemUint64, itemEventId);
		eventInfo.id = itemEventId->get();

		// object
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemObject);
		int object = itemObject->get();
		if (object != EVENT_OBJECT_TRIGGER)
			continue;

		// object id
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemUint64, itemObjectId);
		eventInfo.triggerId = itemObjectId->get();

		// clock
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemSec);
		eventInfo.time.tv_sec = itemSec->get();

		// type
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemValue);
		eventInfo.type = (EventType)itemValue->get();

		// ns
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemNs);
		eventInfo.time.tv_nsec = itemNs->get();

		// push back this event
		eventInfoList.push_back(eventInfo);
	}
}

bool DBClientZabbix::transformEventItemGroupToEventInfo
  (EventInfo &eventInfo, const ItemGroup *eventItemGroup)
{
	// event id
	DEFINE_AND_ASSERT(
	  eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_EVENTID),
	  ItemUint64, itemEventId);
	eventInfo.id = itemEventId->get();

	// object
	DEFINE_AND_ASSERT(
	   eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_OBJECT),
	   ItemInt, itemObject);
	int object = itemObject->get();
	if (object != EVENT_OBJECT_TRIGGER)
		return false;

	// object id
	DEFINE_AND_ASSERT(
	  eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_OBJECTID),
	  ItemUint64, itemObjectId);
	eventInfo.triggerId = itemObjectId->get();

	// clock
	DEFINE_AND_ASSERT(
	  eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_CLOCK),
	  ItemInt, itemSec);
	eventInfo.time.tv_sec = itemSec->get();

	// type
	DEFINE_AND_ASSERT(
	  eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_VALUE),
	  ItemInt, itemValue);
	eventInfo.type = (EventType)itemValue->get();

	// ns
	DEFINE_AND_ASSERT(
	  eventItemGroup->getItem(ITEM_ID_ZBX_EVENTS_NS),
	  ItemInt, itemNs);
	eventInfo.time.tv_nsec = itemNs->get();

	// Trigger's value. This can be transformed from Event's value
	// This value is refered in ActionManager. So we set here.
	switch (eventInfo.type) {
	case EVENT_TYPE_GOOD:
		eventInfo.status = TRIGGER_STATUS_OK;
		break;
	case EVENT_TYPE_BAD:
		eventInfo.status = TRIGGER_STATUS_PROBLEM;
		break;
	case EVENT_TYPE_UNKNOWN:
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
		break;
	default:
		MLPL_ERR("Unknown type: %d\n", eventInfo.type);
		eventInfo.status = TRIGGER_STATUS_UNKNOWN;
	}

	return true;
}

void DBClientZabbix::transformEventsToHatoholFormat
  (EventInfoList &eventInfoList, const ItemTablePtr events, uint32_t serverId)
{
	const ItemGroupList &itemGroupList = events->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		EventInfo eventInfo;
		eventInfo.serverId = serverId;
		if (!transformEventItemGroupToEventInfo(eventInfo, *it))
			continue;
		eventInfoList.push_back(eventInfo);
	}
}

bool DBClientZabbix::transformItemItemGroupToItemInfo
  (ItemInfo &itemInfo, const ItemGroup *itemItemGroup, DBClientZabbix &dbZabbix)
{
	// item id
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_ITEMID),
	  ItemUint64, itemItemId);
	itemInfo.id = itemItemId->get();

	// host id
	DEFINE_AND_ASSERT(
	   itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_HOSTID),
	   ItemUint64, itemHostId);
	itemInfo.hostId = itemHostId->get();

	// brief
	itemInfo.brief = makeItemBrief(itemItemGroup);

	// last value time
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_LASTCLOCK),
	  ItemInt, itemLastClock);
	itemInfo.lastValueTime.tv_sec = itemLastClock->get();
	itemInfo.lastValueTime.tv_nsec = 0;
	if (itemInfo.lastValueTime.tv_sec == 0) {
		// We assume that the item in this case is a kind of
		// template such as 'Incoming network traffic on {#IFNAME}'.
		return false;
	}

	// last value
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_LASTVALUE),
	  ItemString, itemLastValue);
	itemInfo.lastValue = itemLastValue->get();

	// prev value
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_PREVVALUE),
	  ItemString, itemPrevValue);
	itemInfo.prevValue = itemPrevValue->get();

	// itemGroupName
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_APPLICATIONID),
	  ItemUint64, itemApplicationid);
	uint64_t applicationId = itemApplicationid->get();
	itemInfo.itemGroupName = dbZabbix.getApplicationName(applicationId);

	return true;
}

void DBClientZabbix::transformItemsToHatoholFormat
  (ItemInfoList &itemInfoList, const ItemTablePtr items, uint32_t serverId)
{
	DBClientZabbix dbZabbix(serverId);
	const ItemGroupList &itemGroupList = items->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		ItemInfo itemInfo;
		itemInfo.serverId = serverId;
		if (!transformItemItemGroupToItemInfo(itemInfo, *it, dbZabbix))
			continue;
		itemInfoList.push_back(itemInfo);
	}
}

uint64_t DBClientZabbix::getLastEventId(void)
{
	const ColumnDef &columnDefEventId = 
	  COLUMN_DEF_EVENTS_RAW_2_0[IDX_EVENTS_RAW_2_0_EVENTID];

	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_EVENTS_RAW_2_0;
	arg.statements.push_back(
	  StringUtils::sprintf("max(%s)", columnDefEventId.columnName));
	arg.columnTypes.push_back(columnDefEventId.type);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return EVENT_ID_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	const ItemData *lastEventId = (*grpList.begin())->getItemAt(0);
	return ItemDataUtils::getUint64(lastEventId);
}

int DBClientZabbix::getTriggerLastChange(void)
{
	const ColumnDef &columnDefTriggerLastChange =
	  COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_LASTCHANGE];

	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS_RAW_2_0;
	arg.statements.push_back(
	  StringUtils::sprintf("max(%s)",
	                       columnDefTriggerLastChange.columnName));
	arg.columnTypes.push_back(columnDefTriggerLastChange.type);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return TRIGGER_CHANGE_TIME_NOT_FOUND;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	const ItemData *itemData = (*grpList.begin())->getItemAt(0);
	return ItemDataUtils::getInt(itemData);
}

string DBClientZabbix::getApplicationName(uint64_t applicationId)
{
	const ColumnDef &columnAppName =
	   COLUMN_DEF_APPLICATIONS_RAW_2_0[IDX_APPLICATIONS_RAW_2_0_NAME];
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_APPLICATIONS_RAW_2_0;
	arg.pushColumn(columnAppName);
	arg.condition = StringUtils::sprintf("applicationid=%"PRIu64,
	                                           applicationId);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	if (arg.dataTable->getNumberOfRows() == 0)
		return "";

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	const ItemData *applicationName = (*grpList.begin())->getItemAt(0);
	return ItemDataUtils::getString(applicationName);
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
			condition += StringUtils::sprintf("=%"PRIu64, id);
			if (DBClient::isRecordExisting(tableName, condition))
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
			condition += StringUtils::sprintf("=%"PRIu64, id);
			if (DBClient::isRecordExisting(tableName, condition))
				continue;
			absentAppIdVector.push_back(id);
		}
	} DBCLIENT_TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientZabbix::tableInitializerSystem(DBAgent *dbAgent, void *data)
{
	// insert default value
	DBAgentInsertArg insArg;
	insArg.tableName = TABLE_NAME_SYSTEM;
	insArg.numColumns = NUM_COLUMNS_SYSTEM;
	insArg.columnDefs = COLUMN_DEF_SYSTEM;
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, 0); // dummy
	insArg.row = row;
	dbAgent->insert(insArg);
}

void DBClientZabbix::updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data)
{
	THROW_HATOHOL_EXCEPTION(
	  "Not implemented: %s, oldVer: %d, curr: %d, data: %p",
	  __PRETTY_FUNCTION__, oldVer, DBClientZabbix::ZABBIX_DB_VERSION, data);
}

string DBClientZabbix::makeItemBrief(const ItemGroup *itemItemGroup)
{
	// get items.name
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_NAME),
	  ItemString, itemName);
	string name = itemName->get();
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
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_KEY_),
	  ItemString, itemKey_);
	string itemKey = itemKey_->get();

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
void DBClientZabbix::addItems(
  ItemTablePtr tablePtr,
  const string &tableName, size_t numColumns, const ColumnDef *columnDefs,
  int updateCheckIndex)
{
	//
	// We assumed that this function is called in the transaction.
	//
	const ItemGroupList &itemGroupList = tablePtr->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		const ItemGroup *itemGroup = *it;

		// update if needed
		if (updateCheckIndex >= 0) {
			const ColumnDef &columnCheck =
			  columnDefs[updateCheckIndex];
			bool exist = isRecordExisting(itemGroup, tableName,
				                      numColumns, columnCheck);
			if (exist) {
				updateItems(itemGroup, tableName,
				            numColumns, columnDefs);
				continue;
			}
		}

		// insert
		DBAgentInsertArg arg;
		arg.tableName = tableName;
		arg.numColumns = numColumns;
		arg.columnDefs = columnDefs;

		VariableItemGroupPtr row;
		for (size_t i = 0; i < itemGroup->getNumberOfItems(); i++)
			row->add(itemGroup->getItemAt(i));
		arg.row = row;
		insert(arg);
	}
}

void DBClientZabbix::updateItems(
  const ItemGroup *itemGroup,
  const string &tableName, size_t numColumns, const ColumnDef *columnDefs)
{
	//
	// We assumed that this function is called in the transaction.
	//
	DBAgentUpdateArg arg;
	arg.tableName = tableName;
	arg.columnDefs = columnDefs;
	VariableItemGroupPtr row;
	HATOHOL_ASSERT(itemGroup->getNumberOfItems() == numColumns,
	             "Mismatch: %zd, %zd",
	             itemGroup->getNumberOfItems(), numColumns);
	for (size_t i = 0; i < itemGroup->getNumberOfItems(); i++) {
		// exclude primary
		if (columnDefs[i].keyType == SQL_KEY_PRI)
			continue;
		row->add(itemGroup->getItemAt(i));
		arg.columnIndexes.push_back(i);
	}
	arg.row = row;
	update(arg);
}

void DBClientZabbix::makeSelectExArgForTriggerAsHatoholFormat(void)
{
	DBAgentSelectExArg &arg = m_ctx->selectExArgForTriggerAsHatoholFormat;

	// tableName
	const ColumnDef &triggersTriggerid =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_TRIGGERID];
	const ColumnDef &triggersHostid =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_HOSTID];
	const ColumnDef &hostsHostid =
	   COLUMN_DEF_HOSTS_RAW_2_0[IDX_HOSTS_RAW_2_0_HOSTID];

	static const char *VAR_TRIGGERS  = "t";
	static const char *VAR_HOSTS     = "h";
	arg.tableName = StringUtils::sprintf(
	   "%s %s "
	   "left join %s %s on %s.%s=%s.%s",
	   TABLE_NAME_TRIGGERS_RAW_2_0, VAR_TRIGGERS,
	   TABLE_NAME_HOSTS_RAW_2_0, VAR_HOSTS,
	     VAR_TRIGGERS, triggersHostid.columnName,
	     VAR_HOSTS, hostsHostid.columnName);

	//
	// statements and columnTypes
	//
	const ColumnDef &triggersValue =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_VALUE];
	const ColumnDef &triggersSeverity =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_PRIORITY];
	const ColumnDef &triggersLastchange = 
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_LASTCHANGE];
	const ColumnDef &triggersDescription =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_DESCRIPTION];
	const ColumnDef &hostsName =
	   COLUMN_DEF_HOSTS_RAW_2_0[IDX_HOSTS_RAW_2_0_NAME];

	arg.pushColumn(triggersTriggerid,  VAR_TRIGGERS);
	arg.pushColumn(triggersValue,      VAR_TRIGGERS);
	arg.pushColumn(triggersSeverity,   VAR_TRIGGERS);
	arg.pushColumn(triggersLastchange, VAR_TRIGGERS);
	arg.pushColumn(triggersDescription,VAR_TRIGGERS);
	arg.pushColumn(hostsHostid,        VAR_HOSTS);
	arg.pushColumn(hostsName,          VAR_HOSTS);
}

bool DBClientZabbix::isRecordExisting(
  const ItemGroup *itemGroup, const string &tableName,
  size_t numColumns, const ColumnDef &columnCheck)
{
	const ItemData *item = itemGroup->getItem(columnCheck.itemId);
	uint64_t val = ItemDataUtils::getUint64(item);
	string condition = StringUtils::sprintf(
	                     "%s=%"PRIu64, columnCheck.columnName, val);
	return DBClient::isRecordExisting(tableName, condition);
}

