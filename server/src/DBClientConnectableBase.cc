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

#include "DBClientConnectableBase.h"


struct DBClientConnectableBase::PrivateContext {
	static DefaultDBInfoMap defaultDBInfoMap;
	static ReadWriteLock    dbInfoLock;
};

DBClientConnectableBase::DefaultDBInfoMap
  DBClientConnectableBase::PrivateContext::defaultDBInfoMap;
ReadWriteLock DBClientConnectableBase::PrivateContext::dbInfoLock;

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientConnectableBase::addDefaultDBInfo(
  DBDomainId domainId, const char *defaultDBName,
  DBSetupFuncArg *dbSetupFuncArg)
{
	pair<DefaultDBInfoMapIterator, bool> result;
	DefaultDBInfo dbInfo = {defaultDBName, dbSetupFuncArg};
	PrivateContext::dbInfoLock.writeLock();
	result = PrivateContext::defaultDBInfoMap.insert(
	  pair<DBDomainId, DefaultDBInfo>(domainId, dbInfo));
	PrivateContext::dbInfoLock.unlock();
	HATOHOL_ASSERT(result.second,
	               "Failed to insert: Domain ID: %d", domainId);
}

DBClientConnectableBase::DefaultDBInfo
&DBClientConnectableBase::getDefaultDBInfo(DBDomainId domainId)
{
	PrivateContext::dbInfoLock.readLock();
	DefaultDBInfoMapIterator it =
	  PrivateContext::defaultDBInfoMap.find(domainId);
	PrivateContext::dbInfoLock.unlock();
	HATOHOL_ASSERT(it != PrivateContext::defaultDBInfoMap.end(),
	               "Not found default DB info: %d", domainId);
	return it->second;
}
