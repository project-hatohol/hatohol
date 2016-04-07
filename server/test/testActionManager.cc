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

#include <cppcutter.h>
#include <sys/types.h> 
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "ActionManager.h"
#include "SmartBuffer.h"
#include "ActionTp.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
#include "NamedPipe.h"
#include "residentTest.h"
#include "DBTablesTest.h"
#include "ConfigManager.h"
#include "SessionManager.h"
#include "IncidentSenderManager.h"
#include "RedmineAPIEmulator.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

class TestActionManager : public ActionManager
{
public:
	void callExecCommandAction(const ActionDef &actionDef,
	                            const EventInfo &eventInfo,
	                           ActorInfo *actorInfo = NULL)
	{
		ThreadLocalDBCache cache;
		DBTablesAction &dbAction = cache.getAction();
		execCommandAction(actionDef, eventInfo, dbAction, actorInfo);
	}

	void callExecResidentAction(const ActionDef &actionDef,
	                            const EventInfo &eventInfo,
	                            ActorInfo *actorInfo = NULL)
	{
		ThreadLocalDBCache cache;
		DBTablesAction &dbAction = cache.getAction();
		execResidentAction(actionDef, eventInfo, dbAction, actorInfo);
	}

	bool callShouldSkipByLog(const EventInfo &eventInfo)
	{
		ThreadLocalDBCache cache;
		DBTablesAction &dbAction = cache.getAction();
		return shouldSkipByLog(eventInfo, dbAction);
	}

	size_t callGetNumberOfOnstageCommandActors(void)
	{
		return getNumberOfOnstageCommandActors();
	}

	static void callSetupPathForAction(string &path, string &ldLibraryPath)
	{
		setupPathForAction(path, ldLibraryPath);
	}

	HatoholError callRunAction(const ActionDef &actionDef,
	                           const EventInfo &eventInfo,
	                           DBTablesAction &dbAction)
	{
		return runAction(actionDef, eventInfo, dbAction);
	}
};

namespace testActionManager {

static const size_t numWaitingActions = 5;

struct ExecCommandContext : public ResidentPullHelper<ExecCommandContext> {
	pid_t actionTpPid;
	bool timedOut;
	guint timerTag;
	ActorInfo actorInfo;
	ActionDef actDef;
	ActionLog actionLog;
	size_t timeout;
	string pipeName;
	NamedPipe pipeRd, pipeWr;
	bool expectHup;
	bool receivedEventInfo;
	EventInfo eventInfo;
	bool receivedActTpBegin;
	bool receivedActTpArgList;
	SmartBuffer argListBuf;
	string sessionId;
	bool receivedActTpQuit;

	ExecCommandContext(void)
	: actionTpPid(0),
	  timedOut(false),
	  timerTag(0),
	  timeout(5 * 1000),
	  pipeRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeWr(NamedPipe::END_TYPE_MASTER_WRITE),
	  expectHup(false),
	  receivedEventInfo(false),
	  receivedActTpBegin(false),
	  receivedActTpArgList(false),
	  receivedActTpQuit(false)
	{
		initActionDef(actDef);
		initEventInfo(eventInfo);
		eventInfo.hostIdInServer = TEST_HOST_ID_STRING;
		timerTag = g_timeout_add(timeout, timeoutHandler, this);
	}

