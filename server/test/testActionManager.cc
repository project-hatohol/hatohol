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
#include <sys/types.h> 
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "ActionManager.h"
#include "PipeUtils.h"
#include "SmartBuffer.h"
#include "ActionTp.h"
#include "ResidentProtocol.h"

class TestActionManager : public ActionManager
{
public:
	void callMakeExecArg(StringVector &vect, const string &command)
	{
		makeExecArg(vect, command);
	}

	void callExecCommandAction(const ActionDef &actionDef,
	                           ActorInfo *actorInfo = NULL)
	{
		EventInfo eventInfo;
		execCommandAction(actionDef, eventInfo, actorInfo);
	}

	void callExecResidentAction(const ActionDef &actionDef,
	                            ActorInfo *actorInfo = NULL)
	{
		EventInfo eventInfo;
		execResidentAction(actionDef, eventInfo, actorInfo);
	}
};

namespace testActionManager {

struct ExecCommandContext {
	pid_t actionTpPid;
	GMainLoop *loop;
	bool timedOut;
	guint timerTag;
	PipeUtils readPipe, writePipe;
	ActorInfo actorInfo;
	ActionDef actDef;
	ActionLog actionLog;
	DBClientAction dbAction;
	size_t timeout;

	ExecCommandContext(void)
	: actionTpPid(0),
	  loop(NULL),
	  timedOut(false),
	  timerTag(0),
	  timeout(5 * 1000)
	{
	}

