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

#include <map>
#include <list>
#include <Mutex.h>
#include "Params.h"
#include "DBClient.h"
#include "DBCache.h"
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

typedef map<DBDomainId, DBClient *> DBClientMap;
typedef DBClientMap::iterator   DBClientMapIterator;

typedef set<DBClientMap *>       DBClientMapSet;
typedef DBClientMapSet::iterator DBClientMapSetIterator;

typedef list<DBClient *>       DBClientList;
typedef DBClientList::iterator DBClientListIterator;

typedef map<DBClient *, DBClientListIterator> DBClientListItrMap;
typedef DBClientListItrMap::iterator          DBClientListItrMapIterator;

struct CacheLRU {
	DBClientList       list;
	DBClientListItrMap map;

	void touch(DBClient *dbClient)
	{
		DBClientListItrMapIterator it = map.find(dbClient);
		if (it != map.end())
			list.erase(it->second);
		map[dbClient] = list.insert(list.begin(), dbClient);
	}
};

struct DBCache::Impl {
	// This lock is for DBClientMapList. clientMap can be accessed w/o
	// the lock because it is on the thread local storage.
	static Mutex          lock;
	static DBClientMapSet dbClientMapSet;
	// TODO: The limitation of cahed objects is now WIP.
	static size_t maxNumCacheMySQL;
	static size_t maxNumCacheSQLite3;

	static __thread DBClientMap *clientMap;

	static DBClient *get(DBDomainId domainId)
	{
		if (!clientMap) {
			clientMap = new DBClientMap();
			lock.lock();
			dbClientMapSet.insert(clientMap);
			lock.unlock();
		}

		DBClientMapIterator it = clientMap->find(domainId);
		if (it != clientMap->end())
			return it->second;
		DBClient *dbClient = NULL;
		if (domainId == DB_TABLES_ID_MONITORING)
			dbClient = new DBTablesMonitoring();
		else if (domainId == DB_TABLES_ID_USER)
			dbClient = new DBTablesUser();
		else if (domainId == DB_DOMAIN_ID_CONFIG)
			dbClient = new DBTablesConfig();
		else if (domainId == DB_TABLES_ID_ACTION)
			dbClient = new DBTablesAction();
		HATOHOL_ASSERT(dbClient,
		               "ptr is NULL. domainId: %d\n", domainId);
		clientMap->insert(
		  pair<DBDomainId,DBClient *>(domainId, dbClient));

		return dbClient;
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

	static void deleteDBClientMap(DBClientMap *dbClientMap)
	{
		DBClientMapIterator it = dbClientMap->begin();
		for (; it != dbClientMap->end(); ++it)
			delete it->second;
		dbClientMap->clear();
		delete dbClientMap;
	}

	static void cleanup(void)
	{
		if (!clientMap)
			return;
		lock.lock();
		DBClientMapSetIterator it = dbClientMapSet.find(clientMap);
		bool found = (it != dbClientMapSet.end());
		if (found)
			dbClientMapSet.erase(it);
		lock.unlock();
		HATOHOL_ASSERT(found, "Failed to lookup clientMap: %p.",
		               clientMap);
		deleteDBClientMap(clientMap);
		clientMap = NULL;
	}
};
__thread DBClientMap *DBCache::Impl::clientMap = NULL;
Mutex          DBCache::Impl::lock;
DBClientMapSet DBCache::Impl::dbClientMapSet;
size_t DBCache::Impl::maxNumCacheMySQL
  = DEFAULT_MAX_NUM_CACHE_MYSQL;
size_t DBCache::Impl::maxNumCacheSQLite3
  = DEFAULT_MAX_NUM_CACHE_SQLITE3;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBCache::reset(void)
{
	Impl::reset();
}

void DBCache::cleanup(void)
{
	Impl::cleanup();
}

size_t DBCache::getNumberOfDBClientMaps(void)
{
	Impl::lock.lock();
	size_t num = Impl::dbClientMapSet.size();
	Impl::lock.unlock();
	return num;
}

DBCache::DBCache(void)
{
}

DBCache::~DBCache()
{
}

DBTablesConfig &DBCache::getConfig(void)
{
	return *get<DBTablesConfig>(DB_DOMAIN_ID_CONFIG);
}

DBTablesUser &DBCache::getUser(void)
{
	return *get<DBTablesUser>(DB_TABLES_ID_USER);
}


DBTablesAction &DBCache::getAction(void)
{
	return *get<DBTablesAction>(DB_DOMAIN_ID_CONFIG);
}

DBTablesMonitoring *DBCache::getMonitoring(void)
{
	return get<DBTablesMonitoring>(DB_TABLES_ID_MONITORING);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
template <class T>
T *DBCache::get(DBDomainId domainId)
{
	DBClient *dbClient = Impl::get(domainId);
	// Here we use static_cast, although this is downcast.
	// Sub class other than that correspoinding to domainId is
	// never returned from the above get() according to the design.
	return static_cast<T *>(dbClient);
}

