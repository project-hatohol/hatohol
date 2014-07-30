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

#include <exception>
#include "Utils.h"
#include "ConfigManager.h"
#include "CacheServiceDBClient.h"
#include "DBAgentFactory.h"
#include "DBClientAction.h"
#include "DBClientHatohol.h"
#include "MutexLock.h"
#include "ItemGroupStream.h"
using namespace std;
using namespace mlpl;

const char *TABLE_NAME_ACTIONS     = "actions";
const char *TABLE_NAME_ACTION_LOGS = "action_logs";

const static guint DEFAULT_ACTION_DELETE_INTERVAL = 3600 * 1000; // msec(1hour)

// 8 -> 9: Add actions.onwer_user_id
int DBClientAction::ACTION_DB_VERSION = 9;
const char *DBClientAction::DEFAULT_DB_NAME = DBClientConfig::DEFAULT_DB_NAME;

static void operator>>(
  ItemGroupStream &itemGroupStream, ComparisonType &compType)
{
	compType = itemGroupStream.read<int, ComparisonType>();
}

static void operator>>(
  ItemGroupStream &itemGroupStream, ActionType &actionType)
{
	actionType = itemGroupStream.read<int, ActionType>();
}

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
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	"command",                         // columnName
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
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"owner_user_id",                   // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0", // USER_ID_SYSTEM             // defaultValue
},
};

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
	IDX_ACTIONS_COMMAND,
	IDX_ACTIONS_WORKING_DIR,
	IDX_ACTIONS_TIMEOUT,
	IDX_ACTIONS_OWNER_USER_ID,
	NUM_IDX_ACTIONS,
};

static const DBAgent::TableProfile tableProfileActions(
  TABLE_NAME_ACTIONS, COLUMN_DEF_ACTIONS,
  sizeof(COLUMN_DEF_ACTIONS), NUM_IDX_ACTIONS);

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
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	"queuing_time",                    // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
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
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
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
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"event_id",                        // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const DBAgent::TableProfile tableProfileActionLogs(
  TABLE_NAME_ACTION_LOGS, COLUMN_DEF_ACTION_LOGS,
  sizeof(COLUMN_DEF_ACTION_LOGS), NUM_IDX_ACTION_LOGS);

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
{
	&tableProfileActions,
}, {
	&tableProfileActionLogs,
}
};
static const size_t NUM_TABLE_INFO =
sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

static bool addColumnOwnerUserId(DBAgent *dbAgent)
{
	DBAgent::AddColumnsArg addColumnsArg(tableProfileActions);
	addColumnsArg.columnIndexes.push_back(
	  IDX_ACTIONS_OWNER_USER_ID);
	dbAgent->addColumns(addColumnsArg);
	return true;
}

static bool updateDB(DBAgent *dbAgent, int oldVer, void *data)
{
	if (oldVer <= 8) {
		if (!addColumnOwnerUserId(dbAgent))
			return false;
	}
	return true;
}

