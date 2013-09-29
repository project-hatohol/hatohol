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
#include "ActionExecArgMaker.h"
#include "MutexLock.h"
#include "LabelUtils.h"
#include "ConfigManager.h"
#include "TimeCounter.h"

using namespace std;
using namespace mlpl;

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

struct WaitingCommandActionInfo;
struct SpawnPostprocCommandActionCtx {
	// passed to the callback
	ActorInfo *actorInfoCopy;
	size_t     reservationId;
	WaitingCommandActionInfo *waitCmdInfo;

	// constructor
	SpawnPostprocCommandActionCtx(void)
	: actorInfoCopy(NULL),
	  reservationId(-1),
	  waitCmdInfo(NULL)
	{
	}
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
};

MutexLock          ResidentInfo::residentMapLock;
RunningResidentMap ResidentInfo::runningResidentMap;

struct WaitingCommandActionInfo {
	uint64_t  logId;
	ActionDef actionDef;
	EventInfo eventInfo;
	StringVector argVect;

	// The following variable is used only when the waiting action passes
	// a reservation ID from the collectedCallback to the
	// postCollected callback.
	size_t reservationId;
};

struct CommandActionContext {
	// We must take locks with the following order to prevent a deadlock.
	// (1) ActorCollector::lock()
	// (2) ActionManager::PrivateContext::lock
	static MutexLock     lock;

	static deque<WaitingCommandActionInfo *> waitingList;
	static set<uint64_t> runningSet;  // key is logId
	static set<size_t>   reservedSet;

	// methods
	static void reset(void)
	{
		// We assume that this function is called only when the test.
		// So we forcely delete instances without locking.
		deque<WaitingCommandActionInfo *>::iterator it =
		  waitingList.begin();
		for (; it != waitingList.end(); ++it)
			delete *it;
		waitingList.clear();
		runningSet.clear();
		reservedSet.clear();
	}

	/**
	 * Reserve the command action. If the sum of the number of running
	 * actors and reserved actors (the number of onstage actors) is less
	 * than the limit, this function returns a reservationId that is
	 * needed to add the actor to runningSet. Otherwise the action
	 * is added to the queue and executed later.
	 *
	 * @param reservationId
	 * A reservation ID. This is returned only when the number of onstage
	 * actors less than the limit. Otherwise it is not changed.
	 *
	 * @param actionDef A reference of ActionDef.
	 * @param eventInfo A reference of EventInfo.
	 * @param dbAction  A reference of DBClientAction.
	 * @param argVect   A vector that has command line components.
	 *
	 * @return
	 * NULL on success. Otherwise a pointer of WaitingCommandActionInfo
	 * is returned.
	 */
	static WaitingCommandActionInfo *reserve(
	  size_t &reservationId, const ActionDef &actionDef,
	  const EventInfo &eventInfo, DBClientAction &dbAction,
	  const StringVector &argVect)
	{
		WaitingCommandActionInfo *waitCmdInfo = NULL;
		lock.lock();

		// check the number of running actions
		if (isFullHouse()) {
			waitCmdInfo =
			  insertToWaitingCommandActionList(
			    actionDef, eventInfo, dbAction, argVect);
			lock.unlock();
			return waitCmdInfo;
		}

		// search for the available reservation ID and insert it.
		reservationId = reserveAndInsert();
		lock.unlock();
		return NULL;
	}

	static void cancel(const size_t reservationId)
	{
		lock.lock();
		removeReservationId(reservationId);
		lock.unlock();
	}

	static void add(const size_t reservationId, const uint64_t logId)
	{
		lock.lock();
		removeReservationId(reservationId);

		// insert the log ID
		pair<set<uint64_t>::iterator, bool> result =
		   runningSet.insert(logId);
		HATOHOL_ASSERT(result.second,
		               "Failed to insert: logID: %"PRIu64"\n", logId);
		lock.unlock();
	}

	static void remove(const uint64_t logId)
	{
		lock.lock();
		set<uint64_t>::iterator it = runningSet.find(logId);
		HATOHOL_ASSERT(it != runningSet.end(),
		               "Not found log ID: %"PRIu64"\n", logId);
		runningSet.erase(it);
		lock.unlock();
	}

