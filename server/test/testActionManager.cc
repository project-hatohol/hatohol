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
#include "SmartBuffer.h"
#include "ActionTp.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
#include "NamedPipe.h"
#include "residentTest.h"
#include "DBClientTest.h"

class TestActionManager : public ActionManager
{
public:
	void callMakeExecArg(StringVector &vect, const string &command)
	{
		makeExecArg(vect, command);
	}

	void callParseResidentCommand(const string &command,
	                               string &path, string &option)
	{
		parseResidentCommand(command, path, option);
	}

	void callExecCommandAction(const ActionDef &actionDef,
	                            const EventInfo &eventInfo,
	                           ActorInfo *actorInfo = NULL)
	{
		execCommandAction(actionDef, eventInfo, actorInfo);
	}

	void callExecResidentAction(const ActionDef &actionDef,
	                            const EventInfo &eventInfo,
	                            ActorInfo *actorInfo = NULL)
	{
		execResidentAction(actionDef, eventInfo, actorInfo);
	}
};

namespace testActionManager {

struct ExecCommandContext : public ResidentPullHelper<ExecCommandContext> {
	pid_t actionTpPid;
	bool timedOut;
	guint timerTag;
	ActorInfo actorInfo;
	ActionDef actDef;
	ActionLog actionLog;
	DBClientAction dbAction;
	size_t timeout;
	string pipeName;
	NamedPipe pipeRd, pipeWr;
	bool expectHup;
	bool requestedEventInfo;
	bool receivedEventInfo;
	EventInfo eventInfo;
	bool receivedActTpBegin;
	bool receivedActTpArgList;
	SmartBuffer argListBuf;
	bool receivedActTpQuit;

	ExecCommandContext(void)
	: actionTpPid(0),
	  timedOut(false),
	  timerTag(0),
	  timeout(5 * 1000),
	  pipeRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  expectHup(false),
	  requestedEventInfo(false),
	  receivedEventInfo(false),
	  receivedActTpBegin(false),
	  receivedActTpArgList(false),
	  receivedActTpQuit(false)
	{
		timerTag = g_timeout_add(timeout, timeoutHandler, this);
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

		if (timerTag)
			g_source_remove(timerTag);
	}
	
	static gboolean pipeErrCb(GIOChannel *source,
	                          GIOCondition condition, gpointer data)
	{
		ExecCommandContext *ctx =
		  static_cast<ExecCommandContext *>(data);
		if (ctx->expectHup && condition == G_IO_HUP) {
			// When FLASE is returned, the error message is
			// shown on the destructor of NamedPipe due to
			// a failure of removing event source.
			return TRUE;
		}
		cut_fail("pipeError: %s\n",
		         Utils::getStringFromGIOCondition(condition).c_str());
		return FALSE;
	}

	void initPipes(const string &name)
	{
		pipeName = name;
		cppcut_assert_equal(true, pipeRd.init(name, pipeErrCb, this));
		cppcut_assert_equal(true, pipeWr.init(name, pipeErrCb, this));
		initResidentPullHelper(&pipeRd, this);
	}

	static gboolean timeoutHandler(gpointer data)
	{
		ExecCommandContext *ctx = (ExecCommandContext *)data;
		ctx->timedOut = true;
		ctx->timerTag = 0;
		return FALSE;
	}
};
static ExecCommandContext *g_execCommandCtx = NULL;

static void waitConnectCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                          size_t size, ExecCommandContext *ctx)
{
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	cppcut_assert_equal((int)ACTTP_CODE_BEGIN, pktType);
	ctx->receivedActTpBegin = true;
}

static void waitConnect(ExecCommandContext *ctx)
{
	ctx->pullHeader(waitConnectCb);

	while (!ctx->receivedActTpBegin) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}
}

static void getArgumentsBottomHalfCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                     size_t size, ExecCommandContext *ctx)
{
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	ctx->argListBuf.addEx(sbuf.getPointer<void>(), size);
	ctx->receivedActTpArgList = true;
}

static void getArgumentsTopHalfCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                  size_t size, ExecCommandContext *ctx)
{
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);

	// request to get the remaining data
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	cppcut_assert_equal((int)ACTTP_REPLY_GET_ARG_LIST, pktType);

	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);
	size_t requestLen = sbuf.getValueAndIncIndex<uint16_t>();
	ctx->pullData(requestLen, getArgumentsBottomHalfCb);
}

