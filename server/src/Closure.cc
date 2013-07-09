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

#include "Closure.h"

Signal::Signal(void)
{
}

Signal::~Signal (void)
{
}

void Signal::connect (ClosureBase *closure)
{
	m_rwlock.writeLock();
	m_closures.push_back(closure);
	m_rwlock.unlock();
}

void Signal::disconnect (ClosureBase *closure)
{
	ClosureBaseIterator it;
	m_rwlock.writeLock();
	for (it = m_closures.begin(); it != m_closures.end();) {
		if (*closure == *(*it))
			it = m_closures.erase(it);
		else
			it++;
	}
	m_rwlock.unlock();
}

void Signal::clear (void)
{
	m_rwlock.writeLock();
	m_closures.clear();
	m_rwlock.unlock();
}

void Signal::operator() (void)
{
	m_rwlock.readLock();
	ClosureBaseIterator it;
	for (it = m_closures.begin(); it != m_closures.end(); it++) {
		ClosureBase *closure = *it;
		(*closure)();
	}
	m_rwlock.unlock();
}