	virtual ~ExecCommandContext()
	{
		if (actionTpPid) {
			int signo = SIGKILL;
			if (kill(actionTpPid, signo) == -1) {
				// We cannot stop the clean up.
				// So we don't use cut_assert_...() here
				cut_notify(
				  "Failed to call kill: "
				  "pid: %d, signo: %d, %s\n",
				  actionTpPid, signo, strerror(errno));
			}
		}

		if (loop)
			g_main_loop_unref(loop);

		if (timerTag)
			g_source_remove(timerTag);
	}
};
static ExecCommandContext *g_execCommandCtx = NULL;

static gboolean timeoutHandler(gpointer data)
{
	ExecCommandContext *ctx = (ExecCommandContext *)data;
	ctx->timedOut = true;
	ctx->timerTag = 0;
	return FALSE;
}

static void waitConnect(PipeUtils &readPipe, size_t timeout)
{
	// read data
	ActionTpCommHeader header;
	cppcut_assert_equal(
	  true, readPipe.recv(sizeof(header), &header, timeout));
	cppcut_assert_equal((uint32_t)ACTTP_CODE_BEGIN, header.code);
	cppcut_assert_equal((uint32_t)ACTTP_FLAGS_RES,
	                    ACTTP_FLAGS_RES & header.flags);
}

static void getArguments(PipeUtils &readPipe, PipeUtils &writePipe,
                         size_t timeout, StringVector &argVect)
{
	// write command
	ActionTpCommHeader header;
	header.length = sizeof(header);
	header.code = ACTTP_CODE_GET_ARG_LIST;
	header.flags = ACTTP_FLAGS_REQ;
	writePipe.send(header.length, &header);

	// read response
	ActionTpCommHeader response;
	cppcut_assert_equal(
	  true, readPipe.recv(sizeof(response), &response, timeout));
	cppcut_assert_equal((uint32_t)ACTTP_CODE_GET_ARG_LIST, response.code);
	cppcut_assert_equal((uint32_t)ACTTP_FLAGS_RES,
	                    ACTTP_FLAGS_RES & response.flags);

	// check the response
	size_t bodySize = response.length - sizeof(ActionTpCommHeader);
	SmartBuffer pktBody(bodySize);
	cppcut_assert_equal(
	  true, readPipe.recv(bodySize, (uint8_t *)pktBody, timeout));

	size_t numArg = pktBody.getValueAndIncIndex<uint16_t>();
	cppcut_assert_equal(argVect.size(), numArg);
	for (size_t i = 0; i < numArg; i++) {
		size_t actualLen = pktBody.getValueAndIncIndex<uint16_t>();
		string actualArg(pktBody.getPointer<const char>(), actualLen);
		pktBody.incIndex(actualLen);
		cppcut_assert_equal(argVect[i], actualArg);
	}
}

static void sendQuit(PipeUtils &readPipe, PipeUtils &writePipe, size_t timeout)
{
	// write command
	ActionTpCommHeader header;
	header.length = sizeof(header);
	header.code = ACTTP_CODE_QUIT;
	header.flags = ACTTP_FLAGS_REQ;
	writePipe.send(header.length, &header);

	// read response
	ActionTpCommHeader response;
	cppcut_assert_equal(
	  true, readPipe.recv(sizeof(response), &response, timeout));
	cppcut_assert_equal((uint32_t)ACTTP_CODE_QUIT, response.code);
	cppcut_assert_equal((uint32_t)ACTTP_FLAGS_RES,
	                    ACTTP_FLAGS_RES & response.flags);
}

static string getActionLogContent(const ActionLog &actionLog)
{
	string str = StringUtils::sprintf(
	  "ID: %"PRIu64", actionID: %d, status: %d, ",
	  actionLog.id, actionLog.actionId, actionLog.status);
	str += StringUtils::sprintf(
	  "queuingTime: %d, startTime: %d, endTime: %d,",
	  actionLog.queuingTime, actionLog.startTime, actionLog.endTime);
	str += StringUtils::sprintf(
	  "failureCode: %d, exitCode: %d",
	  actionLog.failureCode, actionLog.exitCode);
	return str;
}

static void _assertActionLog(
  ActionLog &actionLog,
 uint64_t id, int actionId, int status, int starterId, int queueintTime,
 int startTime, int endTime, int failureCode, int exitCode, uint32_t nullFlags)
{
	cppcut_assert_equal(nullFlags,    actionLog.nullFlags,
	  cut_message("%s", getActionLogContent(actionLog).c_str()));

	cppcut_assert_equal(id,           actionLog.id);
	cppcut_assert_equal(actionId,     actionLog.actionId);
	cppcut_assert_equal(status,       actionLog.status);
	cppcut_assert_equal(starterId,    actionLog.starterId);
	if (!(nullFlags & ACTLOG_FLAG_QUEUING_TIME))
		cppcut_assert_equal(queueintTime, actionLog.queuingTime);
	if (!(nullFlags & ACTLOG_FLAG_START_TIME))
		assertDatetime(startTime, actionLog.startTime);
	if (!(nullFlags & ACTLOG_FLAG_END_TIME))
		assertDatetime(endTime, actionLog.endTime);
	cppcut_assert_equal(failureCode,  actionLog.failureCode);
	if (!(nullFlags & ACTLOG_FLAG_EXIT_CODE))
		cppcut_assert_equal(exitCode,     actionLog.exitCode);
}
#define assertActionLog(LOG, ID, ACT_ID, STAT, STID, QTIME, STIME, ETIME, FAIL_CODE, EXIT_CODE, NULL_FLAGS) \
cut_trace(_assertActionLog(LOG, ID, ACT_ID, STAT, STID, QTIME, STIME, ETIME, FAIL_CODE, EXIT_CODE, NULL_FLAGS))

static void _assertExecAction(ExecCommandContext *ctx, int id, ActionType type)
{
	// make PIPEs
	cppcut_assert_equal(true, ctx->readPipe.makeFileInTmpAndOpenForRead());
	cppcut_assert_equal(true, ctx->writePipe.makeFileInTmpAndOpenForWrite());
	// execute 
	ctx->actDef.id = id;
	ctx->actDef.type = type;

	// launch ActionTp (the actor)
	TestActionManager actMgr;
	ctx->actorInfo.pid = 0;
	if (type == ACTION_COMMAND) {
		ctx->actDef.path = StringUtils::sprintf(
		  "%s %s %s", cut_build_path(".libs", "ActionTp", NULL),
		  ctx->writePipe.getPath().c_str(),
		  ctx->readPipe.getPath().c_str());
		actMgr.callExecCommandAction(ctx->actDef, &ctx->actorInfo);
	} else if (type == ACTION_RESIDENT) {
		ctx->actDef.path =
		  cut_build_path(".libs", "residentTest.so", NULL);
		actMgr.callExecResidentAction(ctx->actDef, &ctx->actorInfo);
	} else {
		cut_fail("Unknown type: %d\n", type);
	}
	ctx->actionTpPid = ctx->actorInfo.pid;
}
#define assertExecAction(CTX, ID, TYPE) \
cut_trace(_assertExecAction(CTX, ID, TYPE))

void _assertActionLogJustAfterExec(ExecCommandContext *ctx)
{
	uint32_t expectedNullFlags = 
	  ACTLOG_FLAG_QUEUING_TIME | ACTLOG_FLAG_END_TIME |
	  ACTLOG_FLAG_EXIT_CODE;
	cppcut_assert_equal(
	  true, ctx->dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));
	assertActionLog(
	  ctx->actionLog, ctx->actorInfo.logId,
	  ctx->actDef.id, DBClientAction::ACTLOG_STAT_STARTED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  0, /* endTime */
	  DBClientAction::ACTLOG_EXECFAIL_NONE, /* failureCode */
	  0,  /* exitCode */
	  expectedNullFlags);

