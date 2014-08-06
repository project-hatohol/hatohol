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

#include <cppcutter.h>
#include <cutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "Params.h"
#include "DBClientAction.h"
#include "DBClientUser.h"
#include "DBClientTest.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testDBClientAction {

struct TestDBClientAction : public DBClientAction {
	uint64_t callGetLastInsertId(void)
	{
		return getLastInsertId();
	}
};

static string makeExpectedString(const ActionDef &actDef, int expectedId)
{
	const ActionCondition &cond = actDef.condition;
	string expect = StringUtils::sprintf("%d|", expectedId);
	expect += cond.isEnable(ACTCOND_SERVER_ID) ?
	            StringUtils::sprintf("%d|", cond.serverId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_HOST_ID) ?
	            StringUtils::sprintf("%" PRIu64 "|", cond.hostId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_HOST_GROUP_ID) ?
	            StringUtils::sprintf("%" PRIu64 "|", cond.hostgroupId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_TRIGGER_ID) ?
	             StringUtils::sprintf("%" PRIu64 "|", cond.triggerId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_TRIGGER_STATUS) ?
	            StringUtils::sprintf("%d|", cond.triggerStatus) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_TRIGGER_SEVERITY) ?
	            StringUtils::sprintf("%d|", cond.triggerSeverity) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_TRIGGER_SEVERITY) ?
	            StringUtils::sprintf("%d|", cond.triggerSeverityCompType) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += StringUtils::sprintf("%d|%s|%s|%d|%d\n",
	                               actDef.type, actDef.command.c_str(),
	                               actDef.workingDir.c_str(),
	                               actDef.timeout, actDef.ownerUserId);
	return expect;
}

static string makeExpectedLogString(
  const ActionDef &actDef, const EventInfo &eventInfo, uint64_t logId,
  ActionLogStatus status = ACTLOG_STAT_STARTED,
  ActionLogExecFailureCode failureCode = ACTLOG_EXECFAIL_NONE)
{
	int expectedStarterId = 0; // This is currently not used.
	bool shouldHaveExitTime = true;
	if (status == ACTLOG_STAT_STARTED ||
	    status == ACTLOG_STAT_LAUNCHING_RESIDENT ||
	    status == ACTLOG_STAT_LAUNCHING_RESIDENT) {
		shouldHaveExitTime = false;
	}

	const char *expectedExitTime = shouldHaveExitTime ?
	  DBCONTENT_MAGIC_CURR_DATETIME: DBCONTENT_MAGIC_NULL;

	string expect =
	  StringUtils::sprintf(
	    "%" PRIu64 "|%d|%d|%d|%s|%s|%s|%d|%s|%" PRIu32 "|%" PRIu64 "\n",
	    logId, actDef.id, status, expectedStarterId,
	    DBCONTENT_MAGIC_NULL,
	    DBCONTENT_MAGIC_CURR_DATETIME,
	    expectedExitTime, failureCode,
	    DBCONTENT_MAGIC_NULL,
	    eventInfo.serverId, eventInfo.id);
	return expect;
}

static string makeExpectedEndLogString(
  const string &logAtStart, const DBClientAction::LogEndExecActionArg &logArg)
{
	StringVector words;
	StringUtils::split(words, logAtStart, '|');
	cppcut_assert_equal((size_t)NUM_IDX_ACTION_LOGS, words.size(),
	                    cut_message("logAtStart: %s", logAtStart.c_str()));
	words[IDX_ACTION_LOGS_STATUS] =
	   StringUtils::sprintf("%d", ACTLOG_STAT_SUCCEEDED);
	words[IDX_ACTION_LOGS_END_TIME] = DBCONTENT_MAGIC_CURR_DATETIME;
	words[IDX_ACTION_LOGS_EXIT_CODE] = 
	   StringUtils::sprintf("%d", logArg.exitCode);

	return joinStringVector(words, "|", false);
}