static const DBClient::DBSetupFuncArg DB_ACTION_SETUP_FUNC_ARG = {
	DBClientAction::ACTION_DB_VERSION,
	NUM_TABLE_INFO,
	DB_TABLE_INFO,
	&updateDB,
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

struct deleteNoOwnerActionsContext {
	guint timerId;
	guint idleEventId;
};
static deleteNoOwnerActionsContext *g_deleteActionCtx = NULL;

// ---------------------------------------------------------------------------
// LogEndExecActionArg
// ---------------------------------------------------------------------------
DBClientAction::LogEndExecActionArg::LogEndExecActionArg(void)
: logId(INVALID_ACTION_LOG_ID),
  status(ACTLOG_STAT_INVALID),
  exitCode(0),
  failureCode(ACTLOG_EXECFAIL_NONE),
  nullFlags(0)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientAction::init(void)
{
	registerSetupInfo(
	  DB_DOMAIN_ID_ACTION, DEFAULT_DB_NAME, &DB_ACTION_SETUP_FUNC_ARG);

	g_deleteActionCtx = new deleteNoOwnerActionsContext;
	g_deleteActionCtx->idleEventId = INVALID_EVENT_ID;
	g_deleteActionCtx->timerId = g_timeout_add(DEFAULT_ACTION_DELETE_INTERVAL,
	                                           deleteNoOwnerActionsCycl,
	                                           g_deleteActionCtx);
}

void DBClientAction::reset(void)
{
	// Now we assume that a DB server for this class is the same as that
	// for DBClientConfig. So we copy the connectInfo of it.
	DBConnectInfo connInfo = getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	setConnectInfo(DB_DOMAIN_ID_ACTION, connInfo);
}

void DBClientAction::stop(void)
{
	Utils::executeOnGLibEventLoop(stopIdleDeleteAction);
}

const char *DBClientAction::getTableNameActions(void)
{
	return TABLE_NAME_ACTIONS;
}

const char *DBClientAction::getTableNameActionLogs(void)
{
	return TABLE_NAME_ACTION_LOGS;
}

DBClientAction::DBClientAction(void)
: DBClient(DB_DOMAIN_ID_ACTION),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientAction::~DBClientAction()
{
	if (m_ctx)
		delete m_ctx;
}

HatoholError DBClientAction::addAction(ActionDef &actionDef,
                                       const OperationPrivilege &privilege)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	if (!privilege.has(OPPRVLG_CREATE_ACTION))
		return HTERR_NO_PRIVILEGE;

	// Basically an owner is the caller. However, USER_ID_SYSTEM can
	// create an action with any user ID. This is a mechanism for
	// internal system management or a test.
	UserIdType ownerUserId = userId;
	if (userId == USER_ID_SYSTEM)
		ownerUserId = actionDef.ownerUserId;

	DBAgent::InsertArg arg(tableProfileActions);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(actionDef.condition.serverId,
	        getNullFlag(actionDef, ACTCOND_SERVER_ID));
	arg.add(actionDef.condition.hostId,
	        getNullFlag(actionDef, ACTCOND_HOST_ID));
	arg.add(actionDef.condition.hostgroupId,
	        getNullFlag(actionDef, ACTCOND_HOST_GROUP_ID));
	arg.add(actionDef.condition.triggerId,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_ID));
	arg.add(actionDef.condition.triggerStatus,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_STATUS));
	arg.add(actionDef.condition.triggerSeverity,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	arg.add(actionDef.condition.triggerSeverityCompType,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	arg.add(actionDef.type);
	arg.add(actionDef.command);
	arg.add(actionDef.workingDir);
	arg.add(actionDef.timeout);
	arg.add(ownerUserId);

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		actionDef.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();

	return HTERR_OK;
}

HatoholError DBClientAction::getActionList(ActionDefList &actionDefList,
					   const ActionsQueryOption &option)
{
	DBAgent::SelectExArg arg(tableProfileActions);
	arg.add(IDX_ACTIONS_ACTION_ID);
	arg.add(IDX_ACTIONS_SERVER_ID);
	arg.add(IDX_ACTIONS_HOST_ID);
	arg.add(IDX_ACTIONS_HOST_GROUP_ID);
	arg.add(IDX_ACTIONS_TRIGGER_ID);
	arg.add(IDX_ACTIONS_TRIGGER_STATUS);
	arg.add(IDX_ACTIONS_TRIGGER_SEVERITY);
	arg.add(IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE);
	arg.add(IDX_ACTIONS_ACTION_TYPE);
	arg.add(IDX_ACTIONS_COMMAND);
	arg.add(IDX_ACTIONS_WORKING_DIR);
	arg.add(IDX_ACTIONS_TIMEOUT);
	arg.add(IDX_ACTIONS_OWNER_USER_ID);

	// condition
	arg.condition = option.getCondition();

	// Order By
	arg.orderBy = option.getOrderBy();

	// Limit and Offset
	arg.limit = option.getMaximumNumber();
	arg.offset = option.getOffset();

	if (!arg.limit && arg.offset)
		return HTERR_OFFSET_WITHOUT_LIMIT;

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	ActionUserIdSet userIdSet;
	getActionUser(userIdSet);

	if (userIdSet.empty())
	        return HTERR_OK;

	// convert a format of the query result.
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		ActionDef actionDef;

		itemGroupStream >> actionDef.id;

		// conditions
		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_SERVER_ID);
		itemGroupStream >> actionDef.condition.serverId;

		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_HOST_ID);
		itemGroupStream >> actionDef.condition.hostId;

		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_HOST_GROUP_ID);
		itemGroupStream >> actionDef.condition.hostgroupId;

		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_TRIGGER_ID);
		itemGroupStream >> actionDef.condition.triggerId;

		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_TRIGGER_STATUS);
		itemGroupStream >> actionDef.condition.triggerStatus;

		if (!itemGroupStream.getItem()->isNull())
			actionDef.condition.enable(ACTCOND_TRIGGER_SEVERITY);
		itemGroupStream >> actionDef.condition.triggerSeverity;

		itemGroupStream >> actionDef.condition.triggerSeverityCompType;
		itemGroupStream >> actionDef.type;
		itemGroupStream >> actionDef.command;
		itemGroupStream >> actionDef.workingDir;
		itemGroupStream >> actionDef.timeout;
		itemGroupStream >> actionDef.ownerUserId;

		const UserIdType id = actionDef.ownerUserId;
		if (userIdSet.isValidActionOwnerId(id))
			actionDefList.push_back(actionDef);
	}

	return HTERR_OK;
}

