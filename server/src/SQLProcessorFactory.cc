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

#include "SQLProcessorFactory.h"
#include "SQLProcessorZabbix.h"

#ifdef GLIB_VERSION_2_32
GRWLock SQLProcessorFactory::m_lock;
static void reader_lock(GRWLock *lock)
{
	g_rw_lock_reader_lock(lock);
}

static void reader_unlock(GRWLock *lock)
{
	reader_lock(lock);
}

static void writer_lock(GRWLock *lock)
{
	g_rw_lock_writer_lock(lock);
}

static void writer_unlock(GRWLock *lock)
{
	writer_lock(lock);
}

#else
GStaticMutex SQLProcessorFactory::m_lock = G_STATIC_MUTEX_INIT;
static void reader_lock(GStaticMutex *lock)
{
	g_static_mutex_lock(lock);
}

static void reader_unlock(GStaticMutex *lock)
{
	g_static_mutex_unlock(lock);
}

static void writer_lock(GStaticMutex *lock)
{
	g_static_mutex_lock(lock);
}

static void writer_unlock(GStaticMutex *lock)
{
	g_static_mutex_unlock(lock);
}

#endif // GLIB_VERSION_2_32
map<string, SQLProcessorCreatorFunc> SQLProcessorFactory::m_factoryMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessorFactory::init(void)
{
	addFactory(SQLProcessorZabbix::getDBNameStatic(),
	           SQLProcessorZabbix::createInstance);
}

SQLProcessor *SQLProcessorFactory::create(string &DBName)
{
	map<string, SQLProcessorCreatorFunc>::iterator it;
	SQLProcessor *composer = NULL;

	reader_lock(&m_lock);
	it = m_factoryMap.find(DBName);
	if (it != m_factoryMap.end()) {
		SQLProcessorCreatorFunc creator = it->second;
		composer = (*creator)();
	}
	reader_unlock(&m_lock);

	return composer;
}

void SQLProcessorFactory::addFactory(string name, SQLProcessorCreatorFunc factory)
{
	writer_lock(&m_lock);
	m_factoryMap[name] = factory;
	writer_unlock(&m_lock);
}
