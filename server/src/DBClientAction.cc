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

const char *TABLE_NAME_ACTIONS     = "actions";
const char *TABLE_NAME_ACTION_LOGS = "action_logs";

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
	IDX_ACTIONS_COMMAND,
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
	static const string actionDefConditionTemplate;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
	
	static string makeActionDefConditionTemplate(void);
};

const string DBClientAction::PrivateContext::actionDefConditionTemplate
  = makeActionDefConditionTemplate();

string DBClientAction::PrivateContext::makeActionDefConditionTemplate(void)
{
	string cond;

	// server_id;
	const ColumnDef &colDefSvId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_SERVER_ID];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%%d))",
	  colDefSvId.columnName, colDefSvId.columnName);
	cond += " and ";

	// host_id;
	const ColumnDef &colDefHostId = COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_ID];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%%"PRIu64"))",
	  colDefHostId.columnName, colDefHostId.columnName);
	cond += " and ";

#if 0   // TODO: we will enable this condition
	//       after host group feagure is supported.
	// host_group_id;
	const ColumnDef &colDefHostGrpId =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_GROUP_ID];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%%"PRIu64"))",
	  colDefHostGrpId.columnName, colDefHostGrpId.columnName);
	cond += " and "
#endif

	// trigger_id
	const ColumnDef &colDefTrigId =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_ID];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%%"PRIu64"))",
	  colDefTrigId.columnName, colDefTrigId.columnName);
	cond += " and ";

	// trigger_status
	const ColumnDef &colDefTrigStat =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_STATUS];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%%d))",
	  colDefTrigStat.columnName, colDefTrigStat.columnName);
	cond += " and ";

	// trigger_severity
	const ColumnDef &colDefTrigSeve =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY];
	const ColumnDef &colDefTrigSeveCmpType =
	   COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE];
	cond += StringUtils::sprintf(
	  "((%s is NULL) or (%s=%d and %s=%%d) or (%s=%d and %s>=%%d))",
	  colDefTrigSeve.columnName,
	  colDefTrigSeveCmpType.columnName, CMP_EQ, colDefTrigSeve.columnName,
	  colDefTrigSeveCmpType.columnName, CMP_EQ_GT,
	  colDefTrigSeve.columnName);

	return cond;
}

// ---------------------------------------------------------------------------
// LogEndExecActionArg
// ---------------------------------------------------------------------------
DBClientAction::LogEndExecActionArg::LogEndExecActionArg(void)
: logId(INVALID_ACTION_LOG_ID),
  status(ACTLOG_STAT_INVALID),
  exitCode(0),
  failureCode(ACTLOG_EXECFAIL_NONE)
{
}

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

const char *DBClientAction::getTableNameActions(void)
{
	return TABLE_NAME_ACTIONS;
}

const char *DBClientAction::getTableNameActionLogs(void)
{
	return TABLE_NAME_ACTION_LOGS;
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

void DBClientAction::addAction(ActionDef &actionDef)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.numColumns = NUM_COLUMNS_ACTIONS;
	arg.columnDefs = COLUMN_DEF_ACTIONS;

	row->ADD_NEW_ITEM(Int, 0); // This is automaticall set (0 is dummy)
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.serverId,
	                  getNullFlag(actionDef, ACTCOND_SERVER_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.hostId,
	                  getNullFlag(actionDef, ACTCOND_HOST_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.hostGroupId,
	                  getNullFlag(actionDef, ACTCOND_HOST_GROUP_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.triggerId,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_ID));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerStatus,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_STATUS));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerSeverity,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerSeverityCompType,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	row->ADD_NEW_ITEM(Int, actionDef.type);
	row->ADD_NEW_ITEM(String, actionDef.command);
	row->ADD_NEW_ITEM(String, actionDef.workingDir);
	row->ADD_NEW_ITEM(Int, actionDef.timeout);

	arg.row = row;

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		actionDef.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::getActionList(ActionDefList &actionDefList,
                                   const EventInfo *eventInfo)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_GROUP_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_STATUS]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY]);
	arg.pushColumn(
	  COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_TYPE]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_COMMAND]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_WORKING_DIR]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TIMEOUT]);

	if (eventInfo)
		arg.condition = makeActionDefCondition(*eventInfo);

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
		   GET_UINT64_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_ID);

		actionDef.condition.hostGroupId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_GROUP_ID);

		actionDef.condition.triggerId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_ID);

		actionDef.condition.triggerStatus =
		   GET_INT_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_STATUS);

		actionDef.condition.triggerSeverity =
		   GET_INT_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_SEVERITY);

		actionDef.condition.triggerSeverityCompType =
		  static_cast<ComparisonType>
		    (GET_INT_FROM_GRP(itemGroup, idx++));

		actionDef.type =
		   static_cast<ActionType>(GET_INT_FROM_GRP(itemGroup, idx++));
		actionDef.command    = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.workingDir = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.timeout    = GET_INT_FROM_GRP(itemGroup, idx++);
	}
}