void _assertEqual(const ActionDef &expect, const ActionDef &actual)
{
	cppcut_assert_equal(expect.condition.enableBits,
	                    actual.condition.enableBits);
	if (expect.condition.enableBits & ACTCOND_SERVER_ID) {
		cppcut_assert_equal(expect.condition.serverId,
		                    actual.condition.serverId);
	}
	if (expect.condition.enableBits & ACTCOND_HOST_ID) {
		cppcut_assert_equal(expect.condition.hostId,
		                    actual.condition.hostId);
	}
	if (expect.condition.enableBits & ACTCOND_HOST_GROUP_ID) {
		cppcut_assert_equal(expect.condition.hostgroupId,
		                    actual.condition.hostgroupId);
	}
	if (expect.condition.enableBits & ACTCOND_TRIGGER_ID) {
		cppcut_assert_equal(expect.condition.triggerId,
		                    actual.condition.triggerId);
	}
	if (expect.condition.enableBits & ACTCOND_TRIGGER_STATUS) {
		cppcut_assert_equal(expect.condition.triggerStatus,
		                    actual.condition.triggerStatus);
	}
	if (expect.condition.enableBits & ACTCOND_TRIGGER_SEVERITY) {
		cppcut_assert_equal(expect.condition.triggerSeverity,
		                    actual.condition.triggerSeverity);
		cppcut_assert_equal(expect.condition.triggerSeverityCompType,
		                    actual.condition.triggerSeverityCompType);
	} else {
		cppcut_assert_equal(CMP_INVALID,
		                    actual.condition.triggerSeverityCompType);
	} 

	cppcut_assert_equal(expect.type, actual.type);
	cppcut_assert_equal(expect.workingDir, actual.workingDir);
	cppcut_assert_equal(expect.command, actual.command);
	cppcut_assert_equal(expect.timeout, actual.timeout);
	cppcut_assert_equal(expect.ownerUserId, actual.ownerUserId);
}
#define assertEqual(E,A) cut_trace(_assertEqual(E,A))

static void pickupActionIdsFromTestActionDef(
  ActionIdSet &actionIdSet, const OperationPrivilege &privilege,
  const ActionType &actionType = ACTION_USER_DEFINED)
{
	UserIdType expectedOwnerId = privilege.getUserId();
	if (privilege.has(OPPRVLG_GET_ALL_ACTION))
		expectedOwnerId = USER_ID_ANY;

	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actDef = testActionDef[i];
		if (actionType == ACTION_USER_DEFINED) {
			if (actDef.type >= ACTION_INCIDENT_SENDER)
				continue;
		} else if (actionType == ACTION_ALL) {
			if (actDef.type == ACTION_INCIDENT_SENDER &&
			    !privilege.has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS)) {
				continue;
			}
		} else {
			if (actDef.type != actionType)
				continue;
			if (actDef.type == ACTION_INCIDENT_SENDER &&
			    !privilege.has(OPPRVLG_GET_ALL_INCIDENT_SETTINGS)) {
				continue;
			}
		}
		if (actDef.type != ACTION_INCIDENT_SENDER &&
		    expectedOwnerId != USER_ID_ANY &&
		    actDef.ownerUserId != expectedOwnerId) {
			continue;
		}
		const int expectedId = i + 1;
		actionIdSet.insert(expectedId);
	}
}

void _assertGetActionList(
  const UserIdType &operatorId,
  const ActionType &actionType = ACTION_USER_DEFINED)
{
	ActionsQueryOption option(operatorId);
	option.setActionType(actionType);

	DBClientAction dbAction;
	ActionDefList actionDefList;
	assertHatoholError(HTERR_OK,
	                   dbAction.getActionList(actionDefList, option));

	// pick up expected action IDs
	ActionIdSet expectActionIdSet;
	pickupActionIdsFromTestActionDef(expectActionIdSet, option, actionType);

	// check the result
	cppcut_assert_equal(expectActionIdSet.size(), actionDefList.size());
	ActionDefListIterator actionDef = actionDefList.begin();
	for (; actionDef != actionDefList.end(); ++actionDef) {
		ActionIdSetIterator it = expectActionIdSet.find(actionDef->id);
		cppcut_assert_equal(true, it != expectActionIdSet.end());
		expectActionIdSet.erase(it);
	}
}
#define assertGetActionList(U, ...) \
cut_trace(_assertGetActionList(U, ##__VA_ARGS__))

static bool g_existTestDB = false;
static void setupHelperForTestDBUser(void)
{
	const bool dbRecreate = true;
	const bool loadTestData = true;
	setupTestDBUser(dbRecreate, loadTestData);
	g_existTestDB = true;
}

static void setupTestTriggerInfo(void)
{
	DBClientHatohol dbClientHatohol;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		dbClientHatohol.addTriggerInfo(&testTriggerInfo[i]);
}

static void setupTestHostgroupElement(void)
{
	DBClientHatohol dbClientHatohol;
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		dbClientHatohol.addHostgroupElement(&testHostgroupElement[i]);
}

void test_addAction(void);

static void setupTestDBUserAndDBAction(void)
{
	setupHelperForTestDBUser();
	test_addAction(); // add all test actions
}

static void _assertDeleteActions(const bool &deleteMyActions,
                                 const OperationPrivilegeType &type)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const UserIdType userId = findUserWith(type);
	string expect;
	ActionIdList idList;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actDef = testActionDef[i];
		const int expectedId = i + 1;
		bool shouldDelete;
		if (actDef.ownerUserId == userId)
			shouldDelete = deleteMyActions;
		else
			shouldDelete = !deleteMyActions;

		if (shouldDelete)
			idList.push_back(expectedId);
		else
			expect += StringUtils::sprintf("%d\n", expectedId);
	}
	cppcut_assert_equal(true, idList.size() >= 2);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_OK,
	                   dbAction.deleteActions(idList, privilege));

	// check
	string statement = "select action_id from ";
	statement += DBClientAction::getTableNameActions();
	statement += " order by action_id";
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}
#define assertDeleteActions(D,T) cut_trace(_assertDeleteActions(D,T))

