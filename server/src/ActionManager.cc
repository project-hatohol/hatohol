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

#include <cstring>
#include <deque>
#include <errno.h>
#include "ActionManager.h"
#include "ActorCollector.h"
#include "DBTablesAction.h"
#include "DBTablesMonitoring.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
#include "ActionExecArgMaker.h"
#include "Mutex.h"
#include "LabelUtils.h"
#include "ConfigManager.h"
#include "SmartTime.h"
#include "SessionManager.h"
#include "Reaper.h"
#include "ChildProcessManager.h"
#include "IncidentSenderManager.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

struct ResidentInfo;
struct ActionManager::ResidentNotifyInfo {
	ResidentInfo *residentInfo;
	uint64_t  logId;
	EventInfo eventInfo; // a replica
	string    sessionId;

	ResidentNotifyInfo(ResidentInfo *_residentInfo);
	virtual ~ResidentNotifyInfo();

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
	static Mutex              residentMapLock;
	static RunningResidentMap runningResidentMap;
	ActionManager *actionManager;
	const ActionDef actionDef;
	pid_t           pid;
	bool            inRunningResidentMap;

	Mutex               queueLock;
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

	/*
	 * executed on the following thread(s)
	 * - Threads that call checkEvents()
	 *     [from launchResidentActionYard() on failure]
	 * - ActorCollector thread
	 *     [residentActorCollectedCb()]
	 * - Thread that calls reset()
	 *     [currently the main thread of the test (cutter)]
	 */
	virtual ~ResidentInfo(void)
	{
		queueLock.lock();
		while (!notifyQueue.empty()) {
			ActionManager::ResidentNotifyInfo *notifyInfo
			  = notifyQueue.front();
			MLPL_BUG("ResidentNotifyInfo is deleted, "
			         "but not logged: logId: %" PRIu64 "\n",
			         notifyInfo->logId);
			delete notifyInfo;
			notifyQueue.pop_front();
		}
		queueLock.unlock();

		// delete this element from the map.
		if (!inRunningResidentMap)
			return;
		bool found = false;
		ActionIdType actionId = actionDef.id;
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

Mutex              ResidentInfo::residentMapLock;
RunningResidentMap ResidentInfo::runningResidentMap;

// ---------------------------------------------------------------------------
// methods of ResidentNotifyInfo
// ---------------------------------------------------------------------------
ActionManager::ResidentNotifyInfo::ResidentNotifyInfo(ResidentInfo *_residentInfo)
: residentInfo(_residentInfo),
  logId(INVALID_ACTION_LOG_ID)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	sessionId = sessionMgr->create(residentInfo->actionDef.ownerUserId,
	                               SessionManager::NO_TIMEOUT);
}

ActionManager::ResidentNotifyInfo::~ResidentNotifyInfo()
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	if (!sessionMgr->remove(sessionId)) {
		MLPL_ERR("Failed to remove session: %s\n",
		         sessionId.c_str());
	}
}

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
	// (1) ActorCollector's lock: new ActorCollector::locker()
	// (2) ActionManager::Impl::lock
	static Mutex         lock;

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
	 * @param dbAction  A reference of DBTablesAction.
	 * @param argVect   A vector that has command line components.
	 *
	 * @return
	 * NULL on success. Otherwise a pointer of WaitingCommandActionInfo
	 * is returned.
	 */
	static WaitingCommandActionInfo *reserve(
	  size_t &reservationId, const ActionDef &actionDef,
	  const EventInfo &eventInfo, DBTablesAction &dbAction,
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
		               "Failed to insert: logID: %" PRIu64 "\n", logId);
		lock.unlock();
	}

	static void remove(const uint64_t logId)
	{
		lock.lock();
		set<uint64_t>::iterator it = runningSet.find(logId);
		HATOHOL_ASSERT(it != runningSet.end(),
		               "Not found log ID: %" PRIu64 "\n", logId);
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
	  DBTablesAction &dbAction, const StringVector &argVect)
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

Mutex CommandActionContext::lock;
deque<WaitingCommandActionInfo *>
  CommandActionContext::waitingList;
set<size_t> CommandActionContext::reservedSet;
set<uint64_t> CommandActionContext::runningSet;

