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
		execCommandAction(actionDef, actorInfo);
	}
};

namespace testActionManager {

struct execCommandActionContext {
	pid_t actionTpPid;
	GMainLoop *loop;
	bool timedOut;
	guint timerTag;

	execCommandActionContext(void)
	: actionTpPid(0),
	  loop(NULL),
	  timedOut(false),
	  timerTag(0)
	{
	}

	virtual ~execCommandActionContext()
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

static gboolean timeoutHandler(gpointer data)
{
	execCommandActionContext *ctx = (execCommandActionContext *)data;
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

static void _assertActionLog(
  ActionLog &actionLog,
 uint64_t id, int actionId, int status, int starterId, int queueintTime,
 int startTime, int endTime, int failureCode, int exitCode, uint32_t nullFlags)
{
	cppcut_assert_equal(nullFlags,    actionLog.nullFlags);

	cppcut_assert_equal(id,           actionLog.id);
	cppcut_assert_equal(actionId,     actionLog.actionId);
	cppcut_assert_equal(status,       actionLog.status);
	cppcut_assert_equal(starterId,    actionLog.starterId);
	if (!(nullFlags & ACTLOG_FLAG_QUEUING_TIME))
		cppcut_assert_equal(queueintTime, actionLog.queuingTime);
	if (!(nullFlags & ACTLOG_FLAG_START_TIME))
		assertCurrDatetime(actionLog.startTime);
	if (!(nullFlags & ACTLOG_FLAG_END_TIME))
		assertCurrDatetime(actionLog.endTime);
	cppcut_assert_equal(failureCode,  actionLog.failureCode);
	if (!(nullFlags & ACTLOG_FLAG_EXIT_CODE))
		cppcut_assert_equal(exitCode,     actionLog.exitCode);
}
#define assertActionLog(LOG, ID, ACT_ID, STAT, STID, QTIME, STIME, ETIME, FAIL_CODE, EXIT_CODE, NULL_FLAGS) \
cut_trace(_assertActionLog(LOG, ID, ACT_ID, STAT, STID, QTIME, STIME, ETIME, FAIL_CODE, EXIT_CODE, NULL_FLAGS))

void setup(void)
{
	hatoholInit();
	setupTestDBAction();
}

void teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_execCommandAction(void)
{
	execCommandActionContext ctx;

	PipeUtils readPipe, writePipe;
	cppcut_assert_equal(true, readPipe.makeFileInTmpAndOpenForRead());
	cppcut_assert_equal(true, writePipe.makeFileInTmpAndOpenForWrite());

	ActionDef actDef;
	actDef.id = 2343242;
	actDef.type = ACTION_COMMAND;
	actDef.path = StringUtils::sprintf(
	  "%s %s %s", cut_build_path("ActionTp", NULL),
	  writePipe.getPath().c_str(), readPipe.getPath().c_str());

	TestActionManager actMgr;
	ActorInfo actorInfo;
	actorInfo.pid = 0;
	actMgr.callExecCommandAction(actDef, &actorInfo);
	ctx.actionTpPid = actorInfo.pid;

	// check the action log
	ActionLog actionLog;
	DBClientAction dbAction;
	uint32_t expectedNullFlags = 
	  ACTLOG_FLAG_QUEUING_TIME | ACTLOG_FLAG_END_TIME |
	  ACTLOG_FLAG_EXIT_CODE;
	cppcut_assert_equal(true, dbAction.getLog(actionLog, actorInfo.logId));
	assertActionLog(
	  actionLog, actorInfo.logId,
	  actDef.id, DBClientAction::ACTLOG_STAT_STARTED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  0, /* endTime */
	  DBClientAction::ACTLOG_EXECFAIL_NONE, /* failureCode */
	  0,  /* exitCode */
	  expectedNullFlags);

	// connect to action-tp
	size_t timeout = 5 * 1000;
	waitConnect(readPipe, timeout);
	StringVector argVect;
	argVect.push_back(writePipe.getPath());
	argVect.push_back(readPipe.getPath());
	getArguments(readPipe, writePipe, timeout, argVect);

	// exit
	sendQuit(readPipe, writePipe, timeout);

	// check the action log after the actor is terminated
	ctx.timerTag = g_timeout_add(timeout, timeoutHandler, &ctx);
	ctx.loop = g_main_loop_new(NULL, TRUE);
	while (true) {
		// ActionCollector updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations of the loop.
		while (g_main_iteration(TRUE))
			cppcut_assert_equal(false, ctx.timedOut);
		cppcut_assert_equal(
		  true, dbAction.getLog(actionLog, actorInfo.logId));
		if (actionLog.status == DBClientAction::ACTLOG_STAT_STARTED)
			continue;
		assertActionLog(
		  actionLog, actorInfo.logId,
		  actDef.id, DBClientAction::ACTLOG_STAT_SUCCEEDED,
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
