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

#ifndef DBClientAction_h
#define DBClientAction_h

#include <string>
#include "DBClientHatohol.h"
#include "DBClient.h"
#include "Params.h"

const static uint64_t INVALID_ACTION_LOG_ID = -1;

enum ActionType {
	ACTION_USER_DEFINED = -2, // COMMAND & RESIDENT
	ACTION_ALL = -1,
	ACTION_COMMAND,
	ACTION_RESIDENT,
	ACTION_INCIDENT_SENDER,
	NUM_ACTION_TYPES,
};

enum ComparisonType {
	CMP_INVALID,
	CMP_EQ,
	CMP_EQ_GT,
};

enum ActionConditionEnableFlag {
	ACTCOND_SERVER_ID        = (1 << 0),
	ACTCOND_HOST_ID          = (1 << 1),
	ACTCOND_HOST_GROUP_ID    = (1 << 2),
	ACTCOND_TRIGGER_ID       = (1 << 3),
	ACTCOND_TRIGGER_STATUS   = (1 << 4),
	ACTCOND_TRIGGER_SEVERITY = (1 << 5),
};

struct ActionCondition {
	uint32_t enableBits;

	ServerIdType serverId;
	uint64_t hostId;
	uint64_t hostgroupId;
	uint64_t triggerId;
	int      triggerStatus;
	int      triggerSeverity;
	ComparisonType triggerSeverityCompType;

	// methods
	ActionCondition(void)
	: enableBits(0),
	  triggerSeverityCompType(CMP_INVALID)
	{
	}

	ActionCondition(uint32_t _enableBits, const ServerIdType &_serverId,
	                uint64_t _hostId,
	                uint64_t _hostgroupId, uint64_t _triggerId,
	                int _triggerStatus, int _triggerSeverity, 
	                ComparisonType _triggerSeverityCompType)
	: enableBits(_enableBits),
	  serverId(_serverId),
	  hostId(_hostId),
	  hostgroupId(_hostgroupId),
	  triggerId(_triggerId),
	  triggerStatus(_triggerStatus),
	  triggerSeverity(_triggerSeverity),
	  triggerSeverityCompType(_triggerSeverityCompType)
	{
	}

	void enable(int bit)
	{
		enableBits |= bit;
	}

	bool isEnable(int bit) const
	{
		return (enableBits & bit);
	}
};

struct ActionDef {
	int         id;
	ActionCondition condition;
	ActionType  type;
	std::string workingDir;

	//
	// [Command type action]
	// path and command line options:
	// Ex.) /usr/bin/foo -l -o 'IYH... oooo' ABC
	//
	// [Resident type action]
	// module path and options.
	// Ex.) /usr/lib/foo.so -l -o 'IYH... oooo' ABC
	//
	// [IncidentSender type action]
	// IncidentTrackerInfo ID & IncidentSenderOption ID
	// Format: IncidentTrackerId[:IncidentSenderOptionId]
	//   Ex.) 3:17 (with IncidentSenderOption)
	//        3    (without IncidentSenderOption)
	// Note: IncidentSenderOption isn't implemeneted yet
	//
	// Note: A string: "-l -o 'IYH... oooo' ABC" is passed as an
	// argument of module's init() function.
	// Ref: struct ResidentModule in ResidentProtocol.h
	//
	std::string command;

	// Timeout value in millisecond. If this value is 0,
	// the action is executed without timeout.
	int         timeout;

	UserIdType  ownerUserId;

	/**
	 * Parse "command" string for IncidentSender type action.
	 *
	 * @param trackerId 
	 * An ID of IncidentTrackerInfo parsed from command string will be
	 * stored.
	 *
	 * @return true when succeeded to parse, otherwise false.
	 *
	 */
	bool parseIncidentSenderCommand(IncidentTrackerIdType &trackerId) const;
};

typedef std::list<ActionDef>          ActionDefList;
typedef ActionDefList::iterator       ActionDefListIterator;
typedef ActionDefList::const_iterator ActionDefListConstIterator;

typedef std::list<ActionIdType>       ActionIdList;
typedef ActionIdList::iterator        ActionIdListIterator;
typedef ActionIdList::const_iterator  ActionIdListConstIterator;