HatoholError DBClientAction::deleteActions(const ActionIdList &idList,
                                           const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForDelete(privilege);
	if (err != HTERR_OK)
		return err;

	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	DBAgent::DeleteArg arg(tableProfileActions);
	const ColumnDef &colId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID];
	arg.condition = StringUtils::sprintf("%s in (", colId.columnName);
	ActionIdListConstIterator it = idList.begin();
	while (true) {
		const int id = *it;
		arg.condition += StringUtils::sprintf("%d", id);
		++it;
		if (it == idList.end())
			break;
		arg.condition += ",";
	}
	arg.condition += ")";

	// In this point, the caller must have OPPRVLG_DELETE_ACTION,
	// becase it is checked in checkPrevilegeForDelete().
	if (!privilege.has(OPPRVLG_DELETE_ALL_ACTION)) {
		arg.condition += StringUtils::sprintf(
		  " AND %s=%" FMT_USER_ID,
		  COLUMN_DEF_ACTIONS[IDX_ACTIONS_OWNER_USER_ID].columnName,
		  privilege.getUserId());
	}

	uint64_t numAffectedRows = 0;
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
		numAffectedRows = getDBAgent()->getNumberOfAffectedRows();
	} DBCLIENT_TRANSACTION_END();

	// Check the result
	if (numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

void DBClientAction::deleteNoOwnerActions()
{
	ActionIdList actionIdList;

	DBAgent::SelectExArg arg(tableProfileActions);
	arg.add(IDX_ACTIONS_ACTION_ID);
	arg.add(IDX_ACTIONS_OWNER_USER_ID);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	ActionUserIdSet userIdSet;
	getActionUser(userIdSet);

	if (userIdSet.empty())
	        return;

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		ActionDef actionDef;
		uint64_t  actionId;

		itemGroupStream >> actionId;
		itemGroupStream >> actionDef.ownerUserId;

		UserIdType id = actionDef.ownerUserId;
		if (!userIdSet.isValidActionOwnerId(id))
		        actionIdList.push_back(actionId);
	}
	if (actionIdList.empty()){
	        return;
	}

	OperationPrivilege privilege(OPPRVLG_DELETE_ALL_ACTION);
	privilege.setUserId(USER_ID_SYSTEM);

	deleteActions(actionIdList , privilege);

	return;
}

