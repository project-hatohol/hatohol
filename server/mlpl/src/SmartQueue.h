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

#ifndef SmartQueue_h
#define SmartQueue_h

#include <deque>
#include <pthread.h>
#include <semaphore.h>

namespace mlpl {

class SmartQueueElement {
};

class SmartQueue {
public:
	SmartQueue(void);
	virtual ~SmartQueue();
	void push(SmartQueueElement *elem);
	SmartQueueElement *pop(void);

protected:
	void sem_wait_sig_retry(sem_t *sem);

private:
	pthread_mutex_t m_mutex;
	sem_t m_sem;
	std::deque<SmartQueueElement *> m_queue;
};

} // namespace mlpl

#endif // SmartQueue_h