class ActorInfoCopier {
public:
	// constructor and destructor
	ActorInfoCopier(ActorInfo *dest, const ActorInfo *src, uint64_t logId)
	: m_dest(dest),
	  m_src(src),
	  m_logId(logId)
	{
	}

	/*
	 * executed on the following thread(s)
	 * - Threads that call checkEvents()
	 *     [from spawnPostprocCommandAction()]
	 * - ActorCollector thread
	 *     [from spawnPostprocCommandAction()]
	 */
	virtual ~ActorInfoCopier()
	{
		if (!m_dest)
			return;

		if (m_src)
			*m_dest = *m_src;
		else
			m_dest->logId = m_logId;
	}

private:
	ActorInfo       *m_dest;
	const ActorInfo *m_src;
	uint64_t         m_logId;
};

struct ActionManager::Impl {
	static string pathForAction;
	static string ldLibraryPathForAction;
};
string ActionManager::Impl::pathForAction;
string ActionManager::Impl::ldLibraryPathForAction;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
const char *ActionManager::NUM_COMMNAD_ACTION_EVENT_ARG_MAGIC
  = "--hatohol-action-v1";
const char *ActionManager::ENV_NAME_PATH_FOR_ACTION
  = "HATOHOL_ACTION_PATH";
const char *ActionManager::ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION
  = "HATOHOL_ACTION_LD_LIBRARY_PATH";
const char *ActionManager::ENV_NAME_SESSION_ID = "HATOHOL_SESSION_ID";

void ActionManager::reset(void)
{
	setupPathForAction(Impl::pathForAction,
	                   Impl::ldLibraryPathForAction);

	// The following deletion has naturally no effect at the start of
	// Hatohol. This is mainly for the test.
	// NOTE: This is a special case for test, so we don't take
	// ResidentInfo::residentMapLock. Or the deadlock will happen
	// in the desctoructor of ResidentInfo.
	RunningResidentMapIterator it =
	   ResidentInfo::runningResidentMap.begin();
	for (; it != ResidentInfo::runningResidentMap.end(); ++it) {
		ResidentInfo *residentInfo = it->second;
		Utils::deleteOnGLibEventLoop<ResidentInfo>(residentInfo);
	}
	ResidentInfo::runningResidentMap.clear();

	CommandActionContext::reset();
}

ActionManager::ActionManager(void)
: m_impl(new Impl())
{
}

ActionManager::~ActionManager()
{
}

static bool shouldSkipIncidentSender(
  ActionIdType &incidentSenderActionId, const ActionDef &actionDef,
  const EventInfo &eventInfo)
{
	if (actionDef.type != ACTION_INCIDENT_SENDER)
		return false;

	if (incidentSenderActionId > 0) {
		MLPL_DBG("Skip IncidentSenderAction:%" FMT_ACTION_ID " for "
			 "Trigger:%" FMT_TRIGGER_ID " because "
			 "IncidentSenderAction:%" FMT_ACTION_ID " has "
			 "higher priority.",
			 actionDef.id, eventInfo.triggerId.c_str(),
			 incidentSenderActionId);
		return true;
	}

	incidentSenderActionId = actionDef.id;

	return false;
}

void ActionManager::checkEvents(const EventInfoList &eventList)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	EventInfoListConstIterator it = eventList.begin();
	for (; it != eventList.end(); ++it) {
		ActionDefList actionDefList;
		const EventInfo &eventInfo = *it;
		if (eventInfo.id != DISCONNECT_SERVER_EVENT_ID) {
			if (shouldSkipByTime(eventInfo))
				continue;
			if (shouldSkipByLog(eventInfo, dbAction))
				continue;
		}
		ActionsQueryOption option(USER_ID_SYSTEM);
		// TODO: sort IncidentSender type actions by priority
		option.setActionType(ACTION_ALL);
		option.setTargetEventInfo(&eventInfo);
		dbAction.getActionList(actionDefList, option);
		ActionDefListIterator actIt = actionDefList.begin();
		ActionIdType incidentSenderActionId = 0;
		for (; actIt != actionDefList.end(); ++actIt) {
			bool skip = shouldSkipIncidentSender(
				      incidentSenderActionId,
				      *actIt, eventInfo);
			if (!skip)
				runAction(*actIt, eventInfo, dbAction);
		}
	}
}

