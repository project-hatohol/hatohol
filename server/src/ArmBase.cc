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
#include <errno.h>
#include <Logger.h>
#include "ArmBase.h"
#include "HatoholException.h"

using namespace mlpl;

struct ArmBase::PrivateContext
{
	MonitoringServerInfo serverInfo; // we have the copy.
	sem_t          sleepSemaphore;
	volatile int   exitRequest;

	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo),
	  exitRequest(0)
	{
		static const int PSHARED = 1;
		HATOHOL_ASSERT(sem_init(&sleepSemaphore, PSHARED, 0) == 0,
		             "Failed to sem_init(): %d\n", errno);
	}

	virtual ~PrivateContext()
	{
		if (sem_destroy(&sleepSemaphore) != 0)
			MLPL_ERR("Failed to call sem_destroy(): %d\n", errno);
	}

};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmBase::ArmBase(const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

ArmBase::~ArmBase()
{
	if (m_ctx)
		delete m_ctx;
}

void ArmBase::forceUpdate(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	// TODO: wake the sleeping thread and wait one proc.
}

void ArmBase::setPollingInterval(int sec)
{
	m_ctx->serverInfo.pollingIntervalSec = sec;
}

int ArmBase::getPollingInterval(void) const
{
	return m_ctx->serverInfo.pollingIntervalSec;
}

int ArmBase::getRetryInterval(void) const
{
	return m_ctx->serverInfo.retryIntervalSec;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool ArmBase::hasExitRequest(void) const
{
	return g_atomic_int_get(&m_ctx->exitRequest);
}

void ArmBase::requestExit(void)
{
	// to return immediately from the waiting.
	if (sem_post(&m_ctx->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
	g_atomic_int_set(&m_ctx->exitRequest, 1);
}

const MonitoringServerInfo &ArmBase::getServerInfo(void) const
{
	return m_ctx->serverInfo;
}


void ArmBase::sleepInterruptible(int sleepTime)
{
	// sleep with timeout
	timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		sleep(10); // to avoid burnup
	}
	ts.tv_sec += sleepTime;
	int result = sem_timedwait(&m_ctx->sleepSemaphore, &ts);
	if (result == -1) {
		if (errno == ETIMEDOUT)
			; // This is normal case
		else if (errno == EINTR)
			; // In this case, we also do nothing
	}
	// The up of the semaphore is done only from the destructor.
}

gpointer ArmBase::mainThread(HatoholThreadArg *arg)
{
	while (!hasExitRequest()) {
		int sleepTime = getPollingInterval();
		if (!mainThreadOneProc())
			sleepTime = getRetryInterval();
		if (hasExitRequest())
			break;
		sleepInterruptible(sleepTime);
	}
	return NULL;
}
