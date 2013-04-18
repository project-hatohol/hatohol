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

#include <memory>

#include "DBAgentFactory.h"
#include "DBClientZabbix.h"
#include "ItemEnum.h"
#include "ConfigManager.h"
#include "AsuraException.h"
#include "ItemTableUtils.h"

#define TRANSACTION_BEGIN() \
	getDBAgent()->begin(); \
	try

#define TRANSACTION_END() \
	catch (...) { \
		getDBAgent()->rollback(); \
		throw; \
	} \
	getDBAgent()->commit()

const int DBClientZabbix::DB_VERSION = 2;
const int DBClientZabbix::NUM_PRESERVED_GENRATIONS_TRIGGERS = 3;
const int DBClientZabbix::REPLICA_GENERATION_NONE = -1;

static const char *TABLE_NAME_SYSTEM = "system";
static const char *TABLE_NAME_REPLICA_GENERATION = "replica_generation";
static const char *TABLE_NAME_TRIGGERS_RAW_2_0 = "triggers_raw_2_0";
static const char *TABLE_NAME_FUNCTIONS_RAW_2_0 = "functions_raw_2_0";
static const char *TABLE_NAME_ITEMS_RAW_2_0 = "items_raw_2_0";
static const char *TABLE_NAME_HOSTS_RAW_2_0 = "hosts_raw_2_0";

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"version",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"latest_triggers_generation_id",   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"latest_functions_generation_id",  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"latest_items_generation_id",      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"latest_hosts_generation_id",      // columnName
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
	IDX_SYSTEM_VERSION,
	IDX_SYSTEM_LATEST_TRIGGERS_GENERATION_ID,
	IDX_SYSTEM_LATEST_FUNCTIONS_GENERATION_ID,
	IDX_SYSTEM_LATEST_ITEMS_GENERATION_ID,
	IDX_SYSTEM_LATEST_HOSTS_GENERATION_ID,
	NUM_IDX_SYSTEM,
};