static void _assertDeteleNoOwnerActions()
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;
	DBClientUser   dbUser;

	const UserIdType targetId = 2;
	string expect;
	ActionIdList idList;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actDef = testActionDef[i];
		const int expectedId = i + 1;
		if (actDef.ownerUserId != targetId)
			expect += StringUtils::sprintf("%d\n", expectedId);
	}

	OperationPrivilege privilege(ALL_PRIVILEGES);
	HatoholError err = dbUser.deleteUserInfo(targetId, privilege);

	dbAction.deleteNoOwnerActions();

	// check
	string statement = "select action_id from ";
	statement += DBClientAction::getTableNameActions();
	statement += " order by action_id";
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}
#define assertDeteleNoOwnerActions() cut_trace(_assertDeteleNoOwnerActions())

static void assertActionIdsInDB(ActionIdList excludeIdList)
{
	set<ActionIdType> idSet;
	ActionIdListIterator it;
	for (it = excludeIdList.begin(); it != excludeIdList.end(); it++)
		idSet.insert(*it);

	string expect;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const int expectedId = i + 1;
		if (idSet.find(expectedId) != idSet.end())
			continue;
		expect += StringUtils::sprintf("%d\n", expectedId);
	}

	string statement = "select action_id from ";
	statement += DBClientAction::getTableNameActions();
	statement += " order by action_id asc";
	DBClientAction dbAction;
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}
#define assertActionsInDB(E) cut_trace(_assertActionsInDB(E))

void cut_setup(void)
{
	hatoholInit();
	deleteDBClientHatoholDB();
	setupTestDBConfig(true, true);
	setupTestDBAction();
}

