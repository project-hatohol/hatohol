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

#include "DBAgentFactory.h"
#include "DBAgentSQLite3.h"
#include "DBAgentMySQL.h"
#include "Params.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBAgent* DBAgentFactory::create(DBDomainId domainId, bool skipSetup,
                                const DBConnectInfo *connectInfo)
{
	if (domainId == DB_DOMAIN_ID_CONFIG ||
	    domainId == DB_DOMAIN_ID_ACTION ||
	    domainId == DB_DOMAIN_ID_USERS) {
		HATOHOL_ASSERT(connectInfo, "connectInfo: NULL");
		return new DBAgentMySQL(connectInfo->dbName.c_str(),
		                        connectInfo->getUser(),
		                        connectInfo->getPassword(),
		                        connectInfo->getHost(),
		                        connectInfo->port,
		                        domainId, skipSetup);
	}
	return new DBAgentSQLite3(domainId, skipSetup);
}