typedef std::set<ActionIdType>        ActionIdSet;
typedef ActionIdSet::iterator         ActionIdSetIterator;
typedef ActionIdSet::const_iterator   ActionIdSetConstIterator;

enum {
	ACTLOG_FLAG_QUEUING_TIME = (1 << 0),
	ACTLOG_FLAG_START_TIME   = (1 << 1),
	ACTLOG_FLAG_END_TIME     = (1 << 2),
	ACTLOG_FLAG_EXIT_CODE    = (1 << 3),
};

struct ActionLog {
	uint64_t id;
	ActionIdType actionId;
	int      status;
	int      starterId;
	int      queuingTime;
	int      startTime;
	int      endTime;
	int      failureCode;
	int      exitCode;
	uint32_t nullFlags;
};

struct ChildSigInfo {
	pid_t pid;
	int   code;
	int   status;
};

enum {
	IDX_ACTION_LOGS_ACTION_LOG_ID,
	IDX_ACTION_LOGS_ACTION_ID,
	IDX_ACTION_LOGS_STATUS, 
	IDX_ACTION_LOGS_STARTER_ID,
	IDX_ACTION_LOGS_QUEUING_TIME,
	IDX_ACTION_LOGS_START_TIME,
	IDX_ACTION_LOGS_END_TIME,
	IDX_ACTION_LOGS_EXEC_FAILURE_CODE,
	IDX_ACTION_LOGS_EXIT_CODE,
	IDX_ACTION_LOGS_SERVER_ID,
	IDX_ACTION_LOGS_EVENT_ID,
	NUM_IDX_ACTION_LOGS,
};

enum ActionLogStatus {
	ACTLOG_STAT_INVALID,

	// Hatohol limits the number of actions running
	// at the same time. If it excceds the limit,
	// a new action is registered as ACTLOG_STAT_QUEUING.
	ACTLOG_STAT_QUEUING,

	ACTLOG_STAT_STARTED,
	ACTLOG_STAT_SUCCEEDED,
	ACTLOG_STAT_FAILED,

	// For resident mode action, following staus is logged
	// until the launching and setup of hatohol-resident-yard
	// is completed.
	ACTLOG_STAT_LAUNCHING_RESIDENT,

	// Resident acitons for an actio ID are executed in series
	// This status is used for waiting actions.
	ACTLOG_STAT_RESIDENT_QUEUING,
};

enum ActionLogExecFailureCode {
	ACTLOG_EXECFAIL_NONE,
	ACTLOG_EXECFAIL_EXEC_FAILURE,
	ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
	ACTLOG_EXECFAIL_MOD_FAIL_DLOPEN,
	ACTLOG_EXECFAIL_MOD_NOT_FOUND_SYMBOL,
	ACTLOG_EXECFAIL_MOD_VER_INVALID,
	ACTLOG_EXECFAIL_MOD_INIT_FAILURE,
	ACTLOG_EXECFAIL_MOD_NOT_FOUND_NOTIFY_EVENT,
	ACTLOG_EXECFAIL_MOD_UNKNOWN_REASON,
	ACTLOG_EXECFAIL_KILLED_TIMEOUT,
	ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED,
	ACTLOG_EXECFAIL_PIPE_READ_ERR,
	ACTLOG_EXECFAIL_PIPE_WRITE_ERR,
	ACTLOG_EXECFAIL_KILLED_SIGNAL,
	ACTLOG_EXECFAIL_DUMPED_SIGNAL,
	ACTLOG_EXECFAIL_UNEXPECTED_EXIT,
};

class ActionsQueryOption : public DataQueryOption {
public:
	ActionsQueryOption(const UserIdType &userId = INVALID_USER_ID);
	ActionsQueryOption(DataQueryContext *dataQueryContext);
	ActionsQueryOption(const ActionsQueryOption &src);
	~ActionsQueryOption();

	void setTargetEventInfo(const EventInfo *eventInfo);
	const EventInfo *getTargetEventInfo(void) const;
	void setActionType(const ActionType &type);
	const ActionType &getActionType(void);
	void setActionIdList(const ActionIdList &idList);
	const ActionIdList &getActionIdList(void);