void cut_teardown(void)
{
	if (g_existTestDB) {
		const bool dbRecreate = true;
		const bool loadTestData = false;
		setupTestDBUser(dbRecreate, loadTestData);
		g_existTestDB = false;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_dbDomainId(void)
{
	DBClientAction dbAction;
	cppcut_assert_equal(DB_DOMAIN_ID_ACTION,
	                    dbAction.getDBAgent()->getDBDomainId());
}

void test_addAction(void)
{
	setupTestTriggerInfo();
	setupTestHostgroupElement();

	DBClientAction dbAction;
	string expect;
	OperationPrivilege privilege(USER_ID_SYSTEM);
	for (size_t i = 0; i < NumTestActionDef; i++) {
		ActionDef &actDef = testActionDef[i];
		assertHatoholError(HTERR_OK,
		                   dbAction.addAction(actDef, privilege));

		// validation
		const int expectedId = i + 1;
		cppcut_assert_equal(expectedId, actDef.id);
		string statement = "select * from ";
		statement += DBClientAction::getTableNameActions();
		expect += makeExpectedString(actDef, expectedId);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

void test_addActionByInvalidUser(void)
{
	DBClientAction dbAction;
	OperationPrivilege privilege(INVALID_USER_ID);
	ActionDef &actDef = testActionDef[0];
	assertHatoholError(HTERR_INVALID_USER,
	                   dbAction.addAction(actDef, privilege));
}

void test_addActionAndCheckOwner(void)
{
	setupHelperForTestDBUser();

	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	TestDBClientAction dbAction;
	OperationPrivilege privilege(userId);
	ActionDef &actDef = testActionDef[0];
	assertHatoholError(HTERR_OK, dbAction.addAction(actDef, privilege));
	ActionIdType actionId = dbAction.callGetLastInsertId();

	// check
	string expect = StringUtils::sprintf("%" FMT_ACTION_ID "|%" FMT_USER_ID,
	                                     actionId, userId);
	string statement = "select action_id, owner_user_id from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addActionWithoutPrivilege(void)
{
	setupHelperForTestDBUser();

	const UserIdType userId = findUserWithout(OPPRVLG_CREATE_ACTION);
	TestDBClientAction dbAction;
	OperationPrivilege privilege(userId);
	ActionDef &actDef = testActionDef[0];
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbAction.addAction(actDef, privilege));
}

static int findTestActionIdxByType(ActionType type)
{
	for (size_t i = 0; i < NumTestActionDef; i++) {
		if (testActionDef[i].type == type)
			return i;
	}
	return -1;
}

void test_addIncidentSenderActionByIncidentSettingsAdmin(void)
{
	setupHelperForTestDBUser();

	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_CREATE_ACTION);
	const UserIdType userId
		= findUserWith(OPPRVLG_CREATE_INCIDENT_SETTING, excludeFlags);
	TestDBClientAction dbAction;
	OperationPrivilege privilege(userId);
	int idx = findTestActionIdxByType(ACTION_INCIDENT_SENDER);
	ActionIdType expectedId = 1;
	ActionDef actionDef = testActionDef[idx];
	actionDef.ownerUserId = userId;
	assertHatoholError(HTERR_OK,
	                   dbAction.addAction(actionDef, privilege));

	// check: The owner of ACTION_INCIDENT_SENDER shoube be USER_ID_SYSTEM.
	actionDef.id = expectedId;
	string expect = StringUtils::sprintf("%" FMT_ACTION_ID "|%" FMT_USER_ID,
	                                     1, USER_ID_SYSTEM);
	string statement = "select action_id, owner_user_id from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_addIncidentSenderActionWithoutPrivilege(void)
{
	setupHelperForTestDBUser();

	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_CREATE_INCIDENT_SETTING);
	const UserIdType userId
		= findUserWith(OPPRVLG_CREATE_ACTION, excludeFlags);
	TestDBClientAction dbAction;
	OperationPrivilege privilege(userId);
	int idx = findTestActionIdxByType(ACTION_INCIDENT_SENDER);
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbAction.addAction(testActionDef[idx], privilege));
}

void test_deleteAction(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const UserIdType userId = findUserWith(OPPRVLG_DELETE_ACTION);
	const size_t targetIdx = findIndexFromTestActionDef(userId);
	ActionIdList idList;
	idList.push_back(testActionDef[targetIdx].id);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_OK,
	                   dbAction.deleteActions(idList, privilege));
	assertActionIdsInDB(idList);
}

void test_deleteActionWithoutPrivilege(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const UserIdType userId = findUserWithout(OPPRVLG_DELETE_ACTION);
	const size_t targetIdx = findIndexFromTestActionDef(userId);
	ActionIdList idList;
	const int expectedId = testActionDef[targetIdx].id + 1;
	idList.push_back(expectedId);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbAction.deleteActions(idList, privilege));
}

void test_deleteActionByInvalidUser(void)
{
	ActionIdList idList;
	DBClientAction dbAction;
	OperationPrivilege privilege(INVALID_USER_ID);
	assertHatoholError(HTERR_INVALID_USER,
	                   dbAction.deleteActions(idList, privilege));
}

void test_deleteActionMultiple(void)
{
	const bool deleteMyActions = true;
	assertDeleteActions(deleteMyActions, OPPRVLG_DELETE_ACTION);
}

void test_deleteActionOfOthers(void)
{
	const bool deleteMyActions = false;
	assertDeleteActions(deleteMyActions, OPPRVLG_DELETE_ALL_ACTION);
}

void test_deleteNoOwnerAction(void)
{
	assertDeteleNoOwnerActions();
}

void test_deleteActionOfOthersWithoutPrivilege(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_DELETE_ALL_ACTION);
	const UserIdType userId =
	  findUserWith(OPPRVLG_DELETE_ACTION, excludeFlags);

	// Anyone other than caller itself can be a target. We choose a target
	// user whose ID is the next of the caller.
	const UserIdType targetUserId = userId + 1;
	const size_t targetIdx = findIndexFromTestActionDef(targetUserId);

	ActionIdList idList;
	const int expectedId = testActionDef[targetIdx].id + 1;
	idList.push_back(expectedId);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_DELETE_INCOMPLETE,
	                   dbAction.deleteActions(idList, privilege));
}

