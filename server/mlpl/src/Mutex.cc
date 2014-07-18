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

#include <errno.h>
#include "Mutex.h"
#include "SmartTime.h"
#include "Logger.h"

using namespace mlpl;

struct Mutex::PrivateContext {
	pthread_mutex_t lock;

	PrivateContext(void)
	{
		pthread_mutex_init(&lock, NULL);
	}

	~PrivateContext()
	{
		pthread_mutex_destroy(&lock);
	}
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
Mutex::Mutex(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

Mutex::~Mutex()
{
	if (m_ctx)
		delete m_ctx;
}

void Mutex::lock(void)
{
	pthread_mutex_lock(&m_ctx->lock);
}

Mutex::Status Mutex::timedlock(size_t timeoutInMSec)
{
	SmartTime abs(SmartTime::INIT_CURR_TIME);
	struct timespec waitTime;
	waitTime.tv_sec = timeoutInMSec / 1000;
	waitTime.tv_nsec = (timeoutInMSec % 1000) * 1000 * 1000;
	abs += waitTime;
	int err = pthread_mutex_timedlock(&m_ctx->lock, &abs.getAsTimespec());
	switch (err) {
	case 0:
		return STAT_OK;
	case ETIMEDOUT:
		return STAT_TIMEDOUT;
	default:
		MLPL_ERR("Failed to clock_gettime: errno: %d\n", errno);
		return STAT_ERROR_UNKNOWN;
	}
	return STAT_OK;
}

void Mutex::unlock(void)
{
	pthread_mutex_unlock(&m_ctx->lock);
}

bool Mutex::trylock(void)
{
	return pthread_mutex_trylock(&m_ctx->lock) != EBUSY;
}

void Mutex::unlock(Mutex *lock)
{
	pthread_mutex_unlock(&lock->m_ctx->lock);
}

// ----------------------------------------------------------------------------
// AutoMutex
// ----------------------------------------------------------------------------
AutoMutex::AutoMutex(Mutex *mutex)
: m_mutex(mutex)
{
	m_mutex->lock();
}

AutoMutex::~AutoMutex()
{
	m_mutex->unlock();
}