static const ColumnDef COLUMN_DEF_REPLICA_GENERATION[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_REPLICA_GENERATION,     // tableName
	"replica_generation_id",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_REPLICA_GENERATION,     // tableName
	"time",                            // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_REPLICA_GENERATION,     // tableName
	"target_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_UNI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_REPLICA_GENERATION =
  sizeof(COLUMN_DEF_REPLICA_GENERATION) / sizeof(ColumnDef);

enum {
	IDX_REPLICA_GENERATION_ID,
	IDX_REPLICA_GENERATION_TIME,
	IDX_REPLICA_GENERATION_TARGET_ID,
	NUM_IDX_REPLICA_GENERATION,
};

const int DBClientZabbix::REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP[] = {
	IDX_SYSTEM_LATEST_TRIGGERS_GENERATION_ID,
	IDX_SYSTEM_LATEST_FUNCTIONS_GENERATION_ID,
	IDX_SYSTEM_LATEST_ITEMS_GENERATION_ID,
	IDX_SYSTEM_LATEST_HOSTS_GENERATION_ID,
};
static const size_t NUM_REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP =
  sizeof(DBClientZabbix::REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP) / sizeof(int);

static const ColumnDef COLUMN_DEF_TRIGGERS_RAW_2_0[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"replica_generation_id",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_TRIGGERID,    // itemId
	TABLE_NAME_TRIGGERS_RAW_2_0,       // tableName
	"triggerid",                       // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
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
	IDX_TRIG_RAW_2_0_GENERATION_ID,
};

static const ColumnDef COLUMN_DEF_FUNCTIONS_RAW_2_0[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"replica_generation_id",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_FUNCTIONS_FUNCTIONID,  // itemId
	TABLE_NAME_FUNCTIONS_RAW_2_0,      // tableName
	"functionid",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
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
	IDX_FUNC_RAW_2_0_GENERATION_ID,
};

static const ColumnDef COLUMN_DEF_ITEMS_RAW_2_0[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ITEMS_RAW_2_0,          // tableName
	"replica_generation_id",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_ITEMS_ITEMID,          // itemId
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
  IDX_ITEM_RAW_2_0_GENERATION_ID,
};

static const ColumnDef COLUMN_DEF_HOSTS_RAW_2_0[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"replica_generation_id",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_HOSTS_HOSTID,          // itemId
	TABLE_NAME_HOSTS_RAW_2_0,          // tableName
	"hostid",                          // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
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
  IDX_HOST_RAW_2_0_GENERATION_ID,
};

struct DBClientZabbix::PrivateContext
{
	static GMutex mutex;
	static bool   dbInitializedFlags[NumMaxZabbixServers];

	// methods
	PrivateContext(void)
	{
	}

	~PrivateContext()
	{
	}

	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};

GMutex DBClientZabbix::PrivateContext::mutex;
bool   DBClientZabbix::PrivateContext::dbInitializedFlags[NumMaxZabbixServers];

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientZabbix::init(void)
{
	ASURA_ASSERT(
	  NUM_REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP
	  == NUM_REPLICA_GENERATION_TARGET_ID,
	  "Invalid number of elements: REPLICA_TARGET_ID_SYSTEM_LATEST_"
	  "COLUMNS_MAP (%zd), NUM_REPLICA_GENERATION_TARGET_ID (%zd)",
	  NUM_REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP,
	  NUM_REPLICA_GENERATION_TARGET_ID);
}

DBDomainId DBClientZabbix::getDBDomainId(int zabbixServerId)
{
	return DB_DOMAIN_ID_OFFSET_ZABBIX + zabbixServerId;
}

void DBClientZabbix::resetDBInitializedFlags(void)
{
	// This function is mainly for test
	for (size_t i = 0; i < NumMaxZabbixServers; i++)
		PrivateContext::dbInitializedFlags[i] = false;
}

DBClientZabbix::DBClientZabbix(size_t zabbixServerId)
: m_ctx(NULL)
{
	ASURA_ASSERT(zabbixServerId < NumMaxZabbixServers,
	   "The specified zabbix server ID is larger than max: %d",
	   zabbixServerId); 
	m_ctx = new PrivateContext();

	m_ctx->lock();
	if (!m_ctx->dbInitializedFlags[zabbixServerId]) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		prepareSetupFuncCallback(zabbixServerId);
		m_ctx->dbInitializedFlags[zabbixServerId] = true;
	}
	DBDomainId domainId = DB_DOMAIN_ID_OFFSET_ZABBIX + zabbixServerId;
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
	TRANSACTION_BEGIN() {
		int newId = updateReplicaGeneration
		              (REPLICA_GENERATION_TARGET_ID_TRIGGER);
		addReplicatedItems(newId, tablePtr,
		                   TABLE_NAME_TRIGGERS_RAW_2_0,
		                   NUM_COLUMNS_TRIGGERS_RAW_2_0,
		                   COLUMN_DEF_TRIGGERS_RAW_2_0);
		deleteOldReplicatedItems
		  (REPLICA_GENERATION_TARGET_ID_TRIGGER,
		   TABLE_NAME_TRIGGERS_RAW_2_0,
		   COLUMN_DEF_TRIGGERS_RAW_2_0,
		   IDX_TRIG_RAW_2_0_GENERATION_ID);
	}
	TRANSACTION_END();
}

void DBClientZabbix::addFunctionsRaw2_0(ItemTablePtr tablePtr)
{
	TRANSACTION_BEGIN() {
		int newId = updateReplicaGeneration
		              (REPLICA_GENERATION_TARGET_ID_FUNCTION);
		addReplicatedItems(newId, tablePtr,
		                   TABLE_NAME_FUNCTIONS_RAW_2_0,
		                   NUM_COLUMNS_FUNCTIONS_RAW_2_0,
		                   COLUMN_DEF_FUNCTIONS_RAW_2_0);
		deleteOldReplicatedItems
		  (REPLICA_GENERATION_TARGET_ID_FUNCTION,
		   TABLE_NAME_FUNCTIONS_RAW_2_0,
		   COLUMN_DEF_FUNCTIONS_RAW_2_0,
		   IDX_FUNC_RAW_2_0_GENERATION_ID);
	}
	TRANSACTION_END();
}