void test_deleteIncidentSenderActionByIncidentSettingsAdmin(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_DELETE_ACTION);
	const UserIdType userId
	  = findUserWith(OPPRVLG_DELETE_INCIDENT_SETTING, excludeFlags);
	int targetActionId
	  = findTestActionIdxByType(ACTION_INCIDENT_SENDER) + 1;
	ActionIdList idList;
	idList.push_back(targetActionId);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_OK,
	                   dbAction.deleteActions(idList, privilege));
	assertActionIdsInDB(idList);
}

void test_deleteIncidentSenderActionWithoutPrivilege(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;

	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_DELETE_INCIDENT_SETTING);
	const UserIdType userId
	  = findUserWith(OPPRVLG_DELETE_ACTION, excludeFlags);
	int targetActionId
	  = findTestActionIdxByType(ACTION_INCIDENT_SENDER) + 1;
	ActionIdList idList;
	idList.push_back(targetActionId);
	OperationPrivilege privilege(userId);
	assertHatoholError(HTERR_NO_PRIVILEGE,
	                   dbAction.deleteActions(idList, privilege));
}

void test_startExecAction(void)
{
	string expect;
	DBClientAction dbAction;
	EventInfo &eventInfo = testEventInfo[0];
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actDef = testActionDef[i];
		ActionLogStatus status;
		if (actDef.type == ACTION_COMMAND)
			status = ACTLOG_STAT_STARTED;
		else if (actDef.type == ACTION_RESIDENT)
			status = ACTLOG_STAT_LAUNCHING_RESIDENT;
		else if (actDef.type == ACTION_INCIDENT_SENDER) {
			// TODO: Not implemented yet
			continue;
		} else {
			// Set any value to avoid a compiler warning.
			status = ACTLOG_STAT_STARTED;
			cut_fail("Unknown action type: %d\n", actDef.type);
		}
		uint64_t logId =
		  dbAction.createActionLog(actDef, eventInfo,
		                           ACTLOG_EXECFAIL_NONE, status);
		const uint64_t expectedId = i + 1;
		cppcut_assert_equal(expectedId, logId);
		string statement = "select * from ";
		statement += DBClientAction::getTableNameActionLogs();
		expect += makeExpectedLogString(actDef, eventInfo,
		                                expectedId, status);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

void test_startExecActionWithExecFailure(void)
{
	string expect;
	DBClientAction dbAction;
	EventInfo &eventInfo = testEventInfo[0];
	size_t targetIdx = 1;
	const ActionDef &actDef = testActionDef[targetIdx];
	uint64_t logId = dbAction.createActionLog(actDef, eventInfo,
	                                          ACTLOG_EXECFAIL_EXEC_FAILURE);
	const uint64_t expectedId = 1;
	cppcut_assert_equal(expectedId, logId);
	string statement = "select * from ";
	statement += DBClientAction::getTableNameActionLogs();
	expect +=
	  makeExpectedLogString(actDef, eventInfo, expectedId,
	                        ACTLOG_STAT_FAILED,
	                        ACTLOG_EXECFAIL_EXEC_FAILURE);
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_endExecAction(void)
{
	size_t targetIdx = 1;
	int    exitCode = 21;
	DBClientAction dbAction;

	DBClientAction::LogEndExecActionArg logArg;
	logArg.logId = targetIdx + 1;
	logArg.status = ACTLOG_STAT_SUCCEEDED;
	logArg.exitCode = exitCode;

	// make action logs
	string statement = "select * from action_logs";
	test_startExecAction();
	string rows = execSQL(dbAction.getDBAgent(), statement);
	StringVector rowVector;
	StringUtils::split(rowVector, rows, '\n');
	cppcut_assert_equal(getNumberOfTestActions(), rowVector.size());

	// update one log
	dbAction.logEndExecAction(logArg);

	// validate
	string expectedLine =
	   makeExpectedEndLogString(rowVector[targetIdx], logArg);
	rowVector[targetIdx] = expectedLine;
	string expect = joinStringVector(rowVector, "\n");
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_getTriggerActionList(void)
{
	setupHelperForTestDBUser();
	test_addAction(); // save test data into DB.

	// make an EventInfo instance for the test
	int idxTarget = 1;
	const ActionCondition condDummy  = testActionDef[0].condition;
	const ActionCondition condDummy2 = testActionDef[2].condition;
	const ActionCondition condTarget = testActionDef[idxTarget].condition;
	EventInfo eventInfo;
	eventInfo.serverId  = condTarget.serverId;
	eventInfo.id        = 0;
	eventInfo.time.tv_sec  = 1378339653;
	eventInfo.time.tv_nsec = 6889;
	eventInfo.type      = EVENT_TYPE_GOOD;
	eventInfo.triggerId = condDummy.triggerId;
	eventInfo.status    = (TriggerStatusType) condTarget.triggerStatus;
	eventInfo.severity  = (TriggerSeverityType) condTarget.triggerSeverity;
	eventInfo.hostId    = condDummy2.hostId;
	eventInfo.hostName  = "foo";
	eventInfo.brief     = "foo foo foo";

	// get the list and check the number
	DBClientAction dbAction;
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setTargetEventInfo(&eventInfo);
	assertHatoholError(
	  HTERR_OK,
	  dbAction.getActionList(actionDefList, option));
	cppcut_assert_equal((size_t)1, actionDefList.size());

	// check the content
	const ActionDef &actual = *actionDefList.begin();
	assertEqual(testActionDef[idxTarget], actual);
}

void test_getTriggerActionListWithAllCondition(void)
{
	setupHelperForTestDBUser();
	test_addAction(); // save test data into DB.

	// make an EventInfo instance for the test
	int idxTarget = 3;
	const ActionCondition condTarget = testActionDef[idxTarget].condition;
	EventInfo eventInfo;
	eventInfo.serverId  = condTarget.serverId;
	eventInfo.id        = 0;
	eventInfo.time.tv_sec  = 1378339653;
	eventInfo.time.tv_nsec = 6889;
	eventInfo.type      = EVENT_TYPE_GOOD;
	eventInfo.triggerId = condTarget.triggerId;
	eventInfo.status    = (TriggerStatusType) condTarget.triggerStatus;
	eventInfo.severity  = (TriggerSeverityType) condTarget.triggerSeverity;
	eventInfo.hostId    = condTarget.hostId;
	eventInfo.hostName  = "foo";
	eventInfo.brief     = "foo foo foo";

	// get the list and check the number
	DBClientAction dbAction;
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setTargetEventInfo(&eventInfo);
	assertHatoholError(
	  HTERR_OK,
	  dbAction.getActionList(actionDefList, option));
	cppcut_assert_equal((size_t)1, actionDefList.size());

	// check the content
	const ActionDef &actual = *actionDefList.begin();
	assertEqual(testActionDef[idxTarget], actual);
}

static void _assertGetActionWithSeverity(const TriggerSeverityType &severity,
					 const int expectedActionIdx)
{
	setupHelperForTestDBUser();

	test_addAction(); // save test data into DB.

	// make an EventInfo instance for the test
	EventInfo eventInfo;
	const ActionDef &actionDef = testActionDef[expectedActionIdx];
	const ActionCondition condTarget = actionDef.condition;
	eventInfo.serverId  = condTarget.serverId ? : 1129;
	eventInfo.id        = 1192;
	eventInfo.time.tv_sec  = 1378339653;
	eventInfo.time.tv_nsec = 6889;
	eventInfo.type      = EVENT_TYPE_BAD;
	eventInfo.triggerId = condTarget.triggerId ? : 1;
	eventInfo.status    = (TriggerStatusType) condTarget.triggerStatus;
	eventInfo.severity = severity;
	eventInfo.hostId    = condTarget.hostId ? : 1;
	eventInfo.hostName  = "foo";
	eventInfo.brief     = "foo foo foo";

	// get the list and check the number
	DBClientAction dbAction;
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setTargetEventInfo(&eventInfo);

	assertHatoholError(
	  HTERR_OK,
	  dbAction.getActionList(actionDefList, option));

	if (expectedActionIdx < 0) {
		cppcut_assert_equal((size_t)0, actionDefList.size());
	} else {
		cppcut_assert_equal((size_t)1, actionDefList.size());
		// check the content
		const ActionDef &actual = *actionDefList.begin();
		assertEqual(testActionDef[expectedActionIdx], actual);
	}
}
#define assertGetActionWithSeverity(S,E) \
cut_trace(_assertGetActionWithSeverity(S,E))

void test_getActionWithLessSeverityAgainstCmpEqGt(void)
{
	int targetActionIdx = 1;
	int noActionIdx = -1;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)(targetAction.condition.triggerSeverity - 1),
	  noActionIdx);
}

void test_getActionWithEqualSeverityAgainstCmpEqGt(void)
{
	int targetActionIdx = 1;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)targetAction.condition.triggerSeverity,
	  targetActionIdx);
}

