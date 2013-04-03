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

#include "DBAgentSQLite3.h"
#include "DBClientZabbix.h"
#include "ItemEnum.h"
#include "ConfigManager.h"

static const char *tableNameTriggersRaw2_0 = "triggers_raw_2_0";

static ColumnDef triggersRaw2_0[] = {
{
	ITEM_ID_ZBX_TRIGGERS_TRIGGERID,    // itemId
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
	"comments",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_ZBX_TRIGGERS_ERROR,        // itemId
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
	tableNameTriggersRaw2_0,           // tableName
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
static size_t NumTriggersRaw2_0 = sizeof(triggersRaw2_0) / sizeof(ColumnDef);

struct DBClientZabbix::PrivateContext
{
	static GMutex mutex;
	static bool   dbInitializedFlags[NumMaxZabbixServers];
	DBAgent *dbAgent;

	// methods
	PrivateContext(void)
	: dbAgent(NULL)
	{
	}

	~PrivateContext()
	{
		if (dbAgent)
			delete dbAgent;
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
		// the constructor of DBAgentSQLite3() below.
		prepareSetupFuncCallback(zabbixServerId);
		m_ctx->dbInitializedFlags[zabbixServerId] = true;
	}
	m_ctx->unlock();

	DBDomainId domainId = DBDomainIDZabbixRawOffset + zabbixServerId;
	m_ctx->dbAgent = new DBAgentSQLite3(domainId);
}

DBClientZabbix::~DBClientZabbix()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientZabbix::dbSetupFunc(DBDomainId domainId)
{
	const string dbPath = DBAgentSQLite3::findDBPath(domainId);
	if (DBAgentSQLite3::isTableExisting(dbPath, tableNameTriggersRaw2_0))
		return;
	createTableTriggersRaw2_0(dbPath);
}

void DBClientZabbix::createTableTriggersRaw2_0(const string &dbPath)
{
	TableCreationArg arg;
	arg.tableName  = tableNameTriggersRaw2_0;
	arg.numColumns = NumTriggersRaw2_0;
	arg.columnDefs = triggersRaw2_0;
	DBAgentSQLite3::createTable(dbPath, arg);
}

void DBClientZabbix::prepareSetupFuncCallback(size_t zabbixServerId)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	const string &dbDirectory = configMgr->getDatabaseDirectory();

	DBDomainId domainId = DBDomainIDZabbixRawOffset + zabbixServerId;
	DBAgent::addSetupFunction(DBDomainIDZabbixRawOffset, dbSetupFunc);
	string dbPath =
	  StringUtils::sprintf("%s/DBClientZabbix-%d.db",
	                       dbDirectory.c_str(), zabbixServerId);
	DBAgentSQLite3::defineDBPath(domainId, dbPath);
}