uint64_t DBClientAction::createActionLog
  (const ActionDef &actionDef, ActionLogExecFailureCode failureCode,
   ActionLogStatus initialStatus)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.numColumns = NUM_COLUMNS_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	row->ADD_NEW_ITEM(Uint64, 0); // action_log_id (automatically set)
	row->ADD_NEW_ITEM(Int, actionDef.id);

	// status
	ActionLogStatus status;
	if (failureCode == ACTLOG_EXECFAIL_NONE)
		status = initialStatus;
	else
		status = ACTLOG_STAT_FAILED;
	row->ADD_NEW_ITEM(Int, status);

	// TODO: set the appropriate the following starter ID.
	row->ADD_NEW_ITEM(Int, 0); // starter_id

	row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL); // queuing_time
	row->ADD_NEW_ITEM(Int, CURR_DATETIME);     // start_time

	// end_time
	if (failureCode == ACTLOG_EXECFAIL_NONE)
		row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL);
	else
		row->ADD_NEW_ITEM(Int, CURR_DATETIME);

	row->ADD_NEW_ITEM(Int, failureCode);
	row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL); // exit_code

	arg.row = row;
	uint64_t logId;
	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		logId = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
	return logId;
}

void DBClientAction::logEndExecAction(const LogEndExecActionArg &logArg)
{
	VariableItemGroupPtr row;
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	const char *actionLogIdColumnName = 
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%"PRIu64,
	                                     actionLogIdColumnName,
	                                     logArg.logId);
	// status
	row->ADD_NEW_ITEM(Int, logArg.status);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_STATUS);

	// end_time
	row->ADD_NEW_ITEM(Int, CURR_DATETIME);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_END_TIME);

	// exec_failure_code
	row->ADD_NEW_ITEM(Int, logArg.failureCode);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_EXEC_FAILURE_CODE);

	// exit_code
	row->ADD_NEW_ITEM(Int, logArg.exitCode);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_EXIT_CODE);

	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::updateLogStatusToStart(uint64_t logId)
{
	VariableItemGroupPtr row;
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	const char *actionLogIdColumnName = 
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%"PRIu64,
	                                     actionLogIdColumnName, logId);
	// status
	row->ADD_NEW_ITEM(Int, ACTLOG_STAT_STARTED);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_STATUS);

	// start_time
	row->ADD_NEW_ITEM(Int, CURR_DATETIME);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_START_TIME);

	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientAction::getLog(ActionLog &actionLog, uint64_t logId)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	const ColumnDef *def = COLUMN_DEF_ACTION_LOGS;
	const char *idColName = def[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%"PRIu64, idColName, logId);
	arg.pushColumn(def[IDX_ACTION_LOGS_ACTION_LOG_ID]);
	arg.pushColumn(def[IDX_ACTION_LOGS_ACTION_ID]);
	arg.pushColumn(def[IDX_ACTION_LOGS_STATUS]); 
	arg.pushColumn(def[IDX_ACTION_LOGS_STARTER_ID]);
	arg.pushColumn(def[IDX_ACTION_LOGS_QUEUING_TIME]);
	arg.pushColumn(def[IDX_ACTION_LOGS_START_TIME]);
	arg.pushColumn(def[IDX_ACTION_LOGS_END_TIME]);
	arg.pushColumn(def[IDX_ACTION_LOGS_EXEC_FAILURE_CODE]);
	arg.pushColumn(def[IDX_ACTION_LOGS_EXIT_CODE]);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	size_t numGrpList = grpList.size();
	HATOHOL_ASSERT(numGrpList <= 1, "numGrpList: %zd, logId: %"PRIu64,
	               numGrpList, logId);
	// Not found
	if (numGrpList == 0)
		return false;

	ItemGroupListConstIterator it = grpList.begin();
	const ItemGroup *itemGroup = *it;
	int idx = 0;
	actionLog.nullFlags = 0;

	// action log ID
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemUint64, itemLogId);
	actionLog.id = itemLogId->get();

	// action ID
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemActionId);
	actionLog.actionId = itemActionId->get();

	// status
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemStatus);
	actionLog.status = itemStatus->get();

	// starter ID
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemStarterId);
	actionLog.starterId = itemStarterId->get();

	// queuing time
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
	                  ItemInt, itemQueuingTime);
	actionLog.queuingTime = itemQueuingTime->get();
	if (itemQueuingTime->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_QUEUING_TIME;

	// start time
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemStartTime);
	actionLog.startTime = itemStartTime->get();
	if (itemStartTime->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_START_TIME;

	// end time
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemEndTime);
	actionLog.endTime = itemEndTime->get();
	if (itemEndTime->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_END_TIME;

	// failure code
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++),
	                  ItemInt, itemFailureCode);
	actionLog.failureCode = itemFailureCode->get();

	// exit code
	DEFINE_AND_ASSERT(itemGroup->getItemAt(idx++), ItemInt, itemExitCode);
	actionLog.exitCode = itemExitCode->get();
	if (itemExitCode->isNull())
		actionLog.nullFlags |= ACTLOG_FLAG_EXIT_CODE;

	return true;
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

string DBClientAction::makeActionDefCondition(const EventInfo &eventInfo)
{
	HATOHOL_ASSERT(!m_ctx->actionDefConditionTemplate.empty(),
	               "ActionDef condition template is empty.");
	string cond = 
	  StringUtils::sprintf(m_ctx->actionDefConditionTemplate.c_str(),
	                       eventInfo.serverId,
	                       eventInfo.hostId,
	                       // TODO: hostGroupId
	                       eventInfo.triggerId,
	                       eventInfo.status,
	                       eventInfo.severity,
	                       eventInfo.severity);
	return cond;
}