uint64_t DBClientAction::createActionLog(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  ActionLogExecFailureCode failureCode, ActionLogStatus initialStatus)
{
	DBAgent::InsertArg arg(tableProfileActionLogs);
	arg.row->addNewItem(AUTO_INCREMENT_VALUE_U64);
	arg.row->addNewItem(actionDef.id);

	// status
	ActionLogStatus status;
	if (failureCode == ACTLOG_EXECFAIL_NONE)
		status = initialStatus;
	else
		status = ACTLOG_STAT_FAILED;
	arg.row->addNewItem(status);

	// TODO: set the appropriate the following starter ID.
	int starterId = 0;
	arg.row->addNewItem(starterId);

	// queuing_time
	const int dummyTime = 0;
	if (initialStatus == ACTLOG_STAT_QUEUING)
		arg.row->addNewItem(CURR_DATETIME);
	else
		arg.row->addNewItem(dummyTime, ITEM_DATA_NULL);
	arg.row->addNewItem(CURR_DATETIME);     // start_time

	// end_time
	if (failureCode == ACTLOG_EXECFAIL_NONE)
		arg.row->addNewItem(dummyTime, ITEM_DATA_NULL);
	else
		arg.row->addNewItem(CURR_DATETIME);

	arg.row->addNewItem(failureCode);
	arg.row->addNewItem(dummyTime, ITEM_DATA_NULL); // exit_code

	// server ID and event ID
	arg.row->addNewItem(eventInfo.serverId);
	arg.row->addNewItem(eventInfo.id);

	uint64_t logId;
	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		logId = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
	return logId;
}