static void getArguments(ExecCommandContext *ctx,
                         const StringVector &expectedArgs)
{
	// send command
	ResidentCommunicator comm;
	comm.setHeader(0, ACTTP_CODE_GET_ARG_LIST);
	comm.push(ctx->pipeWr); 

	// wait the replay
	ctx->pullData(RESIDENT_PROTO_HEADER_LEN + ACTTP_ARG_LIST_SIZE_LEN,
                      getArgumentsTopHalfCb);
	while (!ctx->receivedActTpArgList) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}

	// check the response
	ctx->argListBuf.resetIndex();
	size_t numArg = ctx->argListBuf.getValueAndIncIndex<uint16_t>();
	cppcut_assert_equal(expectedArgs.size(), numArg);
	for (size_t i = 0; i < numArg; i++) {
		size_t actualLen =
		   ctx->argListBuf.getValueAndIncIndex<uint16_t>();
		string actualArg(
		  ctx->argListBuf.getPointer<const char>(), actualLen);
		ctx->argListBuf.incIndex(actualLen);
		cppcut_assert_equal(expectedArgs[i], actualArg);
	}
}

static void waitActTpQuitCb(GIOStatus stat, SmartBuffer &sbuf,
                            size_t size, ExecCommandContext *ctx)
{
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	cppcut_assert_equal((int)ACTTP_REPLAY_QUIT, pktType);
	ctx->receivedActTpQuit = true;
}

static void sendQuit(ExecCommandContext *ctx)
{
	ResidentCommunicator comm;
	comm.setHeader(0, ACTTP_CODE_QUIT);
	comm.push(ctx->pipeWr); 

	// read response
	ctx->pullHeader(waitActTpQuitCb);

	ctx->expectHup = true;
	while (!ctx->receivedActTpQuit) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}
}

static string getActionLogContent(const ActionLog &actionLog)
{
	string str = StringUtils::sprintf(
	  "ID: %"PRIu64", actionID: %d, status: %d, ",
	  actionLog.id, actionLog.actionId, actionLog.status);
	str += StringUtils::sprintf(
	  "queuingTime: %d, startTime: %d, endTime: %d, ",
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

struct ExecActionArg {

	int        actionId;
	ActionType type;
	bool       usePipe;
	string     command;
	string     option;

	// methods
	ExecActionArg(int _actionId, ActionType _type)
	: actionId(_actionId),
	  type(_type),
	  usePipe(false)
	{
	}
};

static void _assertExecAction(ExecCommandContext *ctx, ExecActionArg &arg)
{
	// execute 
	ctx->actDef.id = arg.actionId;
	ctx->actDef.type = arg.type;

	// make a command line
	if (!arg.command.empty())
		ctx->actDef.command = arg.command;
	else if (arg.type == ACTION_COMMAND) {
		ctx->actDef.command = StringUtils::sprintf(
		  "%s %s", cut_build_path(".libs", "ActionTp", NULL),
		  ctx->pipeName.c_str());
	} else if (arg.type == ACTION_RESIDENT) {
		ctx->actDef.command =
		  cut_build_path(".libs", "residentTest.so", NULL);
	} else {
		cut_fail("Unknown type: %d\n", arg.type);
	}
	if (!arg.option.empty()) {
		ctx->actDef.command += " ";
		ctx->actDef.command += arg.option;
	}

	// launch ActionTp (the actor)
	TestActionManager actMgr;
	ctx->actorInfo.pid = 0;
	ctx->actorInfo.logId = 0;
	if (arg.type == ACTION_COMMAND) {
		if (arg.usePipe)
			cppcut_assert_equal(false, ctx->pipeName.empty());
		actMgr.callExecCommandAction(ctx->actDef, ctx->eventInfo,
		                             &ctx->actorInfo);
	} else if (arg.type == ACTION_RESIDENT) {
		actMgr.callExecResidentAction(ctx->actDef,
		                              ctx->eventInfo, &ctx->actorInfo);
	}
	ctx->actionTpPid = ctx->actorInfo.pid;
}
#define assertExecAction(CTX, ARG) cut_trace(_assertExecAction(CTX, ARG))

void _assertActionLogJustAfterExec(ExecCommandContext *ctx)
{
	uint32_t expectedNullFlags = 
	  ACTLOG_FLAG_QUEUING_TIME | ACTLOG_FLAG_END_TIME |
	  ACTLOG_FLAG_EXIT_CODE;
	cppcut_assert_equal(
	  true, ctx->dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));
	assertActionLog(
	  ctx->actionLog, ctx->actorInfo.logId,
	  ctx->actDef.id, ACTLOG_STAT_STARTED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  0, /* endTime */
	  ACTLOG_EXECFAIL_NONE, /* failureCode */
	  0,  /* exitCode */
	  expectedNullFlags);

	// connect to ActionTp and check the command line options
	waitConnect(ctx);
	const EventInfo &evInf = ctx->eventInfo;
	StringVector expectedArgs;
	expectedArgs.push_back(ctx->pipeName);
	expectedArgs.push_back(
	  ActionManager::NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC);
	expectedArgs.push_back(StringUtils::sprintf("%d", ctx->actDef.id));
	expectedArgs.push_back(StringUtils::sprintf("%"PRIu32, evInf.serverId));
	expectedArgs.push_back(StringUtils::sprintf("%"PRIu64, evInf.hostId));
	expectedArgs.push_back(StringUtils::sprintf("%ld.%ld",
	  evInf.time.tv_sec, evInf.time.tv_nsec));
	expectedArgs.push_back(StringUtils::sprintf("%"PRIu64, evInf.id));
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.type));
	expectedArgs.push_back(
	  StringUtils::sprintf("%"PRIu64, evInf.triggerId));
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.status));
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.severity));
	getArguments(ctx, expectedArgs);
}
#define assertActionLogJustAfterExec(CTX) \
cut_trace(_assertActionLogJustAfterExec(CTX))