void DBClientZabbix::addItemsRaw2_0(ItemTablePtr tablePtr)
{
	TRANSACTION_BEGIN() {
		int newId = updateReplicaGeneration
		              (REPLICA_GENERATION_TARGET_ID_ITEM);
		addReplicatedItems(newId, tablePtr,
		                   TABLE_NAME_ITEMS_RAW_2_0,
		                   NUM_COLUMNS_ITEMS_RAW_2_0,
		                   COLUMN_DEF_ITEMS_RAW_2_0);
		deleteOldReplicatedItems
		  (REPLICA_GENERATION_TARGET_ID_ITEM,
		   TABLE_NAME_ITEMS_RAW_2_0,
		   COLUMN_DEF_ITEMS_RAW_2_0,
		   IDX_ITEM_RAW_2_0_GENERATION_ID);
	}
	TRANSACTION_END();
}

void DBClientZabbix::addHostsRaw2_0(ItemTablePtr tablePtr)
{
	TRANSACTION_BEGIN() {
		int newId = updateReplicaGeneration
		              (REPLICA_GENERATION_TARGET_ID_HOST);
		addReplicatedItems(newId, tablePtr,
		                   TABLE_NAME_HOSTS_RAW_2_0,
		                   NUM_COLUMNS_HOSTS_RAW_2_0,
		                   COLUMN_DEF_HOSTS_RAW_2_0);
		deleteOldReplicatedItems
		  (REPLICA_GENERATION_TARGET_ID_HOST,
		   TABLE_NAME_HOSTS_RAW_2_0,
		   COLUMN_DEF_HOSTS_RAW_2_0,
		   IDX_HOST_RAW_2_0_GENERATION_ID);
	}
	TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientZabbix::dbSetupFunc(DBDomainId domainId)
{
	bool skipSetup = true;
	auto_ptr<DBAgent> dbAgent(DBAgentFactory::create(domainId, skipSetup));
	if (!dbAgent->isTableExisting(TABLE_NAME_SYSTEM)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_SYSTEM, NUM_COLUMNS_SYSTEM,
		            COLUMN_DEF_SYSTEM, tableInitializerSystem);
	} else {
		updateDBIfNeeded(dbAgent.get());
	}

	if (!dbAgent->isTableExisting(TABLE_NAME_REPLICA_GENERATION)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_REPLICA_GENERATION,
		            NUM_COLUMNS_REPLICA_GENERATION,
		            COLUMN_DEF_REPLICA_GENERATION);
	}

	if (!dbAgent->isTableExisting(TABLE_NAME_TRIGGERS_RAW_2_0)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_TRIGGERS_RAW_2_0,
		            NUM_COLUMNS_TRIGGERS_RAW_2_0,
		            COLUMN_DEF_TRIGGERS_RAW_2_0);
	}
	if (!dbAgent->isTableExisting(TABLE_NAME_FUNCTIONS_RAW_2_0)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_FUNCTIONS_RAW_2_0,
		            NUM_COLUMNS_FUNCTIONS_RAW_2_0,
		            COLUMN_DEF_FUNCTIONS_RAW_2_0);
	}
	if (!dbAgent->isTableExisting(TABLE_NAME_ITEMS_RAW_2_0)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_ITEMS_RAW_2_0,
		            NUM_COLUMNS_ITEMS_RAW_2_0,
		            COLUMN_DEF_ITEMS_RAW_2_0);
	}
	if (!dbAgent->isTableExisting(TABLE_NAME_HOSTS_RAW_2_0)) {
		createTable(dbAgent.get(),
		            TABLE_NAME_HOSTS_RAW_2_0,
		            NUM_COLUMNS_HOSTS_RAW_2_0,
		            COLUMN_DEF_HOSTS_RAW_2_0);
	}
}

void DBClientZabbix::createTable
  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
   const ColumnDef *columnDefs, CreateTableInitializer initializer, void *data)
{
	DBAgentTableCreationArg arg;
	arg.tableName  = tableName;
	arg.numColumns = numColumns;
	arg.columnDefs = columnDefs;
	dbAgent->createTable(arg);
	if (initializer)
		(*initializer)(dbAgent, data);
}

