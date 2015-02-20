/*
 * Copyright (C) 2014 Project Hatohol
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

#include "DBHatohol.h"
#include "ConfigManager.h"
#include "DBTablesConfig.h"
#include "DBTablesHost.h" 
#include "DBTablesUser.h"
#include "DBTablesAction.h"
#include "DBTablesMonitoring.h"

using namespace std;

static const char *DEFAULT_DB_NAME = "hatohol";
static const char *DEFAULT_USER_NAME = "hatohol";
static const char *DEFAULT_PASSWORD  = "hatohol";

struct DBHatohol::Impl {
	static SetupContext setupCtx;

	DBTablesConfig  dbTablesConfig;
	DBTablesHost    dbTablesHost;
	DBTablesUser    dbTablesUser;
	DBTablesAction  dbTablesAction;
	DBTablesMonitoring dbTablesMonitoring;

	Impl(DBAgent &dbAgent)
	: dbTablesConfig(dbAgent),
	  dbTablesHost(dbAgent),
	  dbTablesUser(dbAgent),
	  dbTablesAction(dbAgent),
	  dbTablesMonitoring(dbAgent)
	{
	}
};

DB::SetupContext DBHatohol::Impl::setupCtx(typeid(DBHatohol));

// ---------------------------------------------------------------------------
// This function is intended to call from hatohol-db-initiator
// ---------------------------------------------------------------------------
extern "C"
int createDBHatohol(
  const char *dbName, const char *user, const char *password)
{
	try {
		DBHatohol::setDefaultDBParams(dbName, user, password);
		DBHatohol db; // Create all DBTables instances to create tables
	} catch (...) {
		return -1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBHatohol::reset(void)
{
	Impl::setupCtx.initialized = false;
	DBConnectInfo &connInfo = Impl::setupCtx.connectInfo;

	ConfigManager *confMgr = ConfigManager::getInstance();
	connInfo.host = confMgr->getDBServerAddress();
	connInfo.port = confMgr->getDBServerPort();

	connInfo.user     = DEFAULT_USER_NAME;
	connInfo.password = DEFAULT_PASSWORD;
	connInfo.dbName   = DEFAULT_DB_NAME;
}

void DBHatohol::setDefaultDBParams(
  const char *dbName, const char *user, const char *password)
{
	DBConnectInfo &connInfo = Impl::setupCtx.connectInfo;
	if (dbName)
		connInfo.dbName   = dbName;
	if (user)
		connInfo.user     = user;
	if (password)
		connInfo.password = password;
}

DBHatohol::DBHatohol(void)
: DB(Impl::setupCtx),
  m_impl(new Impl(getDBAgent()))
{
}

DBHatohol::~DBHatohol()
{
}

DBTablesConfig &DBHatohol::getDBTablesConfig(void)
{
	return m_impl->dbTablesConfig;
}

DBTablesHost &DBHatohol::getDBTablesHost(void)
{
	return m_impl->dbTablesHost;
}

DBTablesUser &DBHatohol::getDBTablesUser(void)
{
	return m_impl->dbTablesUser;
}

DBTablesAction &DBHatohol::getDBTablesAction(void)
{
	return m_impl->dbTablesAction;
}

DBTablesMonitoring &DBHatohol::getDBTablesMonitoring(void)
{
	return m_impl->dbTablesMonitoring;
}
