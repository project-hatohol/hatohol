/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <Mutex.h>
#include <SimpleSemaphore.h>
#include <Reaper.h>
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

	/**
	 * Return whether the queue is empty or not.
	 *
	 * @return true if the queue is empty.
	 */
	bool empty(void) const
	{
		m_mutex.lock();
		bool empty = m_queue.empty();
		m_mutex.unlock();
		return empty;
	}

	/**
	 * Return the number of elements in the queue.
	 *
	 * @return the number of elements.
	 */
	size_t size(void) const
	{
		m_mutex.lock();
		size_t sz = m_queue.size();
		m_mutex.unlock();
		return sz;
	}

	/**
	 * Return the reference of the first element.
	 *
	 * NOTE: Don't do operatons that changes the queue push() and pop()
	 *       during the reference is used.
	 *
	 * @return the reference to the first element.
	 */
	const T &front(void) const
	{
		return  m_queue.front();
	}

	/**
	 * Pop an element and call the specified function with it for
	 * all elements with in a lock.
	 *
	 * @tparam PrivType
	 * A type name for the second argument of the callback function.
	 *
	 * @param func A callback function.
	 * @param priv An arbitary pointer passed to the callback function.
	 *
	 */
	template <typename PrivType>
	void popAll(void (*func)(T elem, PrivType), PrivType priv)
	{
		mlpl::Reaper<mlpl::Mutex>
		  unlocker(&m_mutex, mlpl::Mutex::unlock);
		m_mutex.lock();
		while (!m_queue.empty()) {
			m_sem.wait();
			(*func)(m_queue.front(), priv);
			m_queue.pop_front();
		}
	}

	/**
	 * Pop an element only if there's an element.
	 *
	 * @param dest The popped value is stored to this variable.
	 * @return true if an element is popped. Otherwise false.
	 */
	bool popIfNonEmpty(T &dest)
	{
		bool exist = false;
		m_mutex.lock();
		if (!m_queue.empty()) {
			exist = true;
			m_sem.wait();
			dest = m_queue.front();
			m_queue.pop_front();
		}
		m_mutex.unlock();
		return exist;
	}

protected:

private:
	mutable mlpl::Mutex   m_mutex;
	mlpl::SimpleSemaphore m_sem;
	std::deque<T>         m_queue;
};

} // namespace mlpl

