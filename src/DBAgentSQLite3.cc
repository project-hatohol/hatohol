/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstring>

#include "Logger.h"
using namespace mlpl;

static const char *DEFAULT_DB_PATH = "/tmp/asura.db";
static const int DB_VERSION = 1;
static const char *TABLE_NAME_SYSTEM = "system";

#include "DBAgentSQLite3.h"
#include "AsuraException.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentSQLite3::init(void)
{
	// TODO: check the DB version.
	//       If the DB version is old, update the content

	DBAgentSQLite3 dbAgent;
	if (!dbAgent.checkExistTable(TABLE_NAME_SYSTEM))
		dbAgent.createTableSystem();
}

DBAgentSQLite3::DBAgentSQLite3(void)
: m_db(NULL),
  m_dbPath(DEFAULT_DB_PATH)
{
	openDatabase();
}

DBAgentSQLite3::~DBAgentSQLite3()
{
	if (m_db) {
		int result = sqlite3_close(m_db);
		if (result != SQLITE_OK) {
			// Should we throw an exception ?
			MLPL_ERR("Failed to close sqlite: %d\n", result);
		}
	}
}

bool DBAgentSQLite3::checkExistTable(const string &tableName)
{
	int result;
	sqlite3_stmt *stmt;
	const char *query = "SELECT COUNT(*) FROM sqlite_master "
	                    "WHERE type='table' AND name='?'";
	result = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}

	sqlite3_reset(stmt);
	result = sqlite3_bind_text(stmt, 1, tableName.c_str(),
	                           -1, SQLITE_STATIC);
	if (result == SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	
	int count = 0;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 1);
	}
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
	return count > 0;
}

void DBAgentSQLite3::addTargetServer
  (MonitoringServerInfo *monitoringServerInfo)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentSQLite3::getTargetServers
  (MonitoringServerInfoList &monitoringServers)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBAgentSQLite3::openDatabase(void)
{
	if (m_db)
		return;

	int result = sqlite3_open_v2(m_dbPath.c_str(), &m_db,
	                             SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,
	                             NULL);
	if (result != SQLITE_OK) {
		THROW_ASURA_EXCEPTION("Failed to open sqlite: %d, %s",
		                      result, m_dbPath.c_str());
	}
}

void DBAgentSQLite3::createTableSystem(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}