	static bool isFullHouse(void)
	{
		// This function assumes that 'lock' is being locked.
		ConfigManager *confMgr = ConfigManager::getInstance();
		 size_t numActorLimit =
		  confMgr->getMaxNumberOfRunningCommandAction();
		size_t numTotalActions = runningSet.size() + reservedSet.size();
		return numTotalActions >= numActorLimit;
	}

	static size_t reserveAndInsert(void)
	{
		// This function assumes that 'lock' is being locked.
		size_t rsvId = getReservationId();
		reservedSet.insert(rsvId);
		return rsvId;
	}

	static size_t getNumberOfOnstageCommandActors(void)
	{
		lock.lock();
		size_t numOnstageActors =
		   runningSet.size() + reservedSet.size();
		lock.unlock();
		return numOnstageActors;
	}

protected:
	static WaitingCommandActionInfo *insertToWaitingCommandActionList(
	  const ActionDef &actionDef, const EventInfo &eventInfo,
	  DBClientAction &dbAction, const StringVector &argVect)
	{
		// This function assumes that 'lock' is being locked.
		WaitingCommandActionInfo *waitCmdInfo =
		  new WaitingCommandActionInfo();
		waitCmdInfo->logId =
		   dbAction.createActionLog(actionDef, eventInfo,
		                            ACTLOG_EXECFAIL_NONE,
		                            ACTLOG_STAT_QUEUING);
		waitCmdInfo->actionDef = actionDef;
		waitCmdInfo->eventInfo = eventInfo;
		waitCmdInfo->argVect   = argVect;
		waitingList.push_back(waitCmdInfo);
		return waitCmdInfo;
	}

	static size_t getReservationId(void)
	{
		size_t rsvId = 0;
		// This function assumes that 'lock' is being locked.
		if (!reservedSet.empty()) {
			size_t lastId = *reservedSet.rbegin();
			rsvId = lastId + 1;
			while (!isAvailable(rsvId))
				rsvId++;
		}
		return rsvId;
	}

	static bool isAvailable(size_t id)
	{
		return (runningSet.find(id) == runningSet.end());
	}

	static void removeReservationId(const size_t reservationId)
	{
		set<size_t>::iterator it =
		   reservedSet.find(reservationId);
		HATOHOL_ASSERT(it != reservedSet.end(),
		               "Not found reservationID: %zd\n", reservationId);
		reservedSet.erase(it);
	}
};

MutexLock CommandActionContext::lock;
deque<WaitingCommandActionInfo *>
  CommandActionContext::waitingList;
set<size_t> CommandActionContext::reservedSet;
set<uint64_t> CommandActionContext::runningSet;

struct ActionManager::PrivateContext {
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
const char *ActionManager::NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC
  = "--hatohol-action-v1";

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

	CommandActionContext::reset();
}

ActionManager::ActionManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActionManager::~ActionManager()
{
	if (m_ctx)
		delete m_ctx;
}

