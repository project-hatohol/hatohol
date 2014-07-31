/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstdio>
#include <set>
#include "DBCGroupRegular.h"

using namespace std;

const char *DEFAULT_DB_NAME = "hatohol";
const char *DEFAULT_USER_NAME = "hatohol";
const char *DEFAULT_PASSWORD  = "hatohol";

struct DBCGroupRegular::PrivateContext {
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
static set<DBDomainId> g_dbDomainIdSet;

void DBCGroupRegular::registerSetupInfo(
  const DBDomainId &domainId, const DBSetupFuncArg *dbSetupFuncArg)
{
	DBClient::registerSetupInfo(domainId, DEFAULT_DB_NAME, dbSetupFuncArg);

	// TODO: Reset mechanism should be performed in more base class.
	g_dbDomainIdSet.insert(domainId);
}

void DBCGroupRegular::reset(void)
{
	// TODO: Reset mechanism should be performed in more base class.
	DBConnectInfo connInfo = getDBConnectInfo(DB_DOMAIN_ID_CONFIG);
	connInfo.user     = DEFAULT_USER_NAME;
	connInfo.password = DEFAULT_PASSWORD;
	connInfo.dbName   = DEFAULT_DB_NAME;
	
	set<DBDomainId>::const_iterator domainIdItr = g_dbDomainIdSet.begin();
	for (; domainIdItr != g_dbDomainIdSet.end(); ++domainIdItr)
		setConnectInfo(*domainIdItr, connInfo);
}

DBCGroupRegular::DBCGroupRegular(const DBDomainId &domainId)
: DBClientGroup(domainId),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBCGroupRegular::~DBCGroupRegular()
{
	delete m_ctx;
}