void ActionManager::reExecuteUnfinishedAction(void)
{
	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();

	ActionLogList actionLogList;
	vector<ActionLogStatus> targetStatuses{ACTLOG_STAT_QUEUING,
	                                       ACTLOG_STAT_STARTED,
	                                       ACTLOG_STAT_RESIDENT_QUEUING,
	                                       ACTLOG_STAT_LAUNCHING_RESIDENT};

	if (!dbAction.getTargetStatusesLogs(actionLogList, targetStatuses)) {
		MLPL_INFO("Hatohol does not have unfinished action.\n");
		return;
	}

	ActionLogListIterator actLogIt = actionLogList.begin();
	for (; actLogIt != actionLogList.end(); ++actLogIt) {
		ActionLog &actionLog = *actLogIt;
		EventInfoList eventList;
		EventsQueryOption eventOption(USER_ID_SYSTEM);

		eventOption.setTargetServerId(actionLog.serverId);
		EventIdType eventId = actionLog.eventId;
		list<EventIdType> eventIds{eventId};
		eventOption.setEventIds(eventIds);
		dbMonitoring.getEventInfoList(eventList, eventOption);
		if (eventList.size() == 0)
			continue;
		EventInfo eventInfo = *eventList.begin();

		ActionDefList actionList;
		ActionsQueryOption actionOption(USER_ID_SYSTEM);
		ActionIdList actionIdList{actionLog.actionId};
		actionOption.setActionIdList(actionIdList);
		dbAction.getActionList(actionList, actionOption);

		if (actionList.size() == 0)
			continue;
		runAction(*actionList.begin(), eventInfo, dbAction);
		dbAction.updateLogStatusToAborted(actionLog.id);
		string message = StringUtils::sprintf("Action log ID(%" FMT_GEN_ID
		                 "): Update log status to aborted(7).\n", actionLog.id);
		MLPL_WARN("%s", message.c_str());
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActionManager::setupPathForAction(string &path, string &ldLibraryPath)
{
	char *env = getenv(ENV_NAME_PATH_FOR_ACTION);
	if (env) {
		path = "PATH=";
		path += env;
	}

	env = getenv(ENV_NAME_LD_LIBRARY_PATH_FOR_ACTION);
	if (env) {
		ldLibraryPath = "LD_LIBRARY_PATH=";
		ldLibraryPath += env;
	}
}

bool ActionManager::shouldSkipByTime(const EventInfo &eventInfo)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	int allowedOldTime = configMgr->getAllowedTimeOfActionForOldEvents();
	if (allowedOldTime == ConfigManager::ALLOW_ACTION_FOR_ALL_OLD_EVENTS)
		return false;

	SmartTime eventTime(eventInfo.time);
	SmartTime passedTime(SmartTime::INIT_CURR_TIME);
	passedTime -= eventTime;
	return passedTime.getAsSec() > allowedOldTime;
}

bool ActionManager::shouldSkipByLog(const EventInfo &eventInfo,
                                    DBTablesAction &dbAction)
{
	ActionLog actionLog;
	bool found;
	found = dbAction.getLog(actionLog, eventInfo.serverId, eventInfo.id);
	// TODO: We shouldn't skip if status is ACTLOG_STAT_QUEUING.
	return found;
}

static bool checkActionOwner(const ActionDef &actionDef)
{
	if (actionDef.type == ACTION_INCIDENT_SENDER) {
		// We will not introduce the ownership concept for this action
		// type. Access control will be realized only by privilege.
		return (actionDef.ownerUserId == USER_ID_SYSTEM);
	}
	HATOHOL_ASSERT(actionDef.ownerUserId != USER_ID_SYSTEM,
	               "Invalid userid: %d\n", actionDef.ownerUserId);
	return true;
}

HatoholError ActionManager::runAction(const ActionDef &actionDef,
                                      const EventInfo &_eventInfo,
                                      DBTablesAction &dbAction)
{
	EventInfo eventInfo(_eventInfo);
	fillTriggerInfoInEventInfo(eventInfo);

	if (!checkActionOwner(actionDef))
		return HTERR_INVALID_USER;

	if (actionDef.type == ACTION_COMMAND) {
		execCommandAction(actionDef, eventInfo, dbAction);
	} else if (actionDef.type == ACTION_RESIDENT) {
		execResidentAction(actionDef, eventInfo, dbAction);
	} else if (actionDef.type == ACTION_INCIDENT_SENDER) {
		execIncidentSenderAction(actionDef, eventInfo, dbAction);
	} else {
		HATOHOL_ASSERT(true, "Unknown type: %d\n", actionDef.type);
	}
	return HTERR_OK;
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from execCommandActionCore()]
 *     [from launchResidentActionYard()]
 * - ActorCollector thread
 *     [from execCommandActionCore() from commandActorPostCollectedCb()]
 */
struct ActionManager::ActorProfile : public ActorCollector::Profile {

	const ActionDef &actionDef;
	const EventInfo &eventInfo;
	SpawnPostproc    postproc;
	void            *postprocPriv;
	WaitingCommandActionInfo *waitCmdInfo;
	ActorInfo                *actorInfo;

	ActorProfile(const ActionDef &_actionDef, const EventInfo &_eventInfo,
	             DBTablesAction  &_dbAction,
	             SpawnPostproc _postproc, void *_postprocPriv)
	: actionDef(_actionDef),
	  eventInfo(_eventInfo),
	  postproc(_postproc),
	  postprocPriv(_postprocPriv),
	  waitCmdInfo(NULL)
	{
		actorInfo = new ActorInfo();
	}

	virtual ActorInfo *successCb(const pid_t &pid) override
	{
		actorInfo->pid = pid;
		ActionLogStatus initialStatus =
		  (actionDef.type == ACTION_COMMAND) ?
		    ACTLOG_STAT_STARTED : ACTLOG_STAT_LAUNCHING_RESIDENT;

		ThreadLocalDBCache cache;
		DBTablesAction &dbAction = cache.getAction();
		if (waitCmdInfo) {
			// A waiting command action has the log ID.
			// So we simply update the status.
			dbAction.updateLogStatusToStart(waitCmdInfo->logId);
			actorInfo->logId = waitCmdInfo->logId;
		} else {
			actorInfo->logId =
			  dbAction.createActionLog(
			    actionDef, eventInfo,
			    ACTLOG_EXECFAIL_NONE, initialStatus);
		}
		return actorInfo;
	}

	virtual void postSuccessCb(void) override
	{
		if (postproc) {
			(*postproc)(actorInfo, actionDef,
			            actorInfo->logId, postprocPriv);
		}
	}

	virtual void errorCb(GError *error) override
	{
		uint64_t logId;
		bool logUpdateFlag = false;
		if (waitCmdInfo) {
			logId = waitCmdInfo->logId;
			logUpdateFlag = true;
		}

		ThreadLocalDBCache cache;
		DBTablesAction &dbAction = cache.getAction();
		postProcSpawnFailure(actionDef, eventInfo, dbAction, actorInfo,
		                     &logId, error, logUpdateFlag);
		delete actorInfo;
		if (postproc)
			(*postproc)(NULL, actionDef, logId, postprocPriv);
	}
};

bool ActionManager::spawn(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBTablesAction &dbAction, const gchar **argv,
  SpawnPostproc postproc, void *postprocPriv)
{
	ActorProfile actorProf(actionDef, eventInfo, dbAction,
	                       postproc, postprocPriv);

	for (; *argv; argv++)
		actorProf.args.push_back(*argv);

	// make envp with HATOHOL_SESSION_ID
	actorProf.envs.push_back(Impl::pathForAction);
	actorProf.envs.push_back(Impl::ldLibraryPathForAction);
	actorProf.envs.push_back(
	  makeSessionIdEnv(actionDef, actorProf.actorInfo->sessionId));

	if (!actionDef.workingDir.empty())
		actorProf.workingDirectory = actionDef.workingDir.c_str();

	if (actionDef.type == ACTION_COMMAND) {
		SpawnPostprocCommandActionCtx *postprocCtx =
		  static_cast<SpawnPostprocCommandActionCtx *>(postprocPriv);
		actorProf.waitCmdInfo = postprocCtx->waitCmdInfo;
	}

	return ActorCollector::debut(actorProf) == HTERR_OK;
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from runAction()]
 */
void ActionManager::execCommandAction(const ActionDef &actionDef,
                                      const EventInfo &eventInfo,
                                      DBTablesAction &dbAction,
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
	argVect.push_back(StringUtils::sprintf("%" PRIu32, eventInfo.serverId));
	argVect.push_back(eventInfo.hostIdInServer);
	argVect.push_back(StringUtils::sprintf("%ld.%ld",
	  eventInfo.time.tv_sec, eventInfo.time.tv_nsec));
	argVect.push_back(eventInfo.id);
	argVect.push_back(StringUtils::sprintf("%d", eventInfo.type));
	argVect.push_back(eventInfo.triggerId);
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
		if (_actorInfo)
			_actorInfo->logId = waitCmdInfo->logId;
		return;
	}

	SpawnPostprocCommandActionCtx postprocCtx;
	postprocCtx.actorInfoCopy = _actorInfo;
	postprocCtx.reservationId = reservationId;

	execCommandActionCore(actionDef, eventInfo, dbAction,
	                      &postprocCtx, argVect);
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from spawn() from execCommandActionCore()]
 * - ActorCollector thread
 *     [from spawn() from execCommandActionCore()]
 */