void ActionManager::checkEvents(const EventInfoList &eventList)
{
	DBClientAction dbAction;
	EventInfoListConstIterator it = eventList.begin();
	for (; it != eventList.end(); ++it) {
		ActionDefList actionDefList;
		const EventInfo &eventInfo = *it;
		if (shouldSkipByTime(eventInfo))
			continue;
		if (shouldSkipByLog(eventInfo, dbAction))
			continue;
		dbAction.getActionList(actionDefList, &eventInfo);
		ActionDefListIterator actIt = actionDefList.begin();
		for (; actIt != actionDefList.end(); ++actIt)
			runAction(*actIt, eventInfo, dbAction);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool ActionManager::shouldSkipByTime(const EventInfo &eventInfo)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	int allowedOldTime = configMgr->getAllowedTimeOfActionForOldEvents();
	if (allowedOldTime == ConfigManager::ALLOW_ACTION_FOR_ALL_OLD_EVENTS)
		return false;

	TimeCounter eventTimeCounter(eventInfo.time);
	TimeCounter passedTime(TimeCounter::INIT_CURR_TIME);
	passedTime -= eventTimeCounter;
	return passedTime.getAsSec() > allowedOldTime;
}

bool ActionManager::shouldSkipByLog(const EventInfo &eventInfo,
                                    DBClientAction &dbAction)
{
	ActionLog actionLog;
	bool found;
	found = dbAction.getLog(actionLog, eventInfo.serverId, eventInfo.id);
	// TODO: We shouldn't skip if status is ACTLOG_STAT_QUEUING.
	return found;
}

void ActionManager::runAction(const ActionDef &actionDef,
                              const EventInfo &_eventInfo,
                              DBClientAction &dbAction)
{
	EventInfo eventInfo(_eventInfo);
	fillTriggerInfoInEventInfo(eventInfo);

	if (actionDef.type == ACTION_COMMAND) {
		execCommandAction(actionDef, eventInfo, dbAction);
	} else if (actionDef.type == ACTION_RESIDENT) {
		execResidentAction(actionDef, eventInfo, dbAction);
	} else {
		HATOHOL_ASSERT(true, "Unknown type: %d\n", actionDef.type);
	}
}

bool ActionManager::spawn(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBClientAction &dbAction, const gchar **argv,
  SpawnPostproc postproc, void *postprocPriv)
{
	const gchar *workingDirectory = NULL;
	if (!actionDef.workingDir.empty())
		workingDirectory = actionDef.workingDir.c_str();

	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD);
	GSpawnChildSetupFunc childSetup = NULL;
	gpointer userData = NULL;
	GError *error = NULL;

	// create an ActorInfo instance.
	ActorInfo *actorInfo = new ActorInfo();

	WaitingCommandActionInfo *waitCmdInfo = NULL;
	if (actionDef.type == ACTION_COMMAND) {
		SpawnPostprocCommandActionCtx *postprocCtx =
		  static_cast<SpawnPostprocCommandActionCtx *>(postprocPriv);
		waitCmdInfo = postprocCtx->waitCmdInfo;
	}

	// We take the lock here to avoid the child spanwed below from
	// not being collected. If the child immediately exits
	// before the following 'ActorCollector::addActor(&childPid)' is
	// called, ActorCollector::notifyChildSiginfo() possibly ignores it,
	// because the pid of the child isn't in the wait child set.
	ActorCollector::lock();
	gboolean succeeded =
	  g_spawn_async(workingDirectory, (gchar **)argv, NULL,
	                flags, childSetup, userData, &actorInfo->pid, &error);
	if (!succeeded) {
		ActorCollector::unlock();
		uint64_t logId;
		bool logUpdateFlag = false;
		if (waitCmdInfo) {
			logId = waitCmdInfo->logId;
			logUpdateFlag = true;
		}
		postProcSpawnFailure(actionDef, eventInfo, dbAction, actorInfo,
		                     &logId, error, logUpdateFlag);
		delete actorInfo;
		if (postproc)
			(*postproc)(NULL, actionDef, logId, postprocPriv);
		return false;
	}

	ActionLogStatus initialStatus =
	  (actionDef.type == ACTION_COMMAND) ?
	    ACTLOG_STAT_STARTED : ACTLOG_STAT_LAUNCHING_RESIDENT;

	if (waitCmdInfo) {
		// A waiting command action has the log ID.
		// So we simply update the status.
		dbAction.updateLogStatusToStart(waitCmdInfo->logId);
		actorInfo->logId = waitCmdInfo->logId;
	} else {
		actorInfo->logId =
		  dbAction.createActionLog(actionDef, eventInfo,
		                           ACTLOG_EXECFAIL_NONE, initialStatus);
	}
	ActorCollector::addActor(actorInfo);
	if (postproc) {
		(*postproc)(actorInfo, actionDef,
		            actorInfo->logId, postprocPriv);
	}
	ActorCollector::unlock();
	return true;
}

