/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <MutexLock.h>
using namespace mlpl;

#include "DBClientZabbix.h"
#include "ItemEnum.h"
#include "ConfigManager.h"
#include "AsuraException.h"
#include "ItemTableUtils.h"
#include "DBAgentFactory.h"

// TODO: This macro is the identical to that in DBAgentSQLite3.cc
//       we will clean up code later.
#define DEFINE_AND_ASSERT(ITEM_DATA, ACTUAL_TYPE, VAR_NAME) \
	const ACTUAL_TYPE *VAR_NAME = \
	  dynamic_cast<const ACTUAL_TYPE *>(ITEM_DATA); \
	ASURA_ASSERT(VAR_NAME != NULL, "Failed to dynamic cast: %s -> %s", \
	             DEMANGLED_TYPE_NAME(*ITEM_DATA), #ACTUAL_TYPE); \

const int DBClientZabbix::ZABBIX_DB_VERSION = 2;
const uint64_t DBClientZabbix::EVENT_ID_NOT_FOUND = -1;

static const char *TABLE_NAME_SYSTEM = "system";
static const char *TABLE_NAME_TRIGGERS_RAW_2_0 = "triggers_raw_2_0";
static const char *TABLE_NAME_FUNCTIONS_RAW_2_0 = "functions_raw_2_0";
static const char *TABLE_NAME_ITEMS_RAW_2_0 = "items_raw_2_0";
static const char *TABLE_NAME_HOSTS_RAW_2_0 = "hosts_raw_2_0";
static const char *TABLE_NAME_EVENTS_RAW_2_0 = "events_raw_2_0";

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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
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
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"lifetime",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	64,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"30",                              // defaultValue
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

struct DBClientZabbix::PrivateContext
{
	static MutexLock mutex;
	static bool   dbInitializedFlags[NUM_MAX_ZABBIX_SERVERS];
	size_t             serverId;
	DBAgentSelectExArg selectExArgForTriggerAsAsuraFormat;

	// methods
	PrivateContext(size_t _serverId)
	: serverId(_serverId)
	{
	}

	~PrivateContext()
	{
	}

	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};

MutexLock DBClientZabbix::PrivateContext::mutex;
bool DBClientZabbix::PrivateContext::dbInitializedFlags[NUM_MAX_ZABBIX_SERVERS];

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientZabbix::init(void)
{
	ASURA_ASSERT(
	  NUM_COLUMNS_TRIGGERS_RAW_2_0 == NUM_IDX_TRIGGERS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_TRIGGERS_RAW_2_0 (%zd), "
	  "NUM_IDX_TRIGGERS_RAW_2_0 (%zd)",
	  NUM_COLUMNS_TRIGGERS_RAW_2_0, NUM_IDX_TRIGGERS_RAW_2_0);

	ASURA_ASSERT(
	  NUM_COLUMNS_FUNCTIONS_RAW_2_0 == NUM_IDX_FUNCTIONS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_FUNCTIONS_RAW_2_0 (%zd), "
	  "NUM_IDX_FUNCTIONS_RAW_2_0 (%zd)",
	  NUM_COLUMNS_FUNCTIONS_RAW_2_0, NUM_IDX_FUNCTIONS_RAW_2_0);

	ASURA_ASSERT(
	  NUM_COLUMNS_ITEMS_RAW_2_0 == NUM_IDX_ITEMS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_ITEMS_RAW_2_0 (%zd), "
	  "NUM_IDX_ITEMS_RAW_2_0 (%zd)",
	  NUM_COLUMNS_ITEMS_RAW_2_0, NUM_IDX_ITEMS_RAW_2_0);

	ASURA_ASSERT(
	  NUM_COLUMNS_HOSTS_RAW_2_0 == NUM_IDX_HOSTS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_HOSTS_RAW_2_0 (%zd), "
	  "NUM_IDX_HOSTS_RAW_2_0 (%zd)",
	  NUM_COLUMNS_HOSTS_RAW_2_0, NUM_IDX_HOSTS_RAW_2_0);

	ASURA_ASSERT(
	  NUM_COLUMNS_EVENTS_RAW_2_0 == NUM_IDX_EVENTS_RAW_2_0,
	  "Invalid number of elements: NUM_COLUMNS_EVENTS_RAW_2_0 (%zd), "
	  "NUM_IDX_EVENTS_RAW_2_0 (%zd)",
	  NUM_COLUMNS_EVENTS_RAW_2_0, NUM_IDX_EVENTS_RAW_2_0);
}

void DBClientZabbix::reset(void)
{
	DBClientZabbix::resetDBInitializedFlags();
}

DBDomainId DBClientZabbix::getDBDomainId(int zabbixServerId)
{
	return DB_DOMAIN_ID_ZABBIX + zabbixServerId;
}

void DBClientZabbix::resetDBInitializedFlags(void)
{
	// This function is mainly for test
	for (size_t i = 0; i < NUM_MAX_ZABBIX_SERVERS; i++)
		PrivateContext::dbInitializedFlags[i] = false;
}

DBClientZabbix::DBClientZabbix(size_t zabbixServerId)
: m_ctx(NULL)
{
	ASURA_ASSERT(zabbixServerId < NUM_MAX_ZABBIX_SERVERS,
	   "The specified zabbix server ID is larger than max: %d",
	   zabbixServerId); 
	m_ctx = new PrivateContext(zabbixServerId);

	m_ctx->lock();
	if (!m_ctx->dbInitializedFlags[zabbixServerId]) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		prepareSetupFuncCallback(zabbixServerId);
		m_ctx->dbInitializedFlags[zabbixServerId] = true;
	}
	DBDomainId domainId = DB_DOMAIN_ID_ZABBIX + zabbixServerId;
	setDBAgent(DBAgentFactory::create(domainId));
	m_ctx->unlock();
}

