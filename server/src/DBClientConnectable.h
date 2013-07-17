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
#include "DBClient.h"
#include "DBAgentFactory.h"
#include "MutexLock.h"

// The reason why this is a template class is that we want to make
// static members shared only in each derived class.
// If this class is not a template class, static members are shared
// among all derived classes. Connected server might be different by
// a drived class. Such behavior is undesirable.

template<DBDomainId DB_DOMAIN_ID,
         const struct DBSetupFuncArg *DB_SETUP_FUNC_ARG,
         const char *DEFAULT_DB_NAME>
class DBClientConnectable : public DBClient {
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
		return m_connectInfo;
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
		m_connectInfo.dbName   = dbName;
		m_connectInfo.user     = user;
		m_connectInfo.password = password;
	}

	DBClientConnectable(const DBConnectInfo *connectInfo = NULL)
	{
		m_mutex.lock();
		if (!connectInfo)
			connectInfo = &m_connectInfo;
		if (!m_initialized) {
			// The setup function: dbSetupFunc() is called from
			// the creation of DBAgent instance below.
			DBAgent::addSetupFunction(
			  DB_DOMAIN_ID, dbSetupFunc, (void *)DB_SETUP_FUNC_ARG);
		}
		m_mutex.unlock();
		bool skipSetup = false;
		setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID, skipSetup,
		                                  connectInfo));
	}

	virtual ~DBClientConnectable()
	{
	}

protected:
	static void resetDBInitializedFlags(void)
	{
		m_initialized = false;
	}

	static void initDefaultDBConnectInfoMaster(void)
	{
		m_connectInfo.host     = "localhost";
		m_connectInfo.port     = 0; // default port
		m_connectInfo.user     = "hatohol";
		m_connectInfo.password = "hatohol";
		m_connectInfo.dbName   = DEFAULT_DB_NAME;
		m_connectInfoMasterInitialized = true;
	}

	static void initDefaultDBConnectInfo(void)
	{
		if (!m_connectInfoMasterInitialized)
			initDefaultDBConnectInfoMaster();

		m_connectInfo.host     = m_connectInfoMaster.host;
		m_connectInfo.port     = m_connectInfoMaster.port;
		m_connectInfo.user     = m_connectInfoMaster.user;
		m_connectInfo.password = m_connectInfoMaster.password;
		m_connectInfo.dbName   = m_connectInfoMaster.dbName;
	}

private:
	static bool m_initialized;
	static mlpl::MutexLock m_mutex;

	// Contents in connectInfo is set to those in connectInfoMaseter
	// on reset(). However, connectInfoMaster is never changed after
	// its contents are set in the initialization.
	static DBConnectInfo m_connectInfo;
	static DBConnectInfo m_connectInfoMaster;
	static bool m_connectInfoMasterInitialized;
};

template<DBDomainId DID, const struct DBSetupFuncArg *ARG, const char *DB_NAME>
  mlpl::MutexLock DBClientConnectable<DID,ARG,DB_NAME>::m_mutex;

template<DBDomainId DID, const struct DBSetupFuncArg *ARG, const char *DB_NAME>
  bool DBClientConnectable<DID,ARG,DB_NAME>::m_initialized = false;

template<DBDomainId DID, const struct DBSetupFuncArg *ARG, const char *DB_NAME>
  DBConnectInfo DBClientConnectable<DID,ARG,DB_NAME>::m_connectInfo;

template<DBDomainId DID, const struct DBSetupFuncArg *ARG, const char *DB_NAME>
  DBConnectInfo DBClientConnectable<DID,ARG,DB_NAME>::m_connectInfoMaster;

template<DBDomainId DID, const struct DBSetupFuncArg *ARG, const char *DB_NAME>
  bool DBClientConnectable<DID,ARG,DB_NAME>::m_connectInfoMasterInitialized
    = false;

#endif // DBClientConnectable_h
