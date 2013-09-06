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

#ifndef DBClientConnectable_h
#define DBClientConnectable_h

#include <list>
#include "DBClientConnectableBase.h"
#include "DBAgentFactory.h"
#include "MutexLock.h"

// The reason why this is a template class is that we want to make
// static members shared only in each derived class.
// If this class is not a template class, static members are shared
// among all derived classes. Connected server might be different by
// a drived class. Such behavior is undesirable.

template<DBDomainId DB_DOMAIN_ID>
class DBClientConnectable : public DBClientConnectableBase {
public:
	static void reset(bool deepReset = false)
	{
		resetDBInitializedFlags();
		if (deepReset)
			initDefaultDBConnectInfoMaster();
		initDefaultDBConnectInfo();
	}

	static const DBConnectInfo &getDBConnectInfo(void)
	{
		return m_ctx.connectInfo;
	}

	/**
	 * set the default parameters to connect the DB.
	 * The parameters set by this function are used for the instance of
	 * DBClientConnectable created with NULL as the first argument.
	 * The paramers are reset to the sysytem default when reset() is called.
	 *
	 * @param dbName A database name.
	 * @param user
	 * A user name. If this paramter is "" (empty string), the name of
	 * the current user is used.
	 * @param passowrd
	 * A password. If this parameter is "" (empty string), no passowrd
	 * is used.
	 */
	static void setDefaultDBParams(const string &dbName,
	                               const string &user = "",
	                               const string &password = "")
	{
		m_ctx.connectInfo.dbName   = dbName;
		m_ctx.connectInfo.user     = user;
		m_ctx.connectInfo.password = password;
	}

	DBClientConnectable(const DBConnectInfo *connectInfo = NULL)
	{
		// m_connectInfo is initizalied in reset(). It isn't updated
		// after reset. So we can refere it without a lock.
		if (!connectInfo)
			connectInfo = &m_ctx.connectInfo;
		m_ctx.mutex.lock();
		if (!m_ctx.initialized) {
			// The setup function: dbSetupFunc() is called from
			// the creation of DBAgent instance below.
			DefaultDBInfo &dbInfo = getDefaultDBInfo(DB_DOMAIN_ID);
			dbInfo.dbSetupFuncArg->connectInfo = connectInfo;
			DBAgent::addSetupFunction(
			  DB_DOMAIN_ID, dbSetupFunc,
			  (void *)dbInfo.dbSetupFuncArg);
			bool skipSetup = false;
			setDBAgent(DBAgentFactory::create(
			  DB_DOMAIN_ID, skipSetup, connectInfo));
			m_ctx.initialized = true;
			m_ctx.mutex.unlock();
		} else {
			m_ctx.mutex.unlock();
			bool skipSetup = true;
			setDBAgent(DBAgentFactory::create(
			  DB_DOMAIN_ID, skipSetup, connectInfo));
		}
	}

	virtual ~DBClientConnectable()
	{
	}

protected:
	static DBConnectInfo &getDBConnectInfoMaster(void)
	{
		return m_ctx.connectInfoMaster;
	}

	static void resetDBInitializedFlags(void)
	{
		m_ctx.initialized = false;
	}

	static void initDefaultDBConnectInfoMaster(void)
	{
		DefaultDBInfo &dbInfo = getDefaultDBInfo(DB_DOMAIN_ID);
		m_ctx.connectInfoMaster.host     = "localhost";
		m_ctx.connectInfoMaster.port     = 0; // default port
		m_ctx.connectInfoMaster.user     = "hatohol";
		m_ctx.connectInfoMaster.password = "hatohol";
		m_ctx.connectInfoMaster.dbName   = dbInfo.dbName;
		m_ctx.connectInfoMasterInitialized = true;
	}

	static void initDefaultDBConnectInfo(void)
	{
		if (!m_ctx.connectInfoMasterInitialized)
			initDefaultDBConnectInfoMaster();

		m_ctx.connectInfo.host     = m_ctx.connectInfoMaster.host;
		m_ctx.connectInfo.port     = m_ctx.connectInfoMaster.port;
		m_ctx.connectInfo.user     = m_ctx.connectInfoMaster.user;
		m_ctx.connectInfo.password = m_ctx.connectInfoMaster.password;
		m_ctx.connectInfo.dbName   = m_ctx.connectInfoMaster.dbName;
	}

private:
	struct PrivateContext {
		bool initialized;
		mlpl::MutexLock mutex;

		// Contents in connectInfo is set to those in connectInfoMaseter
		// on reset(). However, connectInfoMaster is never changed after
		// its contents are set in the initialization.
		DBConnectInfo connectInfo;
		DBConnectInfo connectInfoMaster;
		bool          connectInfoMasterInitialized;

		PrivateContext(void)
		: initialized(false),
		  connectInfoMasterInitialized(false)
		{
		}
	};
	static PrivateContext m_ctx;
};

template<DBDomainId DID> typename DBClientConnectable<DID>::PrivateContext
  DBClientConnectable<DID>::m_ctx;

#endif // DBClientConnectable_h