	virtual ~ExecCommandContext()
	{
		if (actionTpPid) {
			int signo = SIGKILL;
			if (kill(actionTpPid, signo) == -1 && errno != ESRCH) {
				// We cannot stop the clean up.
				// So we don't use cut_assert_...() here
				cut_notify(
				  "Failed to call kill: "
				  "pid: %d, signo: %d, %s\n",
				  actionTpPid, signo, g_strerror(errno));
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
		if (ctx->expectHup && condition == G_IO_HUP)
			return FALSE;
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
static vector<ExecCommandContext *>g_execCommandCtxVect;

static ActionLogExecFailureCode getFailureCodeSignalOrDumpedByRLimit(void)
{
	auto isCorePatternUsingPipe = [] {
		string pattern;
		pattern = executeCommand("cat /proc/sys/kernel/core_pattern");
		return !pattern.empty() && pattern[0] == '|';
	};

	if (isCorePatternUsingPipe())
		return ACTLOG_EXECFAIL_DUMPED_SIGNAL;

	struct rlimit rlim;
	errno = 0;
	getrlimit(RLIMIT_CORE, &rlim);
	cut_assert_errno();
	if (rlim.rlim_cur > 0)
		return ACTLOG_EXECFAIL_DUMPED_SIGNAL;
	return ACTLOG_EXECFAIL_KILLED_SIGNAL;
}

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

static void getSessionIdCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                  size_t size, ExecCommandContext *ctx)
{
	cppcut_assert_equal(G_IO_STATUS_NORMAL, stat);

	// request to get the remaining data
	int pktType = ResidentCommunicator::getPacketType(sbuf);
	cppcut_assert_equal((int)ACTTP_REPLY_GET_SESSION_ID, pktType);

	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);

	// Sent data doesn't has NULL terminator.
	// So we added characters one by one.
	const char *buf = sbuf.getPointer<char>();
	for (size_t i = 0; i < ACTTP_SESSION_ID_LEN; i++)
		ctx->sessionId += buf[i];
}

static void _assertSessionInManager(const string &sessionId,
                                    const UserIdType &ownerUserId)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	SessionPtr session = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, session.hasData());
	cppcut_assert_equal(session->userId, ownerUserId);
}
#define assertSessionInManager(SID, OUID) \
cut_trace(_assertSessionInManager(SID, OUID))

static void _assertSessionId(ExecCommandContext *ctx)
{
	// send command
	ResidentCommunicator comm;
	comm.setHeader(0, ACTTP_CODE_GET_SESSION_ID);
	comm.push(ctx->pipeWr); 

	// wait the replay
	ctx->pullData(RESIDENT_PROTO_HEADER_LEN + ACTTP_SESSION_ID_LEN,
	              getSessionIdCb);
	while (ctx->sessionId.empty()) {
		g_main_context_iteration(NULL, TRUE);
		cppcut_assert_equal(false, ctx->timedOut);
	}

	// check the response
	SessionManager *sessionMgr = SessionManager::getInstance();
	SessionPtr session =
	   sessionMgr->getSession(ctx->sessionId);
	cppcut_assert_equal(true, session.hasData());
	cppcut_assert_equal(session->userId, ctx->actDef.ownerUserId);
}
#define assertSessionId(CTX) cut_trace(_assertSessionId(CTX))

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
	  "ID: %" PRIu64 ", actionID: %" FMT_ACTION_ID ", status: %d, ",
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
 uint64_t id, const ActionIdType &actionId,
 int status, int starterId, int queueingTime,
 int startTime, int endTime, int failureCode, int exitCode, uint32_t nullFlags)
{
	cppcut_assert_equal(nullFlags,    actionLog.nullFlags,
	  cut_message("%s", getActionLogContent(actionLog).c_str()));

	cppcut_assert_equal(id,           actionLog.id);
	cppcut_assert_equal(actionId,     actionLog.actionId);
	cppcut_assert_equal(status,       actionLog.status);
	cppcut_assert_equal(starterId,    actionLog.starterId);
	if (!(nullFlags & ACTLOG_FLAG_QUEUING_TIME))
		assertDatetime(queueingTime, actionLog.queuingTime);
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

	ActionIdType actionId;
	ActionType type;
	bool       usePipe;
	string     command;
	string     option;
	int        timeout;

	// methods
	ExecActionArg(const ActionIdType &_actionId, ActionType _type)
	: actionId(_actionId),
	  type(_type),
	  usePipe(false),
	  timeout(0)
	{
	}
};

static void _assertExecAction(ExecCommandContext *ctx, ExecActionArg &arg)
{
	// execute 
	ctx->actDef.id = arg.actionId;
	ctx->actDef.type = arg.type;
	ctx->actDef.timeout = arg.timeout;

	// make a command line
	if (!arg.command.empty())
		ctx->actDef.command = arg.command;
	else if (arg.type == ACTION_COMMAND) {
		const char *path =
		  cut_build_path(".libs", "ActionTp", NULL);
		ctx->actDef.command = StringUtils::sprintf(
		  "%s %s", path, ctx->pipeName.c_str());
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

void _assertActionLogJustAfterExec(
  ExecCommandContext *ctx,
  ActionLogStatus expectStatus = ACTLOG_STAT_STARTED,
  bool dontCheckOptions = false,
  uint32_t forceExpectedNullFlags = 0)
{
	int expectQueuingTime = 0;
	uint32_t expectedNullFlags = 
	  ACTLOG_FLAG_END_TIME | ACTLOG_FLAG_EXIT_CODE;
	if (forceExpectedNullFlags)
		expectedNullFlags = forceExpectedNullFlags;
	else if (expectStatus != ACTLOG_STAT_QUEUING)
		expectedNullFlags |= ACTLOG_FLAG_QUEUING_TIME;

	if (!(expectedNullFlags & ACTLOG_STAT_QUEUING))
		expectQueuingTime = CURR_DATETIME;

	ThreadLocalDBCache cache;
	cppcut_assert_equal(
	  true, cache.getAction().getLog(ctx->actionLog, ctx->actorInfo.logId));
	assertActionLog(
	  ctx->actionLog, ctx->actorInfo.logId,
	  ctx->actDef.id, expectStatus,
	  0, /* starterId */
	  expectQueuingTime, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  0, /* endTime */
	  ACTLOG_EXECFAIL_NONE, /* failureCode */
	  0,  /* exitCode */
	  expectedNullFlags);

	// connect to ActionTp and check the command line options
	if (dontCheckOptions)
		return;
	waitConnect(ctx);
	const EventInfo &evInf = ctx->eventInfo;
	StringVector expectedArgs;
	expectedArgs.push_back(ctx->pipeName);
	expectedArgs.push_back(
	  ActionManager::NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC);
	expectedArgs.push_back(StringUtils::sprintf("%d", ctx->actDef.id));
	expectedArgs.push_back(StringUtils::sprintf("%" PRIu32,
	                                            evInf.serverId));
	expectedArgs.push_back(StringUtils::sprintf(
	  "%" FMT_LOCAL_HOST_ID, evInf.hostIdInServer.c_str()));
	expectedArgs.push_back(StringUtils::sprintf("%ld.%ld",
	  evInf.time.tv_sec, evInf.time.tv_nsec));
	expectedArgs.push_back(evInf.id);
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.type));
	expectedArgs.push_back(evInf.triggerId);
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.status));
	expectedArgs.push_back(StringUtils::sprintf("%d", evInf.severity));
	getArguments(ctx, expectedArgs);
	assertSessionId(ctx);
}
#define assertActionLogJustAfterExec(CTX, ...) \
cut_trace(_assertActionLogJustAfterExec(CTX, ##__VA_ARGS__))

void _assertWaitForChangeActionLogStatus(ExecCommandContext *ctx,
                                         ActionLogStatus currStatus)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	do {
		g_main_context_iteration(NULL, FALSE);
		cppcut_assert_equal(false, ctx->timedOut);
		cppcut_assert_equal(
		  true, dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));
	} while (ctx->actionLog.status == currStatus);
}
#define assertWaitForChangeActionLogStatus(CTX,STAT) \
cut_trace(_assertWaitForChangeActionLogStatus(CTX,STAT))

