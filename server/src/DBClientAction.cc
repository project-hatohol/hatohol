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

enum ActionLogStatus {
	ACTLOG_STAT_STARTED,
	ACTLOG_STAT_SUCCEEDED,
	ACTLOG_STAT_FAILED,
};

enum ActionLogExecFailureCode {
	ACTLOG_EXECFAIL_NONE,
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
	"working_dir",                     // columnName
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
	false,                             // canBeNull
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
	IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE,
	IDX_ACTIONS_ACTION_TYPE,
	IDX_ACTIONS_PATH,
	IDX_ACTIONS_WORKING_DIR,
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
	true,                              // canBeNull
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

void DBClientAction::addAction(const ActionDef &actionDef)
{
	VariableItemDataPtr item;
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.numColumns = NUM_COLUMNS_ACTIONS;
	arg.columnDefs = COLUMN_DEF_ACTIONS;

	row->ADD_NEW_ITEM(Int, 0); // We set 0 temporarily.

	// After item = new Item..() line, the reference count of ItemData is 2,
	// because substitution to 'item' increases the reference counter.
	// Instead, we use 'false' as the second argment of row->add() not to
	// increase the reference counter.
	// The reference count decreases when the other ItemData object is
	// substitute to 'item' and 'row' is destroyed. As a result,
	// newly create ItemData instances in this function will be destroyed
	// on the exit of this function.
	item = new ItemUint64(actionDef.condition.serverId);
	if (!actionDef.condition.isEnable(ACTCOND_SERVER_ID))
		item->setNull();
	row->add(item, false);

	item = new ItemUint64(actionDef.condition.hostId);
	if (!actionDef.condition.isEnable(ACTCOND_HOST_ID))
		item->setNull();
	row->add(item, false);

	item = new ItemUint64(actionDef.condition.hostGroupId);
	if (!actionDef.condition.isEnable(ACTCOND_HOST_GROUP_ID))
		item->setNull();
	row->add(item, false);

	item = new ItemUint64(actionDef.condition.triggerId);
	if (!actionDef.condition.isEnable(ACTCOND_TRIGGER_ID))
		item->setNull();
	row->add(item, false);

	item = new ItemInt(actionDef.condition.triggerStatus);
	if (!actionDef.condition.isEnable(ACTCOND_TRIGGER_STATUS))
		item->setNull();
	row->add(item, false);

	item = new ItemInt(actionDef.condition.triggerSeverity);
	if (!actionDef.condition.isEnable(ACTCOND_TRIGGER_SEVERITY))
		item->setNull();
	row->add(item, false);

	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerSeverityCompType);
	row->ADD_NEW_ITEM(Int, actionDef.type);
	row->ADD_NEW_ITEM(String, actionDef.path);
	row->ADD_NEW_ITEM(String, actionDef.workingDir);
	row->ADD_NEW_ITEM(Int, actionDef.timeout);

	arg.row = row;

	DBCLIENT_TRANSACTION_BEGIN() {
		// Here we replace the value in ItemData object for Action ID
		// with the newly obtained value. It is generally
		// forbidden in Hatohol framework (such method is not prepared)
		// The following code uses 'const_cast'. In this context,
		// this is safe, because nobody uses it.
		const ItemData *itemActionId = row->getItemAt(0);
		*(const_cast<ItemData *>(itemActionId))
		  = *(new ItemInt(getNewActionId()));
		insert(arg);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::getActionList(const EventInfo &eventInfo,
                                   ActionDefList &actionDefList)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_GROUP_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY]);
	arg.pushColumn(
	  COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_TYPE]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_PATH]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_WORKING_DIR]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TIMEOUT]);

	// TODO: append where section

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

		actionDef.id = GET_INT_FROM_GRP(itemGroup, idx++);

		// conditions
		bool isNull;
		actionDef.condition.serverId =
		   GET_INT_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_SERVER_ID);

		actionDef.condition.hostId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_ID);

		actionDef.condition.hostGroupId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_GROUP_ID);

		actionDef.condition.triggerId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_ID);

		actionDef.condition.triggerStatus =
		   GET_INT_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_STATUS);

		actionDef.condition.triggerSeverity =
		   GET_INT_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_SEVERITY);

		actionDef.condition.triggerSeverityCompType =
		  static_cast<ComparisonType>
		    (GET_INT_FROM_GRP(itemGroup, idx++));

		actionDef.type =
		   static_cast<ActionType>(GET_INT_FROM_GRP(itemGroup, idx++));
		actionDef.path       = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.workingDir = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.timeout    = GET_INT_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientAction::logStartExecAction(const ActionDef &actionDef)
{
	VariableItemDataPtr item;
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.numColumns = NUM_COLUMNS_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	// We set 0 as an action log ID temporarily.
	row->ADD_NEW_ITEM(Uint64, 0); // action_log_id
	row->ADD_NEW_ITEM(Int, actionDef.id);
	row->ADD_NEW_ITEM(Int, ACTLOG_STAT_STARTED);
 	// TODO: set the appropriate the following starter ID.
	row->ADD_NEW_ITEM(Int, 0);  // status
	row->ADD_NEW_ITEM(Int, -1); // start_time: -1 means current time.

	// end_time
	item = new ItemInt(0);
	item->setNull();
	row->add(item, false);

	row->ADD_NEW_ITEM(Int, ACTLOG_EXECFAIL_NONE);

	// exit_code;
	item = new ItemInt(0);
	item->setNull();
	row->add(item, false);

	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		// See also the comment in addAction about the const cast.
		const ItemData *itemActionId = row->getItemAt(0);
		*(const_cast<ItemData *>(itemActionId))
		  = *(new ItemUint64(getNewActionLogId()));
		insert(arg);
	} DBCLIENT_TRANSACTION_END();
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
int DBClientAction::getNewActionId(void)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID]);
	arg.orderBy = COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID].columnName;
	arg.orderBy += " DESC";
	arg.limit = 1;

	// This function doesn't work without transaction.
	select(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return 1;
	return ItemDataUtils::getInt((*grpList.begin())->getItemAt(0));
}

uint64_t DBClientAction::getNewActionLogId(void)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.pushColumn(COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID]);
	arg.orderBy = "DESC";
	arg.limit = 1;

	// This function doesn't work without transaction.
	select(arg);

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return 1;
	return ItemDataUtils::getUint64((*grpList.begin())->getItemAt(0));
}
