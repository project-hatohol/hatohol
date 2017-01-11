/*
 * Copyright (C) 2013-2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <exception>
#include <SeparatorInjector.h>
#include "Utils.h"
#include "ConfigManager.h"
#include "ThreadLocalDBCache.h"
#include "DBAgentFactory.h"
#include "DBTablesAction.h"
#include "DBTablesMonitoring.h"
#include "Mutex.h"
#include "ItemGroupStream.h"
#include "UnifiedDataStore.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

const char *TABLE_NAME_ACTIONS     = "actions";
const char *TABLE_NAME_ACTION_LOGS = "action_logs";

const static guint DEFAULT_ACTION_DELETE_INTERVAL_MSEC = 3600 * 1000; // 1hour

// 8 -> 9: Add actions.onwer_user_id
// -> 1.0
//   * action_def.trigger_id    -> VARCHAR
//   * action_def.host_group_id -> VARCHAR
//   * action_logs.id           -> VARCHAR
const int DBTablesAction::ACTION_DB_VERSION =
  DBTables::Version::getPackedVer(0, 1, 0);

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

class ActionUserIdSet : public UserIdSet {
public:
	bool isValidActionOwnerId(const UserIdType id);

	static void get(UserIdSet &userIdSet);
};

class ActionValidator {
public:
	ActionValidator();
	bool isValid(const ActionDef &actionDef);

protected:
	bool isValidIncidentTracker(const ActionDef &actionDef);

private:
	ActionUserIdSet      m_userIdSet;
	IncidentTrackerIdSet m_incidentTrackerIdSet;
};

static const ColumnDef COLUMN_DEF_ACTIONS[] = {
{
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_id_in_server",               // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_status",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_severity",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"trigger_severity_comp_type",      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"action_type",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"command",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"working_dir",                     // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"timeout",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
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

static const DBAgent::TableProfile tableProfileActions =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_ACTIONS,
			    COLUMN_DEF_ACTIONS,
			    NUM_IDX_ACTIONS);

static const ColumnDef COLUMN_DEF_ACTION_LOGS[] = {
{
	"action_log_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"starter_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"queuing_time",                    // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"start_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"end_time",                        // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"exec_failure_code",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	"exit_code",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	"event_id",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const DBAgent::TableProfile tableProfileActionLogs =
  DBAGENT_TABLEPROFILE_INIT(TABLE_NAME_ACTION_LOGS,
			    COLUMN_DEF_ACTION_LOGS,
			    NUM_IDX_ACTION_LOGS);

static bool addColumnOwnerUserId(DBAgent &dbAgent)
{
	DBAgent::AddColumnsArg addColumnsArg(tableProfileActions);
	addColumnsArg.columnIndexes.push_back(
	  IDX_ACTIONS_OWNER_USER_ID);
	dbAgent.addColumns(addColumnsArg);
	return true;
}

static bool updateDB(
  DBAgent &dbAgent, const DBTables::Version &oldPackedVer, void *data)
{
	const int oldVer = oldPackedVer.minorVer;
	if (oldVer <= 8) {
		if (!addColumnOwnerUserId(dbAgent))
			return false;
	}
	return true;
}

struct DBTablesAction::Impl
{
	Impl(void)
	{
	}

	virtual ~Impl()
	{
	}
};

struct deleteInvalidActionsContext {
	guint timerId;
	guint idleEventId;
};
static deleteInvalidActionsContext *g_deleteActionCtx = NULL;

// ---------------------------------------------------------------------------
// LogEndExecActionArg
// ---------------------------------------------------------------------------
DBTablesAction::LogEndExecActionArg::LogEndExecActionArg(void)
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
void DBTablesAction::init(void)
{
	g_deleteActionCtx = new deleteInvalidActionsContext;
	g_deleteActionCtx->idleEventId = INVALID_EVENT_ID;
	g_deleteActionCtx->timerId = g_timeout_add(DEFAULT_ACTION_DELETE_INTERVAL_MSEC,
	                                           deleteInvalidActionsCycl,
	                                           g_deleteActionCtx);
}

void DBTablesAction::reset(void)
{
	getSetupInfo().initialized = false;
}

const DBTables::SetupInfo &DBTablesAction::getConstSetupInfo(void)
{
	return getSetupInfo();
}

void DBTablesAction::stop(void)
{
	Utils::executeOnGLibEventLoop(stopIdleDeleteAction);
}

const char *DBTablesAction::getTableNameActions(void)
{
	return TABLE_NAME_ACTIONS;
}

const char *DBTablesAction::getTableNameActionLogs(void)
{
	return TABLE_NAME_ACTION_LOGS;
}

DBTablesAction::DBTablesAction(DBAgent &dbAgent)
: DBTables(dbAgent, getSetupInfo()),
  m_impl(new Impl())
{
}

DBTablesAction::~DBTablesAction()
{
}

HatoholError DBTablesAction::addAction(ActionDef &actionDef,
                                       const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForAdd(privilege, actionDef);
	if (err != HTERR_OK)
		return err;

	// Basically an owner is the caller. However, USER_ID_SYSTEM can
	// create an action with any user ID. This is a mechanism for
	// internal system management or a test.
	UserIdType ownerUserId = privilege.getUserId();
	if (ownerUserId == USER_ID_SYSTEM)
		ownerUserId = actionDef.ownerUserId;

	// Owner of ACTION_INCIDENT_SENDER is always USER_ID_SYSTEM
	if (actionDef.type == ACTION_INCIDENT_SENDER)
		ownerUserId = USER_ID_SYSTEM;

	DBAgent::InsertArg arg(tableProfileActions);
	arg.add(AUTO_INCREMENT_VALUE);
	arg.add(actionDef.condition.serverId,
	        getNullFlag(actionDef, ACTCOND_SERVER_ID));
	arg.add(actionDef.condition.hostIdInServer,
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

	getDBAgent().runTransaction(arg, &actionDef.id);
	return HTERR_OK;
}

HatoholError DBTablesAction::updateAction(ActionDef &actionDef,
                                          const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForUpdate(privilege, actionDef);
	if (err != HTERR_OK)
		return err;

	DBAgent::UpdateArg arg(tableProfileActions);

	// Basically an owner is the caller. However, USER_ID_SYSTEM can
	// create an action with any user ID. This is a mechanism for
	// internal system management or a test.
	UserIdType ownerUserId = privilege.getUserId();
	if (ownerUserId == USER_ID_SYSTEM)
		ownerUserId = actionDef.ownerUserId;

	// Owner of ACTION_INCIDENT_SENDER is always USER_ID_SYSTEM
	if (actionDef.type == ACTION_INCIDENT_SENDER)
		ownerUserId = USER_ID_SYSTEM;

	const char *actionIdColumnName =
	  COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%d",
	                                     actionIdColumnName, actionDef.id);
	arg.add(IDX_ACTIONS_SERVER_ID, actionDef.condition.serverId,
	        getNullFlag(actionDef, ACTCOND_SERVER_ID));
	arg.add(IDX_ACTIONS_HOST_ID, actionDef.condition.hostIdInServer,
	        getNullFlag(actionDef, ACTCOND_HOST_ID));
	arg.add(IDX_ACTIONS_HOST_GROUP_ID, actionDef.condition.hostgroupId,
	        getNullFlag(actionDef, ACTCOND_HOST_GROUP_ID));
	arg.add(IDX_ACTIONS_TRIGGER_ID, actionDef.condition.triggerId,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_ID));
	arg.add(IDX_ACTIONS_TRIGGER_STATUS, actionDef.condition.triggerStatus,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_STATUS));
	arg.add(IDX_ACTIONS_TRIGGER_SEVERITY, actionDef.condition.triggerSeverity,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	arg.add(IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE,
	        actionDef.condition.triggerSeverityCompType,
	        getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	arg.add(IDX_ACTIONS_ACTION_TYPE, actionDef.type);
	arg.add(IDX_ACTIONS_COMMAND, actionDef.command);
	arg.add(IDX_ACTIONS_WORKING_DIR, actionDef.workingDir);
	arg.add(IDX_ACTIONS_TIMEOUT, actionDef.timeout);
	arg.add(IDX_ACTIONS_OWNER_USER_ID, ownerUserId);

	getDBAgent().runTransaction(arg);
	return HTERR_OK;
}

HatoholError DBTablesAction::getActionList(ActionDefList &actionDefList,
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

	getDBAgent().runTransaction(arg);

	// convert a format of the query result.
	ActionValidator validator;
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
		itemGroupStream >> actionDef.condition.hostIdInServer;

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

		if (validator.isValid(actionDef))
			actionDefList.push_back(actionDef);
	}

	return HTERR_OK;
}

static string makeIdListCondition(const ActionIdList &idList)
{
	string condition;
	const ColumnDef &colId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID];
	condition = StringUtils::sprintf("%s in (", colId.columnName);
	ActionIdListConstIterator it = idList.begin();
	while (true) {
		const int id = *it;
		condition += StringUtils::sprintf("%d", id);
		++it;
		if (it == idList.end())
			break;
		condition += ",";
	}
	condition += ")";
	return condition;
}

static string makeOwnerCondition(UserIdType userId)
{
	return StringUtils::sprintf(
		 "%s=%" FMT_USER_ID,
		 COLUMN_DEF_ACTIONS[IDX_ACTIONS_OWNER_USER_ID].columnName,
		 userId);
}

static string makeUserActionsCondition(const string &ownerCondition)
{
	if (ownerCondition.empty()) {
		return StringUtils::sprintf(
			"(action_type>=0 AND action_type<%d)",
			ACTION_INCIDENT_SENDER);
	} else {
		return StringUtils::sprintf(
			"(%s AND action_type>=0 AND action_type<%d)",
			ownerCondition.c_str(), ACTION_INCIDENT_SENDER);
	}
}

static string makeConditionForDelete(const ActionIdList &idList,
				     const OperationPrivilege &privilege)
{
	string condition = makeIdListCondition(idList);

	// In this point, the caller must have OPPRVLG_DELETE_ACTION,
	// because it is checked in checkPrivilegeForDelete().
	if (!privilege.has(OPPRVLG_DELETE_ALL_ACTION)) {
		if (!condition.empty())
			condition += " AND ";
		string ownerCondition
		  = makeOwnerCondition(privilege.getUserId());
		condition += "(" + makeUserActionsCondition(ownerCondition);
		if (privilege.has(OPPRVLG_DELETE_INCIDENT_SETTING)) {
			condition +=
			  StringUtils::sprintf(" OR action_type=%d",
					       ACTION_INCIDENT_SENDER);
		}
		condition += ")";
	}
	return condition;
}

HatoholError DBTablesAction::deleteActions(const ActionIdList &idList,
                                           const OperationPrivilege &privilege)
{
	HatoholError err = checkPrivilegeForDelete(privilege, idList);
	if (err != HTERR_OK)
		return err;

	if (idList.empty()) {
		MLPL_WARN("idList is empty.\n");
		return HTERR_INVALID_PARAMETER;
	}

	struct TrxProc : public DBAgent::TransactionProc {
		DBAgent::DeleteArg arg;
		uint64_t numAffectedRows;

		TrxProc (void)
		: arg(tableProfileActions),
		  numAffectedRows(0)
		{
		}

		void operator ()(DBAgent &dbAgent) override
		{
			dbAgent.deleteRows(arg);
			numAffectedRows = dbAgent.getNumberOfAffectedRows();
		}
	} trx;
	trx.arg.condition = makeConditionForDelete(idList, privilege);
	getDBAgent().runTransaction(trx);

	// Check the result
	if (trx.numAffectedRows != idList.size()) {
		MLPL_ERR("affectedRows: %" PRIu64 ", idList.size(): %zd\n",
		         trx.numAffectedRows, idList.size());
		return HTERR_DELETE_INCOMPLETE;
	}

	return HTERR_OK;
}

void DBTablesAction::deleteInvalidActions()
{
	ActionIdList actionIdList;

	DBAgent::SelectExArg arg(tableProfileActions);
	arg.add(IDX_ACTIONS_ACTION_ID);
	arg.add(IDX_ACTIONS_OWNER_USER_ID);
	arg.add(IDX_ACTIONS_ACTION_TYPE);
	arg.add(IDX_ACTIONS_COMMAND);

	getDBAgent().runTransaction(arg);

	ActionValidator validator;
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator itemGrpItr = grpList.begin();
	for (; itemGrpItr != grpList.end(); ++itemGrpItr) {
		ItemGroupStream itemGroupStream(*itemGrpItr);
		ActionDef actionDef;
		uint64_t  actionId;

		itemGroupStream >> actionId;
		itemGroupStream >> actionDef.ownerUserId;
		itemGroupStream >> actionDef.type;
		itemGroupStream >> actionDef.command;

		if (!validator.isValid(actionDef))
		        actionIdList.push_back(actionId);
	}
	if (actionIdList.empty())
	        return;

	OperationPrivilege privilege(OPPRVLG_DELETE_ALL_ACTION);
	privilege.setUserId(USER_ID_SYSTEM);

	deleteActions(actionIdList, privilege);
}

uint64_t DBTablesAction::createActionLog(
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

	ActionLogIdType logId;
	getDBAgent().runTransaction(arg, &logId);
	return logId;
}

void DBTablesAction::logEndExecAction(const LogEndExecActionArg &logArg)
{
	DBAgent::UpdateArg arg(tableProfileActionLogs);

	const char *actionLogIdColumnName =
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_ACTION_LOG_ID,
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

	getDBAgent().runTransaction(arg);
}

void DBTablesAction::updateLogStatusToStart(const ActionLogIdType &logId)
{
	DBAgent::UpdateArg arg(tableProfileActionLogs);

	const char *actionLogIdColumnName =
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_ACTION_LOG_ID,
	                                     actionLogIdColumnName, logId);
	arg.add(IDX_ACTION_LOGS_STATUS, ACTLOG_STAT_STARTED);
	arg.add(IDX_ACTION_LOGS_START_TIME, CURR_DATETIME);

	getDBAgent().runTransaction(arg);
}

void DBTablesAction::updateLogStatusToAborted(const ActionLogIdType &logId)
{
	DBAgent::UpdateArg arg(tableProfileActionLogs);

	const char *actionLogIdColumnName =
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%" FMT_ACTION_LOG_ID,
                                             actionLogIdColumnName, logId);
	arg.add(IDX_ACTION_LOGS_STATUS, ACTLOG_STAT_ABORTED);

	getDBAgent().update(arg);
}

bool DBTablesAction::getLog(ActionLog &actionLog, const ActionLogIdType &logId)
{
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColName = def[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	string condition = StringUtils::sprintf("%s=%" FMT_ACTION_LOG_ID,
	                                        idColName, logId);
	return getLog(actionLog, condition);
}

bool DBTablesAction::getLog(
  ActionLog &actionLog,
  const ServerIdType &serverId, const EventIdType &eventId)
{
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColNameSvId = def[IDX_ACTION_LOGS_SERVER_ID].columnName;
	const char *idColNameEvtId = def[IDX_ACTION_LOGS_EVENT_ID].columnName;
	DBTermCStringProvider rhs(*getDBAgent().getDBTermCodec());
	string condition = StringUtils::sprintf(
	  "%s=%" FMT_SERVER_ID " AND %s=%s",
	  idColNameSvId, serverId, idColNameEvtId, rhs(eventId));
	return getLog(actionLog, condition);
}

bool DBTablesAction::getTargetStatusesLogs(
  ActionLogList &actionLogList, const vector<int> &targetStatuses)
{
	SeparatorInjector commaInjector(",");
	string statuses;
	for (const auto &targetStatus: targetStatuses) {
		commaInjector(statuses);
		statuses += to_string(targetStatus);
	}
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColNameStat = def[IDX_ACTION_LOGS_STATUS].columnName;
	const char *idColNameEvtId = def[IDX_ACTION_LOGS_EVENT_ID].columnName;
	string condition = StringUtils::sprintf(
	                     "%s in (%s) AND %s!=''",
	                     idColNameStat, statuses.c_str(), idColNameEvtId);

	return getLogs(actionLogList, condition);
}

bool DBTablesAction::isIncidentSenderEnabled(void)
{
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setActionType(ACTION_INCIDENT_SENDER);
	// TODO: We have to fix the validator in getActionList() to re-enable
	//       the following line because the validator may drop the action
	//       after fetching it from DB.
	//option.setMaximumNumber(1);
	getActionList(actionDefList, option);
	return (actionDefList.size() > 0);
}

// ---------------------------------------------------------------------------
// Protected methods
DBTables::SetupInfo &DBTablesAction::getSetupInfo(void)
{
	static const TableSetupInfo DB_TABLE_INFO[] = {
	{
		&tableProfileActions,
	}, {
		&tableProfileActionLogs,
	}
	};

	static SetupInfo setupInfo = {
		DB_TABLES_ID_ACTION,
		ACTION_DB_VERSION,
		ARRAY_SIZE(DB_TABLE_INFO),
		DB_TABLE_INFO,
		updateDB,
	};
	return setupInfo;
};

// ---------------------------------------------------------------------------
ItemDataNullFlagType DBTablesAction::getNullFlag
  (const ActionDef &actionDef, ActionConditionEnableFlag enableFlag)
{
	if (actionDef.condition.isEnable(enableFlag))
		return ITEM_DATA_NOT_NULL;
	else
		return ITEM_DATA_NULL;
}

bool DBTablesAction::getLog(ActionLog &actionLog, const string &condition)
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

	getDBAgent().runTransaction(arg);

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

// TODO: Extract commonly used lines in getLog() above and reduce the similar lines. 
bool DBTablesAction::getLogs(ActionLogList &actionLogList,
                             const string &condition)
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
	arg.add(IDX_ACTION_LOGS_SERVER_ID);
	arg.add(IDX_ACTION_LOGS_EVENT_ID);

	getDBAgent().runTransaction(arg);

	const auto &grpList = arg.dataTable->getItemGroupList();
	if (grpList.empty())
		return false;

	for (const auto &itemGrp: grpList) {
		ItemGroupStream itemGroupStream(itemGrp);
		actionLogList.push_back(ActionLog());
		ActionLog &actionLog = actionLogList.back();
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

		// server id
		if (itemGroupStream.getItem()->isNull())
			actionLog.nullFlags |= ACTLOG_FLAG_EXIT_CODE;
		itemGroupStream >> actionLog.serverId;

		// event id
		if (itemGroupStream.getItem()->isNull())
			actionLog.nullFlags |= ACTLOG_FLAG_EXIT_CODE;
		itemGroupStream >> actionLog.eventId;
	}

	return true;
}

HatoholError DBTablesAction::checkPrivilegeForAdd(
  const OperationPrivilege &privilege, const ActionDef &actionDef)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	if (actionDef.type == ACTION_INCIDENT_SENDER) {
		if (privilege.has(OPPRVLG_CREATE_INCIDENT_SETTING))
			return HTERR_OK;
		else
			return HTERR_NO_PRIVILEGE;
	}

	if (!privilege.has(OPPRVLG_CREATE_ACTION))
		return HTERR_NO_PRIVILEGE;

	return HTERR_OK;
}

HatoholError DBTablesAction::checkPrivilegeForDelete(
  const OperationPrivilege &privilege, const ActionIdList &idList)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	// Check whether the idList includes ACTION_INCIDENT_SENDER or not.
	// TODO: It's not efficient.
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setActionIdList(idList);
	option.setActionType(ACTION_INCIDENT_SENDER);
	ActionDefList incidentSenderList;
	getActionList(incidentSenderList, option);
	bool canDeleteIncidentSender
	  = privilege.has(OPPRVLG_DELETE_INCIDENT_SETTING);
	if (!canDeleteIncidentSender && !incidentSenderList.empty())
		return HTERR_NO_PRIVILEGE;
	if (canDeleteIncidentSender &&
	    idList.size() == incidentSenderList.size()) {
		// It includes only IncidentSender type actions.
		return HTERR_OK;
	}

	if (privilege.has(OPPRVLG_DELETE_ALL_ACTION))
		return HTERR_OK;

	if (!privilege.has(OPPRVLG_DELETE_ACTION))
		return HTERR_NO_PRIVILEGE;

	return HTERR_OK;
}

HatoholError DBTablesAction::checkPrivilegeForUpdate(
  const OperationPrivilege &privilege, const ActionDef &actionDef)
{
	UserIdType userId = privilege.getUserId();
	if (userId == INVALID_USER_ID)
		return HTERR_INVALID_USER;

	if (actionDef.type == ACTION_INCIDENT_SENDER) {
		if (privilege.has(OPPRVLG_UPDATE_INCIDENT_SETTING))
			return HTERR_OK;
		else
			return HTERR_NO_PRIVILEGE;
	}

	if (!privilege.has(OPPRVLG_UPDATE_ACTION))
		return HTERR_NO_PRIVILEGE;

	return HTERR_OK;
}

gboolean DBTablesAction::deleteInvalidActionsExec(gpointer data)
{
	struct : public ExceptionCatchable {
		void operator ()(void) override
		{
			ThreadLocalDBCache cache;
			cache.getAction().deleteInvalidActions();
		}
	} deleter;
	deleter.exec();

	deleteInvalidActionsContext *deleteActionCtx = static_cast<deleteInvalidActionsContext *>(data);
	deleteActionCtx->idleEventId = INVALID_EVENT_ID;
	deleteActionCtx->timerId = g_timeout_add(DEFAULT_ACTION_DELETE_INTERVAL_MSEC,
	                                         deleteInvalidActionsCycl,
	                                         deleteActionCtx);
	return G_SOURCE_REMOVE;
}

gboolean DBTablesAction::deleteInvalidActionsCycl(gpointer data)
{
	deleteInvalidActionsContext *deleteActionCtx = static_cast<deleteInvalidActionsContext *>(data);
	deleteActionCtx->timerId = INVALID_EVENT_ID;
	deleteActionCtx->idleEventId = Utils::setGLibIdleEvent(deleteInvalidActionsExec,
	                                                       deleteActionCtx);
	return G_SOURCE_REMOVE;
}

void DBTablesAction::stopIdleDeleteAction(gpointer data)
{
	if (g_deleteActionCtx->timerId != INVALID_EVENT_ID)
		g_source_remove(g_deleteActionCtx->timerId);
	if (g_deleteActionCtx->idleEventId != INVALID_EVENT_ID)
		g_source_remove(g_deleteActionCtx->idleEventId);
}

// ---------------------------------------------------------------------------
// ActionsQueryOption
// ---------------------------------------------------------------------------
struct ActionsQueryOption::Impl {
	static const string conditionTemplate;

	ActionsQueryOption *option;
	// TODO: should have replica?
	const EventInfo *eventInfo;
	ActionType type;
	ActionIdList idList;

	Impl(ActionsQueryOption *_option)
	: option(_option), eventInfo(NULL), type(ACTION_USER_DEFINED)
	{
	}

	static string makeConditionTemplate(void);
	string getActionTypeAndOwnerCondition(void);
};

const string ActionsQueryOption::Impl::conditionTemplate
  = makeConditionTemplate();

string ActionsQueryOption::Impl::makeConditionTemplate(void)
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
	  "((%s IS NULL) OR (%s=%%s))",
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
	  "((%s IS NULL) OR (%s=%%s))",
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
: DataQueryOption(userId), m_impl(new Impl(this))
{
}

ActionsQueryOption::ActionsQueryOption(DataQueryContext *dataQueryContext)
: DataQueryOption(dataQueryContext), m_impl(new Impl(this))
{
}

ActionsQueryOption::ActionsQueryOption(const ActionsQueryOption &src)
: DataQueryOption(src), m_impl(new Impl(this))
{
	*m_impl = *src.m_impl;
}

ActionsQueryOption::~ActionsQueryOption()
{
}

void ActionsQueryOption::setTargetEventInfo(const EventInfo *eventInfo)
{
	m_impl->eventInfo = eventInfo;
}

const EventInfo *ActionsQueryOption::getTargetEventInfo(void) const
{
	return m_impl->eventInfo;
}

void ActionsQueryOption::setActionType(const ActionType &type)
{
	m_impl->type = type;
}

const ActionType &ActionsQueryOption::getActionType(void)
{
	return m_impl->type;
}

void ActionsQueryOption::setActionIdList(const ActionIdList &_idList)
{
	m_impl->idList = _idList;
}

const ActionIdList &ActionsQueryOption::getActionIdList(void)
{
	return m_impl->idList;
}

string ActionsQueryOption::Impl::getActionTypeAndOwnerCondition(void)
{
	string ownerCondition;
	if (!option->has(OPPRVLG_GET_ALL_ACTION))
		ownerCondition += makeOwnerCondition(option->getUserId());

	switch (type) {
	case ACTION_ALL:
	{
		string condition;

		if (option->has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS) &&
		    ownerCondition.empty()) {
			return condition;
		}

		condition = makeUserActionsCondition(ownerCondition);
		if (option->has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS)) {
			return StringUtils::sprintf(
				 "(%s OR action_type=%d)",
				 condition.c_str(), ACTION_INCIDENT_SENDER);
		} else {
			return condition;
		}
	}
	case ACTION_USER_DEFINED:
		return makeUserActionsCondition(ownerCondition);
	case ACTION_INCIDENT_SENDER:
		if (option->has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS)) {
			return StringUtils::sprintf("action_type=%d",
						    (int)type);
		} else {
			return DBHatohol::getAlwaysFalseCondition();
		}
	default:
		if (ownerCondition.empty()) {
			return StringUtils::sprintf("action_type=%d",
						    (int)type);
		} else {
			return StringUtils::sprintf("(%s AND action_type=%d)",
						    ownerCondition.c_str(),
						    (int)type);
		}
	}
}

string ActionsQueryOption::getCondition(void) const
{
	using StringUtils::sprintf;
	string cond;

	// filter by action type
	string actionTypeCondition = m_impl->getActionTypeAndOwnerCondition();
	if (!actionTypeCondition.empty()) {
		if (!cond.empty())
			cond += " AND ";
		cond += actionTypeCondition;
	}

	// filter by ID list
	if (!m_impl->idList.empty()) {
		if (!cond.empty())
			cond += " AND ";
		cond += makeIdListCondition(m_impl->idList);
	}

	// filter by EventInfo
	const EventInfo *eventInfo = m_impl->eventInfo;
	if (!eventInfo)
		return cond;

	HATOHOL_ASSERT(!m_impl->conditionTemplate.empty(),
	               "ActionDef condition template is empty.");
	string hostgroupIdList;
	getHostgroupIdList(
	  hostgroupIdList, eventInfo->serverId, eventInfo->hostIdInServer);
	DBTermCStringProvider rhs(*getDBTermCodec());
	if (hostgroupIdList.empty())
		hostgroupIdList = rhs(DB::getAlwaysFalseCondition());

	if (!cond.empty())
		cond += " AND ";
	// TODO: We can just pass triggerInfo.globalHostId instead of
	//       a pair of server ID and the hostIdInServer.
	cond += sprintf(m_impl->conditionTemplate.c_str(),
	                eventInfo->serverId,
	                rhs(eventInfo->hostIdInServer),
	                hostgroupIdList.c_str(),
	                rhs(eventInfo->triggerId),
	                eventInfo->status,
	                eventInfo->severity, eventInfo->severity);
	return cond;
}

void ActionsQueryOption::getHostgroupIdList(string &stringHostgroupId,
  const ServerIdType &serverId, const LocalHostIdType &hostId)
{
	HostgroupMemberVect hostgrpMembers;
	HostgroupMembersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setTargetHostId(hostId);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	uds->getHostgroupMembers(hostgrpMembers, option);

	if (hostgrpMembers.empty())
		return;

	SeparatorInjector commaInjector(",");
	DBTermCodec dbCodec;
	for (size_t i = 0; i < hostgrpMembers.size(); i++) {
		const HostgroupMember &hostgrpMember = hostgrpMembers[i];
		commaInjector(stringHostgroupId);
		stringHostgroupId += dbCodec.enc(hostgrpMember.hostgroupIdInServer);
	}
}

bool ActionDef::parseIncidentSenderCommand(IncidentTrackerIdType &trackerId) const
{
	int ret = sscanf(command.c_str(), "%" FMT_INCIDENT_TRACKER_ID, &trackerId);
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

void ActionUserIdSet::get(UserIdSet &userIdSet)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	dbUser.getUserIdSet(userIdSet);
}

// ---------------------------------------------------------------------------
// ActionValidator
// ---------------------------------------------------------------------------
ActionValidator::ActionValidator()
{
	ActionUserIdSet::get(m_userIdSet);

	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	dbConfig.getIncidentTrackerIdSet(m_incidentTrackerIdSet);
}

bool ActionValidator::isValidIncidentTracker(const ActionDef &actionDef)
{
	IncidentTrackerIdSet &trackers = m_incidentTrackerIdSet;
	IncidentTrackerIdType trackerId;
	if (!actionDef.parseIncidentSenderCommand(trackerId))
		return false;
	return trackers.find(trackerId) != trackers.end();
}

bool ActionValidator::isValid(const ActionDef &actionDef)
{
	if (!m_userIdSet.isValidActionOwnerId(actionDef.ownerUserId))
		return false;
	if (actionDef.type == ACTION_INCIDENT_SENDER) {
		if (!isValidIncidentTracker(actionDef))
			return false;
	}
	return true;
}
