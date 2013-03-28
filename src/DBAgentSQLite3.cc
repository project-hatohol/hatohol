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
#include <cstdio>
#include <stdarg.h>

#include "Logger.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"

static const char *TABLE_NAME_SYSTEM = "system";
static const char *TABLE_NAME_SERVERS = "servers";
static const char *DEFAULT_DB_PATH = "/tmp/DBAgentSQLite3Default.db";

const int DBAgentSQLite3::DB_VERSION = 1;
string DBAgentSQLite3::m_dbPath = DEFAULT_DB_PATH;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentSQLite3::init(const string &path)
{
	// TODO: check the DB version.
	//       If the DB version is old, update the content
	m_dbPath = path;

	DBAgentSQLite3 dbAgent;
	if (!dbAgent.isTableExisting(TABLE_NAME_SYSTEM))
		dbAgent.createTableSystem();
	else
		dbAgent.updateDBIfNeeded();

	// check the servers table
	if (!dbAgent.isTableExisting(TABLE_NAME_SERVERS))
		dbAgent.createTableServers();
}

DBAgentSQLite3::DBAgentSQLite3(void)
: m_db(NULL)
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

bool DBAgentSQLite3::isTableExisting(const string &tableName)
{
	int result;
	sqlite3_stmt *stmt;
	const char *query = "SELECT COUNT(*) FROM sqlite_master "
	                    "WHERE type='table' AND name=?";
	result = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}

	sqlite3_reset(stmt);
	result = sqlite3_bind_text(stmt, 1, tableName.c_str(),
	                           -1, SQLITE_STATIC);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	
	int count = 0;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	}
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
	return count > 0;
}

bool DBAgentSQLite3::isRecordExisting(const string &tableName,
                                      const string &condition)
{
	int result;
	sqlite3_stmt *stmt;
	string query = StringUtils::sprintf(
	                 "SELECT * FROM %s WHERE %s",
	                 tableName.c_str(), condition.c_str());
	result = sqlite3_prepare(m_db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d: %s",
		  result, query.c_str());
	}
	sqlite3_reset(stmt);
	bool found = false;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		found = true;
		break;
	}
	sqlite3_finalize(stmt);
	if (result != SQLITE_ROW && result != SQLITE_DONE) {
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	return found;
}

void DBAgentSQLite3::addTargetServer
  (MonitoringServerInfo *monitoringServerInfo)
{
	string condition = StringUtils::sprintf("id=%u",
	                                        monitoringServerInfo->id);
	execSql("BEGIN");
	if (!isRecordExisting(TABLE_NAME_SERVERS, condition)) {
		execSql("INSERT INTO %s VALUES(%u,%d,%Q,%Q,%Q)",
		        TABLE_NAME_SERVERS,
		        monitoringServerInfo->id, monitoringServerInfo->type,
		        monitoringServerInfo->hostName.c_str(),
		        monitoringServerInfo->ipAddress.c_str(),
		        monitoringServerInfo->nickname.c_str());
	} else {
		execSql("UPDATE %s SET type=%d, hostname=%Q, "
		        "ip_address=%Q, nickname=%Q",
		        TABLE_NAME_SERVERS,
		        monitoringServerInfo->type,
		        monitoringServerInfo->hostName.c_str(),
		        monitoringServerInfo->ipAddress.c_str(),
		        monitoringServerInfo->nickname.c_str());
	}
	execSql("COMMIT");
}

void DBAgentSQLite3::getTargetServers
  (MonitoringServerInfoList &monitoringServers)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

int DBAgentSQLite3::getDBVersion(void)
{
	int result;
	sqlite3_stmt *stmt;
	string query = "SELECT version FROM ";
	query += TABLE_NAME_SYSTEM;
	result = sqlite3_prepare(m_db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}
	sqlite3_reset(stmt);
	int version = 0;
	int count = 0;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		version = sqlite3_column_int(stmt, 0);
		count++;
	}
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
	ASURA_ASSERT(count == 1,
	             "Returned count of rows is not one (%d)", count);
	return version;
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
	// make table
	string sql = "CREATE TABLE ";
	sql += TABLE_NAME_SYSTEM;
	sql += "(version INTEGER)";
	char *errmsg;
	int result = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	// insert the version
	sql = StringUtils::sprintf("INSERT INTO %s VALUES(%d)",
	                           TABLE_NAME_SYSTEM,  DB_VERSION);
	result = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::updateDBIfNeeded(void)
{
	if (getDBVersion() == DB_VERSION)
		return;
	THROW_ASURA_EXCEPTION("Not implemented: %s", __PRETTY_FUNCTION__);
}

void DBAgentSQLite3::createTableServers(void)
{
	// make table
	string sql = "CREATE TABLE ";
	sql += TABLE_NAME_SERVERS;
	sql += "(id INTEGER PRIMARY KEY, type INTEGER, hostname TEXT, "
	       " ip_address TEXT, nickname TEXT)";
	char *errmsg;
	int result = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::execSql(const char *fmt, ...)
{
	// make a query string
	va_list ap;
	va_start(ap, fmt);
	char *sql = sqlite3_vmprintf(fmt, ap);
	va_end(ap);

	// execute the query
	char *errmsg;
	int result = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		string sqlStr = sql;
		sqlite3_free(errmsg);
		sqlite3_free(sql);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sqlStr.c_str());
	}
	sqlite3_free(sql);
}