void _assertWaitForChangeActionLogStatus(ExecCommandContext *ctx,
                                         ActionLogStatus currStatus)
{
	do {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
		cppcut_assert_equal(
		  true, ctx->dbAction.getLog(ctx->actionLog,
		                             ctx->actorInfo.logId));
	} while (ctx->actionLog.status == currStatus);
}
#define assertWaitForChangeActionLogStatus(CTX,STAT) \
cut_trace(_assertWaitForChangeActionLogStatus(CTX,STAT))

void _assertActionLogAfterEnding(ExecCommandContext *ctx)
{
	// check the action log after the actor is terminated
	while (true) {
		// ActionCollector updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations.
		assertWaitForChangeActionLogStatus(ctx, ACTLOG_STAT_STARTED);
		assertActionLog(
		  ctx->actionLog, ctx->actorInfo.logId,
		  ctx->actDef.id, ACTLOG_STAT_SUCCEEDED,
		  0, /* starterId */
		  0, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  ACTLOG_EXECFAIL_NONE, /* failureCode */
		  EXIT_SUCCESS, /* exitCode */
		  ACTLOG_FLAG_QUEUING_TIME /* nullFlags */);
		break;
	}
}
#define assertActionLogAfterEnding(CTX) \
cut_trace(_assertActionLogAfterEnding(CTX))

void _assertActionLogForFailure(ExecCommandContext *ctx, int failureCode)
{
	cppcut_assert_equal(
	  true, ctx->dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));
	assertActionLog(
	  ctx->actionLog,
	  ctx->actorInfo.logId,
	  ctx->actDef.id,
	  ACTLOG_STAT_FAILED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  CURR_DATETIME, /* endTime */
	  failureCode,
	  0, /* exitCode */
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_EXIT_CODE /* nullFlags */);
}
#define assertActionLogForFailure(CTX,FC) \
cut_trace(_assertActionLogForFailure(CTX,FC))

typedef void (*ResidentLogStatusChangedCB)(
  ActionLogStatus currStatus, ActionLogStatus newStatus,
  ExecCommandContext *ctx);