void test_getActionWithGreaterSeverityAgainstCmpEqGt(void)
{
	int targetActionIdx = 1;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)(targetAction.condition.triggerSeverity + 1),
	  targetActionIdx);
}

void test_getActionWithLessSeverityAgainstCmpEq(void)
{
	int targetActionIdx = 4;
	int noActionIdx = -1;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)(targetAction.condition.triggerSeverity - 1),
	  noActionIdx);
}

void test_getActionWithEqualSeverityAgainstCmpEq(void)
{
	int targetActionIdx = 4;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)targetAction.condition.triggerSeverity,
	  targetActionIdx);
}

void test_getActionWithGreaterSeverityAgainstCmpEq(void)
{
	int targetActionIdx = 4;
	int noActionIdx = -1;
	ActionDef &targetAction = testActionDef[targetActionIdx];
	assertGetActionWithSeverity(
	  (TriggerSeverityType)(targetAction.condition.triggerSeverity + 1),
	  noActionIdx);
}

void test_getActionListWithNormalUser(void)
{
	setupTestDBUserAndDBAction();
	const UserIdType userId = findUserWithout(OPPRVLG_GET_ALL_ACTION);
	assertGetActionList(userId);
}

void test_getActionListWithUserHavingGetAllFlag(void)
{
	setupTestDBUserAndDBAction();
	const UserIdType userId = findUserWith(OPPRVLG_GET_ALL_ACTION);
	assertGetActionList(userId);
}

