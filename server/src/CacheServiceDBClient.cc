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
#include <MutexLock.h>
#include "Params.h"
#include "DBClient.h"
#include "CacheServiceDBClient.h"
using namespace std;
using namespace mlpl;

typedef map<DBDomainId, DBClient *> DBClientMap;
typedef DBClientMap::iterator   DBClientMapIterator;

typedef set<DBClientMap *>       DBClientMapSet;
typedef DBClientMapSet::iterator DBClientMapSetIterator;

struct CacheServiceDBClient::PrivateContext {
	// This lock is for DBClientMapList. clientMap can be accessed w/o
	// the lock because it is on the thread local storage.
	static MutexLock lock;
	static DBClientMapSet dbClientMapSet;

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
		if (domainId == DB_DOMAIN_ID_HATOHOL)
			dbClient = new DBClientHatohol();
		HATOHOL_ASSERT(dbClient,
		               "ptr is NULL. domainId: %d\n", domainId);
		clientMap->insert(
		  pair<DBDomainId,DBClient *>(domainId, dbClient));

		return dbClient;
	}

	static void reset(void)
	{
		lock.lock();
		DBClientMapSetIterator it = dbClientMapSet.begin();
		for (; it != dbClientMapSet.end(); ++it)
			deleteDBClientMap(*it);
		dbClientMapSet.clear();
		lock.unlock();
	}

	static void deleteDBClientMap(DBClientMap *dbClientMap)
	{
		DBClientMapIterator it = dbClientMap->begin();
		for (; it != dbClientMap->end(); ++it)
			delete it->second;
		delete dbClientMap;
	}
};
__thread DBClientMap *CacheServiceDBClient::PrivateContext::clientMap = NULL;
MutexLock      CacheServiceDBClient::PrivateContext::lock;
DBClientMapSet CacheServiceDBClient::PrivateContext::dbClientMapSet;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void CacheServiceDBClient::reset(void)
{
	PrivateContext::reset();
}

CacheServiceDBClient::CacheServiceDBClient(void)
{
}

CacheServiceDBClient::~CacheServiceDBClient()
{
}

DBClientHatohol *CacheServiceDBClient::getHatohol(void)
{
	DBClient *dbHatohol = PrivateContext::get(DB_DOMAIN_ID_HATOHOL);
	// Here we use static_cast, although this is upcast.
	// Sub classes other than DBClientHahol is never returned
	// from the above get() according to the design.
	return static_cast<DBClientHatohol *>(dbHatohol);
}