void _assertActionLogAfterExecResident(
  ExecCommandContext *ctx, uint32_t expectedNullFlags,
  ActionLogStatus currStatus, ActionLogStatus newStatus,
  ResidentLogStatusChangedCB statusChangedCb = NULL,
  ActionLogExecFailureCode expectedFailureCode = ACTLOG_EXECFAIL_NONE,
  int expectedExitCode = RESIDENT_MOD_NOTIFY_EVENT_ACK_OK)
{
	while (true) {
		// ActionManager updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations of the loop.
		assertWaitForChangeActionLogStatus(ctx, currStatus);
		assertActionLog(
		  ctx->actionLog, ctx->actorInfo.logId,
		  ctx->actDef.id, newStatus,
		  0, /* starterId */
		  0, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  expectedFailureCode,
		  expectedExitCode,
		  expectedNullFlags /* nullFlags */);

		if (statusChangedCb)
			(*statusChangedCb)(currStatus, newStatus, ctx);

		if (newStatus == ACTLOG_STAT_SUCCEEDED)
			break;
		if (newStatus == ACTLOG_STAT_FAILED)
			break;
		currStatus = ACTLOG_STAT_STARTED;
		newStatus = ACTLOG_STAT_SUCCEEDED;
		expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	}
}
#define assertActionLogAfterExecResident(CTX, ...) \
cut_trace(_assertActionLogAfterExecResident(CTX, ##__VA_ARGS__))

static void setExpectedValueForResidentManyEvents(
  size_t idx, uint32_t &expectedNullFlags,
  ActionLogStatus &currStatus, ActionLogStatus &newStatus)
{
	if (idx == 0) {
		expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME |
		                    ACTLOG_FLAG_END_TIME |
		                    ACTLOG_FLAG_EXIT_CODE;
		currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
		newStatus = ACTLOG_STAT_STARTED;
	} else {
		expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
		currStatus = ACTLOG_STAT_STARTED;
		newStatus = ACTLOG_STAT_SUCCEEDED;
	}
}

static void statusChangedCbForArgCheck(
  ActionLogStatus currStatus, ActionLogStatus newStatus,
  ExecCommandContext *ctx)
{
	if (currStatus != ACTLOG_STAT_STARTED)
		return;
	if (ctx->requestedEventInfo)
		return;
	
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_TEST_CMD_GET_EVENT_INFO);
	comm.push(ctx->pipeWr); 
	ctx->requestedEventInfo = true;
}

static void replyEventInfoCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                             size_t size, ExecCommandContext *ctx)
{
	// status
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);

	// packet type
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	cppcut_assert_equal((int)RESIDENT_TEST_REPLY_GET_EVENT_INFO, pktType);

	// get event information
	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);
	ResidentNotifyEventArg *eventArg = 
	  sbuf.getPointer<ResidentNotifyEventArg>();
	cppcut_assert_equal(ctx->actDef.id, (int)eventArg->actionId);

	// check each component
	const EventInfo &expected = ctx->eventInfo;
	cppcut_assert_equal(expected.serverId, eventArg->serverId);
	cppcut_assert_equal(expected.hostId, eventArg->hostId);
	cppcut_assert_equal(expected.time.tv_sec, eventArg->time.tv_sec);
	cppcut_assert_equal(expected.time.tv_nsec, eventArg->time.tv_nsec);
	cppcut_assert_equal(expected.id, eventArg->eventId);
	cppcut_assert_equal(expected.type, (EventType)eventArg->eventType);
	cppcut_assert_equal(expected.triggerId, eventArg->triggerId);
	cppcut_assert_equal(expected.status,
	                    (TriggerStatusType)eventArg->triggerStatus);
	cppcut_assert_equal(expected.severity,
	                    (TriggerSeverityType)eventArg->triggerSeverity);

	// set a flag to exit the loop in _assertWaitEventBody()
	ctx->receivedEventInfo = true;
}

static void _assertWaitEventBody(ExecCommandContext *ctx)
{
	while (!ctx->receivedEventInfo) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}
}
#define assertWaitEventBody(CTX) cut_trace(_assertWaitEventBody(CTX))

static void _assertWaitRemoveWatching(ExecCommandContext *ctx)
{
	while (ActorCollector::isWatching(ctx->actionTpPid)) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}
}
#define assertWaitRemoveWatching(CTX) cut_trace(_assertWaitRemoveWatching(CTX))

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

	string pipeName = "test-command-action";
	ctx->initPipes(pipeName);

	ctx->eventInfo = testEventInfo[0];
	ExecActionArg arg(2343242, ACTION_COMMAND);
	assertExecAction(ctx, arg);
	assertActionLogJustAfterExec(ctx);
	sendQuit(ctx);
	assertActionLogAfterEnding(ctx);
}

void test_execCommandActionWithWrongPath(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(7869, ACTION_COMMAND);
	arg.usePipe = false;
	arg.command = "wrong-command-dayo";
	assertExecAction(ctx, arg);
	assertActionLogForFailure(ctx, ACTLOG_EXECFAIL_ENTRY_NOT_FOUND);
}

