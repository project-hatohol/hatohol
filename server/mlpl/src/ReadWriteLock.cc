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

#include "ReadWriteLock.h"

using namespace mlpl;

struct ReadWriteLock::PrivateContext {
	pthread_rwlock_t rwlock;

	PrivateContext(void)
	{
		pthread_rwlock_init(&rwlock, NULL);
	}

	~PrivateContext()
	{
		pthread_rwlock_destroy(&rwlock);
	}
};

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
ReadWriteLock::ReadWriteLock(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ReadWriteLock::~ReadWriteLock()
{
	if (m_ctx)
		delete m_ctx;
}

void ReadWriteLock::readLock(void)
{
	pthread_rwlock_rdlock(&m_ctx->rwlock);
}

void ReadWriteLock::writeLock(void)
{
	pthread_rwlock_wrlock(&m_ctx->rwlock);
}

void ReadWriteLock::unlock(void)
{
	pthread_rwlock_unlock(&m_ctx->rwlock);
}
