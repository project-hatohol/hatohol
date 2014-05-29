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

#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <cstring>
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "Logger.h"
#include "MutexLock.h"
#include "SessionManager.h"
#include "Reaper.h"
#include "ChildProcessManager.h"
using namespace std;
using namespace mlpl;

typedef map<pid_t, ActorInfo *>      WaitChildSet;
typedef WaitChildSet::iterator       WaitChildSetIterator;
typedef WaitChildSet::const_iterator WaitChildSetConstIterator;

struct ActorCollector::PrivateContext {
	static MutexLock lock;
	static WaitChildSet waitChildSet;

	struct Locker
	{
		Locker(void)
		{
			lock.lock();
		}

		virtual ~Locker()
		{
			lock.unlock();
		}
	};
};

MutexLock ActorCollector::PrivateContext::lock;
WaitChildSet ActorCollector::PrivateContext::waitChildSet;

struct ActorCollector::ActorContext {
	ActorInfo *actorInfo;
	DBClientAction::LogEndExecActionArg logArg;

	ActorContext(void)
	: actorInfo(NULL)
	{
	}
};

// ---------------------------------------------------------------------------
// ActorInfo
// ---------------------------------------------------------------------------
ActorInfo::ActorInfo(void)
: pid(0),
  logId(-1),
  dontLog(false),
  collectedCb(NULL),
  postCollectedCb(NULL),
  collectedCbPriv(NULL),
  timerTag(INVALID_EVENT_ID)
{
}

ActorInfo &ActorInfo::operator=(const ActorInfo &actorInfo)
{
	pid             = actorInfo.pid;
	logId           = actorInfo.logId;
	dontLog         = actorInfo.dontLog;
	// Don't copy sessionId to avoid multiple call of
	// SessionManager::remove() for the session.
	collectedCb     = actorInfo.collectedCb;
	postCollectedCb = actorInfo.postCollectedCb;
	collectedCbPriv = actorInfo.collectedCbPriv;
	timerTag        = actorInfo.timerTag;

	return *this;
}

