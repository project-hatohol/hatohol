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

#include <MutexLock.h>
using namespace std;

#include "DBAgentFactory.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "DBClientUtils.h"
#include "DBAgentSQLite3.h"

static const char *TABLE_NAME_SYSTEM  = "system";
static const char *TABLE_NAME_SERVERS = "servers";

int DBClientConfig::CONFIG_DB_VERSION = 3;
const char *DBClientConfig::DEFAULT_DB_NAME = "asura-config.db";

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"enable_face_mysql",               // columnName
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
	"face_rest_port",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
},
};
static const size_t NUM_COLUMNS_SYSTEM =
  sizeof(COLUMN_DEF_SYSTEM) / sizeof(ColumnDef);

enum {
	IDX_SYSTEM_ENABLE_FACE_MYSQL,
	IDX_SYSTEM_FACE_REST_PORT,
	NUM_IDX_SYSTEM,
};

static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
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
	TABLE_NAME_SERVERS,                // tableName
	"ip_address",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"nickname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"port",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"polling_interval_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"retry_interval_sec",              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_SERVERS =
  sizeof(COLUMN_DEF_SERVERS) / sizeof(ColumnDef);

enum {
	IDX_SERVERS_ID,
	IDX_SERVERS_TYPE,
	IDX_SERVERS_HOSTNAME,
	IDX_SERVERS_IP_ADDRESS,
	IDX_SERVERS_NICKNAME,
	IDX_SERVERS_PORT,
	IDX_SERVERS_POLLING_INTERVAL_SEC,
	IDX_SERVERS_RETRY_INTERVAL_SEC,
	NUM_IDX_SERVERS,
};

struct DBClientConfig::PrivateContext
{
	static MutexLock mutex;
	static bool   initialized;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
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
MutexLock DBClientConfig::PrivateContext::mutex;
bool   DBClientConfig::PrivateContext::initialized = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientConfig::reset(void)
{
	resetDBInitializedFlags();
}

void DBClientConfig::parseCommandLineArgument(CommandLineArg &cmdArg)
{
	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &arg = cmdArg[i];
		if (arg == "--config-db") {
			ASURA_ASSERT(i < cmdArg.size()-1,
			             "--config-db needs database path.");
			i++;
			string dbPath = cmdArg[i];
			MLPL_INFO("Configuration DB: %s\n", dbPath.c_str());
			DBAgentSQLite3::defineDBPath(DB_DOMAIN_ID_CONFIG,
			                             dbPath);
		}
	}
}

DBClientConfig::DBClientConfig(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	m_ctx->lock();
	if (!m_ctx->initialized) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		prepareSetupFunction();
	}
	m_ctx->unlock();
	setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID_CONFIG));
}

DBClientConfig::~DBClientConfig()
{
	if (m_ctx)
		delete m_ctx;
}

bool DBClientConfig::isFaceMySQLEnabled(void)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_FACE_MYSQL);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ASURA_ASSERT(!grpList.empty(), "Objtained Table: empty");
	return ItemDataUtils::getInt((*grpList.begin())->getItemAt(0));
}