void _assertSessionIsDeleted(const string &sessionId)
{
	if (sessionId.empty())
		return;
	SessionManager *sessionMgr = SessionManager::getInstance();

	// It is hard to get the timing of the session deletion.
	// So we just retry some times.
	while (true) {
		// If the session is not deleted, ctx->timeout is expired
		// and fails the test.
		while (g_main_context_iteration(NULL, FALSE))
			;
		SessionPtr session = sessionMgr->getSession(sessionId);
		if (!session.hasData())
			break;
		usleep(5 * 1000);
	}
}
#define assertSessionIsDeleted(SID) cut_trace(_assertSessionIsDeleted(SID))

void _assertActionLogAfterEnding(
  ExecCommandContext *ctx,
  int expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME)
{
	int expectedQueuingTime = 0;
	if (!(expectedNullFlags & ACTLOG_FLAG_QUEUING_TIME))
		expectedQueuingTime = CURR_DATETIME;

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
		  expectedQueuingTime, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  ACTLOG_EXECFAIL_NONE, /* failureCode */
		  EXIT_SUCCESS, /* exitCode */
		  expectedNullFlags /* nullFlags */);
		break;
	}

	// SessionId should be removed after the action finishes
	assertSessionIsDeleted(ctx->sessionId);
}
#define assertActionLogAfterEnding(CTX, ...) \
cut_trace(_assertActionLogAfterEnding(CTX, ##__VA_ARGS__))

void _assertActionLogForFailure(ExecCommandContext *ctx, int failureCode,
  int expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_EXIT_CODE,
  int expectedExitCode = 0)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	cppcut_assert_equal(
	  true, dbAction.getLog(ctx->actionLog, ctx->actorInfo.logId));

	int expectedQueuingTime = 0;
	if (!(expectedNullFlags & ACTLOG_FLAG_QUEUING_TIME))
		expectedQueuingTime = CURR_DATETIME;

	assertActionLog(
	  ctx->actionLog,
	  ctx->actorInfo.logId,
	  ctx->actDef.id,
	  ACTLOG_STAT_FAILED,
	  0, /* starterId */
	  expectedQueuingTime, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  CURR_DATETIME, /* endTime */
	  failureCode,
	  expectedExitCode, /* exitCode */
	  expectedNullFlags);
}
#define assertActionLogForFailure(CTX, FC, ...) \
cut_trace(_assertActionLogForFailure(CTX, FC, ##__VA_ARGS__))

typedef void (*ResidentLogStatusChangedCB)(
  ActionLogStatus currStatus, ActionLogStatus newStatus,
  ExecCommandContext *ctx);

struct AssertActionLogArg
{
	ExecCommandContext        *ctx;
	uint32_t                   expectedNullFlags;
	ActionLogStatus            currStatus;
	ActionLogStatus            newStatus;
	ResidentLogStatusChangedCB statusChangedCb;
	ActionLogExecFailureCode   expectedFailureCode;
	int                        expectedExitCode;
	int                        maxLoopCount;

	AssertActionLogArg(ExecCommandContext *_ctx)
	: ctx(_ctx),
	  expectedNullFlags(0),
	  currStatus(ACTLOG_STAT_INVALID),
	  newStatus(ACTLOG_STAT_INVALID),
	  statusChangedCb(NULL),
	  expectedFailureCode(ACTLOG_EXECFAIL_NONE),
	  expectedExitCode(RESIDENT_MOD_NOTIFY_EVENT_ACK_OK),
	  maxLoopCount(-1)
	{
	}
};

void _assertActionLogAfterExecResident(AssertActionLogArg &arg)
{
	int loopCount = 0;
	ExecCommandContext *ctx = arg.ctx;
	while (true) {
		// ActionManager updates the aciton log in the wake of GLIB's
		// events. So we can wait for the log update with
		// iterations of the loop.
		assertWaitForChangeActionLogStatus(arg.ctx, arg.currStatus);
		assertActionLog(
		  ctx->actionLog, ctx->actorInfo.logId,
		  ctx->actDef.id, arg.newStatus,
		  0, /* starterId */
		  0, /* queuingTime */
		  CURR_DATETIME, /* startTime */
		  CURR_DATETIME, /* endTime */
		  arg.expectedFailureCode,
		  arg.expectedExitCode,
		  arg.expectedNullFlags /* nullFlags */);

		if (arg.statusChangedCb) {
			(*arg.statusChangedCb)(arg.currStatus,
			                       arg.newStatus, ctx);
		}

		if (arg.newStatus == ACTLOG_STAT_SUCCEEDED)
			break;
		if (arg.newStatus == ACTLOG_STAT_FAILED)
			break;
		arg.currStatus = ACTLOG_STAT_STARTED;
		arg.newStatus = ACTLOG_STAT_SUCCEEDED;
		arg.expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
		
		loopCount++;
		if (arg.maxLoopCount > 0 && loopCount >= arg.maxLoopCount)
			break;
	}

	// SessionId should be removed after a notify finishes
	assertSessionIsDeleted(ctx->sessionId);
}
#define assertActionLogAfterExecResident(ARG) \
cut_trace(_assertActionLogAfterExecResident(ARG))

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
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_TEST_CMD_GET_EVENT_INFO);
	comm.push(ctx->pipeWr); 
}

static void sendAllowReplyNotifyEvent(ExecCommandContext *ctx)
{
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_TEST_CMD_UNBLOCK_REPLY_NOTIFY_EVENT);
	comm.push(ctx->pipeWr);
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
	cppcut_assert_equal(ctx->actDef.id, (ActionIdType)eventArg->actionId);

	// check each component
	const EventInfo &expected = ctx->eventInfo;
	cppcut_assert_equal(static_cast<uint32_t>(expected.serverId),
	                    eventArg->serverId);
	cppcut_assert_equal(TEST_HOST_ID_REPLY_MAGIC_CODE,
	                    eventArg->hostIdInServer);
	cppcut_assert_equal(expected.time.tv_sec, eventArg->time.tv_sec);
	cppcut_assert_equal(expected.time.tv_nsec, eventArg->time.tv_nsec);
	cppcut_assert_equal(TEST_EVENT_ID_REPLY_MAGIC_CODE,
	                    eventArg->eventId);
	cppcut_assert_equal(expected.type, (EventType)eventArg->eventType);
	cppcut_assert_equal(TEST_TRIGGER_ID_REPLY_MAGIC_CODE,
	                    eventArg->triggerId);
	cppcut_assert_equal(expected.status,
	                    (TriggerStatusType)eventArg->triggerStatus);
	cppcut_assert_equal(expected.severity,
	                    (TriggerSeverityType)eventArg->triggerSeverity);
	assertSessionInManager(eventArg->sessionId, ctx->actDef.ownerUserId);
	ctx->sessionId = eventArg->sessionId;

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
		g_main_context_iteration(NULL, FALSE);
		cppcut_assert_equal(false, ctx->timedOut);
	}
}
#define assertWaitRemoveWatching(CTX) cut_trace(_assertWaitRemoveWatching(CTX))

