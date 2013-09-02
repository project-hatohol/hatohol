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

#include <cstring>
#include <deque>
#include <errno.h>
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"

using namespace std;

struct ResidentInfo;
struct ActionManager::ResidentNotifyInfo {
	ResidentInfo *residentInfo;
	uint64_t  logId;
	EventInfo eventInfo; // a replica

	ResidentNotifyInfo(ResidentInfo *_residentInfo)
	: residentInfo(_residentInfo),
	  logId(INVALID_ACTION_LOG_ID)
	{
	}

	// NOTE: Currently this instance is handled only on GLIB event
	// callbacks. This ensures that the deletion of the instance does
	// not happen during the callback, because it is also invoked on
	// a GLIB event callback.
	// If we use this instance from multiple threads, something MT-safe
	// mechanism have to be implemented.
};

typedef deque<ActionManager::ResidentNotifyInfo *> ResidentNotifyQueue;
typedef ResidentNotifyQueue::iterator              ResidentNotifyQueueIterator;

typedef map<int, ResidentInfo *>     RunningResidentMap;
typedef RunningResidentMap::iterator RunningResidentMapIterator;

enum ResidentStatus {
	RESIDENT_STAT_INIT,
	RESIDENT_STAT_WAIT_LAUNCHED,
	RESIDENT_STAT_WAIT_PARAM_ACK,
	RESIDENT_STAT_WAIT_NOTIFY_ACK,
	RESIDENT_STAT_IDLE,
};

struct ResidentInfo :
   public ResidentPullHelper<ActionManager::ResidentNotifyInfo> {
	static MutexLock residentMapLock;
	static RunningResidentMap runningResidentMap;
	ActionManager *actionManager;
	const ActionDef actionDef;
	pid_t           pid;
	bool            inRunningResidentMap;

	MutexLock           queueLock;
	ResidentNotifyQueue notifyQueue; // should be used with queueLock.
	ResidentStatus      status;      // should be used with queueLock.

	NamedPipe pipeRd, pipeWr;
	string pipeName;
	string modulePath;
	string moduleOption;

	ResidentInfo(ActionManager *actMgr, const ActionDef &_actionDef)
	: actionManager(actMgr),
	  actionDef(_actionDef),
	  pid(0),
	  inRunningResidentMap(false),
	  status(RESIDENT_STAT_INIT),
	  pipeRd(NamedPipe::END_TYPE_MASTER_READ),
	  pipeWr(NamedPipe::END_TYPE_MASTER_WRITE)
	{
		pipeName = StringUtils::sprintf("resident-%d", actionDef.id);
		HATOHOL_ASSERT(modulePath.size() < PATH_MAX,
		               "moudlePath: %zd, PATH_MAX: %u\n",
		               modulePath.size(), PATH_MAX);
		initResidentPullHelper(&pipeRd, NULL);
	}

	virtual ~ResidentInfo(void)
	{
		queueLock.lock();
		while (!notifyQueue.empty()) {
			ActionManager::ResidentNotifyInfo *notifyInfo
			  = notifyQueue.front();
			// TODO: log the fact that the notification is deleted
			MLPL_BUG("ResidentNotifyInfo is deleted, "
			         "but not logged: logId: %"PRIu64"\n",
			         notifyInfo->logId);
			delete notifyInfo;
			notifyQueue.pop_front();
		}
		queueLock.unlock();

		// delete this element from the map.
		if (!inRunningResidentMap)
			return;
		bool found = false;
		int actionId = actionDef.id;
		ResidentInfo::residentMapLock.lock();
		RunningResidentMapIterator it =
		  runningResidentMap.find(actionId);
		if (it != runningResidentMap.end()) {
			found = true;
			runningResidentMap.erase(it);
		}
		ResidentInfo::residentMapLock.unlock();
		if (!found) {
			MLPL_BUG("Not found residentInfo in the map: %d\n",
			         actionId);
		}

		// Currently the caller of destructor (i.e. delete statement)
		// is only the callback from ActorCollector. This means that
		// this function is called only after hatohol-resident-yard 
		// is dead.
		// NOTE: Exceptionally ActionManger::reset() calls delete
		//       explicitly. However, this should be used in the test.
	}

	bool init(GIOFunc funcRd, GIOFunc funcWr)
	{
		if(!pipeRd.init(pipeName, funcRd, this))
			return false;
		if (!pipeWr.init(pipeName, funcWr, this))
			return false;
		return true;
	}

	void setStatus(ResidentStatus stat)
	{
		queueLock.lock();
		status = stat;
		queueLock.unlock();
	}

	void deleteFrontNotifyInfo(void)
	{
		queueLock.lock();
		HATOHOL_ASSERT(!notifyQueue.empty(), "Queue is empty.");
		ActionManager::ResidentNotifyInfo *notifyInfo
		   = notifyQueue.front();
		notifyQueue.pop_front();
		queueLock.unlock();

		delete notifyInfo;
	}

	void deleteNotifyInfo(ActionManager::ResidentNotifyInfo *notifyInfo)
	{
		bool found = false;
		ResidentNotifyQueueIterator it;
		queueLock.lock();
		it = notifyQueue.begin();
		for (; it != notifyQueue.end(); ++it) {
			if (*it == notifyInfo) {
				found = true;
				notifyQueue.erase(it);
				break;
			}
		}
		queueLock.unlock();

		if (!found) {
			MLPL_BUG("Not found: notifyInfo: %p, "
			         "logId: %"PRIu64"\n", notifyInfo->logId);
		}
		delete notifyInfo;
	}
};

