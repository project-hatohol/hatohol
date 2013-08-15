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
#include "Helpers.h"

namespace testDBClientAction {

static ActionDef testActionDef[] = {
{
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID |
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  1,                        // serverId
	  10,                       // hostId
	  5,                        // hostGroupId
	  3,                        // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_INVALID               // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"",                // working dir
	"/bin/hoge",       // path
	300,               // timeout
}, {
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_TRIGGER_STATUS,   // enableBits
	  0,                        // serverId
	  0,                        // hostId
	  0,                        // hostGroupId
	  0x12345,                  // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_INFO,    // triggerSeverity
	  CMP_EQ_GT                 // triggerSeverityCompType;
	), // condition
	ACTION_COMMAND,    // type
	"/home/%%\"'@#!()+-~<>?:;",  // working dir
	"/usr/libexec/w",  // path
	30,                // timeout
}, {
	0,                 // id (this filed is ignored)
	ActionCondition(
	  ACTCOND_SERVER_ID | ACTCOND_HOST_ID | ACTCOND_HOST_GROUP_ID |
	  ACTCOND_TRIGGER_ID | ACTCOND_TRIGGER_STATUS,   // enableBits
	  100,                      // serverId
	  0x7fffffffffffffff,       // hostId
	  0x8000000000000000,       // hostGroupId
	  0xfedcba9876543210,       // triggerId
	  TRIGGER_STATUS_PROBLEM,   // triggerStatus
	  TRIGGER_SEVERITY_WARN,    // triggerSeverity
	  CMP_EQ                    // triggerSeverityCompType;
	), // condition
	ACTION_RESIDENT,   // type
	"/tmp",            // working dir
	"/usr/lib/liba.so",// path
	60,                // timeout
},
};

const static size_t NUM_TEST_ACTION_DEF =
  sizeof(testActionDef) / sizeof(ActionDef);

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
	                               actDef.type, actDef.path.c_str(),
	                               actDef.workingDir.c_str(),
	                               actDef.timeout);
	return expect;
}

static string makeExpectedLogString(const ActionDef &actDef, uint64_t logId)
{
	int expectedStarterId = 0; // This is currently not used.
	string expect =
	  StringUtils::sprintf(
	    "%"PRIu64"|%d|%d|%d|%s|%s|%d|%s\n",
	    logId, actDef.id, DBClientAction::ACTLOG_STAT_STARTED,
	    expectedStarterId,
	    DBCONTENT_MAGIC_CURR_DATETIME,
	    DBCONTENT_MAGIC_NULL,
	    DBClientAction::ACTLOG_EXECFAIL_NONE,
	    DBCONTENT_MAGIC_NULL);
	return expect;
}

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
	for (size_t i = 0; i < NUM_TEST_ACTION_DEF; i++) {
		ActionDef &actDef = testActionDef[i];
		dbAction.addAction(actDef);

		// validation
		const int expectedId = i + 1;
		cppcut_assert_equal(expectedId, actDef.id);
		string statement = "select * from actions";
		expect += makeExpectedString(actDef, expectedId);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

void test_startExecAction(void)
{
	string expect;
	DBClientAction dbAction;
	for (size_t i = 0; i < NUM_TEST_ACTION_DEF; i++) {
		const ActionDef &actDef = testActionDef[i];
		uint64_t logId = dbAction.logStartExecAction(actDef);
		const uint64_t expectedId = i + 1;
		cppcut_assert_equal(expectedId, logId);
		string statement = "select * from action_logs";
		expect += makeExpectedLogString(actDef, expectedId);
		assertDBContent(dbAction.getDBAgent(), statement, expect);
	}
}

} // namespace testDBClientAction
