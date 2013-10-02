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
#include "Params.h"
#include "DBClient.h"
#include "CacheServiceDBClient.h"
using namespace std;

typedef map<DBDomainId, void *> DBClientMap;
typedef DBClientMap::iterator       DBClientMapIterator;

struct CacheServiceDBClient::PrivateContext {
	static __thread DBClientMap *clientMap;

	static void *get(DBDomainId domainId)
	{
		if (!clientMap)
			clientMap = new DBClientMap();
		DBClientMapIterator it = clientMap->find(domainId);
		if (it != clientMap->end())
			return it->second;
		void *dbClient = NULL;
		if (domainId == DB_DOMAIN_ID_HATOHOL)
			dbClient = new DBClientHatohol();
		HATOHOL_ASSERT(dbClient,
		               "ptr is NULL. domainId: %d\n", domainId);
		clientMap->insert(pair<DBDomainId, void *>(domainId, dbClient));
		return dbClient;
	}
};
__thread DBClientMap *CacheServiceDBClient::PrivateContext::clientMap = NULL;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
CacheServiceDBClient::CacheServiceDBClient(void)
{
}

CacheServiceDBClient::~CacheServiceDBClient()
{
}

DBClientHatohol *CacheServiceDBClient::getHatohol(void)
{
	void *dbHatohol = PrivateContext::get(DB_DOMAIN_ID_HATOHOL);
	return static_cast<DBClientHatohol *>(dbHatohol);
}
