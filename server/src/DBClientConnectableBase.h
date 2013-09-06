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

#ifndef DBClientConnectableBase_h
#define DBClientConnectableBase_h

#include <list>
#include "DBClient.h"
#include "DBAgentFactory.h"
#include "MutexLock.h"

class DBClientConnectableBase : public DBClient {
protected:
	struct DefaultDBInfo {
		const char     *dbName;
		DBSetupFuncArg *dbSetupFuncArg;
	};
	typedef map<DBDomainId, DefaultDBInfo>      DefaultDBInfoMap;
	typedef DefaultDBInfoMap::iterator DefaultDBInfoMapIterator;

	static void addDefaultDBInfo(
	  DBDomainId domainId, const char *defaultDBName,
	  DBSetupFuncArg *dbSetupFuncArg);
	static DefaultDBInfo &getDefaultDBInfo(DBDomainId domainId);

private:
	struct PrivateContext;
};

#endif // DBClientConnectableBase_h