void ActionManager::spawnPostprocCommandAction(ActorInfo *actorInfo,
                                               const ActionDef &actionDef,
                                               uint64_t logId, void *priv)
{
	SpawnPostprocCommandActionCtx *ctx =
	  static_cast<SpawnPostprocCommandActionCtx *>(priv);

	// AcotorInfo is copied at the end of this method.
	ActorInfoCopier actorInfoCopier(ctx->actorInfoCopy, actorInfo, logId);

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

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from runAction()]
 * - ActorCollector thread
 *     [from commandActorPostCollectedCb()]
 */
void ActionManager::execCommandActionCore(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBTablesAction &dbAction, void *postprocCtx,
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
	string absPath =
	  ConfigManager::getInstance()->getActionCommandDirectory();
	absPath += "/";
	absPath += path;
	path = absPath;
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from runAction()]
 */
void ActionManager::execResidentAction(const ActionDef &actionDef,
                                       const EventInfo &eventInfo,
                                       DBTablesAction &dbAction,
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

/*
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by init()]
 */
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

/*
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by init()]
 */
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

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by pullData()]
 */
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
		MLPL_ERR("Invalid body length: %" PRIu32 ", "
		         "expect: 0\n", bodyLen);
		obj->closeResident(notifyInfo,
		                   ACTLOG_EXECFAIL_PIPE_READ_DATA_UNEXPECTED);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType != RESIDENT_PROTO_PKT_TYPE_LAUNCHED) {
		MLPL_ERR("Invalid packet type: %" PRIu16 ", "
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

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by pullData()]
 */
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
		         "code: %" PRIu32 "\n", resultCode);
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

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by pullData()]
 */
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
	               "log ID: %" PRIx64, notifyInfo->logId);
	DBTablesAction::LogEndExecActionArg logArg;
	logArg.logId = notifyInfo->logId;
	logArg.status = ACTLOG_STAT_SUCCEEDED,
	logArg.exitCode = resultCode;

	ThreadLocalDBCache cache;
	cache.getAction().logEndExecAction(logArg);

	// remove the notifyInfo 
	residentInfo->deleteFrontNotifyInfo();

	// send the next notificaiton if it exists
	residentInfo->setStatus(RESIDENT_STAT_IDLE);
	obj->tryNotifyEvent(residentInfo);
}

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [from launchedCb()]
 */
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