void ActionManager::execCommandAction(const ActionDef &actionDef,
                                      const EventInfo &eventInfo,
                                      DBClientAction &dbAction,
                                      ActorInfo *_actorInfo)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_COMMAND,
	               "Invalid type: %d\n", actionDef.type);
	StringVector argVect;
	ActionExecArgMaker argMaker;
	argMaker.makeExecArg(argVect, actionDef.command);
	if (argVect.empty())
		MLPL_WARN("argVect empty.\n");
	else
		addCommandDirectory(argVect[0]);
	argVect.push_back(NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC);
	argVect.push_back(StringUtils::sprintf("%d", actionDef.id));
	argVect.push_back(StringUtils::sprintf("%"PRIu32, eventInfo.serverId));
	argVect.push_back(StringUtils::sprintf("%"PRIu64, eventInfo.hostId));
	argVect.push_back(StringUtils::sprintf("%ld.%ld",
	  eventInfo.time.tv_sec, eventInfo.time.tv_nsec));
	argVect.push_back(StringUtils::sprintf("%"PRIu64, eventInfo.id));
	argVect.push_back(StringUtils::sprintf("%d", eventInfo.type));
	argVect.push_back(StringUtils::sprintf("%"PRIu64, eventInfo.triggerId));
	argVect.push_back(StringUtils::sprintf("%d", eventInfo.status));
	argVect.push_back(StringUtils::sprintf("%d", eventInfo.severity));

	size_t reservationId = -1;
	WaitingCommandActionInfo *waitCmdInfo =
	  CommandActionContext::reserve(reservationId, actionDef, eventInfo,
	                                dbAction, argVect);
	// If the number of running command actions exceeds the limit,
	// reserveCommandAction() returns a pointer of
	// WaitingCommandActionInfo. In the case, the action is queued
	// and will be executed later.
	if (waitCmdInfo) {
		_actorInfo->logId = waitCmdInfo->logId;
		return;
	}

	SpawnPostprocCommandActionCtx postprocCtx;
	postprocCtx.actorInfoCopy = _actorInfo;
	postprocCtx.reservationId = reservationId;

	execCommandActionCore(actionDef, eventInfo, dbAction,
	                      &postprocCtx, argVect);
}

void ActionManager::spawnPostprocCommandAction(ActorInfo *actorInfo,
                                               const ActionDef &actionDef,
                                               uint64_t logId, void *priv)
{
	SpawnPostprocCommandActionCtx *ctx =
	  static_cast<SpawnPostprocCommandActionCtx *>(priv);
	copyActorInfoForExecResult(ctx->actorInfoCopy, actorInfo, logId);
	if (actorInfo) // Successfully executed
		CommandActionContext::add(ctx->reservationId, logId);
	else // Failed to execute the actor
		CommandActionContext::cancel(ctx->reservationId);
	if (!actorInfo)
		return;
	actorInfo->collectedCb = commandActorCollectedCb;

	// set the timeout
	if (actionDef.timeout <= 0)
		return;
	actorInfo->timerTag =
	   g_timeout_add(actionDef.timeout, commandActionTimeoutCb, actorInfo);
}

void ActionManager::execCommandActionCore(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBClientAction &dbAction, void *postprocCtx,
  const StringVector &argVect)
{
	const gchar *argv[argVect.size()+1];
	for (size_t i = 0; i < argVect.size(); i++)
		argv[i] = argVect[i].c_str();
	argv[argVect.size()] = NULL;

	spawn(actionDef, eventInfo, dbAction, argv,
	      spawnPostprocCommandAction, postprocCtx);
	// spawnPostprocCommandAction() is called in the above spawn().
}

void ActionManager::addCommandDirectory(string &path)
{
	// add the action command directory
	string absPath = ConfigManager::getActionCommandDirectory();
	absPath += "/";
	absPath += path;
	path = absPath;
}