void data_getActionListWithActionType(void)
{
	gcut_add_datum("All",
		       "type", G_TYPE_INT, (int)ACTION_ALL,
		       NULL);
	gcut_add_datum("Command",
		       "type", G_TYPE_INT, (int)ACTION_COMMAND,
		       NULL);
	gcut_add_datum("Resident",
		       "type", G_TYPE_INT, (int)ACTION_RESIDENT,
		       NULL);
	gcut_add_datum("IncidentSender",
		       "type", G_TYPE_INT, (int)ACTION_INCIDENT_SENDER,
		       NULL);
}

void test_getActionListWithActionType(gconstpointer data)
{
	setupTestDBUserAndDBAction();
	const UserIdType userId = findUserWith(OPPRVLG_GET_ALL_ACTION);
	ActionType type = (ActionType)gcut_data_get_int(data, "type");
	assertGetActionList(userId, type);
}

void data_getActionListByIncidentSettingsAdmin(void)
{
	data_getActionListWithActionType();
}

void test_getActionListByIncidentSettingsAdmin(gconstpointer data)
{
	setupTestDBUserAndDBAction();
	const OperationPrivilegeFlag excludeFlags
	  = OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_ACTION);
	const UserIdType userId
		= findUserWith(OPPRVLG_GET_ALL_INCIDENT_SETTINGS, excludeFlags);
	ActionType type = (ActionType)gcut_data_get_int(data, "type");
	assertGetActionList(userId, type);
}

void test_parseIncidentSenderCommand(void)
{
	ActionDef action;
	action.command = "3";
	IncidentTrackerIdType trackerId;
	cppcut_assert_equal(true, action.parseIncidentSenderCommand(trackerId));
	cppcut_assert_equal(3, trackerId);
}

void test_parseInvalidIncidentSenderCommand(void)
{
	ActionDef action;
	action.command = "hoge3";
	IncidentTrackerIdType trackerId;
	cppcut_assert_equal(false,
			    action.parseIncidentSenderCommand(trackerId));
}

void test_incidentSenderIsEnabled(void)
{
	setupTestDBUserAndDBAction();
	DBClientAction dbAction;
	cppcut_assert_equal(true, dbAction.isIncidentSenderEnabled());
}

void test_incidentSenderIsNotEnabled(void)
{
	setupHelperForTestDBUser();
	DBClientAction dbAction;
	cppcut_assert_equal(false, dbAction.isIncidentSenderEnabled());
}

} // namespace testDBClientAction

namespace testDBClientActionDefault {

void cut_setup(void)
{
	hatoholInit();
}

void test_databaseName(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_ACTION);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_DB_NAME),
	                    connInfo.dbName);
}

void test_databaseUser(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_ACTION);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_USER_NAME),
	                    connInfo.user);
}

void test_databasePassword(void)
{
	DBConnectInfo connInfo =
	  DBClient::getDBConnectInfo(DB_DOMAIN_ID_ACTION);
	cppcut_assert_equal(string(DBClientConfig::DEFAULT_USER_NAME),
	                    connInfo.user);
}

} // namespace testDBClientActionDefault

