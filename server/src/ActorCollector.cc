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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <cstring>
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "Logger.h"
#include "MutexLock.h"

using namespace mlpl;

typedef map<pid_t, ActorInfo>        WaitChildSet;
typedef WaitChildSet::iterator       WaitChildSetIterator;
typedef WaitChildSet::const_iterator WaitChildSetConstIterator;

struct ActorCollector::PrivateContext {
	static bool    initialized;
	static int     pipefd[2];
	static MutexLock lock;
	static WaitChildSet waitChildSet;
};

bool ActorCollector::PrivateContext::initialized = false;
int ActorCollector::PrivateContext::pipefd[2];
MutexLock ActorCollector::PrivateContext::lock;
WaitChildSet ActorCollector::PrivateContext::waitChildSet;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ActorCollector::init(void)
{
	setupHandlerForSIGCHLD();
}

void ActorCollector::reset(void)
{
	// This function is mainly for the test. In the normal use,
	// waitChildSet is of course empty when this function is called
	// at the start-up.
	PrivateContext::waitChildSet.clear();
}

void ActorCollector::stop(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void ActorCollector::lock(void)
{
	PrivateContext::lock.lock();
}

void ActorCollector::unlock(void)
{
	PrivateContext::lock.unlock();
}

void ActorCollector::addActor(const ActorInfo &actorInfo)
{
	// This function is currently called only from
	// ActionManager::execCommandAction(). Because lock() and unlock() are
	// called in it, so they aren't called here.
	pair<WaitChildSetIterator, bool> result =
	  PrivateContext::waitChildSet.insert
	    (pair<pid_t, ActorInfo>(actorInfo.pid, actorInfo));
	if (!result.second) {
		MLPL_BUG("pid: %d (logId: %"PRIu64 ") is already regstered.\n",
		         actorInfo.pid, actorInfo.logId);
		return;
	}
}

ActorCollector::ActorCollector(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActorCollector::~ActorCollector()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActorCollector::setupHandlerForSIGCHLD(void)
{
	// We assume that this function (implictly ActorCollector::init) is
	// never called concurrently.
	if (PrivateContext::initialized)
		return;

	// open pipe
	HATOHOL_ASSERT(pipe(PrivateContext::pipefd) == 0,
	               "Failed to open pipe: errno: %d", errno);

	// set signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = ActorCollector::signalHandlerChild;
	sa.sa_flags |= (SA_RESTART|SA_SIGINFO);
	HATOHOL_ASSERT(sigaction(SIGCHLD, &sa, NULL ) == 0,
	               "Failed to set SIGCHLD, errno: %d\n", errno);
	
	// set glib callback handler for the pipe
	GIOChannel *ioch = g_io_channel_unix_new(PrivateContext::pipefd[0]);
	GError *error = NULL;
	GIOStatus status = g_io_channel_set_encoding(ioch, NULL, &error);
	if (status == G_IO_STATUS_ERROR) {
		THROW_HATOHOL_EXCEPTION("status: G_IO_STATUS_ERROR: %s",
		                        error->message);
		g_error_free(error);
	} else if (status != G_IO_STATUS_NORMAL) {
		THROW_HATOHOL_EXCEPTION("Illegal status: %d", status);
	}
	g_io_add_watch(ioch,
	               (GIOCondition)(G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP),
	               ActorCollector::checkExitProcess, NULL);
	PrivateContext::initialized = true;
}

void ActorCollector::signalHandlerChild(int signo, siginfo_t *info, void *arg)
{
	// We use write() to notify the reception of SIGCHLD.
	// because write() is one of the asynchronus SIGNAL safe functions.
	ChildSigInfo childSigInfo;
	childSigInfo.pid      = info->si_pid;
	childSigInfo.code     = info->si_code;
	childSigInfo.status   = info->si_status;
	ssize_t ret = write(PrivateContext::pipefd[1],
	                    &childSigInfo, sizeof(ChildSigInfo));
	if (ret == -1) {
		// We cannot call printf() and other
		// signal unsafe output function.
		abort();
	}
}

gboolean ActorCollector::checkExitProcess
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	if (condition & G_IO_ERR) {
		THROW_HATOHOL_EXCEPTION("GIO watch: Error\n");
	} else if (condition & G_IO_HUP) {
		THROW_HATOHOL_EXCEPTION("GIO watch: HUP\n");
	}

	GError *error = NULL;
	ChildSigInfo childSigInfo;
	GIOStatus stat;
	gsize bytesRead;
	gsize requestSize = sizeof(childSigInfo);
	gchar *buf = reinterpret_cast<gchar *>(&childSigInfo);
	while (true) {
		stat = g_io_channel_read_chars(source, buf, requestSize,
		                               &bytesRead, &error);
		if (stat == G_IO_STATUS_AGAIN) {
			continue;
		} else if (stat == G_IO_STATUS_EOF) {
			THROW_HATOHOL_EXCEPTION("Unexcepted EOF\n");
		} else if (stat == G_IO_STATUS_ERROR) {
			THROW_HATOHOL_EXCEPTION("ERROR: %s\n",
			                        error->message);
			g_error_free(error);
		} else if (stat != G_IO_STATUS_NORMAL) {
			THROW_HATOHOL_EXCEPTION("Unknown stat: %d\n", stat);
		}

		if (bytesRead >= requestSize)
			break;
		requestSize -= bytesRead;
		buf += bytesRead;
	}

	// check the reason of the signal.
	DBClientAction::LogEndExecActionArg logArg;
	if (childSigInfo.code == CLD_EXITED) {
		logArg.status = DBClientAction::ACTLOG_STAT_SUCCEEDED;
		// failureCode is set to ACTLOG_EXECFAIL_NONE in the
		// LogEndExecActionArg's constructor.
	} else if (childSigInfo.code == CLD_KILLED) {
		logArg.status = DBClientAction::ACTLOG_STAT_FAILED;
		logArg.failureCode =
		  DBClientAction::ACTLOG_EXECFAIL_KILLED_SIGNAL;
	} else if (childSigInfo.code == CLD_DUMPED) {
		logArg.status = DBClientAction::ACTLOG_STAT_FAILED;
		logArg.failureCode =
		  DBClientAction::ACTLOG_EXECFAIL_DUMPED_SIGNAL;
	} else {
		// The received-signal candidates are
		// CLD_TRAPPED, CLD_STOPPED, and CLD_CONTINUED.
		MLPL_INFO("Actor (%d) received a signal: %d\n",
		          childSigInfo.pid, childSigInfo.status);
		return TRUE;
	}
	logArg.exitCode = childSigInfo.status;

	// update the action log.
	bool found = false;
	bool dontLog = false;
	lock();
	WaitChildSetIterator it =
	   PrivateContext::waitChildSet.find(childSigInfo.pid);
	if (it != PrivateContext::waitChildSet.end()) {
		found = true;
		const ActorInfo &actorInfo = it->second;
		logArg.logId = actorInfo.logId;
		dontLog = actorInfo.dontLog;
		if (actorInfo.collectedCb)
			(*actorInfo.collectedCb)(actorInfo.collectedCbPriv);
		PrivateContext::waitChildSet.erase(it);
	}
	unlock();

	if (!found)
		return TRUE;
	if (dontLog)
		return TRUE;

	DBClientAction dbAction;
	dbAction.logEndExecAction(logArg);

	return TRUE;
}

bool ActorCollector::isWatching(pid_t pid)
{
	bool found = false;
	lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end())
		found = true;
	unlock();
	return found;
}

void ActorCollector::setDontLog(pid_t pid)
{
	bool found = false;
	lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end()) {
		it->second.dontLog = true;
		found = true;
	}
	unlock();
	if (!found)
		MLPL_WARN("Not found pid: %d for setDontLog().", pid);
}
