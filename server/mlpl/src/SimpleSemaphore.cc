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

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include "SimpleSemaphore.h"
#include "SmartTime.h"
#include "Logger.h"

using namespace mlpl;

struct SimpleSemaphore::PrivateContext {
	sem_t sem;

	PrivateContext(const int &count)
	{
		init(count);
	}

	virtual ~PrivateContext()
	{
		if (sem_destroy(&sem) == -1) {
			MLPL_CRIT("Failed to call sem_destroy: errno: %d\n",
			          errno);
			abort();
		}
	}

	void init(const int &count)
	{
		int ret = sem_init(&sem, 0 /* pshared */, count);
		if (ret == -1) {
			MLPL_CRIT("Failed to call sem_init: errno: %d\n",
			          errno);
			abort();
		}
	}
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
SimpleSemaphore::SimpleSemaphore(const int &count)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(count);
}

SimpleSemaphore::~SimpleSemaphore()
{
	if (m_ctx)
		delete m_ctx;
}

int SimpleSemaphore::post(void)
{
retry:
	int ret = sem_post(&m_ctx->sem);
	if (ret == -1) {
		if (errno == EINTR)
			goto retry;
		return errno;
	}
	return 0;
}

void SimpleSemaphore::post(SimpleSemaphore *sem)
{
	sem->post();
}

int SimpleSemaphore::wait(void)
{
retry:
	int ret = sem_wait(&m_ctx->sem);
	if (ret == -1) {
		if (errno == EINTR)
			goto retry;
		return errno;
	}
	return 0;
}

SimpleSemaphore::Status SimpleSemaphore::timedWait(size_t timeoutInMSec)
{
	SmartTime abs(SmartTime::INIT_CURR_TIME);
	struct timespec waitTime;
	waitTime.tv_sec = timeoutInMSec / 1000;
	waitTime.tv_nsec = (timeoutInMSec % 1000) * 1000 * 1000;
	abs += waitTime;
retry:
	int ret = sem_timedwait(&m_ctx->sem, &abs.getAsTimespec());
	if (ret == 0)
		return STAT_OK;

	switch (errno) {
	case EINTR:
		goto retry;
	case ETIMEDOUT:
		return STAT_TIMEDOUT;
	}
	return STAT_ERROR_UNKNOWN;
}

void SimpleSemaphore::init(const int &count)
{
	m_ctx->init(count);
}
