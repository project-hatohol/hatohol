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

#include <map>
#include <list>
#include <Mutex.h>
#include "Params.h"
#include "ThreadLocalDBCache.h"
using namespace std;
using namespace mlpl;

// TODO:
// The default max connections of MySQL is 151 according to
// the following MySQL command.
//   > show global variables like 'max_connections';
// We chose the default maximum value that doesn't exceed
// the above value.
// However, it naturally be changed by user setting. So this parameter
// should also be configurable.
static const size_t DEFAULT_MAX_NUM_CACHE_MYSQL = 100;

// A DBAgentSQLite3 instance keeps to open a databa file (i.e.
// use a file descriptor). So we cannot have the instances unlimitedly.
static const size_t DEFAULT_MAX_NUM_CACHE_SQLITE3 = 100;

typedef list<DB *>         DBList;
typedef DBList::iterator   DBListIterator;

typedef map<DB *, DBListIterator> DBListItrMap;
typedef DBListItrMap::iterator    DBListItrMapIterator;

struct ThreadLocalDBCacheLRU {
	DBList       list;
	DBListItrMap map;

	void touch(DB *db)
	{
		DBListItrMapIterator it = map.find(db);
		if (it != map.end())
			list.erase(it->second);
		map[db] = list.insert(list.begin(), db);
	}

	void erase(DB *db)
	{
		DBListItrMapIterator it = map.find(db);
		HATOHOL_ASSERT(it != map.end(), "Not found. db: %p", db);
		list.erase(it->second);
		map.erase(it);
	}
};

struct ThreadContext {
	DBHatohol   *dbHatohol;

	ThreadContext(void)
	: dbHatohol(NULL)
	{
	}

	virtual ~ThreadContext()
	{
		delete dbHatohol;
	}
};

typedef set<ThreadContext *>       ThreadContextSet;
typedef ThreadContextSet::iterator ThreadContextSetIterator;

struct ThreadLocalDBCache::Impl {
	// TODO: The limitation of cahed objects is now WIP.
	static size_t maxNumCacheMySQL;
	static size_t maxNumCacheSQLite3;

	static Mutex                   lock;
	static ThreadContextSet        ctxSet;
	static ThreadLocalDBCacheLRU   dbCacheLRU;
	static __thread ThreadContext *ctx;

	static ThreadContext *getContext(void)
	{
		AutoMutex autoMutex(&lock);
		if (!ctx) {
			ctx = new ThreadContext();
			ctxSet.insert(ctx);
		}
		return ctx;
	}

	static void reset(void)
	{
		// This only free the DBClientMap instance of the current
		// thread. For the other, cleanup() should be called from
		// HatoholThreadBase::threadCleanup(). This implies this
		// method is called from the main thread that is not based
		// on HatoholThreadBase.
		//
		// NOTE: We don't free all elements in dbClientMapSet here.
		// because some threads (such as ActorCollector) don't exit
		// at reset().
		cleanup();
	}

	static void insert(DB *db)
	{
		AutoMutex autoMutex(&lock);
		dbCacheLRU.touch(db);
	}

	static void cleanup(void)
	{
		if (!ctx)
			return;
		AutoMutex autoMutex(&lock);
		ThreadContextSetIterator it = ctxSet.find(ctx);
		const bool found = (it != ctxSet.end());
		HATOHOL_ASSERT(found, "Failed to found: ctx: %p\n", ctx);
		ctxSet.erase(it);
		dbCacheLRU.erase(ctx->dbHatohol);
		delete ctx;
		ctx = NULL;
	}
};

size_t ThreadLocalDBCache::Impl::maxNumCacheMySQL
  = DEFAULT_MAX_NUM_CACHE_MYSQL;
size_t ThreadLocalDBCache::Impl::maxNumCacheSQLite3
  = DEFAULT_MAX_NUM_CACHE_SQLITE3;

Mutex                   ThreadLocalDBCache::Impl::lock;
ThreadContextSet        ThreadLocalDBCache::Impl::ctxSet;
ThreadLocalDBCacheLRU              ThreadLocalDBCache::Impl::dbCacheLRU;
__thread ThreadContext *ThreadLocalDBCache::Impl::ctx = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ThreadLocalDBCache::reset(void)
{
	Impl::reset();
}

void ThreadLocalDBCache::cleanup(void)
{
	Impl::cleanup();
}

size_t ThreadLocalDBCache::getNumberOfDBClientMaps(void)
{
	AutoMutex autoMutex(&Impl::lock);
	return Impl::dbCacheLRU.list.size();
}

ThreadLocalDBCache::ThreadLocalDBCache(void)
{
}

ThreadLocalDBCache::~ThreadLocalDBCache()
{
}

DBHatohol &ThreadLocalDBCache::getDBHatohol(void)
{
	ThreadContext *ctx = Impl::getContext();
	if (!ctx->dbHatohol)
		ctx->dbHatohol = new DBHatohol();
	Impl::insert(ctx->dbHatohol);
	return *ctx->dbHatohol;
}

DBTablesConfig &ThreadLocalDBCache::getConfig(void)
{
	return getDBHatohol().getDBTablesConfig();
}

DBTablesUser &ThreadLocalDBCache::getUser(void)
{
	return getDBHatohol().getDBTablesUser();
}

DBTablesAction &ThreadLocalDBCache::getAction(void)
{
	return getDBHatohol().getDBTablesAction();
}

DBTablesMonitoring &ThreadLocalDBCache::getMonitoring(void)
{
	return getDBHatohol().getDBTablesMonitoring();
}