void DBClientConfig::addTargetServer(MonitoringServerInfo *monitoringServerInfo)
{
	string condition = StringUtils::sprintf("id=%u",
	                                        monitoringServerInfo->id);
	VariableItemGroupPtr row;
	DBCLIENT_TRANSACTION_BEGIN() {
		if (!isRecordExisting(TABLE_NAME_SERVERS, condition)) {
			DBAgentInsertArg arg;
			arg.tableName = TABLE_NAME_SERVERS;
			arg.numColumns = NUM_COLUMNS_SERVERS;
			arg.columnDefs = COLUMN_DEF_SERVERS;
			row->ADD_NEW_ITEM(Int, monitoringServerInfo->id);
			row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
			row->ADD_NEW_ITEM(String,
			                  monitoringServerInfo->hostName);
			row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->ipAddress);
			row->ADD_NEW_ITEM(String,
			                  monitoringServerInfo->nickname);
			row->ADD_NEW_ITEM(Int, monitoringServerInfo->port);
			row->ADD_NEW_ITEM
			  (Int, monitoringServerInfo->pollingIntervalSec);
			row->ADD_NEW_ITEM
			  (Int, monitoringServerInfo->retryIntervalSec);
			arg.row = row;
			insert(arg);
		} else {
			DBAgentUpdateArg arg;
			arg.tableName = TABLE_NAME_SERVERS;
			arg.columnDefs = COLUMN_DEF_SERVERS;

			row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
			arg.columnIndexes.push_back(IDX_SERVERS_TYPE);

			row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->hostName);
			arg.columnIndexes.push_back(IDX_SERVERS_HOSTNAME);

			row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->ipAddress);
			arg.columnIndexes.push_back(IDX_SERVERS_IP_ADDRESS);

			row->ADD_NEW_ITEM(String,
			                  monitoringServerInfo->nickname);
			arg.columnIndexes.push_back(IDX_SERVERS_NICKNAME);

			row->ADD_NEW_ITEM(Int, monitoringServerInfo->port);
			arg.columnIndexes.push_back(IDX_SERVERS_PORT);

			row->ADD_NEW_ITEM
			  (Int, monitoringServerInfo->pollingIntervalSec);
			arg.columnIndexes.push_back
			  (IDX_SERVERS_POLLING_INTERVAL_SEC);
			row->ADD_NEW_ITEM
			  (Int, monitoringServerInfo->retryIntervalSec);
			arg.columnIndexes.push_back
			  (IDX_SERVERS_RETRY_INTERVAL_SEC);
			arg.row = row;
			update(arg);
		}
	} DBCLIENT_TRANSACTION_END();
}

void DBClientConfig::getTargetServers
  (MonitoringServerInfoList &monitoringServers)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SERVERS;
	arg.columnDefs = COLUMN_DEF_SERVERS;
	arg.columnIndexes.push_back(IDX_SERVERS_ID);
	arg.columnIndexes.push_back(IDX_SERVERS_TYPE);
	arg.columnIndexes.push_back(IDX_SERVERS_HOSTNAME);
	arg.columnIndexes.push_back(IDX_SERVERS_IP_ADDRESS);
	arg.columnIndexes.push_back(IDX_SERVERS_NICKNAME);
	arg.columnIndexes.push_back(IDX_SERVERS_PORT);
	arg.columnIndexes.push_back(IDX_SERVERS_POLLING_INTERVAL_SEC);
	arg.columnIndexes.push_back(IDX_SERVERS_RETRY_INTERVAL_SEC);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		monitoringServers.push_back(MonitoringServerInfo());
		MonitoringServerInfo &svInfo = monitoringServers.back();

		svInfo.id        = GET_INT_FROM_GRP(itemGroup, idx++);
		int type         = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.type      = static_cast<MonitoringSystemType>(type);
		svInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.ipAddress = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.nickname  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.port      = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.pollingIntervalSec
		                 = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.retryIntervalSec
		                 = GET_INT_FROM_GRP(itemGroup, idx++);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientConfig::resetDBInitializedFlags(void)
{
	PrivateContext::initialized = false;
}

void DBClientConfig::tableInitializerSystem(DBAgent *dbAgent, void *data)
{
	const ColumnDef &columnDefFaceRestPort =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_FACE_REST_PORT];

	// insert default value
	DBAgentInsertArg insArg;
	insArg.tableName = TABLE_NAME_SYSTEM;
	insArg.numColumns = NUM_COLUMNS_SYSTEM;
	insArg.columnDefs = COLUMN_DEF_SYSTEM;
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, 0); // enable_face_mysql

	// face_reset_port
	row->ADD_NEW_ITEM(Int, atoi(columnDefFaceRestPort.defaultValue));

	insArg.row = row;
	dbAgent->insert(insArg);
}

void DBClientConfig::prepareSetupFunction(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_SYSTEM,
		NUM_COLUMNS_SYSTEM,
		COLUMN_DEF_SYSTEM,
		tableInitializerSystem,
	}, {
		TABLE_NAME_SERVERS,
		NUM_COLUMNS_SERVERS,
		COLUMN_DEF_SERVERS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		CONFIG_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBAgent::addSetupFunction(DB_DOMAIN_ID_CONFIG,
	                          dbSetupFunc, (void *)&DB_SETUP_FUNC_ARG);
}