MutexLock          ResidentInfo::residentMapLock;
RunningResidentMap ResidentInfo::runningResidentMap;

struct ActionManager::PrivateContext {
	static ActorCollector collector;

	// This can only be used on the owner's context (thread),
	// It's danger to use from a GLIB's event callback context or
	// other thread, becuase DBClientAction is MT-thread unsafe.
	DBClientAction dbAction;

	// member for a command line parsing
	SeparatorCheckerWithCallback separator;
	bool inQuot;
	bool byBackSlash;
	string currWord;
	StringVector *argVect;

	PrivateContext(void)
	: separator(" '\\"),
	  inQuot(false),
	  byBackSlash(false),
	  argVect(NULL)
	{
	}

	void resetParser(StringVector *_argVect)
	{
		inQuot = false;
		byBackSlash = false;
		currWord.clear();
		argVect = _argVect;
	}

	void pushbackCurrWord(void)
	{
		if (currWord.empty())
			return;
		argVect->push_back(currWord);
		currWord.clear();
	}
};

ActorCollector ActionManager::PrivateContext::collector;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ActionManager::reset(void)
{
	// The following deletion is naturally no effect at the start of
	// Hatohol. This is mainly for the test in which this function is 
	// calls many times.
	// NOTE: This is a special case for test, so we don't take
	// ResidentInfo::residentMapLock. Or the deadlock will happen
	// in the desctoructor of ResidentInfo.
	RunningResidentMapIterator it =
	   ResidentInfo::runningResidentMap.begin();
	for (; it != ResidentInfo::runningResidentMap.end(); ++it) {
		ResidentInfo *residentInfo = it->second;
		delete residentInfo;
	}
	ResidentInfo::runningResidentMap.clear();
}

ActionManager::ActionManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  (' ', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\'', separatorCallback, m_ctx);
	m_ctx->separator.setCallbackTempl<PrivateContext>
	  ('\\', separatorCallback, m_ctx);
}

ActionManager::~ActionManager()
{
	if (m_ctx)
		delete m_ctx;
}