/**
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 */
gboolean ActionManager::commandActionTimeoutCb(gpointer data)
{
	DBTablesAction::LogEndExecActionArg logArg;
	ActorInfo *actorInfo = static_cast<ActorInfo *>(data);

	// update an action log
	logArg.logId = actorInfo->logId;
	logArg.status = ACTLOG_STAT_FAILED;
	logArg.failureCode = ACTLOG_EXECFAIL_KILLED_TIMEOUT;
	logArg.exitCode = 0;
	ThreadLocalDBCache cache;
	cache.getAction().logEndExecAction(logArg);
	ActorCollector::setDontLog(actorInfo->pid);
	kill(actorInfo->pid, SIGKILL);
	actorInfo->timerTag = INVALID_EVENT_ID;
	return FALSE;
}

/**
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 */
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

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [ from spwan() from execResidentAction()]
 */
void ActionManager::spawnPostprocResidentAction(ActorInfo *actorInfo,
                                                const ActionDef &actionDef,
                                                uint64_t logId, void *priv)
{
	SpawnPostprocResidentActionCtx *ctx =
	  static_cast<SpawnPostprocResidentActionCtx *>(priv);

	// AcotorInfo is copied at the end of this method.
	ActorInfoCopier actorInfoCopier(ctx->actorInfoCopy, actorInfo, logId);

	if (!actorInfo)
		return;
	actorInfo->collectedCb = residentActorCollectedCb;
	actorInfo->collectedCbPriv = ctx->residentInfo;
	ctx->residentInfo->pid = actorInfo->pid;
	ctx->logId = logId;
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from execResidentAction()]
 */
