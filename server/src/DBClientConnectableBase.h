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

public:
	static void reset(void);

	/**
	 * set the default parameters to connect the DB.
	 * The parameters set by this function are used for the instance of
	 * DBClientConnectable created with NULL as the first argument.
	 * The paramers are reset to the sysytem default when reset() is called.
	 *
	 * @param domainId A DB domain ID.
	 * @param dbName A database name.
	 * @param user
	 * A user name. If this paramter is "" (empty string), the name of
	 * the current user is used.
	 * @param passowrd
	 * A password. If this parameter is "" (empty string), no passowrd
	 * is used.
	 */
	static void setDefaultDBParams(DBDomainId domainId,
	                               const string &dbName,
	                               const string &user = "",
	                               const string &password = "");
	static DBConnectInfo getDBConnectInfo(DBDomainId domainId);

	DBClientConnectableBase(DBDomainId domainId);

protected:
	struct DBSetupContext;
	typedef map<DBDomainId, DBSetupContext *> DBSetupContextMap;
	typedef DBSetupContextMap::iterator       DBSetupContextMapIterator;

	static void registerSetupInfo(DBDomainId domainId, const string &dbName,
	                              DBSetupFuncArg *dbSetupFuncArg);
	static void setConnectInfo(DBDomainId domainId,
	                           const DBConnectInfo &connectInfo);

private:
	struct PrivateContext;
};

#endif // DBClientConnectableBase_h