void ActionManager::checkEvents(const EventInfoList &eventList)
{
	EventInfoListConstIterator it = eventList.begin();
	for (; it != eventList.end(); ++it) {
		ActionDefList actionDefList;
		m_ctx->dbAction.getActionList(*it, actionDefList);
		ActionDefListIterator actIt = actionDefList.begin();
		for (; actIt != actionDefList.end(); ++actIt)
			runAction(*actIt, *it);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActionManager::separatorCallback(const char sep, PrivateContext *ctx)
{
	if (sep == ' ') {
		if (ctx->inQuot)
			ctx->currWord += ' ';
		else
			ctx->pushbackCurrWord();
		ctx->byBackSlash = false;
	} else if (sep == '\'') {
		if (ctx->byBackSlash)
			ctx->currWord += "'";
		else
			ctx->inQuot = !ctx->inQuot;
		ctx->byBackSlash = false;
	} else if (sep == '\\') {
		if (ctx->byBackSlash)
			ctx->currWord += '\\';
		ctx->byBackSlash = !ctx->byBackSlash;
	}
}

void ActionManager::runAction(const ActionDef &actionDef,
                              const EventInfo &eventInfo)
{
	if (actionDef.type == ACTION_COMMAND) {
		execCommandAction(actionDef, eventInfo);
	} else if (actionDef.type == ACTION_RESIDENT) {
		execResidentAction(actionDef, eventInfo);
	} else {
		HATOHOL_ASSERT(true, "Unknown type: %d\n", actionDef.type);
	}
}

void ActionManager::makeExecArg(StringVector &argVect, const string &cmd)
{
	m_ctx->resetParser(&argVect);
	ParsableString parsable(cmd);
	while (!parsable.finished()) {
		string word = parsable.readWord(m_ctx->separator);
		m_ctx->currWord += word;
		m_ctx->byBackSlash = false;
	}
	m_ctx->pushbackCurrWord();
}

void ActionManager::parseResidentCommand(
  const string &command, string &path, string &option)
{
	size_t posSpace = command.find(' ');
	if (posSpace == string::npos) {
		path = command;
		option.clear();
		return;
	}
	
	path = string(command, 0, posSpace);
	string optionRaw = string(command, posSpace);
	option = StringUtils::stripBothEndsSpaces(optionRaw);
}

bool ActionManager::spawn(const ActionDef &actionDef, ActorInfo *actorInfo,
                          const gchar **argv)
{
	const gchar *workingDirectory = NULL;
	if (!actionDef.workingDir.empty())
		workingDirectory = actionDef.workingDir.c_str();

	GSpawnFlags flags =
	  (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GError *error = NULL;

	// We take the lock here to avoid the child spanwed below from
	// not being collected. If the child immediately exits
	// before the following 'm_ctx->collector.addActor(&childPid)' is
	// called, ActorCollector::checkExitProcess() possibly ignores it,
	// because the pid of the child isn't in the wait child set.
	m_ctx->collector.lock();
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)argv, NULL,
	                flags, childSetup, userData, &actorInfo->pid, &error);
	if (!succeeded) {
		m_ctx->collector.unlock();
		string msg = StringUtils::sprintf(
		  "Failed to execute command: %s, action ID: %d",
		  error->message, actionDef.id);
		DBClientAction::ActionLogExecFailureCode failureCode =
		  error->code == G_SPAWN_ERROR_NOENT ?
		    DBClientAction::ACTLOG_EXECFAIL_ENTRY_NOT_FOUND :
		    DBClientAction::ACTLOG_EXECFAIL_EXEC_FAILURE;
		g_error_free(error);
		actorInfo->logId =
		  m_ctx->dbAction.createActionLog(actionDef, failureCode);
		MLPL_ERR("%s, logID: %"PRIu64"\n",
		         msg.c_str(), actorInfo->logId);
		return false;
	}
	DBClientAction::ActionLogStatus initialStatus =
	  (actionDef.type == ACTION_COMMAND) ?
	    DBClientAction::ACTLOG_STAT_STARTED :
	    DBClientAction::ACTLOG_STAT_LAUNCHING_RESIDENT;
	actorInfo->logId =
	   m_ctx->dbAction.createActionLog(
	     actionDef, DBClientAction::ACTLOG_EXECFAIL_NONE, initialStatus);
	m_ctx->collector.addActor(*actorInfo);
	m_ctx->collector.unlock();

	return true;
}

void ActionManager::execCommandAction(const ActionDef &actionDef,
                                      const EventInfo &eventInfo,
                                      ActorInfo *_actorInfo)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_COMMAND,
	               "Invalid type: %d\n", actionDef.type);
	StringVector argVect;
	makeExecArg(argVect, actionDef.command);

	const gchar *argv[argVect.size()+1];
	for (size_t i = 0; i < argVect.size(); i++)
		argv[i] = argVect[i].c_str();
	argv[argVect.size()] = NULL;

	ActorInfo actorInfo;
	spawn(actionDef, &actorInfo, argv);
	if (_actorInfo)
		*_actorInfo = actorInfo;
}

