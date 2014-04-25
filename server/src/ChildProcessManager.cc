/*
 * Copyright (C) 2014 Project Hatohol
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

#include <map>
#include <semaphore.h>
#include <errno.h>
#include <Logger.h>
#include <AtomicValue.h>
#include "ChildProcessManager.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

struct ChildInfo {
	pid_t pid;
	ChildProcessManager::EventCallback *eventCb;

	ChildInfo(void)
	: pid(0),
	  eventCb(NULL)
	{
	}
};

typedef map<pid_t, ChildInfo *>  ChildMap;
typedef ChildMap::iterator       ChildMapIterator;
typedef ChildMap::const_iterator ChildMapConstIterator;

struct ChildProcessManager::PrivateContext {
	static ChildProcessManager *instance;
	static ReadWriteLock        instanceLock;

	AtomicValue<bool> resetRequest;
	sem_t             resetSem;

	sem_t         waitChildSem;
	ReadWriteLock childrenMapLock;
	ChildMap      childrenMap;

	PrivateContext(void)
	: resetRequest(false)
	{
		HATOHOL_ASSERT(sem_init(&resetSem, 0, 0) == 0,
		               "Failed to call sem_init(): %d\n", errno);
		HATOHOL_ASSERT(sem_init(&waitChildSem, 0, 0) == 0,
		               "Failed to call sem_init(): %d\n", errno);
	}

	virtual ~PrivateContext(void)
	{
		// TODO: wait all children
	}

	void resetOnCollectThread(void)
	{
		// We assume this function is called only from a test.
		// So we don't use childrenMapLock.
		while (!childrenMap.empty()) {
			ChildMapIterator childInfoItr = childrenMap.begin();
			const pid_t pid = childInfoItr->first;
			ChildInfo *childInfo = childInfoItr->second;
			childrenMap.erase(childInfoItr);
			int ret = kill(pid, SIGKILL);
			if (ret == -1 && errno == ESRCH) {
				MLPL_INFO("No process w/ pid: %d\n", pid);
				continue;
			}
			HATOHOL_ASSERT(
			  ret == 0, "Failed to send kill (%d): %d\n",
			  pid, errno);
			delete childInfo;
		}
		resetRequest = false;
		HATOHOL_ASSERT(sem_init(&waitChildSem, 0, 0) == 0,
		               "Failed to call sem_init(): %d\n", errno);
		sem_post(&resetSem);
	}
};

ChildProcessManager *ChildProcessManager::PrivateContext::instance = NULL;
ReadWriteLock        ChildProcessManager::PrivateContext::instanceLock;

// ---------------------------------------------------------------------------
// CreateArg
// ---------------------------------------------------------------------------
ChildProcessManager::CreateArg::CreateArg(void)
: flags((GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD),
  eventCb(NULL),
  pid(0)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ChildProcessManager *ChildProcessManager::getInstance(void)
{
	PrivateContext::instanceLock.readLock();
	bool hasInstance = PrivateContext::instance;
	PrivateContext::instanceLock.unlock();
	if (hasInstance)
		return PrivateContext::instance;

	PrivateContext::instanceLock.writeLock();
	if (!PrivateContext::instance) {
		PrivateContext::instance = new ChildProcessManager();
		PrivateContext::instance->start();
	}
	PrivateContext::instanceLock.unlock();
	return PrivateContext::instance;
}

void ChildProcessManager::reset(void)
{
	m_ctx->resetRequest = true;
	int ret = sem_post(&m_ctx->waitChildSem);
	HATOHOL_ASSERT(ret == 0, "sem_post() failed: %d", errno);
	while (true) {
		if (sem_wait(&m_ctx->resetSem) == -1) {
			if (errno == EINTR)
				continue;
		}
		break;
	}
}

HatoholError ChildProcessManager::create(CreateArg &arg)
{
	const gchar *workingDir =
	  arg.workingDirectory.empty() ? NULL : arg.workingDirectory.c_str();
	const gchar **envp = NULL;
	const GSpawnChildSetupFunc childSetup = NULL;
	const gpointer userData = NULL;
	GError *error = NULL;

	const size_t numArgs = arg.args.size();
	if (numArgs == 0)
		return HTERR_INVALID_ARGS;
	const gchar *argv[numArgs+1];
	for (size_t i = 0; i < numArgs; i++)
		argv[i] = arg.args[i].c_str();
	argv[numArgs] = NULL;

	m_ctx->childrenMapLock.writeLock();
	gboolean succeeded =
	  g_spawn_async(workingDir, (gchar **)argv, (gchar **)envp,
	                arg.flags, childSetup, userData, &arg.pid, &error);
	if (!succeeded) {
		m_ctx->childrenMapLock.unlock();
		string reason = "<Unknown reason>";
		if (error) {
			reason = error->message;
			g_error_free(error);
		}
		MLPL_ERR("Failed to create process: (%s), %s\n",
		         argv[0], reason.c_str());
		return HatoholError(HTERR_FAILED_TO_SPAWN, reason);
	}

	ChildInfo *childInfo = new ChildInfo();
	childInfo->pid = arg.pid;
	childInfo->eventCb = arg.eventCb;

	pair<ChildMapIterator, bool> result =
	  m_ctx->childrenMap.insert(pair<
	    pid_t, ChildInfo *>(arg.pid, childInfo));
	m_ctx->childrenMapLock.unlock();
	if (!result.second) {
		// TODO: Recovery
		HATOHOL_ASSERT(true,
		  "The previous data might still remain: %d\n", arg.pid);
	}
	if (sem_post(&m_ctx->waitChildSem) == -1) {
		HATOHOL_ASSERT(true,
		  "Failed to post semaphore: %d\n", errno);
	}

	return HTERR_OK;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ChildProcessManager::ChildProcessManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ChildProcessManager::~ChildProcessManager()
{
	if (m_ctx)
		delete m_ctx;
}

gpointer ChildProcessManager::mainThread(HatoholThreadArg *arg)
{
	MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	while (true) {
		int ret = sem_wait(&m_ctx->waitChildSem);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			HATOHOL_ASSERT(
			  true, "Failed to sem_wait(): %d\n", errno);
		}
		
		if (m_ctx->resetRequest) {
			m_ctx->resetOnCollectThread();
			continue;
		}

		siginfo_t siginfo;
		while (true) {
			ret = waitid(P_ALL, 0, &siginfo, WEXITED);
			if (ret == -1 && errno == EINTR)
				continue;
			break;
		}
		if (ret == -1) {
			MLPL_ERR("Failed to call wait_id: %d\n", errno);
			continue;
		}
		collected(&siginfo);
	}
	return NULL;
}

void ChildProcessManager::collected(const siginfo_t *siginfo)
{
	ChildInfo *childInfo = NULL;

	m_ctx->childrenMapLock.writeLock();
	ChildMapIterator it = m_ctx->childrenMap.find(siginfo->si_pid);
	if (it != m_ctx->childrenMap.end()) {
		childInfo = it->second;
		m_ctx->childrenMap.erase(it);
	}
	m_ctx->childrenMapLock.unlock();

	if (!childInfo) {
		MLPL_INFO("Collected unwatched child: %d\n", siginfo->si_pid);
		return;
	}

	if (childInfo->eventCb)
		childInfo->eventCb->onCollected(siginfo);
	delete childInfo;
}