void DBClientZabbix::tableInitializerSystem(DBAgent *dbAgent, void *data)
{
	// insert default value
	DBAgentInsertArg insArg;
	insArg.tableName = TABLE_NAME_SYSTEM;
	insArg.numColumns = NUM_COLUMNS_SYSTEM;
	insArg.columnDefs = COLUMN_DEF_SYSTEM;
	insArg.row->add(new ItemInt(DB_VERSION), false);
	insArg.row->add(new ItemInt(REPLICA_GENERATION_NONE), false);
	insArg.row->add(new ItemInt(REPLICA_GENERATION_NONE), false);
	insArg.row->add(new ItemInt(REPLICA_GENERATION_NONE), false);
	insArg.row->add(new ItemInt(REPLICA_GENERATION_NONE), false);
	dbAgent->insert(insArg);
}

void DBClientZabbix::updateDBIfNeeded(DBAgent *dbAgent)
{
	if (getDBVersion(dbAgent) == DB_VERSION)
		return;
	THROW_ASURA_EXCEPTION("Not implemented: %s", __PRETTY_FUNCTION__);
}

int DBClientZabbix::getDBVersion(DBAgent *dbAgent)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_VERSION);
	dbAgent->select(arg);

	const ItemGroupList &itemGroupList = arg.dataTable->getItemGroupList();
	ASURA_ASSERT(
	  itemGroupList.size() == 1,
	  "itemGroupList.size(): %zd", itemGroupList.size());
	ItemGroupListConstIterator it = itemGroupList.begin();
	const ItemGroup *itemGroup = *it;
	ASURA_ASSERT(
	  itemGroup->getNumberOfItems() == 1,
	  "itemGroup->getNumberOfItems: %zd", itemGroup->getNumberOfItems());
	ItemInt *itemVersion =
	  dynamic_cast<ItemInt *>(itemGroup->getItemAt(0));
	ASURA_ASSERT(itemVersion != NULL, "type: itemVersion: %s\n",
	             DEMANGLED_TYPE_NAME(*itemVersion));
	return itemVersion->get();
}

//
// Non-static methods
//
void DBClientZabbix::prepareSetupFuncCallback(size_t zabbixServerId)
{
	DBDomainId domainId = getDBDomainId(zabbixServerId);
	DBAgent::addSetupFunction(domainId, dbSetupFunc);
}

int DBClientZabbix::getLatestGenerationId(void)
{
	DBAgentSelectExArg arg;
	const ColumnDef columnDefReplicaGenId = 
	  COLUMN_DEF_REPLICA_GENERATION[IDX_REPLICA_GENERATION_ID];
	arg.tableName = TABLE_NAME_REPLICA_GENERATION;
	arg.statements.push_back(columnDefReplicaGenId.columnName);
	arg.columnTypes.push_back(columnDefReplicaGenId.type);
	arg.orderBy = columnDefReplicaGenId.columnName;
	arg.orderBy += " DESC";
	arg.limit = 1;
	select(arg);

	const ItemGroupList &itemGroupList = arg.dataTable->getItemGroupList();
	if (itemGroupList.empty())
		return REPLICA_GENERATION_NONE;
	ASURA_ASSERT(
	  itemGroupList.size() == 1,
	  "itemGroupList.size(): %zd", itemGroupList.size());
	ItemGroupListConstIterator it = itemGroupList.begin();
	const ItemGroup *itemGroup = *it;
	ASURA_ASSERT(
	  itemGroup->getNumberOfItems() == 1,
	  "itemGroup->getNumberOfItems: %zd", itemGroup->getNumberOfItems());
	ItemInt *item =
	  dynamic_cast<ItemInt *>(itemGroup->getItemAt(0));
	ASURA_ASSERT(item != NULL, "type: itemVersion: %s\n",
	             DEMANGLED_TYPE_NAME(*item));
	return item->get();
}