ResidentInfo *ActionManager::launchResidentActionYard
  (const ActionDef &actionDef, const EventInfo &eventInfo,
   DBTablesAction &dbAction, ActorInfo *actorInfoCopy)
{
	// make a ResidentInfo instance.
	ResidentInfo *residentInfo = new ResidentInfo(this, actionDef);
	if (!residentInfo->init(residentReadErrCb, residentWriteErrCb)) {
		delete residentInfo;
		return NULL;
	}

	string absPath =
	  ConfigManager::getInstance()->getResidentYardDirectory();
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

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from execResidentAction()]
 * - The default GLIB event dispacther thread (main)
 *     [from moduleLoadedCb()]
 *     [from gotNotifyEventAckCb]
 */
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

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from notifyEvent()]
 * - The default GLIB event dispacther thread (main)
 *     [from notifyEvent()]
 */
void ActionManager::notifyEvent(ResidentInfo *residentInfo,
                                ResidentNotifyInfo *notifyInfo)
{
	ResidentCommunicator comm;
	comm.setNotifyEventBody(residentInfo->actionDef.id,
	                        notifyInfo->eventInfo,
	                        notifyInfo->sessionId);
	comm.push(residentInfo->pipeWr);

	// wait for result code
	residentInfo->setPullCallbackArg(notifyInfo);
	residentInfo->pullData(RESIDENT_PROTO_HEADER_LEN +
	                       RESIDENT_PROTO_EVENT_ACK_CODE_LEN,
	                       gotNotifyEventAckCb);

	// create or update an action log
	HATOHOL_ASSERT(notifyInfo->logId != INVALID_ACTION_LOG_ID,
	               "An action log ID is not set.");
	ThreadLocalDBCache cache;
	cache.getAction().updateLogStatusToStart(notifyInfo->logId);
}

/*
 * executed on the following thread(s)
 * - IncidentSender thread
 */
static void onIncidentSenderJobStatusChanged(
  const IncidentSender &sender, const EventInfo &info,
  const IncidentSender::JobStatus &status, void *userData)
{
	DBTablesAction::LogEndExecActionArg *logArg
	  = static_cast<DBTablesAction::LogEndExecActionArg *>(userData);
	bool completed = false;

	ThreadLocalDBCache cache;
	DBTablesAction &dbAction = cache.getAction();
	switch (status) {
	case IncidentSender::JOB_STARTED:
	{
		dbAction.updateLogStatusToStart(logArg->logId);
		break;
	}
	case IncidentSender::JOB_SUCCEEDED:
		logArg->status = ACTLOG_STAT_SUCCEEDED;
		completed = true;
		break;
	case IncidentSender::JOB_FAILED:
		logArg->status = ACTLOG_STAT_FAILED;
		// TODO: add more detailed failure code
		logArg->failureCode = ACTLOG_EXECFAIL_EXEC_FAILURE;
		completed = true;
		break;
	default:
		break;
	}

	if (completed) {
		dbAction.logEndExecAction(*logArg);
		delete logArg;
	}
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from runAction()]
 */