void DBClientAction::logEndExecAction(const LogEndExecActionArg &logArg)
{
	DBAgent::UpdateArg arg(tableProfileActionLogs);

	const char *actionLogIdColumnName = 
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" PRIu64,
	                                     actionLogIdColumnName,
	                                     logArg.logId);
	// status
	arg.add(IDX_ACTION_LOGS_STATUS, logArg.status);
	if (!(logArg.nullFlags & ACTLOG_FLAG_END_TIME))
		arg.add(IDX_ACTION_LOGS_END_TIME, CURR_DATETIME);

	// exec_failure_code
	arg.add(IDX_ACTION_LOGS_EXEC_FAILURE_CODE, logArg.failureCode);

	// exit_code
	if (!(logArg.nullFlags & ACTLOG_FLAG_EXIT_CODE))
		arg.add(IDX_ACTION_LOGS_EXIT_CODE, logArg.exitCode);

	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::updateLogStatusToStart(uint64_t logId)
{
	DBAgent::UpdateArg arg(tableProfileActionLogs);

	const char *actionLogIdColumnName = 
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" PRIu64,
	                                     actionLogIdColumnName, logId);
	arg.add(IDX_ACTION_LOGS_STATUS, ACTLOG_STAT_STARTED);
	arg.add(IDX_ACTION_LOGS_START_TIME, CURR_DATETIME);

	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientAction::getLog(ActionLog &actionLog, uint64_t logId)
{
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColName = def[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	string condition = StringUtils::sprintf("%s=%" PRIu64,
	                                        idColName, logId);
	return getLog(actionLog, condition);
}

bool DBClientAction::getLog(ActionLog &actionLog,
                            const ServerIdType &serverId, uint64_t eventId)
{
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColNameSvId = def[IDX_ACTION_LOGS_SERVER_ID].columnName;
	const char *idColNameEvtId = def[IDX_ACTION_LOGS_EVENT_ID].columnName;
	string condition = StringUtils::sprintf(
	  "%s=%" FMT_SERVER_ID " AND %s=%" PRIu64,
	  idColNameSvId, serverId, idColNameEvtId, eventId);
	return getLog(actionLog, condition);
}

bool DBClientAction::isIssueSenderEnabled(void)
{
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setActionType(ACTION_ISSUE_SENDER);
	option.setMaximumNumber(1);
	getActionList(actionDefList, option);
	return (actionDefList.size() == 1);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemDataNullFlagType DBClientAction::getNullFlag
  (const ActionDef &actionDef, ActionConditionEnableFlag enableFlag)
{
	if (actionDef.condition.isEnable(enableFlag))
		return ITEM_DATA_NOT_NULL;
	else
		return ITEM_DATA_NULL;
}

static void takeTriggerInfo(TriggerInfo &triggerInfo,
  const ServerIdType &serverId, const TriggerIdType &triggerId)
{
	DBClientHatohol dbClientHatohol;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setTargetId(triggerId);
	dbClientHatohol.getTriggerInfo(triggerInfo, option);
}

static void getHostgroupIdStringList(string &stringHostgroupId,
  const ServerIdType &serverId, const HostIdType &hostId)
{
	DBClientHatohol dbClientHatohol;
	HostgroupElementList hostgroupElementList;
	HostgroupElementQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setTargetHostId(hostId);
	dbClientHatohol.getHostgroupElementList(hostgroupElementList,option);

	HostgroupElementListIterator it = hostgroupElementList.begin();
	for(; it != hostgroupElementList.end(); ++it) {
		HostgroupElement hostgroupElement = *it;
		stringHostgroupId += StringUtils::sprintf(
		  "%" FMT_HOST_GROUP_ID ",", hostgroupElement.groupId);
	}
	if (!stringHostgroupId.empty())
		stringHostgroupId.erase(--stringHostgroupId.end());
	else
		stringHostgroupId = "0";
}

bool DBClientAction::getLog(ActionLog &actionLog, const string &condition)
{
	DBAgent::SelectExArg arg(tableProfileActionLogs);
	arg.condition = condition;
	arg.add(IDX_ACTION_LOGS_ACTION_LOG_ID);
	arg.add(IDX_ACTION_LOGS_ACTION_ID);
	arg.add(IDX_ACTION_LOGS_STATUS); 
	arg.add(IDX_ACTION_LOGS_STARTER_ID);
	arg.add(IDX_ACTION_LOGS_QUEUING_TIME);
	arg.add(IDX_ACTION_LOGS_START_TIME);
	arg.add(IDX_ACTION_LOGS_END_TIME);
	arg.add(IDX_ACTION_LOGS_EXEC_FAILURE_CODE);
	arg.add(IDX_ACTION_LOGS_EXIT_CODE);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	size_t numGrpList = grpList.size();

	// Not found
	if (numGrpList == 0)
		return false;

	ItemGroupStream itemGroupStream(*grpList.begin());
	actionLog.nullFlags = 0;

	itemGroupStream >> actionLog.id;
	itemGroupStream >> actionLog.actionId;
	itemGroupStream >> actionLog.status;
	itemGroupStream >> actionLog.starterId;

	// queing time
	if (itemGroupStream.getItem()->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_QUEUING_TIME;
	itemGroupStream >> actionLog.queuingTime;

	// start time
	if (itemGroupStream.getItem()->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_START_TIME;
	itemGroupStream >> actionLog.startTime;

	// end time
	if (itemGroupStream.getItem()->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_END_TIME;
	itemGroupStream >> actionLog.endTime;

	// failure code
	itemGroupStream >> actionLog.failureCode;

	// exit code
	if (itemGroupStream.getItem()->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_EXIT_CODE;
	itemGroupStream >> actionLog.exitCode;

	return true;
}

HatoholError DBClientAction::checkPrivilegeForDelete(
  const OperationPrivilege &privilege)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	if (privilege.has(OPPRVLG_DELETE_ALL_ACTION))
		return HTERR_OK;

	if (!privilege.has(OPPRVLG_DELETE_ACTION))
		return HTERR_NO_PRIVILEGE;

	return HTERR_OK;
}

void DBClientAction::getActionUser(UserIdSet &userIdSet)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	dbUser->getUserIdSet(userIdSet);

	return;
}

gboolean DBClientAction::deleteNoOwnerActionsExec(gpointer data)
{
	deleteNoOwnerActionsContext *g_deleteActionCtx = static_cast<deleteNoOwnerActionsContext *>(data);
	CacheServiceDBClient cache;
	DBClientAction *dbAction = cache.getAction();
	try {
		dbAction->deleteNoOwnerActions();
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
	}
	g_deleteActionCtx->idleEventId = INVALID_EVENT_ID;
	g_deleteActionCtx->timerId = g_timeout_add(DEFAULT_ACTION_DELETE_INTERVAL, 
	                                           deleteNoOwnerActionsCycl,
	                                           g_deleteActionCtx);
	return G_SOURCE_REMOVE;
}

gboolean DBClientAction::deleteNoOwnerActionsCycl(gpointer data)
{
	deleteNoOwnerActionsContext *g_deleteActionCtx = static_cast<deleteNoOwnerActionsContext *>(data);
	g_deleteActionCtx->timerId = INVALID_EVENT_ID;
	g_deleteActionCtx->idleEventId = Utils::setGLibIdleEvent(deleteNoOwnerActionsExec,
	                                                         g_deleteActionCtx);
	return G_SOURCE_REMOVE;
}

void DBClientAction::stopIdleDeleteAction(gpointer data)
{
	if (g_deleteActionCtx->timerId != INVALID_EVENT_ID)
		g_source_remove(g_deleteActionCtx->timerId);
	if (g_deleteActionCtx->idleEventId != INVALID_EVENT_ID)
		g_source_remove(g_deleteActionCtx->idleEventId);
}

// ---------------------------------------------------------------------------
// ActionsQueryOption
// ---------------------------------------------------------------------------
struct ActionsQueryOption::PrivateContext {
	static const string conditionTemplate;

	// TODO: should have replica?
	const EventInfo *eventInfo;
	ActionType type;

	PrivateContext()
	: eventInfo(NULL), type(ACTION_USER_DEFINED)
	{
	}

	static string makeConditionTemplate(void);
	string getActionTypeCondition(void);
};

const string ActionsQueryOption::PrivateContext::conditionTemplate
  = makeConditionTemplate();

string ActionsQueryOption::PrivateContext::makeConditionTemplate(void)
{
	string cond;

	// server_id;
	const ColumnDef &colDefSvId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_SERVER_ID];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%d))",
	  colDefSvId.columnName, colDefSvId.columnName);
	cond += " AND ";

	// host_id;
	const ColumnDef &colDefHostId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_ID];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%" PRIu64 "))",
	  colDefHostId.columnName, colDefHostId.columnName);
	cond += " AND ";

	// host_group_id;
	const ColumnDef &colDefHostGrpId =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_GROUP_ID];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR %s IN (%%s))",
	  colDefHostGrpId.columnName, colDefHostGrpId.columnName);
	cond += " AND ";

	// trigger_id
	const ColumnDef &colDefTrigId =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_ID];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%" PRIu64 "))",
	  colDefTrigId.columnName, colDefTrigId.columnName);
	cond += " AND ";

	// trigger_status
	const ColumnDef &colDefTrigStat =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_STATUS];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%%d))",
	  colDefTrigStat.columnName, colDefTrigStat.columnName);
	cond += " AND ";

	// trigger_severity
	const ColumnDef &colDefTrigSeve =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY];
	const ColumnDef &colDefTrigSeveCmpType =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE];
	cond += StringUtils::sprintf(
	  "((%s IS NULL) OR (%s=%d AND %s=%%d) OR (%s=%d AND %s<=%%d))",
	  colDefTrigSeve.columnName,
	  colDefTrigSeveCmpType.columnName, CMP_EQ, colDefTrigSeve.columnName,
	  colDefTrigSeveCmpType.columnName, CMP_EQ_GT,
	  colDefTrigSeve.columnName);

	return cond;
}