	// connect to ActionTp
	waitConnect(ctx->readPipe, ctx->timeout);
	StringVector argVect;
	argVect.push_back(ctx->writePipe.getPath());
	argVect.push_back(ctx->readPipe.getPath());
	getArguments(ctx->readPipe, ctx->writePipe, ctx->timeout, argVect);
}
#define assertActionLogJustAfterExec(CTX) \
cut_trace(_assertActionLogJustAfterExec(CTX))

void _assertActionLogAfterEnding(ExecCommandContext *ctx)
{
	// check the action log after the actor is terminated
	ctx->timerTag = g_timeout_add(ctx->timeout, timeoutHandler, ctx);
	ctx->loop = g_main_loop_new(NULL, TRUE);
	while (true) {
		// ActionCollector updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations of the loop.
		while (g_main_iteration(TRUE))
			cppcut_assert_equal(false, ctx->timedOut);
		cppcut_assert_equal(
		  true, ctx->dbAction.getLog(ctx->actionLog,
		                             ctx->actorInfo.logId));
		if (ctx->actionLog.status ==
		    DBClientAction::ACTLOG_STAT_STARTED)
			continue;
		assertActionLog(
		  ctx->actionLog, ctx->actorInfo.logId,
		  ctx->actDef.id, DBClientAction::ACTLOG_STAT_SUCCEEDED,
		  0, /* starterId */
		  0, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  DBClientAction::ACTLOG_EXECFAIL_NONE, /* failureCode */
		  EXIT_SUCCESS, /* exitCode */
		  ACTLOG_FLAG_QUEUING_TIME /* nullFlags */);
		break;
	}
}
#define assertActionLogAfterEnding(CTX) \
cut_trace(_assertActionLogAfterEnding(CTX))