DBClientZabbix::~DBClientZabbix()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientZabbix::addTriggersRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgentDeleteArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS_RAW_2_0;
		deleteRows(arg);
		addItems(tablePtr, TABLE_NAME_TRIGGERS_RAW_2_0,
		         NUM_COLUMNS_TRIGGERS_RAW_2_0,
		         COLUMN_DEF_TRIGGERS_RAW_2_0);
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
		// TODO: This implementaion is transitional. We'll implement
		// a function that get data partially.
		DBAgentDeleteArg arg;
		arg.tableName = TABLE_NAME_HOSTS_RAW_2_0,
		deleteRows(arg);
		addItems(tablePtr, TABLE_NAME_HOSTS_RAW_2_0,
		         NUM_COLUMNS_HOSTS_RAW_2_0,
		         COLUMN_DEF_HOSTS_RAW_2_0);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::addEventsRaw2_0(ItemTablePtr tablePtr)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addItems(tablePtr, TABLE_NAME_EVENTS_RAW_2_0,
		         NUM_COLUMNS_EVENTS_RAW_2_0, COLUMN_DEF_EVENTS_RAW_2_0);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientZabbix::getTriggersAsAsuraFormat(TriggerInfoList &triggerInfoList)
{
	// get data from data base
	DBAgentSelectExArg &arg = m_ctx->selectExArgForTriggerAsAsuraFormat;
	if (arg.tableName.empty())
		makeSelectExArgForTriggerAsAsuraFormat();
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

		// status
		DEFINE_AND_ASSERT(
		   itemGroup->getItemAt(idx++), ItemInt, itemStatus);
		trigInfo.status = (TriggerStatusType)itemStatus->get();

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

void DBClientZabbix::getEventsAsAsuraFormat(EventInfoList &eventInfoList)
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

	return true;
}

void DBClientZabbix::transformEventsToAsuraFormat
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
  (ItemInfo &itemInfo, const ItemGroup *itemItemGroup)
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
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_DESCRIPTION),
	  ItemString, itemDescription);
	itemInfo.brief = itemDescription->get();

	// last value time
	DEFINE_AND_ASSERT(
	  itemItemGroup->getItem(ITEM_ID_ZBX_ITEMS_LASTCLOCK),
	  ItemInt, itemLastClock);
	itemInfo.lastValueTime.tv_sec = itemLastClock->get();
	itemInfo.lastValueTime.tv_nsec = 0;

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

	return true;
}

void DBClientZabbix::transformItemsToAsuraFormat
  (ItemInfoList &eventInfoList, const ItemTablePtr events, uint32_t serverId)
{
	const ItemGroupList &itemGroupList = events->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		ItemInfo eventInfo;
		eventInfo.serverId = serverId;
		if (!transformItemItemGroupToItemInfo(eventInfo, *it))
			continue;
		eventInfoList.push_back(eventInfo);
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
	THROW_ASURA_EXCEPTION(
	  "Not implemented: %s, oldVer: %d, curr: %d, data: %p",
	  __PRETTY_FUNCTION__, oldVer, DBClientZabbix::ZABBIX_DB_VERSION, data);
}

