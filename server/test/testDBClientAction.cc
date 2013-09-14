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

#include <cppcutter.h>
#include <cutter.h>
#include "Hatohol.h"
#include "Params.h"
#include "DBClientAction.h"
#include "DBClientTest.h"
#include "Helpers.h"

namespace testDBClientAction {

static string makeExpectedString(const ActionDef &actDef, int expectedId)
{
	const ActionCondition &cond = actDef.condition;
	string expect = StringUtils::sprintf("%d|", expectedId);
	expect += cond.isEnable(ACTCOND_SERVER_ID) ?
	            StringUtils::sprintf("%d|", cond.serverId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_HOST_ID) ?
	            StringUtils::sprintf("%"PRIu64"|", cond.hostId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_HOST_GROUP_ID) ?
	            StringUtils::sprintf("%"PRIu64"|", cond.hostGroupId) :
	            DBCONTENT_MAGIC_NULL "|";
	expect += cond.isEnable(ACTCOND_TRIGGER_ID) ?
	             StringUtils::sprintf("%"PRIu64"|", cond.triggerId) :
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
	expect += StringUtils::sprintf("%d|%s|%s|%d\n",
	                               actDef.type, actDef.command.c_str(),
	                               actDef.workingDir.c_str(),
	                               actDef.timeout);
	return expect;
}

static string makeExpectedLogString(
  const ActionDef &actDef, uint64_t logId,
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
	    "%"PRIu64"|%d|%d|%d|%s|%s|%s|%d|%s\n",
	    logId, actDef.id, status, expectedStarterId,
	    DBCONTENT_MAGIC_NULL,
	    DBCONTENT_MAGIC_CURR_DATETIME,
	    expectedExitTime, failureCode,
	    DBCONTENT_MAGIC_NULL);
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
		cppcut_assert_equal(expect.condition.hostGroupId,
		                    actual.condition.hostGroupId);
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
}
#define assertEqual(E,A) cut_trace(_assertEqual(E,A))

void setup(void)
{
	hatoholInit();
	setupTestDBAction();
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
	DBClientAction dbAction;
	string expect;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		ActionDef &actDef = testActionDef[i];
		dbAction.addAction(actDef);

		// validation
		const int expectedId = i + 1;
		cppcut_assert_equal(expectedId, actDef.id);
		string statement = "select * from ";
		statement += DBClientAction::getTableNameActions();
		expect += makeExpectedString(actDef, expectedId);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

void test_deleteAction(void)
{
	DBClientAction dbAction;
	test_addAction(); // add all test actions

	// we delete 2nd one
	size_t targetIdx = 1;
	ActionIdList idList;
	idList.push_back(testActionDef[targetIdx].id);
	dbAction.deleteActions(idList);

	// check
	string expect;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		if (i == targetIdx)
			continue;
		const int expectedId = i + 1;
		expect += StringUtils::sprintf("%d\n", expectedId);
	}

	string statement = "select action_id from ";
	statement += DBClientAction::getTableNameActions();
	assertDBContent(dbAction.getDBAgent(), statement, expect);
}

void test_startExecAction(void)
{
	string expect;
	DBClientAction dbAction;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actDef = testActionDef[i];
		ActionLogStatus status;
		if (actDef.type == ACTION_COMMAND)
			status = ACTLOG_STAT_STARTED;
		else if (actDef.type == ACTION_RESIDENT)
			status = ACTLOG_STAT_LAUNCHING_RESIDENT;
		else {
			// Set any value to avoid a compiler warning.
			status = ACTLOG_STAT_STARTED;
			cut_fail("Unknown action type: %d\n", actDef.type);
		}
		uint64_t logId =
		  dbAction.createActionLog(actDef, ACTLOG_EXECFAIL_NONE, status);
		const uint64_t expectedId = i + 1;
		cppcut_assert_equal(expectedId, logId);
		string statement = "select * from ";
		statement += DBClientAction::getTableNameActionLogs();
		expect += makeExpectedLogString(actDef, expectedId, status);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

void test_startExecActionWithExecFailure(void)
{
	string expect;
	DBClientAction dbAction;
	size_t targetIdx = 1;
	const ActionDef &actDef = testActionDef[targetIdx];
	uint64_t logId = dbAction.createActionLog(actDef,
	                                          ACTLOG_EXECFAIL_EXEC_FAILURE);
	const uint64_t expectedId = 1;
	cppcut_assert_equal(expectedId, logId);
	string statement = "select * from ";
	statement += DBClientAction::getTableNameActionLogs();
	expect +=
	  makeExpectedLogString(actDef, expectedId, ACTLOG_STAT_FAILED,
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
	cppcut_assert_equal(NumTestActionDef, rowVector.size());

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
	eventInfo.type      = EVENT_TYPE_ACTIVATED;
	eventInfo.triggerId = condDummy.triggerId;
	eventInfo.status    = (TriggerStatusType) condTarget.triggerStatus;
	eventInfo.severity  = (TriggerSeverityType) condTarget.triggerSeverity;
	eventInfo.hostId    = condDummy2.hostId;
	eventInfo.hostName  = "foo";
	eventInfo.brief     = "foo foo foo";

	// get the list and check the number
	DBClientAction dbAction;
	ActionDefList actionDefList;
	dbAction.getActionList(actionDefList, &eventInfo);
	cppcut_assert_equal((size_t)1, actionDefList.size());

	// check the content
	const ActionDef &actual = *actionDefList.begin();
	assertEqual(testActionDef[idxTarget], actual);
}

void test_getTriggerActionListWithAllCondition(void)
{
	test_addAction(); // save test data into DB.

	// make an EventInfo instance for the test
	int idxTarget = 3;
	const ActionCondition condTarget = testActionDef[idxTarget].condition;
	EventInfo eventInfo;
	eventInfo.serverId  = condTarget.serverId;
	eventInfo.id        = 0;
	eventInfo.time.tv_sec  = 1378339653;
	eventInfo.time.tv_nsec = 6889;
	eventInfo.type      = EVENT_TYPE_ACTIVATED;
	eventInfo.triggerId = condTarget.triggerId;
	eventInfo.status    = (TriggerStatusType) condTarget.triggerStatus;
	eventInfo.severity  = (TriggerSeverityType) condTarget.triggerSeverity;
	eventInfo.hostId    = condTarget.hostId;
	eventInfo.hostName  = "foo";
	eventInfo.brief     = "foo foo foo";

	// get the list and check the number
	DBClientAction dbAction;
	ActionDefList actionDefList;
	dbAction.getActionList(actionDefList, &eventInfo);
	cppcut_assert_equal((size_t)1, actionDefList.size());

	// check the content
	const ActionDef &actual = *actionDefList.begin();
	assertEqual(testActionDef[idxTarget], actual);
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
