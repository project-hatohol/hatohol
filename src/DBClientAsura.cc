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
#include "DBClientAsura.h"
#include "ConfigManager.h"
#include "DBClientUtils.h"

static const char *TABLE_NAME_TRIGGERS = "triggers";

static const ColumnDef COLUMN_DEF_TRIGGERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_ns",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
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
	TABLE_NAME_TRIGGERS,               // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

static const size_t NUM_COLUMNS_TRIGGERS =
  sizeof(COLUMN_DEF_TRIGGERS) / sizeof(ColumnDef);

enum {
	IDX_TRIGGERS_ID,
	IDX_TRIGGERS_STATUS,
	IDX_TRIGGERS_SEVERITY,
	IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
	IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
	IDX_TRIGGERS_SERVER_ID,
	IDX_TRIGGERS_HOST_ID,
	IDX_TRIGGERS_HOSTNAME,
	IDX_TRIGGERS_BRIEF,
	NUM_IDX_TRIGGERS,
};

struct DBClientAsura::PrivateContext
{
	static GMutex mutex;
	static bool   initialized;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
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
GMutex DBClientAsura::PrivateContext::mutex;
bool   DBClientAsura::PrivateContext::initialized = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientAsura::resetDBInitializedFlags(void)
{
	PrivateContext::initialized = false;
}

DBClientAsura::DBClientAsura(void)
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
	setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID_ASURA));
}

DBClientAsura::~DBClientAsura()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientAsura::addTriggerInfo(TriggerInfo *triggerInfo)
{
	string condition = StringUtils::sprintf("id=%"PRIu64, triggerInfo->id);
	DBCLIENT_TRANSACTION_BEGIN() {
		if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
			DBAgentInsertArg arg;
			arg.tableName = TABLE_NAME_TRIGGERS;
			arg.numColumns = NUM_COLUMNS_TRIGGERS;
			arg.columnDefs = COLUMN_DEF_TRIGGERS;
			arg.row->ADD_NEW_ITEM(Uint64, triggerInfo->id);
			arg.row->ADD_NEW_ITEM(Int, triggerInfo->status);
			arg.row->ADD_NEW_ITEM(Int, triggerInfo->severity),
			arg.row->ADD_NEW_ITEM
			  (Int, triggerInfo->lastChangeTime.tv_sec); 
			arg.row->ADD_NEW_ITEM
			  (Int, triggerInfo->lastChangeTime.tv_nsec); 
			arg.row->ADD_NEW_ITEM(Int, triggerInfo->serverId);
			arg.row->ADD_NEW_ITEM(String, triggerInfo->hostId);
			arg.row->ADD_NEW_ITEM(String, triggerInfo->hostName);
			arg.row->ADD_NEW_ITEM(String, triggerInfo->brief);
			insert(arg);
		} else {
			DBAgentUpdateArg arg;
			arg.tableName = TABLE_NAME_TRIGGERS;
			arg.columnDefs = COLUMN_DEF_TRIGGERS;

			arg.row->ADD_NEW_ITEM(Int, triggerInfo->status);
			arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);

			arg.row->ADD_NEW_ITEM(Int, triggerInfo->severity);
			arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);

			arg.row->ADD_NEW_ITEM
			  (Int, triggerInfo->lastChangeTime.tv_sec); 
			arg.columnIndexes.push_back
			  (IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);

			arg.row->ADD_NEW_ITEM
			  (Int, triggerInfo->lastChangeTime.tv_nsec); 
			arg.columnIndexes.push_back
			  (IDX_TRIGGERS_LAST_CHANGE_TIME_NS);

			arg.row->ADD_NEW_ITEM(Int, triggerInfo->serverId);
			arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);

			arg.row->ADD_NEW_ITEM(String, triggerInfo->hostId);
			arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);

			arg.row->ADD_NEW_ITEM(String, triggerInfo->hostName);
			arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);

			arg.row->ADD_NEW_ITEM(String, triggerInfo->brief);
			arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);
			update(arg);
		}
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAsura::getTriggerInfoList(TriggerInfoList &triggerInfoList)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	arg.columnDefs = COLUMN_DEF_TRIGGERS;
	arg.columnIndexes.push_back(IDX_TRIGGERS_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_NS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);
	arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		triggerInfoList.push_back(TriggerInfo());
		TriggerInfo &trigInfo = triggerInfoList.back();

		trigInfo.id        = GET_UINT64_FROM_GRP(itemGroup, idx++);
		int status         = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.status    = static_cast<TriggerStatusType>(status);
		int severity       = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.severity  = static_cast<TriggerSeverityType>(severity);
		trigInfo.lastChangeTime.tv_sec = 
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.lastChangeTime.tv_nsec =
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.serverId  = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.hostId    = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientAsura::prepareSetupFunction(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_TRIGGERS,
		NUM_COLUMNS_TRIGGERS,
		COLUMN_DEF_TRIGGERS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBAgent::addSetupFunction(DB_DOMAIN_ID_ASURA,
	                          dbSetupFunc, (void *)&DB_SETUP_FUNC_ARG);
}