int DBClientZabbix::updateReplicaGeneration(int replicaTargetId)
{
	// We assumed that this function is called in the transcation.
	int id = getLatestGenerationId();
	int newId = id + 1;

	// insert the generation id
	uint64_t currTime = Utils::getCurrTimeAsMicroSecond();
	DBAgentInsertArg insertArg;
	insertArg.tableName = TABLE_NAME_REPLICA_GENERATION;
	insertArg.numColumns = NUM_COLUMNS_REPLICA_GENERATION;
	insertArg.columnDefs = COLUMN_DEF_REPLICA_GENERATION;
	insertArg.row->add(new ItemInt(newId), false);
	insertArg.row->add(new ItemUint64(currTime), false);
	insertArg.row->add(new ItemInt(replicaTargetId), false);
	insert(insertArg);

	// update the latest generation
	DBAgentUpdateArg updateArg;
	updateArg.tableName = TABLE_NAME_SYSTEM;
	updateArg.columnDefs = COLUMN_DEF_SYSTEM;
	int columnIdx =
	   REPLICA_TARGET_ID_SYSTEM_LATEST_COLUMNS_MAP[replicaTargetId];
	updateArg.columnIndexes.push_back(columnIdx);
	updateArg.row->add(new ItemInt(newId), false);
	update(updateArg);

	return newId;
}

void DBClientZabbix::addReplicatedItems(
  int generationId, ItemTablePtr tablePtr,
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

		arg.row->add(new ItemInt(generationId), false);
		for (size_t i = 0; i < itemGroup->getNumberOfItems(); i++)
			arg.row->add(itemGroup->getItemAt(i));
		insert(arg);
	}
}

void DBClientZabbix::deleteOldReplicatedItems
  (int replicaTargetId, const string &tableName, const ColumnDef *columnDefs,
   int generationIdIdx)
{
	// check the number of generations
	int startId = getStartIdToRemove(replicaTargetId);
	if (startId == REPLICA_GENERATION_NONE)
		return;

	// delete unncessary rows from the replica table
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = tableName;
	const ColumnDef &columnDefGenId = columnDefs[generationIdIdx];
	deleteArg.condition =
	   StringUtils::sprintf("%s<=%d", columnDefGenId.columnName, startId);
	deleteRows(deleteArg);

	// delete unncessary rows from the replica generation table
	DBAgentDeleteArg argGen;
	argGen.tableName = TABLE_NAME_REPLICA_GENERATION;
	const ColumnDef &columnDefReplicaGenId =
	  COLUMN_DEF_REPLICA_GENERATION[IDX_REPLICA_GENERATION_ID];
	const ColumnDef &columnDefReplicaTargetId =
	  COLUMN_DEF_REPLICA_GENERATION[IDX_REPLICA_GENERATION_TARGET_ID];
	argGen.condition = StringUtils::sprintf(
	                     "%s<=%d and %s=%d",
	                     columnDefReplicaGenId.columnName, startId,
	                     columnDefReplicaTargetId.columnName,
	                     replicaTargetId);
	deleteRows(argGen);
}

int DBClientZabbix::getStartIdToRemove(int replicaTargetId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_REPLICA_GENERATION;

	const ColumnDef &columnDefGenId =
	  COLUMN_DEF_REPLICA_GENERATION[IDX_REPLICA_GENERATION_ID];
	arg.statements.push_back(columnDefGenId.columnName);
	arg.columnTypes.push_back(columnDefGenId.type);

	const ColumnDef &columnDefTargetId = 
	  COLUMN_DEF_REPLICA_GENERATION[IDX_REPLICA_GENERATION_TARGET_ID];
	arg.condition =
	   StringUtils::sprintf("%s=%d",
	                        columnDefTargetId.columnName, replicaTargetId);
	arg.orderBy = StringUtils::sprintf("%s DESC",
	                                   columnDefGenId.columnName);
	arg.limit = 1;
	ConfigManager *confMgr = ConfigManager::getInstance();
	arg.offset = confMgr->getNumberOfPreservedReplicaGeneration();
	select(arg);
	if (arg.dataTable->getItemGroupList().empty())
		return REPLICA_GENERATION_NONE;

	int startIdToRemove = ItemTableUtils::getFirstRowData
	                        <int, ItemInt>(arg.dataTable);
	return startIdToRemove;
}