static void _assertShouldSkipByLog(bool EvenEventNotLog)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();

	// make a test data;
	TestActionManager actMgr;
	ActionDef actDef;
	actDef.id = 102;
	actDef.type = ACTION_COMMAND;
	actDef.timeout = 0;
	DBTablesAction::LogEndExecActionArg logArg;
	logArg.status = ACTLOG_STAT_SUCCEEDED;
	logArg.exitCode = 0;
	logArg.failureCode = ACTLOG_EXECFAIL_NONE;
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		if (EvenEventNotLog && (i % 2 == 0))
			continue;
		const EventInfo &eventInfo = testEventInfo[i];
		logArg.logId = dbAction.createActionLog(actDef, eventInfo);
		dbAction.logEndExecAction(logArg);
	}

	// check
	for (size_t i = 0; i < NumTestEventInfo; i++) {
		bool expect = true;
		if (EvenEventNotLog && (i % 2 == 0))
			expect = false;
		const EventInfo &eventInfo = testEventInfo[i];
		cppcut_assert_equal(expect,
		                    actMgr.callShouldSkipByLog(eventInfo));
	}
}
#define assertShouldSkipByLog(E) cut_trace(_assertShouldSkipByLog(E))

static void _assertExecuteAction(
  const ActionIdType &actionId,
  ActionLogStatus expectStatus = ACTLOG_STAT_STARTED,
  bool dontCheckOptions = false,
  const string &commandName = "")
{
	ExecCommandContext *ctx = new ExecCommandContext();
	g_execCommandCtxVect.push_back(ctx); // just an alias

	string pipeName = StringUtils::sprintf(
	  "test-command-action-%" FMT_ACTION_ID, actionId);
	ctx->initPipes(pipeName);

	ctx->eventInfo = testEventInfo[0];
	ExecActionArg arg(actionId, ACTION_COMMAND);
	if (!commandName.empty())
		arg.command = commandName;
	assertExecAction(ctx, arg);
	assertActionLogJustAfterExec(ctx, expectStatus, dontCheckOptions);
}
#define assertExecuteAction(ID, ...) \
cut_trace(_assertExecuteAction(ID, ##__VA_ARGS__))

static bool isFailureCase(size_t idx)
{
	return (idx % 3) == 0;
}

static void deleteGlobalExecCommandCtx(size_t idx)
{
	if (!g_execCommandCtxVect[idx])
		return;
	delete g_execCommandCtxVect[idx];
	g_execCommandCtxVect[idx] = NULL;
}

static string g_pathForAction;
static string g_ldLibraryPathForAction;

static void clearEnvString(const string &envName, string &str)
{
	if (str.empty())
		return;

	const int overwrite = 1;
	errno = 0;
	setenv(envName.c_str(), str.c_str(), overwrite);
	cut_assert_errno();
	str.clear();
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	acquireDefaultContext();
	ConfigManager::getInstance()->setActionCommandDirectory(getBaseDir());

	const char *residentYardDir =
	  cut_build_path(getBaseDir().c_str(), "..", "src", ".libs", NULL);
	ConfigManager::getInstance()->setResidentYardDirectory(residentYardDir);
}

void cut_teardown(void)
{
	delete g_execCommandCtx;
	g_execCommandCtx = NULL;

	for (size_t i = 0; i < g_execCommandCtxVect.size(); i++)
		deleteGlobalExecCommandCtx(i);
	g_execCommandCtxVect.clear();
	releaseDefaultContext();

	clearEnvString(ActionManager::ENV_NAME_PATH_FOR_ACTION,
	               g_pathForAction);
	clearEnvString(ActionManager::ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION,
	               g_ldLibraryPathForAction);
}

void cut_shutdown(void)
{
	// To collect all spawned actions
	ActorCollector::reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_setupPathForAction(void)
{
	// backup the original environemnt variables
	char *env = getenv(ActionManager::ENV_NAME_PATH_FOR_ACTION);
	if (env)
		g_pathForAction = env;
	env = getenv(ActionManager::ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION);
	if (env)
		g_ldLibraryPathForAction = env;

	// set test environment variable
	const int overwrite = 1;
	const string testPath = "/usr/bin:/bin:/usr/sbin:/bin:/opt/bin";
	const string testLdLibPath = "/usr/lib:/lib:/opt/lib";

	errno = 0;
	setenv(ActionManager::ENV_NAME_PATH_FOR_ACTION,
	       testPath.c_str(), overwrite);
	cut_assert_errno();
	setenv(ActionManager::ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION,
	       testLdLibPath.c_str(), overwrite);
	cut_assert_errno();

	// get the value and assert
	string path;
	string ldLibraryPath;
	TestActionManager::callSetupPathForAction(path, ldLibraryPath);
	cppcut_assert_equal("PATH=" + testPath, path);
	cppcut_assert_equal("LD_LIBRARY_PATH=" + testLdLibPath, ldLibraryPath);
}

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

void test_execCommandActionCrashSoon(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	string pipeName = "test-command-action";
	ctx->initPipes(pipeName);

	ExecActionArg arg(2343242, ACTION_COMMAND);
	arg.option = OPTION_CRASH_SOON;
	assertExecAction(ctx, arg);
	assertWaitForChangeActionLogStatus(ctx, ACTLOG_STAT_STARTED);
	assertActionLogForFailure(ctx, getFailureCodeSignalOrDumpedByRLimit(),
	                          ACTLOG_FLAG_QUEUING_TIME, SIGSEGV);
}

void test_execCommandActionTimeout(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	string pipeName = "test-command-action";
	ctx->initPipes(pipeName);

	ExecActionArg arg(2343242, ACTION_COMMAND);
	arg.option = OPTION_STALL;
	arg.timeout = 10;
	assertExecAction(ctx, arg);
	assertWaitForChangeActionLogStatus(ctx, ACTLOG_STAT_STARTED);
	assertActionLogForFailure(ctx, ACTLOG_EXECFAIL_KILLED_TIMEOUT,
	                          ACTLOG_FLAG_QUEUING_TIME);
}

void test_execCommandActionTimeoutNotExpired(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	string pipeName = "test-command-action";
	ctx->initPipes(pipeName);

	ctx->eventInfo = testEventInfo[0];
	ExecActionArg arg(2343242, ACTION_COMMAND);
	arg.timeout = 1 * 1000;
	assertExecAction(ctx, arg);
	assertActionLogJustAfterExec(ctx);
	sendQuit(ctx);
	assertActionLogAfterEnding(ctx);

	// We expect that the timer event is removed. So the return value
	// should be FALSE (i.e. not found).
	GTimer *timer = g_timer_new();
	g_timer_start(timer);
	while (g_timer_elapsed(timer, NULL) < 0.1)
		g_main_context_iteration(NULL, FALSE);
	g_timer_destroy(timer);
	cppcut_assert_equal(FALSE, g_source_remove(ctx->actorInfo.timerTag));
}

void test_execResidentAction(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	assertExecAction(ctx, arg);

	AssertActionLogArg logarg(ctx);
	logarg.expectedNullFlags =
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|ACTLOG_FLAG_EXIT_CODE;
	logarg.currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
	logarg.newStatus  = ACTLOG_STAT_STARTED;
	assertActionLogAfterExecResident(logarg);
}

void test_execResidentActionCrashInInit(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.option = "--crash-init";
	assertExecAction(ctx, arg);

	AssertActionLogArg logarg(ctx);
	logarg.expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	logarg.currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
	logarg.newStatus  = ACTLOG_STAT_FAILED;
	logarg.expectedFailureCode = getFailureCodeSignalOrDumpedByRLimit();
	logarg.expectedExitCode = SIGSEGV;
	assertActionLogAfterExecResident(logarg);
}

void test_execResidentActionCrashInNotifyEvent(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.option = "--crash-notify-event";
	assertExecAction(ctx, arg);

	// wait for FAILED
	assertWaitForChangeActionLogStatus(ctx, ACTLOG_STAT_LAUNCHING_RESIDENT);
	assertWaitForChangeActionLogStatus(ctx, ACTLOG_STAT_STARTED);

	// check the log
	assertActionLog(
	  ctx->actionLog, ctx->actorInfo.logId,
	  ctx->actDef.id, ACTLOG_STAT_FAILED,
	  0, /* starterId */
	  0, /* queuingTime */
	  CURR_DATETIME, /* startTime */
	  CURR_DATETIME, /* endTime */
	  getFailureCodeSignalOrDumpedByRLimit(), SIGSEGV,
	  ACTLOG_FLAG_QUEUING_TIME);
}

void test_execResidentActionTimeoutInInit(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.option = "--stall-init";
	arg.timeout = 10;
	assertExecAction(ctx, arg);

	AssertActionLogArg logarg(ctx);
	logarg.expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	logarg.currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
	logarg.newStatus  = ACTLOG_STAT_FAILED;
	logarg.expectedFailureCode = ACTLOG_EXECFAIL_KILLED_TIMEOUT;
	logarg.expectedExitCode = 0;
	assertActionLogAfterExecResident(logarg);
}

void test_execResidentActionWithWrongPath(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.command = "wrong-command-dayo.so";
	assertExecAction(ctx, arg);

	AssertActionLogArg logarg(ctx);
	logarg.expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	logarg.currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
	logarg.newStatus  = ACTLOG_STAT_FAILED;
	logarg.expectedFailureCode = ACTLOG_EXECFAIL_ENTRY_NOT_FOUND;
	logarg.expectedExitCode = 0;
	assertActionLogAfterExecResident(logarg);
	assertWaitRemoveWatching(ctx);

	// reconfirm the action log. This confirms that the log is not
	// updated in ActorCollector::checkExitProcess().
	ThreadLocalDBCache cache;
	cppcut_assert_equal(
	  true, cache.getAction().getLog(ctx->actionLog, ctx->actorInfo.logId));
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

	AssertActionLogArg logarg(ctx);
	size_t numEvents = 10;
	for (size_t i = 0; i < numEvents; i++) {
		setExpectedValueForResidentManyEvents(
		  i, logarg.expectedNullFlags,
		  logarg.currStatus, logarg.newStatus);
		ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
		assertExecAction(ctx, arg);
		assertActionLogAfterExecResident(logarg);
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

	AssertActionLogArg logarg(ctx);
	for (size_t i = 0; i < numEvents; i++) {
		setExpectedValueForResidentManyEvents(
		  i, logarg.expectedNullFlags,
		  logarg.currStatus, logarg.newStatus);
		ctx->actorInfo = actorVect[i];
		assertActionLogAfterExecResident(logarg);
	}
}

void test_execResidentActionCheckArg(void)
{
	g_execCommandCtx = new ExecCommandContext();
	ExecCommandContext *ctx = g_execCommandCtx; // just an alias

	string pipeName = "test-resident-action";
	string option = "--pipename " + pipeName;
	option += " --block-reply-notify-event";
	ctx->initPipes(pipeName);
	ctx->pullData(RESIDENT_PROTO_HEADER_LEN +
	              RESIDENT_TEST_REPLY_GET_EVENT_INFO_BODY_LEN,
	              replyEventInfoCb);
	ctx->eventInfo = testEventInfo[0];
	ExecActionArg arg(0x4ab3fd32, ACTION_RESIDENT);
	arg.option = option;
	assertExecAction(ctx, arg);

	// wait for the status: ACTLOG_STAT_STARTED
	AssertActionLogArg logarg(ctx);
	logarg.expectedNullFlags = 
	  ACTLOG_FLAG_QUEUING_TIME|ACTLOG_FLAG_END_TIME|ACTLOG_FLAG_EXIT_CODE;
	logarg.currStatus = ACTLOG_STAT_LAUNCHING_RESIDENT;
	logarg.newStatus  = ACTLOG_STAT_STARTED;
	logarg.statusChangedCb = statusChangedCbForArgCheck;
	logarg.maxLoopCount = 1;
	assertActionLogAfterExecResident(logarg);

	assertWaitEventBody(ctx);
	sendAllowReplyNotifyEvent(ctx);

	// wait for the status: ACTLOG_STAT_SUCCEEDED;
	logarg = AssertActionLogArg(ctx); // clear value
	logarg.expectedNullFlags = ACTLOG_FLAG_QUEUING_TIME;
	logarg.currStatus = ACTLOG_STAT_STARTED;
	logarg.newStatus  = ACTLOG_STAT_SUCCEEDED;
	assertActionLogAfterExecResident(logarg);
}

void test_shouldSkipByLog(void)
{
	assertShouldSkipByLog(false);
}

void test_shouldSkipByLogNotFound(void)
{
	assertShouldSkipByLog(true);
}

void test_limitCommandAction(void)
{
	ConfigManager *confMgr = ConfigManager::getInstance();
	size_t maxNum = confMgr->getMaxNumberOfRunningCommandAction();
	ActionIdType actionId = 352;
	size_t idx = 0;
	for (; idx < maxNum; idx++, actionId++)
		assertExecuteAction(actionId, ACTLOG_STAT_STARTED, false);
	for (; idx < maxNum + numWaitingActions; idx++, actionId++) {
		string commandName;
		// We set the wrong command when the index is multiples of thee.
		if (isFailureCase(idx))
			commandName = "wrong-commandoooooooo";
		assertExecuteAction(actionId, ACTLOG_STAT_QUEUING, true,
		                    commandName);
	}
}

void test_executeWaitedCommandAction(void)
{
	ConfigManager *confMgr = ConfigManager::getInstance();
	int maxNum = confMgr->getMaxNumberOfRunningCommandAction();
	test_limitCommandAction(); // make normal actions and waiting actions.
	for (size_t i = 0; i < numWaitingActions; i++) {
		cppcut_assert_equal(true, i < g_execCommandCtxVect.size());
		ExecCommandContext *ctx = g_execCommandCtxVect[i];

		// Quit a normally executed tp quit.
		sendQuit(ctx);
		assertActionLogAfterEnding(ctx);
		deleteGlobalExecCommandCtx(i);

		// check the status of the waiting action.
		size_t idxWait = maxNum + i;
		cppcut_assert_equal(
		  true, idxWait < g_execCommandCtxVect.size(),
		  cut_message("idxWait: %zd, VectSize: %zd",
		              idxWait, g_execCommandCtxVect.size()));
		ExecCommandContext *ctxWait = g_execCommandCtxVect[idxWait];
		assertWaitForChangeActionLogStatus(ctxWait,
		                                   ACTLOG_STAT_QUEUING);
		// check the status: ACTLOG_STAT_STARTE or Failure
		if (!isFailureCase(idxWait)) {
			assertActionLogJustAfterExec(
			  ctxWait, ACTLOG_STAT_STARTED, false,
			  ACTLOG_FLAG_END_TIME | ACTLOG_FLAG_EXIT_CODE);
		} else {
			assertActionLogForFailure(
			  ctxWait, ACTLOG_EXECFAIL_ENTRY_NOT_FOUND,
			  ACTLOG_FLAG_EXIT_CODE);
			deleteGlobalExecCommandCtx(idxWait);
		}
	}
}

void test_checkExitWaitedCommandAction(void)
{
	test_executeWaitedCommandAction();

	// gather alive ActionTps
	set<size_t> aliveIndexSet;
	ConfigManager *confMgr = ConfigManager::getInstance();
	int maxNum = confMgr->getMaxNumberOfRunningCommandAction();
	for (size_t i = 0; i < maxNum + numWaitingActions; i++) {
		if (g_execCommandCtxVect[i])
			aliveIndexSet.insert(i);
	}

	// quit process at the head and the tail alternately
	while (!aliveIndexSet.empty()) {
		set<size_t>::iterator it = aliveIndexSet.begin();
		size_t idx = *it;
		ExecCommandContext *ctx = g_execCommandCtxVect[idx];
		sendQuit(ctx);
		bool waiting = (idx >= (size_t)maxNum);
		int expectedNullFlags = waiting ? 0 : ACTLOG_FLAG_QUEUING_TIME;
		assertActionLogAfterEnding(ctx, expectedNullFlags);
		aliveIndexSet.erase(it);
	}

	TestActionManager actMgr;
	cppcut_assert_equal((size_t)0,
	                    actMgr.callGetNumberOfOnstageCommandActors());
}

static void _assertRunAction(
 const HatoholErrorCode expectedErrorCode, const ActionDef &actDef,
 const EventInfo &eventInfo)
{
	loadTestDBTablesConfig();
	loadTestDBTablesUser();

	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	TestActionManager actMgr;
	HatoholError err = actMgr.callRunAction(actDef, eventInfo, dbAction);
	assertHatoholError(expectedErrorCode, err);
}
#define assertRunAction(X,A,E) \
cut_trace(_assertRunAction(X,A,E))

void test_runAction(void)
{
	assertRunAction(HTERR_OK, testActionDef[0], testEventInfo[0]);
}

void test_runIncidentSenderActionWithSystemUser(void)
{
	size_t idx = findIndexFromTestActionDef(ACTION_INCIDENT_SENDER);
	// make a copy to overwrite ownerUserId
	ActionDef actDef = testActionDef[idx];
	actDef.ownerUserId = USER_ID_SYSTEM;
	actDef.command = "Don't run actual IncidentSender.";
	assertRunAction(HTERR_OK, actDef, testEventInfo[0]);
}

void test_runIncidentSenderActionWithExistingUser(void)
{
	size_t idx = findIndexFromTestActionDef(ACTION_INCIDENT_SENDER);
	// make a copy to overwrite ownerUserId
	ActionDef actDef = testActionDef[idx];
	const UserIdType existingUserId = NumTestUserInfo - 1;
	actDef.ownerUserId = existingUserId;
	actDef.command = "Don't run actual IncidentSender.";
	assertRunAction(HTERR_INVALID_USER, actDef, testEventInfo[0]);
}

void test_runIncidentSenderActionWithNonExistingUser(void)
{
	size_t idx = findIndexFromTestActionDef(ACTION_INCIDENT_SENDER);
	// make a copy to overwrite ownerUserId
	ActionDef actDef = testActionDef[idx];
	const UserIdType nonExistingUserId = NumTestUserInfo + 5;
	actDef.ownerUserId = nonExistingUserId;
	actDef.command = "Don't run actual IncidentSender.";
	assertRunAction(HTERR_INVALID_USER, actDef, testEventInfo[0]);
}

void test_checkEventsWithMultipleIncidentSender(void)
{
	// prepare two incident sender actions
	loadTestDBTablesConfig();

	ActionDef actDef = {
	  0,                      // id (this field is ignored)
	  ActionCondition(
	    ACTCOND_TRIGGER_STATUS | ACTCOND_TRIGGER_SEVERITY, // enableBits
	    0,                        // serverId
	    "",                       // hostId
	    "",                       // hostgroupId
	    "",                       // triggerId
	    TRIGGER_STATUS_PROBLEM,   // triggerStatus
	    TRIGGER_SEVERITY_INFO,    // triggerSeverity
	    CMP_EQ_GT                 // triggerSeverityCompType;
	  ), // condition
	  ACTION_INCIDENT_SENDER, // type
	  "",                     // working dir
	  "3",                    // command
	  0,                      // timeout
	  0,                      // ownerUserId
	};

	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	dbAction.addAction(actDef, privilege);
	dbAction.addAction(actDef, privilege);

	// prepare an event
	EventInfo event = testEventInfo[0];
	event.time.tv_sec = time(NULL);
	EventInfoList eventList;
	eventList.push_back(event);

	// We get the first entry of the action list that is obtained by
	// DBTablesAction::getActionList(). It changes depending on the
	// storage engine. For example, HATOHOL_MYSQL_ENGINE_MEMORY=1
	// is set, the first entry may be different from that w/o it.
	//
	// However, this way is not the best. The expected action ID
	// may change when the implementation of Hatohol server is modified.
	ActionDefList actionDefList;
	ActionsQueryOption option(USER_ID_SYSTEM);
	option.setActionType(ACTION_ALL);
	option.setTargetEventInfo(&event);
	dbAction.getActionList(actionDefList, option);
	cppcut_assert_equal(false, actionDefList.empty());
	const ActionIdType expectedActionId = actionDefList.begin()->id;

	// run actions
	// TODO: we shoud prepare stub of IncidentSenderManager
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[2];
	g_redmineEmulator.addUser(tracker.userName, tracker.password);

	TestActionManager actionManager;
	actionManager.checkEvents(eventList);

	IncidentSenderManager &manager = IncidentSenderManager::getInstance();
	while (!manager.isIdling())
		usleep(100 * 1000);

	// check action log
	string expected
		= StringUtils::sprintf("%" FMT_ACTION_ID, expectedActionId);
	string statement = "select action_id from ";
	statement += DBTablesAction::getTableNameActionLogs();
	statement += " order by action_log_id";
	assertDBContent(&dbAction.getDBAgent(), statement, expected);
}

} // namespace testActionManager
