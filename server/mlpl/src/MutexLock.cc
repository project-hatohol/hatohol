/*
 * Copyright (C) 2013 Hatohol Project
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
#include "MutexLock.h"

using namespace mlpl;

struct MutexLock::PrivateContext {
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
MutexLock::MutexLock(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

MutexLock::~MutexLock()
{
	if (m_ctx)
		delete m_ctx;
}

void MutexLock::lock(void)
{
	pthread_mutex_lock(&m_ctx->lock);
}

void MutexLock::unlock(void)
{
	pthread_mutex_unlock(&m_ctx->lock);
}

bool MutexLock::trylock(void)
{
	return pthread_mutex_trylock(&m_ctx->lock) != EBUSY;
}
