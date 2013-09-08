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

// This structure instnace is created once every DB_DOMAIN_ID
struct DBClientConnectableBase::DBSetupContext {
	bool              initialized;
	MutexLock         mutex;
	string            dbName;
	DBSetupFuncArg   *dbSetupFuncArg;
	DBConnectInfo     connectInfo;

	DBSetupContext(void)
	: initialized(false),
	  dbSetupFuncArg(NULL)
	{
	}
};

struct DBClientConnectableBase::PrivateContext {
	static DBSetupContextMap dbSetupCtxMap;
	static ReadWriteLock     dbSetupCtxMapLock;

	static DBSetupContext *getDBSetupContext(DBDomainId domainId)
	{
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.find(domainId);
		HATOHOL_ASSERT(it != dbSetupCtxMap.end(),
		               "Failed to find. domainId: %d\n", domainId);
		DBSetupContext *setupCtx = it->second;
		setupCtx->mutex.lock();
		dbSetupCtxMapLock.unlock();
		return setupCtx;
	}

	static void registerSetupInfo(DBDomainId domainId, const string &dbName,
	                              DBSetupFuncArg *dbSetupFuncArg)
	{
		DBSetupContext *setupCtx = NULL;
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.find(domainId);
		if (it == dbSetupCtxMap.end()) {
			setupCtx = new DBSetupContext();
			dbSetupCtxMap[domainId] = setupCtx;
		}
		else {
			setupCtx = it->second;
		}
		setupCtx->dbName = dbName;
		setupCtx->dbSetupFuncArg = dbSetupFuncArg;
		dbSetupCtxMapLock.unlock();
	}

	static void clearInitializedFlag(void)
	{
		dbSetupCtxMapLock.readLock();
		DBSetupContextMapIterator it = dbSetupCtxMap.begin();
		for (; it != dbSetupCtxMap.end(); ++it) {
			DBSetupContext *setupCtx = it->second;
			setupCtx->initialized = false;
			setupCtx->connectInfo.reset();
		}
		dbSetupCtxMapLock.unlock();
	}
};

DBClientConnectableBase::DBSetupContextMap
  DBClientConnectableBase::PrivateContext::dbSetupCtxMap;
ReadWriteLock DBClientConnectableBase::PrivateContext::dbSetupCtxMapLock;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientConnectableBase::reset(void)
{
	// We assume that this function is called in the test.
	PrivateContext::clearInitializedFlag();
}

void DBClientConnectableBase::setDefaultDBParams(
  DBDomainId domainId,
  const string &dbName, const string &user, const string &password)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	setupCtx->connectInfo.dbName   = dbName;
	setupCtx->connectInfo.user     = user;
	setupCtx->connectInfo.password = password;
	setupCtx->mutex.unlock();
}

DBConnectInfo DBClientConnectableBase::getDBConnectInfo(DBDomainId domainId)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	DBConnectInfo connInfo = setupCtx->connectInfo;
	setupCtx->mutex.unlock();
	return connInfo; // we return the copy
}

DBClientConnectableBase::DBClientConnectableBase(DBDomainId domainId)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	if (!setupCtx->initialized) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		DBAgent::addSetupFunction(
		  domainId, dbSetupFunc, (void *)setupCtx->dbSetupFuncArg);
		bool skipSetup = false;
		setDBAgent(DBAgentFactory::create(
		  domainId, skipSetup, &setupCtx->connectInfo));
		setupCtx->initialized = true;
		setupCtx->mutex.unlock();
	} else {
		setupCtx->mutex.unlock();
		bool skipSetup = true;
		setDBAgent(DBAgentFactory::create(
		  domainId, skipSetup, &setupCtx->connectInfo));
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientConnectableBase::registerSetupInfo(
  DBDomainId domainId, const string &dbName,
  DBSetupFuncArg *dbSetupFuncArg)
{
	PrivateContext::registerSetupInfo(domainId, dbName, dbSetupFuncArg);
}

void DBClientConnectableBase::setConnectInfo(
  DBDomainId domainId, const DBConnectInfo &connectInfo)
{
	DBSetupContext *setupCtx = PrivateContext::getDBSetupContext(domainId);
	setupCtx->connectInfo = connectInfo;
	setupCtx->dbSetupFuncArg->connectInfo = &setupCtx->connectInfo;
	setupCtx->mutex.unlock();
}