void ActionManager::execResidentAction(const ActionDef &actionDef,
                                       const EventInfo &eventInfo,
                                       ActorInfo *_actorInfo)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_RESIDENT,
	               "Invalid type: %d\n", actionDef.type);
	ResidentInfo::residentMapLock.lock();
	RunningResidentMapIterator it =
	   ResidentInfo::runningResidentMap.find(actionDef.id);
	if (it != ResidentInfo::runningResidentMap.end()) {
		ResidentInfo *residentInfo = it->second;
		residentInfo->queueLock.lock();
		ResidentInfo::residentMapLock.unlock();

		// queue ResidentNotifyInfo and try to notify
		DBClientAction dbAction;
		ResidentNotifyInfo *notifyInfo =
		   new ResidentNotifyInfo(residentInfo);
		notifyInfo->eventInfo = eventInfo; // just copy
		notifyInfo->logId =
		   m_ctx->dbAction.createActionLog(
		     actionDef, DBClientAction::ACTLOG_EXECFAIL_NONE,
		     DBClientAction::ACTLOG_STAT_RESIDENT_QUEUING);

		residentInfo->notifyQueue.push_back(notifyInfo);
		residentInfo->queueLock.unlock();
		tryNotifyEvent(residentInfo);

		if (_actorInfo) {
			_actorInfo->pid = residentInfo->pid;
			_actorInfo->logId = notifyInfo->logId;
		}
		return;
	}

	// make a new resident instance
	ActorInfo actorInfo;
	ResidentInfo *residentInfo =
	  launchResidentActionYard(actionDef, eventInfo, &actorInfo);
	if (residentInfo) {
		ResidentInfo::runningResidentMap[actionDef.id] = residentInfo;
		residentInfo->inRunningResidentMap = true;
	}
	ResidentInfo::residentMapLock.unlock();
	if (_actorInfo)
		*_actorInfo = actorInfo;
}

gboolean ActionManager::residentReadErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(data);
	ActionManager *obj = residentInfo->actionManager;
	MLPL_ERR("Read error: condition: %s (%x)\n",
	         Utils::getStringFromGIOCondition(condition).c_str(),
	         condition);
	obj->closeResident(residentInfo);
	return FALSE;
}

gboolean ActionManager::residentWriteErrCb(
  GIOChannel *source, GIOCondition condition, gpointer data)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(data);
	ActionManager *obj = residentInfo->actionManager;
	MLPL_ERR("Write error: condition: %s (%x)\n",
	         Utils::getStringFromGIOCondition(condition).c_str(),
	         condition);
	obj->closeResident(residentInfo);
	return FALSE;
}

