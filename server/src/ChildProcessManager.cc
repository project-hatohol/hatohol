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
#include "ChildProcessManager.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

struct ChildInfo {
	pid_t pid;
};

typedef map<pid_t, ChildInfo>    ChildMap;
typedef ChildMap::iterator       ChildMapIterator;
typedef ChildMap::const_iterator ChildMapConstIterator;

struct ChildProcessManager::PrivateContext {
	static ChildProcessManager *instance;
	static ReadWriteLock        instanceLock;

	sem_t         waitChildSem;
	ReadWriteLock childrenMapLock;
	ChildMap      childrenMap;

	PrivateContext(void)
	{
		HATOHOL_ASSERT(sem_init(&waitChildSem, 0, 0) == 0,
		               "Failed to call sem_init(): %d\n", errno);
	}

	virtual ~PrivateContext(void)
	{
		// TODO: wait all children
	}
};

ChildProcessManager *ChildProcessManager::PrivateContext::instance = NULL;
ReadWriteLock        ChildProcessManager::PrivateContext::instanceLock;

// ---------------------------------------------------------------------------
// CreateArg
// ---------------------------------------------------------------------------
ChildProcessManager::CreateArg::CreateArg(void)
: flags((GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD),
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
	if (!PrivateContext::instance)
		PrivateContext::instance = new ChildProcessManager();
	PrivateContext::instanceLock.unlock();
	return PrivateContext::instance;
}

HatoholError ChildProcessManager::create(CreateArg &arg)
{
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

	gboolean succeeded =
	  g_spawn_async(arg.workingDirectory.c_str(),
	                (gchar **)argv, (gchar **)envp,
	                arg.flags, childSetup, userData, &arg.pid, &error);
	if (!succeeded) {
		string reason = "<Unknown reason>";
		if (error) {
			reason = error->message;
			g_error_free(error);
		}
		MLPL_ERR("Failed to create process: (%s), %s\n",
		         argv[0], reason.c_str());
		return HatoholError(HTERR_FAILED_TO_SPAWN, reason);
	}

	ChildInfo childInfo;
	childInfo.pid = arg.pid;

	m_ctx->childrenMapLock.writeLock();
	pair<ChildMapIterator, bool> result =
	  m_ctx->childrenMap.insert(pair<pid_t, ChildInfo>(arg.pid, childInfo));
	m_ctx->childrenMapLock.unlock();
	if (!result.second) {
		// TODO: Recovery
		MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	}

	return HTERR_OK;
}

gpointer ChildProcessManager::mainThread(HatoholThreadArg *arg)
{
	MLPL_BUG("Not implemented yet: %s\n", __PRETTY_FUNCTION__);
	return NULL;
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
