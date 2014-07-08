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

#ifndef SmartQueue_h
#define SmartQueue_h

#include <MutexLock.h>
#include <SimpleSemaphore.h>
#include <deque>

namespace mlpl {

template<typename T>
class SmartQueue {
public:
	SmartQueue(void)
	{
	}

	virtual ~SmartQueue()
	{
	}

	void push(T elem)
	{
		m_mutex.lock();
		m_queue.push_back(elem);
		m_mutex.unlock();
		m_sem.post();
	}

	T pop(void)
	{
		m_sem.wait();
		m_mutex.lock();
		T elem = m_queue.front();
		m_queue.pop_front();
		m_mutex.unlock();
		return elem;
	}

protected:

private:
	mlpl::MutexLock       m_mutex;
	mlpl::SimpleSemaphore m_sem;
	std::deque<T>         m_queue;
};

} // namespace mlpl

#endif // SmartQueue_h