void ActionManager::launchedCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                               size_t size, ResidentNotifyInfo *notifyInfo)
{
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	ActionManager *obj = residentInfo->actionManager;
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction:: ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	// check the packet type
	uint32_t bodyLen = *sbuf.getPointerAndIncIndex<uint32_t>();
	if (bodyLen != 0) {
		MLPL_ERR("Invalid body length: %"PRIu32", "
		         "expect: 0\n", bodyLen);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction::ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_LAUNCHED) {
		MLPL_ERR("Invalid packet type: %"PRIu16", "
		         "expect: %d\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction::ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
	}

	sendParameters(residentInfo);
	residentInfo->pullData(RESIDENT_PROTO_HEADER_LEN +
	                       RESIDENT_PROTO_MODULE_LOADED_CODE_LEN,
	                       moduleLoadedCb);
	residentInfo->setStatus(RESIDENT_STAT_WAIT_PARAM_ACK);
}

void ActionManager::moduleLoadedCb(GIOStatus stat, SmartBuffer &sbuf,
                                   size_t size, ResidentNotifyInfo *notifyInfo)
{
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	ActionManager *obj = residentInfo->actionManager;
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction:: ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED) {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction::ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	// check the result
	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);
	uint32_t resultCode = sbuf.getValueAndIncIndex<uint32_t>();
	if (resultCode != RESIDENT_PROTO_MODULE_LOADED_CODE_SUCCESS) {
		DBClientAction::ActionLogExecFailureCode code;
		MLPL_ERR("Failed to load module. "
		         "code: %"PRIu32"\n", resultCode);
		switch (resultCode) {
		case RESIDENT_PROTO_MODULE_LOADED_CODE_FAIL_DLOPEN:
			code = DBClientAction::ACTLOG_EXECFAIL_ENTRY_NOT_FOUND;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_MOD_SYMBOL:
			code = DBClientAction::ACTLOG_EXECFAIL_MOD_NOT_FOUND_SYMBOL;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_MOD_VER_INVALID:
			code = DBClientAction::ACTLOG_EXECFAIL_MOD_VER_INVALID;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_INIT_FAILURE:
			code = DBClientAction::ACTLOG_EXECFAIL_MOD_INIT_FAILURE;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_NOTIFY_EVENT:
			code = DBClientAction::ACTLOG_EXECFAIL_MOD_NOT_FOUND_NOTIFY_EVENT;
			break;
		default:
			code = DBClientAction::ACTLOG_EXECFAIL_MOD_UNKNOWN_REASON;
			break;
		}
		obj->closeResident(notifyInfo, code);
		return;
	}

	residentInfo->setStatus(RESIDENT_STAT_IDLE);
	obj->tryNotifyEvent(residentInfo);
}

void ActionManager::gotNotifyEventAckCb(GIOStatus stat, SmartBuffer &sbuf,
                                        size_t size,
                                        ResidentNotifyInfo *notifyInfo)
{
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	ActionManager *obj = residentInfo->actionManager;
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction::ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT_ACK) {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		obj->closeResident(
		  notifyInfo,
		  DBClientAction::ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	// get the result code
	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);
	uint32_t resultCode = *sbuf.getPointer<uint32_t>();

	// log the end of action
	HATOHOL_ASSERT(notifyInfo->logId != INVALID_ACTION_LOG_ID,
	               "log ID: %"PRIx64, notifyInfo->logId);
	DBClientAction::LogEndExecActionArg logArg;
	logArg.logId = notifyInfo->logId;
	logArg.status = DBClientAction::ACTLOG_STAT_SUCCEEDED,
	logArg.exitCode = resultCode;

	DBClientAction dbAction;
	dbAction.logEndExecAction(logArg);

	// remove the notifyInfo 
	residentInfo->deleteFrontNotifyInfo();

	// send the next notificaiton if it exists
	residentInfo->setStatus(RESIDENT_STAT_IDLE);
	obj->tryNotifyEvent(residentInfo);
}

void ActionManager::sendParameters(ResidentInfo *residentInfo)
{
	parseResidentCommand(residentInfo->actionDef.command,
	                     residentInfo->modulePath,
	                     residentInfo->moduleOption);
	size_t bodyLen = RESIDENT_PROTO_PARAM_MODULE_PATH_LEN
	                 + residentInfo->modulePath.size()
	                 + RESIDENT_PROTO_PARAM_MODULE_OPTION_LEN
	                 + residentInfo->moduleOption.size();
	ResidentCommunicator comm;
	comm.setHeader(bodyLen, RESIDENT_PROTO_PKT_TYPE_PARAMETERS);
	comm.addModulePath(residentInfo->modulePath);
	comm.addModuleOption(residentInfo->moduleOption);
	comm.push(residentInfo->pipeWr);
}