//
// Non-static methods
//
void DBClientZabbix::prepareSetupFuncCallback(size_t zabbixServerId)
{
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
	},
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		ZABBIX_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBDomainId domainId = getDBDomainId(zabbixServerId);
	DBAgent::addSetupFunction(domainId, dbSetupFunc,
	                          (void *)&DB_SETUP_FUNC_ARG);
}

void DBClientZabbix::addItems(
  ItemTablePtr tablePtr,
  const string &tableName, size_t numColumns, const ColumnDef *columnDefs)
{
	//
	// We assumed that this function is called in the transaction.
	//
	const ItemGroupList &itemGroupList = tablePtr->getItemGroupList();
	ItemGroupListConstIterator it = itemGroupList.begin();
	for (; it != itemGroupList.end(); ++it) {
		const ItemGroup *itemGroup = *it;
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

void DBClientZabbix::makeSelectExArgForTriggerAsAsuraFormat(void)
{
	DBAgentSelectExArg &arg = m_ctx->selectExArgForTriggerAsAsuraFormat;

	// tableName
	const ColumnDef &triggersTriggerid =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_TRIGGERID];
	const ColumnDef &functionsTriggerid =
	   COLUMN_DEF_FUNCTIONS_RAW_2_0[IDX_FUNCTIONS_RAW_2_0_TRIGGERID];
	const ColumnDef &functionsItemid =
	   COLUMN_DEF_FUNCTIONS_RAW_2_0[IDX_FUNCTIONS_RAW_2_0_ITEMID];
	const ColumnDef &itemsItemid =
	   COLUMN_DEF_ITEMS_RAW_2_0[IDX_ITEMS_RAW_2_0_ITEMID];
	const ColumnDef &itemsHostid =
	   COLUMN_DEF_ITEMS_RAW_2_0[IDX_ITEMS_RAW_2_0_HOSTID];
	const ColumnDef &hostsHostid =
	   COLUMN_DEF_HOSTS_RAW_2_0[IDX_HOSTS_RAW_2_0_HOSTID];

	static const char *VAR_TRIGGERS  = "t";
	static const char *VAR_FUNCTIONS = "f";
	static const char *VAR_ITEMS     = "i";
	static const char *VAR_HOSTS     = "h";
	arg.tableName = StringUtils::sprintf(
	   "%s %s "
	   "inner join %s %s on %s.%s=%s.%s "
	   "inner join %s %s on %s.%s=%s.%s "
	   "inner join %s %s on %s.%s=%s.%s",
	   TABLE_NAME_TRIGGERS_RAW_2_0, VAR_TRIGGERS,
	   TABLE_NAME_FUNCTIONS_RAW_2_0, VAR_FUNCTIONS,
	     VAR_TRIGGERS, triggersTriggerid.columnName,
	     VAR_FUNCTIONS, functionsTriggerid.columnName,
	   TABLE_NAME_ITEMS_RAW_2_0, VAR_ITEMS,
	     VAR_FUNCTIONS, functionsItemid.columnName,
	     VAR_ITEMS, itemsItemid.columnName,
	   TABLE_NAME_HOSTS_RAW_2_0, VAR_HOSTS,
	     VAR_ITEMS, itemsHostid.columnName,
	     VAR_HOSTS, hostsHostid.columnName);

	//
	// statements and columnTypes
	//
	const ColumnDef &triggersStatus =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_STATUS];
	const ColumnDef &triggersSeverity =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_PRIORITY];
	const ColumnDef &triggersLastchange = 
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_LASTCHANGE];
	const ColumnDef &triggersDescription =
	   COLUMN_DEF_TRIGGERS_RAW_2_0[IDX_TRIGGERS_RAW_2_0_DESCRIPTION];
	const ColumnDef &hostsName =
	   COLUMN_DEF_HOSTS_RAW_2_0[IDX_HOSTS_RAW_2_0_NAME];

	arg.pushColumn(triggersTriggerid,  VAR_TRIGGERS);
	arg.pushColumn(triggersStatus,     VAR_TRIGGERS);
	arg.pushColumn(triggersSeverity,   VAR_TRIGGERS);
	arg.pushColumn(triggersLastchange, VAR_TRIGGERS);
	arg.pushColumn(triggersDescription,VAR_TRIGGERS);
	arg.pushColumn(hostsHostid,        VAR_HOSTS);
	arg.pushColumn(hostsName,          VAR_HOSTS);
}
