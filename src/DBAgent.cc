/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DBAgent.h"

typedef multimap<DBDomainId, DBSetupFunc> DBSetupFuncMap;
typedef DBSetupFuncMap::iterator          DBSetupFuncMapIterator;

struct DBAgent::PrivateContext
{
	static GMutex             mutex;
	static DBSetupFuncMap     setupFuncMap;

	// methods
	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};

GMutex         DBAgent::PrivateContext::mutex;
DBSetupFuncMap DBAgent::PrivateContext::setupFuncMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgent::addSetupFunction(DBDomainId domainId, DBSetupFunc setupFunc)
{
	PrivateContext::lock();
	PrivateContext::setupFuncMap.insert
	  (pair<DBDomainId, DBSetupFunc>(domainId, setupFunc));
	PrivateContext::unlock();
}

DBAgent::DBAgent(DBDomainId domainId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	PrivateContext::lock();
	pair<DBSetupFuncMapIterator, DBSetupFuncMapIterator> matchedRange = 
	  PrivateContext::setupFuncMap.equal_range(domainId);
	DBSetupFuncMapIterator it = matchedRange.first;
	for (; it != matchedRange.second; ++it) {
		DBSetupFunc setupFunc = it->second;
		try {
			(*setupFunc)(domainId);
		} catch (...) {
			// Note: contetns in DBSetupFuncMap remains.
			PrivateContext::unlock();
			throw;
		}
	}
	PrivateContext::setupFuncMap.erase
	  (matchedRange.first, matchedRange.second);
	PrivateContext::unlock();
}

DBAgent::~DBAgent()
{
	if (m_ctx)
		delete m_ctx;
}