ResidentInfo *ActionManager::launchResidentActionYard
  (const ActionDef &actionDef, const EventInfo &eventInfo, ActorInfo *actorInfo)
{
	// make a ResidentInfo instance.
	ResidentInfo *residentInfo = new ResidentInfo(this, actionDef);
	if (!residentInfo->init(residentReadErrCb, residentWriteErrCb)) {
		delete residentInfo;
		return NULL;
	}

	const gchar *argv[] = {
	  "hatohol-resident-yard",
	  residentInfo->pipeName.c_str(),
	  NULL};
	actorInfo->collectedCb = actorCollectedCb;
	actorInfo->collectedCbPriv = residentInfo;
	if (!spawn(actionDef, actorInfo, argv)) {
		delete residentInfo;
		return NULL;
	}
	residentInfo->pid = actorInfo->pid;

	// We can push the notifyInfo to the queue without locking,
	// because no other users of this instance at this point.
	ResidentNotifyInfo *notifyInfo = new ResidentNotifyInfo(residentInfo);
	notifyInfo->logId = actorInfo->logId;
	notifyInfo->eventInfo = eventInfo;
	residentInfo->notifyQueue.push_back(notifyInfo);

	// We don't use setStatus() because this fucntion is called with
	// taking queueLock. Using it causes a deadlock.
	residentInfo->status = RESIDENT_STAT_WAIT_LAUNCHED;

	residentInfo->setPullCallbackArg(notifyInfo);
	residentInfo->pullHeader(launchedCb);
	return residentInfo;
}

void ActionManager::tryNotifyEvent(ResidentInfo *residentInfo)
{
	residentInfo->queueLock.lock();
	ResidentNotifyInfo *notifyInfo = NULL;
	if (residentInfo->status != RESIDENT_STAT_IDLE) {
		residentInfo->queueLock.unlock();
		return;
	}
	if (!residentInfo->notifyQueue.empty()) {
		notifyInfo = residentInfo->notifyQueue.front();
		residentInfo->status = RESIDENT_STAT_WAIT_NOTIFY_ACK;
	}
	residentInfo->queueLock.unlock();
	if (notifyInfo)
		notifyEvent(residentInfo, notifyInfo);
}

void ActionManager::notifyEvent(ResidentInfo *residentInfo,
                                ResidentNotifyInfo *notifyInfo)
{
	ResidentCommunicator comm;
	comm.setNotifyEventBody(residentInfo->actionDef.id,
	                        notifyInfo->eventInfo);
	comm.push(residentInfo->pipeWr);

	// wait for result code
	residentInfo->setPullCallbackArg(notifyInfo);
	residentInfo->pullData(RESIDENT_PROTO_HEADER_LEN +
	                       RESIDENT_PROTO_EVENT_ACK_CODE_LEN,
	                       gotNotifyEventAckCb);

	// create or update an action log
	DBClientAction dbAction;
	HATOHOL_ASSERT(notifyInfo->logId != INVALID_ACTION_LOG_ID,
	               "An action log ID is not set.");
	dbAction.updateLogStatusToStart(notifyInfo->logId);
}

void ActionManager::actorCollectedCb(void *priv)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(priv);
	// Pending notifyInfo instancess will be deleted in the destructor.
	delete residentInfo;
}

void ActionManager::closeResident(ResidentInfo *residentInfo)
{
	// kill hatohol-resident-yard.
	// After hatohol-resident-yard is killed, actorCollectedCb()
	// will be called form ActorCollector::checkExitProcess().
	pid_t pid = residentInfo->pid;
	if (pid && kill(pid, SIGKILL))
		MLPL_ERR("Failed to kill. pid: %d, %s\n", pid, strerror(errno));
}

void ActionManager::closeResident(
  ResidentNotifyInfo *notifyInfo,
  DBClientAction::ActionLogExecFailureCode failureCode)
{
	DBClientAction::LogEndExecActionArg logArg;
	logArg.logId = notifyInfo->logId;
	logArg.status = DBClientAction::ACTLOG_STAT_FAILED;
	logArg.failureCode = failureCode;
	logArg.exitCode = 0;
	DBClientAction dbAction;
	dbAction.logEndExecAction(logArg);

	// remove this notifyInfo from the queue in the parent ResidentInfo
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	pid_t pid = residentInfo->pid;
	ActorCollector::setDontLog(pid);
	residentInfo->deleteNotifyInfo(notifyInfo);
	// NOTE: Hereafter we cannot access 'notifyInfo', because it is
	//       deleted in the above function.

	closeResident(residentInfo);
}