void _assertActionLogAfterExecResident(
  ExecCommandContext *ctx, uint32_t expectedNullFlags,
  DBClientAction::ActionLogStatus currStatus,
  DBClientAction::ActionLogStatus newStatus)
{
	// check the action log after the actor is terminated
	ctx->timerTag = g_timeout_add(ctx->timeout, timeoutHandler, ctx);
	ctx->loop = g_main_loop_new(NULL, TRUE);

	while (true) {
		// ActionManager updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations of the loop.
		while (g_main_iteration(TRUE))
			cppcut_assert_equal(false, ctx->timedOut);
		cppcut_assert_equal(
		  true, ctx->dbAction.getLog(ctx->actionLog,
		                             ctx->actorInfo.logId));
		if (ctx->actionLog.status == currStatus)
			continue;
		assertActionLog(
		  ctx->actionLog, ctx->actorInfo.logId,
		  ctx->actDef.id, newStatus,
		  0, /* starterId */
		  0, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  DBClientAction::ACTLOG_EXECFAIL_NONE, /* failureCode */
		  NOTIFY_EVENT_ACK_OK, /* exitCode */
		  expectedNullFlags /* nullFlags */);

		if (newStatus == DBClientAction::ACTLOG_STAT_SUCCEEDED)
			break;
		currStatus = DBClientAction::ACTLOG_STAT_STARTED;
		newStatus = DBClientAction::ACTLOG_STAT_SUCCEEDED;
		expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	}
}
#define assertActionLogAfterExecResident(CTX, ...) \
cut_trace(_assertActionLogAfterExecResident(CTX, ##__VA_ARGS__))

void setup(void)
{
	hatoholInit();
	setupTestDBAction();
}

void teardown(void)
{
	if (g_execCommandCtx) {
		delete g_execCommandCtx;
		g_execCommandCtx = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_execCommandAction(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	assertExecAction(ctx, 2343242, ACTION_COMMAND);
	assertActionLogJustAfterExec(ctx);
	sendQuit(ctx->readPipe, ctx->writePipe, ctx->timeout);
	assertActionLogAfterEnding(ctx);
}

void test_execResidentAction(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	assertExecAction(ctx, 0x4ab3fd32, ACTION_RESIDENT);

	uint32_t expectedNullFlags =
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|ACTLOG_FLAG_EXIT_CODE;
	assertActionLogAfterExecResident(
	  ctx, expectedNullFlags,
	  DBClientAction::ACTLOG_STAT_LAUNCHING_RESIDENT,
	  DBClientAction::ACTLOG_STAT_STARTED);
}

void test_execResidentActionManyEvents(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	uint32_t expectedNullFlags;
	DBClientAction::ActionLogStatus currStatus;
	DBClientAction::ActionLogStatus newStatus;
	for (size_t i = 0; i < 3; i++) {
		if (i == 0) {
			expectedNullFlags =
			  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|
			  ACTLOG_FLAG_EXIT_CODE;
			currStatus =
			  DBClientAction::ACTLOG_STAT_LAUNCHING_RESIDENT;
			newStatus =
			  DBClientAction::ACTLOG_STAT_STARTED;
		} else {
			expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
			currStatus =
			  DBClientAction::ACTLOG_STAT_STARTED;
			newStatus =
			  DBClientAction::ACTLOG_STAT_SUCCEEDED;
		}
		assertExecAction(ctx, 0x4ab3fd32, ACTION_RESIDENT);
		assertActionLogAfterExecResident(ctx, expectedNullFlags,
		                                 currStatus, newStatus);
	}
}

void test_execResidentActionManyEventsGenThenCheckLog(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	vector<ActorInfo> actorVect;
	size_t numEvents = 3;
	for (size_t i = 0; i < numEvents; i++) {
		assertExecAction(ctx, 0x4ab3fd32, ACTION_RESIDENT);
		actorVect.push_back(ctx->actorInfo);
	}

	uint32_t expectedNullFlags;
	DBClientAction::ActionLogStatus currStatus;
	DBClientAction::ActionLogStatus newStatus;
	for (size_t i = 0; i < numEvents; i++) {
		if (i == 0) {
			expectedNullFlags =
			  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|
			  ACTLOG_FLAG_EXIT_CODE;
			currStatus =
			  DBClientAction::ACTLOG_STAT_LAUNCHING_RESIDENT;
			newStatus =
			  DBClientAction::ACTLOG_STAT_STARTED;
		} else {
			expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
			currStatus =
			  DBClientAction::ACTLOG_STAT_STARTED;
			newStatus =
			  DBClientAction::ACTLOG_STAT_SUCCEEDED;
		}
		ctx->actorInfo = actorVect[i];
		assertActionLogAfterExecResident(ctx, expectedNullFlags,
		                                 currStatus, newStatus);
	}
}

// TODO: make tests for the following error cases.
// - The path of the actor is wrong (failed to launch)
// - An actor is killed by the timed out.
// - The pipe of actor connected to hatohol unexpectedly is closed.
// - Unexpected exit of the actor.

} // namespace testActionManager

namespace testActionManagerWithoutDB {

static void _assertMakeExecArgs(const string &cmdLine,
                                const char *firstExpect, ...)
{
	va_list ap;
	va_start(ap, firstExpect);
	StringVector inArgVect;
	inArgVect.push_back(firstExpect);
	while (true) {
		const char *word = va_arg(ap, const char *);
		if (!word)
			break;
		inArgVect.push_back(word);
	}
	va_end(ap);

	TestActionManager actMgr;
	StringVector argVect;
	actMgr.callMakeExecArg(argVect, cmdLine);
	cppcut_assert_equal(inArgVect.size(), argVect.size());
	for (size_t i = 0; i < inArgVect.size(); i++) {
		cppcut_assert_equal(inArgVect[i], argVect[i],
		                    cut_message("index: %zd", i));
	}
}
#define assertMakeExecArgs(CMD_LINE, FIRST, ...) \
cut_trace(_assertMakeExecArgs(CMD_LINE, FIRST, ##__VA_ARGS__))

void setup(void)
{
	static bool initDone = false;
	if (!initDone) {
		hatoholInit();
		setupTestDBAction();
		initDone = true;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_makeExecArg(void)
{
	assertMakeExecArgs("ls", "ls", NULL);
}

void test_makeExecArgTwo(void)
{
	assertMakeExecArgs("ls -l", "ls", "-l", NULL);
}

void test_makeExecArgQuot(void)
{
	assertMakeExecArgs("abc -n 'big bang'", "abc", "-n", "big bang", NULL);
}

void test_makeExecArgQuotEsc(void)
{
	assertMakeExecArgs("abc 'I\\'m a dog'", "abc", "I'm a dog", NULL);
}

void test_makeExecArgQuotEscDouble(void)
{
	assertMakeExecArgs("abc '\\'\\'dog'", "abc", "''dog", NULL);
}

void test_makeExecArgBackslashEscDouble(void)
{
	assertMakeExecArgs("abc '\\\\dog'", "abc", "\\dog", NULL);
}

void test_makeExecArgSimpleQuot(void)
{
	assertMakeExecArgs("I\\'m \\'p_q\\'", "I'm", "'p_q'", NULL);
}

void test_makeExecArgSimpleBackslashInQuot(void)
{
	assertMakeExecArgs("'\\ABC'", "ABC", NULL);
}

} // namespace testActionManagerWithoutDB