void test_execResidentAction(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	assertExecAction(ctx, arg);

	uint32_t expectedNullFlags =
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|ACTLOG_FLAG_EXIT_CODE;
	assertActionLogAfterExecResident(ctx, expectedNullFlags,
	                                 ACTLOG_STAT_LAUNCHING_RESIDENT,
	                                 ACTLOG_STAT_STARTED);
}

void test_execResidentActionWithWrongPath(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.command = "wrong-command-dayo.so";
	assertExecAction(ctx, arg);
	assertActionLogAfterExecResident(
	  ctx, ACTLOG_FLAG_QUEUING_TIME,
	  ACTLOG_STAT_LAUNCHING_RESIDENT, ACTLOG_STAT_FAILED,
	  NULL, // statusChangedCb
	  ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
	  0     // expectedExitCode
	);
	assertWaitRemoveWatching(ctx);

	// reconfirm the action log. This confirms that the log is not
	// updated in ActorCollector::checkExitProcess().
	cppcut_assert_equal(
	  true, ctx->dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));
	assertActionLog(
	  ctx->actionLog, ctx->actorInfo.logId,
	  ctx->actDef.id, ACTLOG_STAT_FAILED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  CURR_DATETIME, /* endTime */
	  ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
	  0, // exitCode
	  ACTLOG_FLAG_QUEUING_TIME);
}

void test_execResidentActionManyEvents(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	uint32_t expectedNullFlags;
	ActionLogStatus currStatus;
	ActionLogStatus newStatus;
	size_t numEvents = 10;
	for (size_t i = 0; i < numEvents; i++) {
		setExpectedValueForResidentManyEvents(i, expectedNullFlags,
		                                      currStatus, newStatus);
		ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
		assertExecAction(ctx, arg);
		assertActionLogAfterExecResident(ctx, expectedNullFlags,
		                                 currStatus, newStatus);
	}
}

void test_execResidentActionManyEventsGenThenCheckLog(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	vector<ActorInfo> actorVect;
	size_t numEvents = 10;
	for (size_t i = 0; i < numEvents; i++) {
		ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
		assertExecAction(ctx, arg);
		actorVect.push_back(ctx->actorInfo);
	}

	uint32_t expectedNullFlags;
	ActionLogStatus currStatus;
	ActionLogStatus newStatus;
	for (size_t i = 0; i < numEvents; i++) {
		setExpectedValueForResidentManyEvents(i, expectedNullFlags,
		                                      currStatus, newStatus);
		ctx->actorInfo = actorVect[i];
		assertActionLogAfterExecResident(ctx, expectedNullFlags,
		                                 currStatus, newStatus);
	}
}

void test_execResidentActionCheckArg(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	string pipeName = "test-resident-action";
	string option = "--pipename " + pipeName;
	ctx->initPipes(pipeName);
	ctx->pullData(RESIDENT_PROTO_HEADER_LEN +
	              RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN,
	              replyEventInfoCb);
	ctx->eventInfo = testEventInfo[0];
	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.option = option;
	assertExecAction(ctx, arg);

	uint32_t expectedNullFlags =
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|ACTLOG_FLAG_EXIT_CODE;
	assertActionLogAfterExecResident(
	  ctx, expectedNullFlags,
	  ACTLOG_STAT_LAUNCHING_RESIDENT,
	  ACTLOG_STAT_STARTED,
	  statusChangedCbForArgCheck);

	assertWaitEventBody(ctx);
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

static void _assertResidentCommand(const string &expectedPath,
                                   const string &expectedOption,
                                   const char *format)
{
	TestActionManager actMgr;
	string path, option;
	string command = StringUtils::sprintf(
	  format, expectedPath.c_str(), expectedOption.c_str());
	actMgr.callParseResidentCommand(command, path, option);
	cppcut_assert_equal(expectedPath, path);
	cppcut_assert_equal(expectedOption, option);
}
#define assertResidentCommand(PATH, OPT, FMT) \
cut_trace(_assertResidentCommand(PATH, OPT, FMT))

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

void test_parseResidentCommand(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "foo goo", "%s %s");
}

void test_parseResidentCommandNoOption(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "", "%s %s");
}

void test_parseResidentCommandSpaces(void)
{
	assertResidentCommand("/usr/lib/foo.so.1", "foo  goo", "%s   %s  ");
}

} // namespace testActionManagerWithoutDB
