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

#include <MutexLock.h>
using namespace mlpl;

#include "DBAgent.h"

struct DBSetupInfo {
	DBSetupFunc func;
	void       *data;
};

typedef multimap<DBDomainId, DBSetupInfo> DBSetupInfoMap;
typedef DBSetupInfoMap::iterator          DBSetupInfoMapIterator;

DBAgentSelectExArg::DBAgentSelectExArg(void)
: limit(0),
  offset(0)
{
}

void DBAgentSelectExArg::pushColumn
  (const ColumnDef &columnDef, const string &varName)
{
	string statement;
	if (!varName.empty()) {
		statement = varName;
		statement += ".";
	}
	statement += columnDef.columnName;
	statements.push_back(statement);
	columnTypes.push_back(columnDef.type);
}

struct DBAgent::PrivateContext
{
	static MutexLock          mutex;
	static DBSetupInfoMap     setupInfoMap;

	// methods
	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};

MutexLock      DBAgent::PrivateContext::mutex;
DBSetupInfoMap DBAgent::PrivateContext::setupInfoMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgent::addSetupFunction(DBDomainId domainId,
                               DBSetupFunc setupFunc, void *data)
{
	DBSetupInfo setupInfo;
	setupInfo.func = setupFunc;
	setupInfo.data = data;
	PrivateContext::lock();
	PrivateContext::setupInfoMap.insert
	  (pair<DBDomainId, DBSetupInfo>(domainId, setupInfo));
	PrivateContext::unlock();
}

DBAgent::DBAgent(DBDomainId domainId, bool skipSetup)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	if (skipSetup)
		return;

	PrivateContext::lock();
	pair<DBSetupInfoMapIterator, DBSetupInfoMapIterator> matchedRange = 
	  PrivateContext::setupInfoMap.equal_range(domainId);
	DBSetupInfoMapIterator it = matchedRange.first;
	for (; it != matchedRange.second; ++it) {
		DBSetupInfo &setupInfo = it->second;
		DBSetupFunc &setupFunc = setupInfo.func;
		void        *data      = setupInfo.data;
		try {
			(*setupFunc)(domainId, data);
		} catch (...) {
			// Note: contetns in DBSetupInfoMap remains.
			PrivateContext::unlock();
			throw;
		}
	}
	PrivateContext::setupInfoMap.erase
	  (matchedRange.first, matchedRange.second);
	PrivateContext::unlock();
}

DBAgent::~DBAgent()
{
	if (m_ctx)
		delete m_ctx;
}
