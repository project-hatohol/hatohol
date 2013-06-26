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

#include "SmartQueue.h"
#include "Logger.h"
using namespace mlpl;

#include "errno.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SmartQueue::SmartQueue(void)
{
	pthread_mutex_init(&m_mutex, NULL);
	sem_init(&m_sem, 0, 0);
}

SmartQueue::~SmartQueue()
{
	sem_destroy(&m_sem);
}

void SmartQueue::push(SmartQueueElement *elem)
{
	pthread_mutex_lock(&m_mutex);
	m_queue.push_back(elem);
	pthread_mutex_unlock(&m_mutex);
	sem_post(&m_sem);
}

SmartQueueElement *SmartQueue::pop(void)
{
	sem_wait(&m_sem);
	pthread_mutex_lock(&m_mutex);
	SmartQueueElement *elem = m_queue.front();
	m_queue.pop_front();
	pthread_mutex_unlock(&m_mutex);
	return elem;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void SmartQueue::sem_wait_sig_retry(sem_t *sem)
{
	while (true) {
		int ret = sem_wait(sem);
		if (ret == 0)
			break;
		if (ret == -1 && errno == EINTR)
			continue;
		// TODO: add code when error
	}
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