	virtual std::string getCondition(void) const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

class DBClientAction : public DBClient {

public:
	struct LogEndExecActionArg {
		uint64_t logId;
		ActionLogStatus status;
		int   exitCode;
		ActionLogExecFailureCode failureCode;
		int nullFlags;

		// constructor: just for initialization
		LogEndExecActionArg(void);
	};

	static int ACTION_DB_VERSION;
	static const char *DEFAULT_DB_NAME;

	static void init(void);
	static void reset(void);
	static void stop(void);
	static const char *getTableNameActions(void);
	static const char *getTableNameActionLogs(void);

	DBClientAction(void);
	virtual ~DBClientAction();
	HatoholError addAction(ActionDef &actionDef,
	                       const OperationPrivilege &privilege);
	HatoholError getActionList(ActionDefList &actionDefList,
	                           const ActionsQueryOption &option);
	HatoholError deleteActions(const ActionIdList &idList,
	                           const OperationPrivilege &privilege);

	/**
	 * make an action log.
	 *
	 * @param actionDef
	 * A reference of ActionDef.
	 *
	 * @param actionDef
	 * A reference of ActionDef.
	 *
	 * @param failureCode
	 * A failure code. This is typically set when the execution of 
	 * action fails. In the normal case in which the action is still
	 * running, ACTLOG_EXECFAIL_NONE should be set.
	 *
	 * @param initialStatus
	 * A status logged as a initial status. This parameter is valid 
	 * only when failureCOde is ACTLOG_EXECFAIL_NONE.
	 *
	 * @return a created action log ID.
	 */
	uint64_t createActionLog
	  (const ActionDef &actionDef, const EventInfo &eventInfo,
	   ActionLogExecFailureCode failureCode = ACTLOG_EXECFAIL_NONE,
	   ActionLogStatus initialStatus = ACTLOG_STAT_STARTED);

	/**
	 * update an action log.
	 *
	 * @param logArg
	 * A reference of LogEndExecActionArg. The record with the ID:
	 * logArg.logId is updated. The updated columns are status, end_time,
	 * exec_failure_code, and exit_code. Note that other members in
	 * logArg are ignored.
	 *
	 */
	void logEndExecAction(const LogEndExecActionArg &logArg);

	/**
	 * Update the status in action log to ACTLOG_STAT_STARTED.
	 * The column: start_time is also updated to the current time.
	 *
	 * @param logId A logID to be updated.
	 */
	void updateLogStatusToStart(uint64_t logId);

	/**
	 * Get the action log.
	 * @param actionLog
	 * The returned values are filled in this instance.
	 * @param logId
	 * The log ID to be searched.
	 *
	 * @return true if the log is found. Otherwise false.
	 */
	bool getLog(ActionLog &actionLog, uint64_t logId);

	/**
	 * Get the event log.
	 * @param actionLog
	 * The returned values are filled in this instance.
	 * @param serverId A server ID.
	 * @param eventId  An event ID.
	 *
	 * @return true if the log is found. Otherwise false.
	 */
	bool getLog(ActionLog &actionLog, const ServerIdType &serverId,
	            uint64_t eventId);

	/**
	 * Check whether IncidentSender type action exists or not
	 *
	 * @return true if IncidentSender type action exists
	 */
	bool isIncidentSenderEnabled(void);

	void deleteInvalidActions(void);

protected:
	ItemDataNullFlagType getNullFlag(const ActionDef &actionDef,
	                                 ActionConditionEnableFlag enableFlag);

	/**
	 * Get the action log.
	 * @param actionLog
	 * The returned values are filled in this instance.
	 * @param condition
	 * The condtion that are used a string in where clause
	 * in a SQL statement.
	 *
	 * @return true if the log is found. Otherwise false.
	 */
	bool getLog(ActionLog &actionLog, const std::string &condition);

	HatoholError checkPrivilegeForAdd(
	  const OperationPrivilege &privilege, const ActionDef &actionDef);
	HatoholError checkPrivilegeForDelete(
	  const OperationPrivilege &privilege, const ActionIdList &idList);

	static gboolean deleteInvalidActionsCycl(gpointer data);

	static gboolean deleteInvalidActionsExec(gpointer data);

	static void stopIdleDeleteAction(gpointer data);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

};

#endif // DBClientAction_h

