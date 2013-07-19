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

#include "ConfigManager.h"
#include "DBAgentFactory.h"
#include "DBClientAction.h"
#include "DBClientUtils.h"
#include "MutexLock.h"

using namespace mlpl;

enum ComparisonType {
	CMP_EQ,
	CMP_EQ_GT,
};

enum ActionLogExecFailureCode {
	ACTLOG_EXECFAIL_EXEC_FAILURE,
	ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
	ACTLOG_EXECFAIL_KILLED_TIMEOUT,
	ACTLOG_EXECFAIL_PIPE_READ_HUP,
	ACTLOG_EXECFAIL_PIPE_READ_ERR,
	ACTLOG_EXECFAIL_PIPE_WRITE_ERR,
	ACTLOG_EXECFAIL_UNEXPECTED_EXIT,
};

static const char *TABLE_NAME_ACTIONS     = "actions";
static const char *TABLE_NAME_ACTION_LOGS = "action_logs";

int DBClientAction::ACTION_DB_VERSION = 1;
const char *DBClientAction::DEFAULT_DB_NAME = "action";

static const ColumnDef COLUMN_DEF_ACTIONS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_status",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_severity",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_severity_comp_type",      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"action_type",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"path",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"timeout",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};
static const size_t NUM_COLUMNS_ACTIONS =
  sizeof(COLUMN_DEF_ACTIONS) / sizeof(ColumnDef);

enum {
	IDX_ACTIONS_ACTION_ID,
	IDX_ACTIONS_SERVER_ID,
	IDX_ACTIONS_HOST_ID,
	IDX_ACTIONS_HOST_GROUP_ID,
	IDX_ACTIONS_TRIGGER_ID,
	IDX_ACTIONS_TRIGGER_STATUS,
	IDX_ACTIONS_TRIGGER_SEVERITY,
	IDX_ACTIONS_SEVERITY_COMP_TYPE,
	IDX_ACTIONS_ACTION_TYPE,
	IDX_ACTIONS_PATH,
	IDX_ACTIONS_TIMEOUT,
	NUM_IDX_ACTIONS,
};

static const ColumnDef COLUMN_DEF_ACTION_LOGS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"action_log_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
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
	TABLE_NAME_ACTION_LOGS,            // tableName
	"starter_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"start_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"end_time",                        // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"exec_failure_code",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"exit_code",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const size_t NUM_COLUMNS_ACTION_LOGS =
  sizeof(COLUMN_DEF_ACTION_LOGS) / sizeof(ColumnDef);

enum {
	IDX_ACTION_LOGS_ACTION_LOG_ID,
	IDX_ACTION_LOGS_ACTION_ID,
	IDX_ACTION_LOGS_STATUS, 
	IDX_ACTION_LOGS_STARTER_ID,
	IDX_ACTION_LOGS_START_TIME,
	IDX_ACTION_LOGS_END_TIME,
	IDX_ACTION_LOGS_EXEC_FAILURE_CODE,
	IDX_ACTION_LOGS_EXIT_CODE,
	NUM_IDX_ACTION_LOGS,
};

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
{
	TABLE_NAME_ACTIONS,
	NUM_COLUMNS_ACTIONS,
	COLUMN_DEF_ACTIONS,
}, {
	TABLE_NAME_ACTION_LOGS,
	NUM_COLUMNS_ACTION_LOGS,
	COLUMN_DEF_ACTION_LOGS,
}
};
static const size_t NUM_TABLE_INFO =
sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

static DBClient::DBSetupFuncArg DB_ACTION_SETUP_FUNC_ARG = {
	DBClientAction::ACTION_DB_VERSION,
	NUM_TABLE_INFO,
	DB_TABLE_INFO,
};

struct DBClientAction::PrivateContext
{
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientAction::init(void)
{
	HATOHOL_ASSERT(NUM_COLUMNS_ACTIONS == NUM_IDX_ACTIONS,
	  "NUM_COLUMNS_ACTIONS: %zd, NUM_IDX_ACTIONS: %d",
	  NUM_COLUMNS_ACTIONS, NUM_IDX_ACTIONS);

	HATOHOL_ASSERT(NUM_COLUMNS_ACTION_LOGS == NUM_IDX_ACTION_LOGS,
	  "NUM_COLUMNS_ACTION_LOGS: %zd, NUM_IDX_ACTION_LOGS: %d",
	  NUM_COLUMNS_ACTION_LOGS, NUM_IDX_ACTION_LOGS);

	addDefaultDBInfo(
	  DB_DOMAIN_ID_ACTION, DEFAULT_DB_NAME, &DB_ACTION_SETUP_FUNC_ARG);
}

DBClientAction::DBClientAction(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientAction::~DBClientAction()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientAction::getActionList(const EventInfo &eventInfo,
                                   ActionDefList &actionDefList)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_TYPE]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_PATH]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TIMEOUT]);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		actionDefList.push_back(ActionDef());
		ActionDef &actionDef = actionDefList.back();

		// TODO: Check the NULL value
		// TODO: support working dir.
		actionDef.id      = GET_INT_FROM_GRP(itemGroup, idx++);
		actionDef.type    =
		   static_cast<ActionType>(GET_INT_FROM_GRP(itemGroup, idx++));
		actionDef.path    = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.timeout = GET_INT_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientAction::logStartExecAction(const ActionDef &actionDef)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBClientAction::logEndExecAction(const ExitChildInfo &exitChildInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBClientAction::logErrExecAction(const ActionDef &actionDef,
                                      const string &msg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
