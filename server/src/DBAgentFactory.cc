/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "DBAgentFactory.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "DBHatohol.h"
#include "Params.h"
using namespace std;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBAgent* DBAgentFactory::create(const type_info &dbClassType,
                                const DBConnectInfo &connectInfo)
{
	// TODO: The actual DAgent type should be decided by a config file.
	DBAgent *dbAgent = NULL;
	if (dbClassType == typeid(DBHatohol)) {
		dbAgent = newDBAgentMySQL(connectInfo);
	} else {
		HATOHOL_ASSERT(false,
		               "Unknown DB Class: %s\n", dbClassType.name());
	}
	return dbAgent;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
DBAgent *DBAgentFactory::newDBAgentMySQL(const DBConnectInfo &connectInfo)
{
	return new DBAgentMySQL(connectInfo.dbName.c_str(),
	                        connectInfo.getUser(),
	                        connectInfo.getPassword(),
	                        connectInfo.getHost(),
	                        connectInfo.port);
}

DBAgent *DBAgentFactory::newDBAgentSQLite3(const DBConnectInfo &connectInfo)
{
	return new DBAgentSQLite3(connectInfo.dbName);
}