namespace testActionsQueryOption {

void cut_setup(void)
{
	hatoholInit();
	deleteDBClientHatoholDB();
	setupTestDBConfig(true, true);
}

void test_withoutUser(void)
{
	ActionsQueryOption option;
	cppcut_assert_equal(
	  string("(owner_user_id=-1 AND action_type>=0 AND action_type<2)"),
	  option.getCondition());
}

void test_withSystemUser(void)
{
	ActionsQueryOption option(USER_ID_SYSTEM);
	cppcut_assert_equal(
	  string("(action_type>=0 AND action_type<2)"),
	  option.getCondition());
}

void test_withPrivilege(void)
{
	setupTestDBUser(true, true);
	UserIdType id = findUserWith(OPPRVLG_GET_ALL_ACTION);
	ActionsQueryOption option(id);
	cppcut_assert_equal(
	  string("(action_type>=0 AND action_type<2)"),
	  option.getCondition());
}

void test_withoutPrivilege(void)
{
	setupTestDBUser(true, true);
	UserIdType id = findUserWithout(OPPRVLG_GET_ALL_ACTION);
	ActionsQueryOption option(id);
	string expected
	  = StringUtils::sprintf(
	    "(owner_user_id=%" FMT_USER_ID
	    " AND action_type>=0 AND action_type<2)", id);
	cppcut_assert_equal(expected, option.getCondition());
}

void test_withEventInfo(void)
{
	setupTestDBUser(true, true);
	UserIdType id = findUserWithout(OPPRVLG_GET_ALL_ACTION);
	EventInfo &event = testEventInfo[0];
	ActionsQueryOption option(id);
	option.setTargetEventInfo(&event);
	string expected
	  = StringUtils::sprintf(
	    "(owner_user_id=%" FMT_USER_ID " AND "
	    "action_type>=0 AND action_type<2) AND "
	    "((server_id IS NULL) OR (server_id=%" FMT_SERVER_ID ")) AND "
	    "((host_id IS NULL) OR (host_id=%" FMT_HOST_ID ")) AND "
	    // test with empty hostgroups
	    "((host_group_id IS NULL) OR host_group_id IN (0)) AND "
	    "((trigger_id IS NULL) OR (trigger_id=%" FMT_TRIGGER_ID ")) AND "
	    "((trigger_status IS NULL) OR (trigger_status=%d)) AND "
	    "((trigger_severity IS NULL) OR "
	    "(trigger_severity_comp_type=1 AND trigger_severity=%d) OR "
	    "(trigger_severity_comp_type=2 AND trigger_severity<=%d))",
	    id, event.serverId, event.hostId, event.triggerId,
	    event.status, event.severity, event.severity);
	cppcut_assert_equal(expected, option.getCondition());
}

void data_actionType(void)
{
	gcut_add_datum("All",
		       "type", G_TYPE_INT, (int)ACTION_ALL,
		       "condition", G_TYPE_STRING, "",
		       NULL);
	gcut_add_datum("Command",
		       "type", G_TYPE_INT, (int)ACTION_COMMAND,
		       "condition", G_TYPE_STRING, "action_type=0",
		       NULL);
	gcut_add_datum("Resident",
		       "type", G_TYPE_INT, (int)ACTION_RESIDENT,
		       "condition", G_TYPE_STRING, "action_type=1",
		       NULL);
	gcut_add_datum("IncidentSender",
		       "type", G_TYPE_INT, (int)ACTION_INCIDENT_SENDER,
		       "condition", G_TYPE_STRING, "action_type=2",
		       NULL);
}

void test_actionType(gconstpointer data)
{
	setupTestDBUser(true, true);
	UserIdType id = findUserWith(OPPRVLG_GET_ALL_ACTION);
	ActionsQueryOption option(id);
	ActionType type = (ActionType)gcut_data_get_int(data, "type");
	string condition = gcut_data_get_string(data, "condition");
	option.setActionType(type);
	cppcut_assert_equal(type, option.getActionType());
	cppcut_assert_equal(condition, option.getCondition());
}

void test_idList(void)
{
	ActionsQueryOption option(USER_ID_SYSTEM);
	ActionIdList idList;
	idList.push_back(2);
	idList.push_back(7);
	option.setActionIdList(idList);
	string expected = "(action_type>=0 AND action_type<2)";
	expected += " AND action_id in (2,7)";
	cppcut_assert_equal(expected, option.getCondition());
}

}
