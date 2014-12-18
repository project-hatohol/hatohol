/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <cppcutter.h>
#include "Hatohol.h"
#include "FaceRest.h"
#include "Helpers.h"
#include "JSONParser.h"
#include "DBTablesTest.h"
#include "MultiLangTest.h"
#include "testDBTablesMonitoring.h"
#include "ThreadLocalDBCache.h"
#include "FaceRestTestUtils.h"
using namespace std;
using namespace mlpl;

namespace testFaceRestAction {

static JSONParser *g_parser = NULL;

struct LocaleInfo {
	string lcAll;
	string lang;
};

static LocaleInfo *g_localeInfo = NULL;

static void changeLocale(const char *locale)
{
	// save the current locale
	if (!g_localeInfo) {
		g_localeInfo = new LocaleInfo();
		char *env;
		env = getenv("LC_ALL");
		if (env)
			g_localeInfo->lcAll = env;
		env = getenv("LANG");
		if (env)
			g_localeInfo->lang = env;
	}

	// set new locale
	setenv("LC_ALL", locale, 1);
	setenv("LANG", locale, 1);
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

void cut_teardown(void)
{
	stopFaceRest();

	delete g_parser;
	g_parser = NULL;

	if (g_localeInfo) {
		if (!g_localeInfo->lcAll.empty())
			setenv("LC_ALL", g_localeInfo->lcAll.c_str(), 1);
		if (!g_localeInfo->lang.empty())
			setenv("LANG", g_localeInfo->lang.c_str(), 1);
		delete g_localeInfo;
		g_localeInfo = NULL;
	}
}

template<typename T>
static void _assertActionCondition(
  JSONParser *parser, const ActionCondition &cond,
   const string &member, ActionConditionEnableFlag bit, T expect)
{
	if (cond.isEnable(bit)) {
		assertValueInParser(parser, member, expect);
	} else {
		assertNullInParser(parser, member);
	}
}
#define asssertActionCondition(PARSER, COND, MEMBER, BIT, T, EXPECT) \
cut_trace(_assertActionCondition<T>(PARSER, COND, MEMBER, BIT, EXPECT))

static void _assertActions(const string &path, const string &callbackName = "",
			   const ActionType actionType = ACTION_USER_DEFINED)
{
	startFaceRest();
	RequestArg arg(path, callbackName);
	arg.parameters["type"]
	  = StringUtils::sprintf("%d", actionType);
	arg.userId = findUserWith(OPPRVLG_GET_ALL_USER);
	g_parser = getResponseAsJSONParser(arg);
	assertErrorCode(g_parser);
	assertValueInParser(g_parser, "numberOfActions",
			    getNumberOfTestActions(actionType));
	assertStartObject(g_parser, "actions");
	size_t idx = 0;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const ActionDef &actionDef = testActionDef[i];
		if (filterOutAction(actionDef, actionType))
			continue;
		g_parser->startElement(idx++);
		const ActionCondition &cond = actionDef.condition;
		assertValueInParser(g_parser, "actionId", actionDef.id);

		assertValueInParser(g_parser, "enableBits", cond.enableBits);
		asssertActionCondition(
		  g_parser, cond, "serverId", ACTCOND_SERVER_ID,
		  ServerIdType, cond.serverId);
		asssertActionCondition(
		  g_parser, cond, "hostId", ACTCOND_HOST_ID,
		  string, StringUtils::toString(cond.hostId));
		asssertActionCondition(
		  g_parser, cond, "hostgroupId", ACTCOND_HOST_GROUP_ID,
		  uint64_t, cond.hostgroupId);
		asssertActionCondition(
		  g_parser, cond, "triggerId", ACTCOND_TRIGGER_ID,
		  string, StringUtils::toString(cond.triggerId));
		asssertActionCondition(
		  g_parser, cond, "triggerStatus", ACTCOND_TRIGGER_STATUS,
		  uint32_t, cond.triggerStatus);
		asssertActionCondition(
		  g_parser, cond, "triggerSeverity", ACTCOND_TRIGGER_SEVERITY,
		  uint32_t, cond.triggerSeverity);

		uint32_t expectCompType
		  = cond.isEnable(ACTCOND_TRIGGER_SEVERITY) ?
		    (uint32_t)cond.triggerSeverityCompType : CMP_INVALID;
		assertValueInParser(g_parser,
		                    "triggerSeverityComparatorType",
		                    expectCompType);

		assertValueInParser(g_parser, "type", actionDef.type);
		assertValueInParser(g_parser, "workingDirectory",
		                    actionDef.workingDir);
		assertValueInParser(g_parser, "command", actionDef.command);
		assertValueInParser(g_parser, "timeout", actionDef.timeout);
		g_parser->endElement();
	}
	g_parser->endObject();
	assertServersIdNameHashInParser(g_parser);
}
#define assertActions(P,...) cut_trace(_assertActions(P,##__VA_ARGS__))

#define assertAddAction(P, ...) \
cut_trace(_assertAddRecord(P, "/action", ##__VA_ARGS__))

void data_actionsJSONP(void)
{
	gcut_add_datum("Normal actions",
		       "type", G_TYPE_INT, ACTION_USER_DEFINED,
		       NULL);
	gcut_add_datum("IncidentSenderAction",
		       "type", G_TYPE_INT, ACTION_INCIDENT_SENDER,
		       NULL);
	gcut_add_datum("All",
		       "type", G_TYPE_INT, ACTION_ALL,
		       NULL);
}

void test_actionsJSONP(gconstpointer data)
{
	loadTestDBAction();

	const ActionType actionType
	  = static_cast<ActionType>(gcut_data_get_int(data, "type"));
	assertActions("/action", "foo", actionType);
}

void test_addAction(void)
{
	int type = ACTION_COMMAND;
	const string command = "makan-kosappo";
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", type);
	params["command"] = command;
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

	// check the content in the DB
	string statement = "select * from ";
	statement += DBTablesAction::getTableNameActions();
	string expect;
	int expectedId = 1;
	expect += StringUtils::sprintf("%d|", expectedId);
	expect += "#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|#NULL#|";
	expect += StringUtils::sprintf("%d|",type);
	expect += command;
	expect += "||0"; /* workingDirectory and timeout */
	expect += StringUtils::sprintf("|%" FMT_USER_ID, userId);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, expect);
}

void test_addActionParameterFull(void)
{
	const string command = "/usr/bin/pochi";
	const string workingDir = "/usr/local/wani";
	int type = ACTION_COMMAND;
	int timeout = 300;
	int serverId= 50;
	uint64_t hostId = 50;
	uint64_t hostgroupId = 1000;
	uint64_t triggerId = 333;
	int triggerStatus = TRIGGER_STATUS_PROBLEM;
	int triggerSeverity = TRIGGER_SEVERITY_CRITICAL;
	int triggerSeverityCompType = CMP_EQ_GT;

	StringMap params;
	params["type"]        = StringUtils::sprintf("%d", type);
	params["command"]     = command;
	params["workingDirectory"] = workingDir;
	params["timeout"]     = StringUtils::sprintf("%d", timeout);
	params["serverId"]    = StringUtils::sprintf("%d", serverId);
	params["hostId"]      = StringUtils::sprintf("%" PRIu64, hostId);
	params["hostgroupId"] = StringUtils::sprintf("%" PRIu64, hostgroupId);
	params["triggerId"]   = StringUtils::sprintf("%" PRIu64, triggerId);
	params["triggerStatus"]   = StringUtils::sprintf("%d", triggerStatus);
	params["triggerSeverity"] = StringUtils::sprintf("%d", triggerSeverity);
	params["triggerSeverityCompType"] =
	   StringUtils::sprintf("%d", triggerSeverityCompType);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

	// check the content in the DB
	string statement = "select * from ";
	statement += DBTablesAction::getTableNameActions();
	string expect;
	int expectedId = 1;
	expect += StringUtils::sprintf("%d|%d|", expectedId, serverId);
	expect += StringUtils::sprintf("%" PRIu64 "|%" PRIu64 "|%" PRIu64 "|",
	  hostId, hostgroupId, triggerId);
	expect += StringUtils::sprintf(
	  "%d|%d|%d|", triggerStatus, triggerSeverity, triggerSeverityCompType);
	expect += StringUtils::sprintf("%d|", type);
	expect += command;
	expect += "|";
	expect += workingDir;
	expect += "|";
	expect += StringUtils::sprintf("%d|%" FMT_USER_ID, timeout, userId);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, expect);
}

void test_addActionParameterOver32bit(void)
{
	const string command = "/usr/bin/pochi";
	uint64_t hostId = 0x89abcdef01234567;
	uint64_t hostgroupId = 0xabcdef0123456789;
	uint64_t triggerId = 0x56789abcdef01234;

	StringMap params;
	params["type"]        = StringUtils::sprintf("%d", ACTION_RESIDENT);
	params["command"]     = command;
	params["hostId"]      = StringUtils::sprintf("%" PRIu64, hostId);
	params["hostgroupId"] = StringUtils::sprintf("%" PRIu64, hostgroupId);
	params["triggerId"]   = StringUtils::sprintf("%" PRIu64, triggerId);
	const UserIdType userId = findUserWith(OPPRVLG_CREATE_ACTION);
	assertAddAction(params, userId);

	// check the content in the DB
	string statement = "select host_id, host_group_id, trigger_id from ";
	statement += DBTablesAction::getTableNameActions();
	string expect;
	expect += StringUtils::sprintf("%" PRIu64 "|%" PRIu64 "|%" PRIu64,
	  hostId, hostgroupId, triggerId);
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, expect);
}