ActorInfo::~ActorInfo()
{
	// If the following function fails, MPL_ERR() is called in it.
	// So we do nothing here.
	Utils::removeEventSourceIfNeeded(timerTag);

	if (!sessionId.empty()) {
		SessionManager *sessionMgr = SessionManager::getInstance();
		if (!sessionMgr->remove(sessionId)) {
			MLPL_WARN("Failed to remove session: %s\n",
			          sessionId.c_str());
		}
	}
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ActorCollector::reset(void)
{
	ChildProcessManager::getInstance()->reset();
	PrivateContext::Locker locker;
	HATOHOL_ASSERT(PrivateContext::waitChildSet.empty(),
	               "waitChildSet is not empty (%zd).",
	               PrivateContext::waitChildSet.size());
}

HatoholError ActorCollector::debut(Profile &profile)
{
	struct EventCb : public ChildProcessManager::EventCallback {
		Profile &profile;
		ChildProcessManager::CreateArg &arg;
		ActorContext actorCtx;

		EventCb(Profile &_profile, ChildProcessManager::CreateArg &_arg)
		: profile(_profile),
		  arg(_arg)
		{
		}

		virtual void onExecuted(
		  bool const &succeeded, GError *gerror) override
		{
			if (succeeded) {
				actorCtx.actorInfo = profile.successCb(arg.pid);
				addActor(actorCtx.actorInfo);
				profile.postSuccessCb();
			} else {
				profile.errorCb(gerror);
			}
		}

		virtual void onCollected(const siginfo_t *siginfo) override
		{
			notifyChildSiginfo(siginfo, actorCtx);
		}

		virtual void onFinalized(void) override
		{
			if (!actorCtx.actorInfo)
				return ;
			postCollectedProc(actorCtx);
		}

		virtual void onReset(void) override
		{
			cleanupChildInfo(actorCtx.actorInfo->pid);
		}
	};

	ChildProcessManager::CreateArg arg;
	arg.args = profile.args;
	arg.envs = profile.envs;
	arg.workingDirectory = profile.workingDirectory;
	arg.eventCb = new EventCb(profile, arg);
	return ChildProcessManager::getInstance()->create(arg);
}

void ActorCollector::addActor(ActorInfo *actorInfo)
{
	PrivateContext::Locker locker;
	pair<WaitChildSetIterator, bool> result =
	  PrivateContext::waitChildSet.insert
	    (pair<pid_t, ActorInfo *>(actorInfo->pid, actorInfo));
	if (!result.second) {
		MLPL_BUG("pid: %d (logId: %" PRIu64 ") is already regstered.\n",
		         actorInfo->pid, actorInfo->logId);
		return;
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool ActorCollector::isWatching(pid_t pid)
{
	bool found = false;
	PrivateContext::lock.lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end())
		found = true;
	PrivateContext::lock.unlock();
	return found;
}

void ActorCollector::setDontLog(pid_t pid)
{
	bool found = false;
	PrivateContext::lock.lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end()) {
		it->second->dontLog = true;
		found = true;
	}
	PrivateContext::lock.unlock();
	if (!found)
		MLPL_WARN("Not found pid: %d for setDontLog().\n", pid);
}

size_t ActorCollector::getNumberOfWaitingActors(void)
{
	PrivateContext::lock.lock();
	size_t num = PrivateContext::waitChildSet.size();
	PrivateContext::lock.unlock();
	return num;
}

void ActorCollector::notifyChildSiginfo(
  const siginfo_t *info, ActorContext &actorCtx)
{
	DBClientAction::LogEndExecActionArg &logArg = actorCtx.logArg;

	ChildSigInfo childSigInfo;
	childSigInfo.pid      = info->si_pid;
	childSigInfo.code     = info->si_code;
	childSigInfo.status   = info->si_status;

	// check the reason of the signal.
	if (childSigInfo.code == CLD_EXITED) {
		logArg.status = ACTLOG_STAT_SUCCEEDED;
		// failureCode is set to ACTLOG_EXECFAIL_NONE in the
		// LogEndExecActionArg's constructor.
	} else if (childSigInfo.code == CLD_KILLED) {
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = ACTLOG_EXECFAIL_KILLED_SIGNAL;
	} else if (childSigInfo.code == CLD_DUMPED) {
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = ACTLOG_EXECFAIL_DUMPED_SIGNAL;
	} else {
		// The received-signal candidates are
		// CLD_TRAPPED, CLD_STOPPED, and CLD_CONTINUED.
		MLPL_INFO("Actor: %d, status: %d\n",
		          childSigInfo.pid, childSigInfo.status);
		return;
	}
	logArg.exitCode = childSigInfo.status;

	// execute the callback function
	ActorInfo *actorInfo = actorCtx.actorInfo;
	HATOHOL_ASSERT(actorInfo, "actorInfo is NULL.");
	if (actorInfo->collectedCb)
		(*actorInfo->collectedCb)(actorInfo);
}

void ActorCollector::postCollectedProc(ActorContext &actorCtx)
{
	ActorInfo *actorInfo = actorCtx.actorInfo;
	DBClientAction::LogEndExecActionArg &logArg = actorCtx.logArg;
	// log the action result if needed
	if (!actorInfo->dontLog) {
		DBClientAction dbAction;
		logArg.logId = actorInfo->logId;
		dbAction.logEndExecAction(logArg);
	}

	// execute the callback function without the lock
	if (actorInfo->postCollectedCb)
		(*actorInfo->postCollectedCb)(actorInfo);

	cleanupChildInfo(actorInfo->pid);
}

void ActorCollector::cleanupChildInfo(const pid_t &pid)
{
	// Remove actorInfo from waitChildSet
	PrivateContext::Locker locker;
	WaitChildSetIterator it =
	  PrivateContext::waitChildSet.find(pid);
	ActorInfo *actorInfo = it->second;
	HATOHOL_ASSERT(it != PrivateContext::waitChildSet.end(),
	               "Not found: pid: %d\n", actorInfo->pid);
	PrivateContext::waitChildSet.erase(it);
	
	// ActionManager::commandActionTimeoutCb() may be running on the
	// default GLib event loop. So we delete actorInfo on that.
	Utils::deleteOnGLibEventLoop<ActorInfo>(actorInfo, ASYNC);
}