void ActionManager::execIncidentSenderAction(const ActionDef &actionDef,
					     const EventInfo &eventInfo,
					     DBTablesAction &dbAction)
{
	HATOHOL_ASSERT(actionDef.type == ACTION_INCIDENT_SENDER,
	               "Invalid type: %d\n", actionDef.type);

	IncidentTrackerIdType trackerId;
	bool succeeded = actionDef.parseIncidentSenderCommand(trackerId);
	if (!succeeded) {
		MLPL_ERR("Invalid IncidentSender command: %s\n",
			 actionDef.command.c_str());
		return;
	}
	IncidentSenderManager &senderManager
	  = IncidentSenderManager::getInstance();
	DBTablesAction::LogEndExecActionArg *logArg
	  = new DBTablesAction::LogEndExecActionArg();
	logArg->logId = dbAction.createActionLog(actionDef, eventInfo,
						 ACTLOG_EXECFAIL_NONE,
						 ACTLOG_STAT_QUEUING);
	senderManager.queue(trackerId, eventInfo,
			    onIncidentSenderJobStatusChanged, logArg);
}

/*
 * executed on the following thread(s)
 * - ActorCollector thread
 */
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

/**
 * executed on the following thread(s)
 * - ActorCollector thread
 */
void ActionManager::commandActorPostCollectedCb(const ActorInfo *actorInfo)
{
	WaitingCommandActionInfo *waitCmdInfo =
	  static_cast<WaitingCommandActionInfo *>(actorInfo->collectedCbPriv);
	
	SpawnPostprocCommandActionCtx postprocCtx;
	postprocCtx.actorInfoCopy = NULL;
	postprocCtx.reservationId = waitCmdInfo->reservationId;
	postprocCtx.waitCmdInfo = waitCmdInfo;
	ThreadLocalDBCache cache;
	execCommandActionCore(waitCmdInfo->actionDef, waitCmdInfo->eventInfo,
	                      cache.getAction(),
	                      &postprocCtx, waitCmdInfo->argVect);
	delete waitCmdInfo;
}

/*
 * executed on the following thread(s)
 * - ActorCollector thread
 */
void ActionManager::residentActorCollectedCb(const ActorInfo *actorInfo)
{
	// residentInfo has to be deleted without lock
	actorInfo->postCollectedCb = residentActorPostCollectedCb;
}

/*
 * executed on the following thread(s)
 * - ActorCollector thread
 */
void ActionManager::residentActorPostCollectedCb(const ActorInfo *actorInfo)
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

	// 1. Pending notifyInfo instancess will be deleted in the destructor.
	// 2. residentInfo uses GLib's timeout event and GIOChannel.
	//    The following prevents residentInfo from being destoryed
	//    when the event handlers are running.
	Utils::deleteOnGLibEventLoop<ResidentInfo>(residentInfo);
}

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [from residentReadErrCb()]
 *     [from residentWriteErrCb()]
 *     [from residentActionTimeoutCb()]
 *     [from closeResident()]
 */
void ActionManager::closeResident(ResidentInfo *residentInfo)
{
	// kill hatohol-resident-yard.
	// After hatohol-resident-yard is killed, residentActorCollectedCb()
	// will be called back from ActorCollector::checkExitProcess().
	pid_t pid = residentInfo->pid;
	if (pid && kill(pid, SIGKILL))
		MLPL_ERR("Failed to kill. pid: %d, %s\n", pid, g_strerror(errno));
}

/*
 * executed on the following thread(s)
 * - The default GLIB event dispacther thread (main)
 *     [callback registered by pullData()]
 *     [from residentActionTimeoutCb()]
 *     [from gotNotifyEventAckCb()]
 *     [from launchedCb()]
 *     [from moduleLoadedCb()]
 */
