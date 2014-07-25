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

#include "Synchronizer.h"
using namespace mlpl;

Synchronizer::Synchronizer(void)
: m_mutex(NULL)
{
	m_mutex = new Mutex();
}

Synchronizer::~Synchronizer()
{
	delete m_mutex;
}

void Synchronizer::lock(void)
{
	m_mutex->lock();
}

void Synchronizer::unlock(void)
{
	m_mutex->unlock();
}

void Synchronizer::reset(void)
{
	delete m_mutex;
	m_mutex = new Mutex();
}

bool Synchronizer::trylock(void)
{
	return m_mutex->trylock();
}

bool Synchronizer::isLocked(void)
{
	bool locked = false;
	if (trylock())
		unlock();
	else
		locked = true;
	return locked;
}

void Synchronizer::wait(void)
{
	lock();
	unlock();
}

