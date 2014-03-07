/*
 * Copyright (C) 2013-2014 Project Hatohol
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
#include <AtomicValue.h>
#include "ArmBase.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

struct ArmBase::PrivateContext
{
	string               name;
	MonitoringServerInfo serverInfo; // we have the copy.
	timespec             lastPollingTime;
	sem_t                sleepSemaphore;
	AtomicValue<bool>    exitRequest;
	ArmBase::UpdateType  updateType;
	bool                 isCopyOnDemandEnabled;
	ReadWriteLock        rwlock;
	ArmStatus            armStatus;

	PrivateContext(const string &_name,
	               const MonitoringServerInfo &_serverInfo)
	: name(_name),
	  serverInfo(_serverInfo),
	  exitRequest(false),
	  updateType(UPDATE_POLLING),
	  isCopyOnDemandEnabled(false)
	{
		static const int PSHARED = 1;
		HATOHOL_ASSERT(sem_init(&sleepSemaphore, PSHARED, 0) == 0,
		             "Failed to sem_init(): %d\n", errno);
		lastPollingTime.tv_sec = 0;
		lastPollingTime.tv_nsec = 0;
	}

	virtual ~PrivateContext()
	{
		if (sem_destroy(&sleepSemaphore) != 0)
			MLPL_ERR("Failed to call sem_destroy(): %d\n", errno);
	}

	void stampLastPollingTime(void)
	{
		if (getUpdateType() != UPDATE_POLLING)
			return;

		int result = clock_gettime(CLOCK_REALTIME,
					   &lastPollingTime);
		if (result == 0) {
			MLPL_DBG("lastPollingTime: %ld\n",
				 lastPollingTime.tv_sec);
		} else {
			MLPL_ERR("Failed to call clock_gettime: %d\n",
				 errno);
			lastPollingTime.tv_sec = 0;
			lastPollingTime.tv_nsec = 0;
		}
	}

	int getSecondsToNextPolling(void)
	{
		time_t interval = serverInfo.pollingIntervalSec;
		timespec currentTime;

		int result = clock_gettime(CLOCK_REALTIME, &currentTime);
		if (result == 0 && lastPollingTime.tv_sec != 0) {
			time_t elapsed
			  = currentTime.tv_sec - lastPollingTime.tv_sec;
			if (elapsed < interval)
				interval -= elapsed;
		}

		if (interval <= 0 || interval > serverInfo.pollingIntervalSec)
			interval = serverInfo.pollingIntervalSec;

		return interval;
	}

	UpdateType getUpdateType(void)
	{
		rwlock.readLock();
		ArmBase::UpdateType type = updateType;
		rwlock.unlock();
		return type;
	}

	void setUpdateType(ArmBase::UpdateType type)
	{
		rwlock.writeLock();
		updateType = type;
		rwlock.unlock();
	}

	Signal updatedSignal;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmBase::ArmBase(
  const string &name, const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(name, serverInfo);
}

ArmBase::~ArmBase()
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("%s [%d:%s]: destruction: completed.\n",
	          getName().c_str(), svInfo.id, svInfo.hostName.c_str());
	if (m_ctx)
		delete m_ctx;
}

void ArmBase::start(void)
{
	HatoholThreadBase::start();
	m_ctx->armStatus.setRunningStatus(true);
}

void ArmBase::waitExit(void)
{
	HatoholThreadBase::waitExit();
	m_ctx->armStatus.setRunningStatus(false);
}

void ArmBase::fetchItems(ClosureBase *closure)
{
	setUpdateType(UPDATE_ITEM_REQUEST);
	m_ctx->updatedSignal.connect(closure);
	if (sem_post(&m_ctx->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
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

const string &ArmBase::getName(void) const
{
	return m_ctx->name;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ArmBase::requestExitAndWait(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	
	MLPL_INFO("%s [%d:%s]: requested to exit.\n",
	          getName().c_str(), svInfo.id, svInfo.hostName.c_str());

	// wait for the finish of the thread
	requestExit();
	waitExit();
}

bool ArmBase::hasExitRequest(void) const
{
	return m_ctx->exitRequest;
}

void ArmBase::requestExit(void)
{
	m_ctx->exitRequest = true;

	// to return immediately from the waiting.
	if (sem_post(&m_ctx->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
}

const MonitoringServerInfo &ArmBase::getServerInfo(void) const
{
	return m_ctx->serverInfo;
}

const ArmStatus &ArmBase::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

void ArmBase::sleepInterruptible(int sleepTime)
{
	// sleep with timeout
	timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		sleep(10); // to avoid burnup
		return;
	}
	ts.tv_sec += sleepTime;
retry:
	int result = sem_timedwait(&m_ctx->sleepSemaphore, &ts);
	if (result == -1) {
		if (errno == ETIMEDOUT)
			; // This is normal case
		else if (errno == EINTR)
			goto retry;
		else
			MLPL_ERR("sem_timedwait(): errno: %d\n", errno);
	}
	// The up of the semaphore is done only from the destructor.
}

ArmBase::UpdateType ArmBase::getUpdateType(void) const
{
	return m_ctx->getUpdateType();
}

void ArmBase::setUpdateType(UpdateType updateType)
{
	m_ctx->setUpdateType(updateType);
}

bool ArmBase::getCopyOnDemandEnabled(void) const
{
	return m_ctx->isCopyOnDemandEnabled;
}

void ArmBase::setCopyOnDemandEnabled(bool enable)
{
	m_ctx->isCopyOnDemandEnabled = enable;
}

gpointer ArmBase::mainThread(HatoholThreadArg *arg)
{
	while (!hasExitRequest()) {
		int sleepTime = m_ctx->getSecondsToNextPolling();
		if (!mainThreadOneProc())
			sleepTime = getRetryInterval();

		m_ctx->stampLastPollingTime();
		m_ctx->setUpdateType(UPDATE_POLLING);
		m_ctx->updatedSignal();
		m_ctx->updatedSignal.clear();

		if (hasExitRequest())
			break;
		sleepInterruptible(sleepTime);
	}
	return NULL;
}

ArmStatus &ArmBase::getNonConstArmStatus(void)
{
	return m_ctx->armStatus;
}