void ActionManager::closeResident(ResidentNotifyInfo *notifyInfo,
                                  ActionLogExecFailureCode failureCode)
{
	DBTablesAction::LogEndExecActionArg logArg;
	logArg.logId = notifyInfo->logId;
	logArg.status = ACTLOG_STAT_FAILED;
	logArg.failureCode = failureCode;
	logArg.exitCode = 0;
	ThreadLocalDBCache cache;
	cache.getAction().logEndExecAction(logArg);

	// remove this notifyInfo from the queue in the parent ResidentInfo
	ResidentInfo *residentInfo = notifyInfo->residentInfo;
	pid_t pid = residentInfo->pid;
	ActorCollector::setDontLog(pid);
	// NOTE: Hereafter we cannot access 'notifyInfo', because it is
	//       deleted in the above function.

	closeResident(residentInfo);
}

/*
 * executed on the following thread(s)
 * - Threads that call checkEvents()
 *     [from spawn()]
 * - ActorCollector thread
 *     [from spawn()]
 */
void ActionManager::postProcSpawnFailure(
  const ActionDef &actionDef, const EventInfo &eventInfo,
  DBTablesAction &dbAction, ActorInfo *actorInfo,
  uint64_t *logId, const GError *error, bool logUpdateFlag)
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
		DBTablesAction::LogEndExecActionArg logArg;
		logArg.logId = *logId;
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = failureCode;
		logArg.nullFlags = ACTLOG_FLAG_EXIT_CODE;
		dbAction.logEndExecAction(logArg);
	}

	// MLPL log
	MLPL_ERR(
	  "%s, action ID: %d, log ID: %" FMT_ACTION_LOG_ID ", "
	  "server ID: %" FMT_SERVER_ID ", event ID: %" FMT_EVENT_ID ", "
	  "time: %ld.%09ld, type: %s, "
	  "trigger ID: %" FMT_TRIGGER_ID ", status: %s, severity: %s, "
	  "host ID: %" FMT_LOCAL_HOST_ID "\n",
	  error->message, actionDef.id, actorInfo->logId,
	  eventInfo.serverId, eventInfo.id.c_str(),
	  eventInfo.time.tv_sec, eventInfo.time.tv_nsec,
	  LabelUtils::getEventTypeLabel(eventInfo.type).c_str(),
	  eventInfo.triggerId.c_str(),
	  LabelUtils::getTriggerStatusLabel(eventInfo.status).c_str(),
	  LabelUtils::getTriggerSeverityLabel(eventInfo.severity).c_str(),
	  eventInfo.hostIdInServer.c_str());

	// copy the log ID
	if (logId)
		*logId = actorInfo->logId;
}

void ActionManager::fillTriggerInfoInEventInfo(EventInfo &eventInfo)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfo triggerInfo;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(eventInfo.serverId);
	option.setTargetId(eventInfo.triggerId);
	bool succedded = dbMonitoring.getTriggerInfo(triggerInfo, option);
	if (succedded) {
		eventInfo.severity = triggerInfo.severity;
		eventInfo.globalHostId   = triggerInfo.globalHostId;
		eventInfo.hostIdInServer = triggerInfo.hostIdInServer;
		eventInfo.hostName = triggerInfo.hostName;
		eventInfo.brief    = triggerInfo.brief;
	} else {
		if (DO_NOT_ASSOCIATE_TRIGGER_ID != eventInfo.triggerId) {
			MLPL_ERR("Not found: svID: %" FMT_SERVER_ID ", "
				 "trigID: %" FMT_TRIGGER_ID "\n",
				 eventInfo.serverId, eventInfo.triggerId.c_str());
		}
		eventInfo.severity = TRIGGER_SEVERITY_UNKNOWN;
		eventInfo.globalHostId = INVALID_HOST_ID;
		eventInfo.hostIdInServer.clear();
		eventInfo.hostName.clear();
		eventInfo.brief.clear();
	}
}

size_t ActionManager::getNumberOfOnstageCommandActors(void)
{
	return CommandActionContext::getNumberOfOnstageCommandActors();
}

string ActionManager::makeSessionIdEnv(const ActionDef &actionDef,
                                       string &sessionId)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	sessionId = sessionMgr->create(actionDef.ownerUserId,
	                               SessionManager::NO_TIMEOUT);
	string sessionIdEnv = ActionManager::ENV_NAME_SESSION_ID;
	sessionIdEnv += "=";
	sessionIdEnv += sessionId;
	return sessionIdEnv;
}