void ActionManager::execResidentAction(const ActionDef &actionDef,
                                       const EventInfo &eventInfo,
                                       DBClientAction &dbAction,
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
		   dbAction.createActionLog(
		     actionDef, eventInfo, ACTLOG_EXECFAIL_NONE,
		     ACTLOG_STAT_RESIDENT_QUEUING);

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
	ResidentInfo *residentInfo =
	  launchResidentActionYard(actionDef, eventInfo, dbAction, _actorInfo);
	if (residentInfo) {
		ResidentInfo::runningResidentMap[actionDef.id] = residentInfo;
		residentInfo->inRunningResidentMap = true;
	}
	ResidentInfo::residentMapLock.unlock();
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
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	// check the packet type
	uint32_t bodyLen = *sbuf.getPointerAndIncIndex<uint32_t>();
	if (bodyLen != 0) {
		MLPL_ERR("Invalid body length: %"PRIu32", "
		         "expect: 0\n", bodyLen);
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_LAUNCHED) {
		MLPL_ERR("Invalid packet type: %"PRIu16", "
		         "expect: %d\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
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
		obj->closeResident(notifyInfo, ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED) {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	// check the result
	sbuf.resetIndex();
	sbuf.incIndex(RESIDENT_PROTO_HEADER_LEN);
	uint32_t resultCode = sbuf.getValueAndIncIndex<uint32_t>();
	if (resultCode != RESIDENT_PROTO_MODULE_LOADED_CODE_SUCCESS) {
		ActionLogExecFailureCode code;
		MLPL_ERR("Failed to load module. "
		         "code: %"PRIu32"\n", resultCode);
		switch (resultCode) {
		case RESIDENT_PROTO_MODULE_LOADED_CODE_FAIL_DLOPEN:
			code = ACTLOG_EXECFAIL_ENTRY_NOT_FOUND;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_MOD_SYMBOL:
			code = ACTLOG_EXECFAIL_MOD_NOT_FOUND_SYMBOL;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_MOD_VER_INVALID:
			code = ACTLOG_EXECFAIL_MOD_VER_INVALID;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_INIT_FAILURE:
			code = ACTLOG_EXECFAIL_MOD_INIT_FAILURE;
			break;
		case RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_NOTIFY_EVENT:
			code = ACTLOG_EXECFAIL_MOD_NOT_FOUND_NOTIFY_EVENT;
			break;
		default:
			code = ACTLOG_EXECFAIL_MOD_UNKNOWN_REASON;
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
		obj->closeResident(notifyInfo, ACTLOG_EXECFAIL_PIPE_READ_ERR);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT_ACK) {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
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
	logArg.status = ACTLOG_STAT_SUCCEEDED,
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
	ActionExecArgMaker::parseResidentCommand(
	  residentInfo->actionDef.command, residentInfo->modulePath,
	  residentInfo->moduleOption);
	
	addCommandDirectory(residentInfo->modulePath);
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

gboolean ActionManager::commandActionTimeoutCb(gpointer data)
{
	DBClientAction dbAction;
	DBClientAction::LogEndExecActionArg logArg;
	ActorInfo *actorInfo = static_cast<ActorInfo *>(data);

	// update an action log
	logArg.logId = actorInfo->logId;
	logArg.status = ACTLOG_STAT_FAILED;
	logArg.failureCode = ACTLOG_EXECFAIL_KILLED_TIMEOUT;
	logArg.exitCode = 0;
	dbAction.logEndExecAction(logArg);
	ActorCollector::setDontLog(actorInfo->pid);
	kill(actorInfo->pid, SIGKILL);
	return FALSE;
}

void ActionManager::residentActionTimeoutCb(NamedPipe *namedPipe, gpointer data)
{
	ResidentInfo *residentInfo = static_cast<ResidentInfo *>(data);
	ActionManager *obj = residentInfo->actionManager;

	ResidentNotifyInfo *notifyInfo = NULL;
	residentInfo->queueLock.lock();
	if (!residentInfo->notifyQueue.empty())
		notifyInfo = residentInfo->notifyQueue.front();
	residentInfo->queueLock.unlock();

	// Check if the queue is empty
	if (!notifyInfo) {
		MLPL_BUG("notifyQueue is empty\n");
		obj->closeResident(residentInfo);
		return;
	}

	// log the incident and kill the resident.
	obj->closeResident(notifyInfo, ACTLOG_EXECFAIL_KILLED_TIMEOUT);
}

struct SpawnPostprocResidentActionCtx {
	// passed to the callback
	ActorInfo    *actorInfoCopy;
	ResidentInfo *residentInfo;

	// set in the callback
	uint64_t logId;
};

void ActionManager::spawnPostprocResidentAction(ActorInfo *actorInfo,
                                                const ActionDef &actionDef,
                                                uint64_t logId, void *priv)
{
	SpawnPostprocResidentActionCtx *ctx =
	  static_cast<SpawnPostprocResidentActionCtx *>(priv);
	copyActorInfoForExecResult(ctx->actorInfoCopy, actorInfo, logId);
	if (!actorInfo)
		return;
	actorInfo->collectedCb = residentActorCollectedCb;
	actorInfo->collectedCbPriv = ctx->residentInfo;
	ctx->residentInfo->pid = actorInfo->pid;
	ctx->logId = logId;
}

ResidentInfo *ActionManager::launchResidentActionYard
  (const ActionDef &actionDef, const EventInfo &eventInfo,
   DBClientAction &dbAction, ActorInfo *actorInfoCopy)
{
	// make a ResidentInfo instance.
	ResidentInfo *residentInfo = new ResidentInfo(this, actionDef);
	if (!residentInfo->init(residentReadErrCb, residentWriteErrCb)) {
		delete residentInfo;
		return NULL;
	}

	string absPath = ConfigManager::getResidentYardDirectory();
	absPath += "/hatohol-resident-yard";
	const gchar *argv[] = {
	  absPath.c_str(),
	  residentInfo->pipeName.c_str(),
	  NULL};
	
	SpawnPostprocResidentActionCtx postprocCtx;
	postprocCtx.actorInfoCopy = actorInfoCopy;
	postprocCtx.residentInfo  = residentInfo;
	bool succeeded = spawn(actionDef, eventInfo, dbAction, argv,
	                       spawnPostprocResidentAction, &postprocCtx);
	// spawnPostprocResidentAction() is called in the above spawn() and
	// the member in postprocArg is set.
	if (!succeeded) {
		delete residentInfo;
		return NULL;
	}
	if (actionDef.timeout > 0) {
		residentInfo->pipeRd.setTimeout(actionDef.timeout,
		                                residentActionTimeoutCb,
		                                residentInfo);
		residentInfo->pipeWr.setTimeout(actionDef.timeout,
		                                residentActionTimeoutCb,
		                                residentInfo);
	}

	// We can push the notifyInfo to the queue without locking,
	// because no other users of this instance at this point.
	ResidentNotifyInfo *notifyInfo = new ResidentNotifyInfo(residentInfo);
	notifyInfo->logId = postprocCtx.logId;
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

void ActionManager::commandActorCollectedCb(const ActorInfo *actorInfo)
{
	// remove this actor from CommandActionContext::runningSet.
	CommandActionContext::remove(actorInfo->logId);

	CommandActionContext::lock.lock();
	// check if there is a waiting command action.
	if (CommandActionContext::waitingList.empty()) {
		CommandActionContext::lock.unlock();
		return;
	}

	if (CommandActionContext::isFullHouse()) {
		CommandActionContext::lock.unlock();
		return;
	}
	// exectute a command
	WaitingCommandActionInfo *waitCmdInfo =
	   CommandActionContext::waitingList.front();
	waitCmdInfo->reservationId = CommandActionContext::reserveAndInsert();
	CommandActionContext::waitingList.pop_front();
	CommandActionContext::lock.unlock();

	// set the post collected callback
	actorInfo->postCollectedCb = commandActorPostCollectedCb;
	actorInfo->collectedCbPriv = waitCmdInfo;
}

void ActionManager::commandActorPostCollectedCb(const ActorInfo *actorInfo)
{
	DBClientAction dbAction;
	WaitingCommandActionInfo *waitCmdInfo =
	  static_cast<WaitingCommandActionInfo *>(actorInfo->collectedCbPriv);
	
	SpawnPostprocCommandActionCtx postprocCtx;
	postprocCtx.actorInfoCopy = NULL;
	postprocCtx.reservationId = waitCmdInfo->reservationId;
	postprocCtx.waitCmdInfo = waitCmdInfo;
	execCommandActionCore(waitCmdInfo->actionDef, waitCmdInfo->eventInfo,
	                      dbAction, &postprocCtx, waitCmdInfo->argVect);
	delete waitCmdInfo;
}

void ActionManager::residentActorCollectedCb(const ActorInfo *actorInfo)
{
	ResidentInfo *residentInfo =
	   static_cast<ResidentInfo *>(actorInfo->collectedCbPriv);
	residentInfo->queueLock.lock();
	bool isQueueEmpty = residentInfo->notifyQueue.empty();
	residentInfo->queueLock.unlock();

	if (!isQueueEmpty) {
		// TODO: Add a mechanism to re-launch hatohol-resident-yard.
		MLPL_WARN(
		  "ResidentInfo instance will be removed. However, "
		  "notifyQueue is not empty.\n");
	}

	// Pending notifyInfo instancess will be deleted in the destructor.
	delete residentInfo;
}

void ActionManager::closeResident(ResidentInfo *residentInfo)
{
	// kill hatohol-resident-yard.
	// After hatohol-resident-yard is killed, residentActorCollectedCb()
	// will be called form ActorCollector::checkExitProcess().
	pid_t pid = residentInfo->pid;
	if (pid && kill(pid, SIGKILL))
		MLPL_ERR("Failed to kill. pid: %d, %s\n", pid, strerror(errno));
}

void ActionManager::closeResident(ResidentNotifyInfo *notifyInfo,
                                  ActionLogExecFailureCode failureCode)
{
	DBClientAction::LogEndExecActionArg logArg;
	logArg.logId = notifyInfo->logId;
	logArg.status = ACTLOG_STAT_FAILED;
	logArg.failureCode = failureCode;
	logArg.exitCode = 0;
	DBClientAction dbAction;
	dbAction.logEndExecAction(logArg);

	// remove this notifyInfo from the queue in the parent ResidentInfo
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	pid_t pid = residentInfo->pid;
	ActorCollector::setDontLog(pid);
	// NOTE: Hereafter we cannot access 'notifyInfo', because it is
	//       deleted in the above function.

	closeResident(residentInfo);
}

void ActionManager::copyActorInfoForExecResult
  (ActorInfo *actorInfoDest, const ActorInfo *actorInfoSrc, uint64_t logId)
{
	if (!actorInfoDest)
		return;

	if (actorInfoSrc)
		*actorInfoDest = *actorInfoSrc;
	else
		actorInfoDest->logId = logId;
}

void ActionManager::postProcSpawnFailure(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBClientAction &dbAction, ActorInfo *actorInfo,
  uint64_t *logId, GError *error, bool logUpdateFlag)
{
	// make an action log
	ActionLogExecFailureCode failureCode =
	  error->code == G_SPAWN_ERROR_NOENT ?
	    ACTLOG_EXECFAIL_ENTRY_NOT_FOUND : ACTLOG_EXECFAIL_EXEC_FAILURE;
	if (!logUpdateFlag) {
		actorInfo->logId =
		  dbAction.createActionLog(actionDef, eventInfo, failureCode);
	} else {
		actorInfo->logId = *logId;
		DBClientAction::LogEndExecActionArg logArg;
		logArg.logId = *logId;
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = failureCode;
		logArg.nullFlags = ACTLOG_FLAG_EXIT_CODE;
		dbAction.logEndExecAction(logArg);
	}

	// MLPL log
	MLPL_ERR(
	  "%s, action ID: %d, log ID: %"PRIu64", "
	  "server ID: %d, event ID: %"PRIu64", "
	  "time: %ld.%09ld, type: %s, "
	  "trigger ID: %d, status: %s, severity: %s, host ID: %"PRIu64"\n", 
	  error->message, actionDef.id, actorInfo->logId,
	  eventInfo.serverId, eventInfo.id,
	  eventInfo.time.tv_sec, eventInfo.time.tv_nsec,
	  LabelUtils::getEventTypeLabel(eventInfo.type).c_str(),
	  eventInfo.triggerId,
	  LabelUtils::getTriggerStatusLabel(eventInfo.status).c_str(),
	  LabelUtils::getTriggerSeverityLabel(eventInfo.severity).c_str(),
	  eventInfo.hostId);

	g_error_free(error);

	// copy the log ID
	if (logId)
		*logId = actorInfo->logId;
}

void ActionManager::fillTriggerInfoInEventInfo(EventInfo &eventInfo)
{
	DBClientHatohol dbHatohol;
	TriggerInfo triggerInfo;
	bool succedded =
	   dbHatohol.getTriggerInfo(triggerInfo,
	                            eventInfo.serverId, eventInfo.triggerId);
	if (succedded) {
		eventInfo.severity = triggerInfo.severity;
		eventInfo.hostId   = triggerInfo.hostId;
		eventInfo.hostName = triggerInfo.hostName;
		eventInfo.brief    = triggerInfo.brief;
	} else {
		MLPL_ERR("Not found: svID: %"PRIu32", trigID: %"PRIu64"\n",
		         eventInfo.serverId, eventInfo.triggerId);
		eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		eventInfo.hostId   = INVALID_HOST_ID;
		eventInfo.hostName.clear();
		eventInfo.brief.clear();
	}
}

size_t ActionManager::getNumberOfOnstageCommandActors(void)
{
	return CommandActionContext::getNumberOfOnstageCommandActors();
}