ActionsQueryOption::ActionsQueryOption(const UserIdType &userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActionsQueryOption::ActionsQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActionsQueryOption::ActionsQueryOption(const ActionsQueryOption &src)
: DataQueryOption(src), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

ActionsQueryOption::~ActionsQueryOption()
{
	delete m_ctx;
}

void ActionsQueryOption::setTargetEventInfo(const EventInfo *eventInfo)
{
	m_ctx->eventInfo = eventInfo;
}

const EventInfo *ActionsQueryOption::getTargetEventInfo(void) const
{
	return m_ctx->eventInfo;
}

void ActionsQueryOption::setActionType(const ActionType &type)
{
	m_ctx->type = type;
}

const ActionType &ActionsQueryOption::getActionType(void)
{
	return m_ctx->type;
}

string ActionsQueryOption::PrivateContext::getActionTypeCondition(void)
{
	switch (type) {
	case ACTION_USER_DEFINED:
		return StringUtils::sprintf(
			 "(action_type>=0 AND action_type<%d)",
			 ACTION_ISSUE_SENDER);
	case ACTION_ALL:
		return string();
	default:
		return StringUtils::sprintf("action_type=%d", (int)type);
	}
}

string ActionsQueryOption::getCondition(void) const
{
	string cond;

	if (!has(OPPRVLG_GET_ALL_ACTION)) {
		cond += StringUtils::sprintf(
		  "%s=%" FMT_USER_ID,
		  COLUMN_DEF_ACTIONS[IDX_ACTIONS_OWNER_USER_ID].columnName,
		  getUserId());
	}

	// filter by action type
	string actionTypeCondition = m_ctx->getActionTypeCondition();
	if (!actionTypeCondition.empty()) {
		if (!cond.empty())
			cond += " AND ";
		cond += actionTypeCondition;
	}

	// filter by EventInfo
	const EventInfo *eventInfo = m_ctx->eventInfo;
	if (!eventInfo)
		return cond;

	HATOHOL_ASSERT(!m_ctx->conditionTemplate.empty(),
	               "ActionDef condition template is empty.");
	TriggerInfo triggerInfo;
	// TODO: eventInfo should always be filled before this function
	//       is called. (The conditional branch here is not good)
	if ((!eventInfo->hostId) &&
	   (eventInfo->severity == TRIGGER_SEVERITY_UNKNOWN)) {
		takeTriggerInfo(
		  triggerInfo, eventInfo->serverId, eventInfo->triggerId);
	} else {
		triggerInfo.serverId = eventInfo->serverId;
		triggerInfo.hostId   = eventInfo->hostId;
		triggerInfo.severity = eventInfo->severity;
	}
	string hostgroupIdList;
	getHostgroupIdStringList(hostgroupIdList,
	  triggerInfo.serverId, triggerInfo.hostId);
	if (!cond.empty())
		cond += " AND ";
	cond += StringUtils::sprintf(m_ctx->conditionTemplate.c_str(),
	                       eventInfo->serverId,
	                       triggerInfo.hostId,
	                       hostgroupIdList.c_str(),
	                       eventInfo->triggerId,
	                       eventInfo->status,
	                       triggerInfo.severity,
	                       triggerInfo.severity);
	return cond;
}

bool ActionDef::parseIssueSenderCommand(IssueTrackerIdType &trackerId) const
{
	int ret = sscanf(command.c_str(), "%" FMT_ISSUE_TRACKER_ID, &trackerId);
	return ret == 1;
}


// ---------------------------------------------------------------------------
// ActionUserIdSet
// ---------------------------------------------------------------------------
bool ActionUserIdSet::isValidActionOwnerId(const UserIdType id)
{
	/*
	 * determine whether Action Owner User ID is invalid or valid
	 * if OwnerId is registration in the User tables -> true
	 * if there is no registration in the User tables -> false
	 * but , OwnerId is USER_ID_SYSTEM -> true
	 */
	if (find(id) == end()) {
		if (id != USER_ID_SYSTEM)
			return false;
	}
	return true;
}
