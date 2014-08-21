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

#include "DBHatohol.h"
#include "ConfigManager.h"
#include "DBTablesConfig.h"
// #include "DBTablesHost.h" 
// #include "DBTablesUser.h"
// #include "DBTablesAction.h"

using namespace std;

static const char *DEFAULT_DB_NAME = "hatohol";
static const char *DEFAULT_USER_NAME = "hatohol";
static const char *DEFAULT_PASSWORD  = "hatohol";

// These are stub to pass the build
class DBTablesHost {
};

class DBTablesAction {
};

class DBTablesMonitor {
};

struct DBHatohol::Impl {
	static SetupContext setupCtx;

	DBTablesConfig  dbTablesConfig;
	DBTablesHost    dbTablesHost;
	DBTablesUser    dbTablesUser;
	DBTablesAction  dbTablesAction;
	DBTablesMonitor dbTablesMonitor;

	Impl(DBAgent &dbAgent)
	: dbTablesConfig(/*dbAgent*/),
	  dbTablesHost(/*dbAgent*/),
	  dbTablesUser(/*dbAgent*/),
	  dbTablesAction(/*dbAgent*/),
	  dbTablesMonitor(/*dbAgent*/)
	{
	}
};

DB::SetupContext DBHatohol::Impl::setupCtx(DB::DB_MYSQL);

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
  const string &dbName, const string &user, const string &password)
{
	DBConnectInfo &connInfo = Impl::setupCtx.connectInfo;
	connInfo.dbName   = dbName;
	connInfo.user     = user;
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

DBTablesMonitor &DBHatohol::getDBTablesMonitor(void)
{
	return m_impl->dbTablesMonitor;
}