void test_addActionComplicatedCommand(void)
{
	const string command =
	   "/usr/bin/@hoge -l '?ABC+{[=:;|.,#*`!$%\\~]}FOX-' --X '$^' --name \"@'v'@\"'";
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION));

	// check the content in the DB
	string statement = "select command from ";
	statement += DBTablesAction::getTableNameActions();
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, command);
}

void test_addActionCommandWithJapanese(void)
{
	changeLocale("en.UTF-8");

	const string command = COMMAND_EX_JP;
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	params["command"] = command;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION));

	// check the content in the DB
	string statement = "select command from ";
	statement += DBTablesAction::getTableNameActions();
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, command);
}

void test_addActionWithoutType(void)
{
	StringMap params;
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionWithoutCommand(void)
{
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", ACTION_COMMAND);
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_NOT_FOUND_PARAMETER);
}

void test_addActionInvalidType(void)
{
	StringMap params;
	params["type"] = StringUtils::sprintf("%d", NUM_ACTION_TYPES);
	assertAddAction(params, findUserWith(OPPRVLG_CREATE_ACTION),
	                HTERR_INVALID_PARAMETER);
}

void test_deleteAction(void)
{
	loadTestDBAction();

	startFaceRest();

	int targetId = 2;
	string url = StringUtils::sprintf("/action/%d", targetId);
	RequestArg arg(url, "cbname");
	arg.request = "DELETE";
	arg.userId = findUserWith(OPPRVLG_DELETE_ACTION);
	g_parser = getResponseAsJSONParser(arg);

	// check the reply
	assertErrorCode(g_parser);

	// check DB
	string expect;
	for (size_t i = 0; i < NumTestActionDef; i++) {
		const int expectedId = i + 1;
		if (expectedId == targetId)
			continue;
		expect += StringUtils::sprintf("%d\n", expectedId);
	}
	string statement = "select action_id from ";
	statement += DBTablesAction::getTableNameActions();
	statement += " order by action_id asc";
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getAction().getDBAgent(), statement, expect);
}

} // namespace testFaceRestAction
