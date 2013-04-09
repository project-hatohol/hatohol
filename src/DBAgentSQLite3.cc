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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define MAKE_SQL_STATEMENT_FROM_VAARG(LAST_ARG, STR_NAME) \
string STR_NAME; \
{ \
	va_list ap; \
	va_start(ap, fmt); \
	char *_sql = sqlite3_vmprintf(fmt, ap); \
	STR_NAME = _sql; \
	sqlite3_free(_sql); \
	va_end(ap); \
} \

#define DEFINE_AND_ASSERT(ITEM_DATA, ACTUAL_TYPE, VAR_NAME) \
	ACTUAL_TYPE *VAR_NAME = dynamic_cast<ACTUAL_TYPE *>(ITEM_DATA); \
	ASURA_ASSERT(VAR_NAME != NULL, "Failed to cast: %s -> %s", \
	             DEMANGLED_TYPE_NAME(*ITEM_DATA), #ACTUAL_TYPE); \

static const char *TABLE_NAME_SYSTEM = "system";
static const char *TABLE_NAME_SERVERS = "servers";
static const char *TABLE_NAME_TRIGGERS = "triggers";

const int DBAgentSQLite3::DB_VERSION = 1;

typedef map<DBDomainId, string>     DBDomainIdPathMap;
typedef DBDomainIdPathMap::iterator DBDomainIdPathMapIterator;

struct DBAgentSQLite3::PrivateContext {
	static GMutex            mutex;
	static DBDomainIdPathMap domainIdPathMap;

	string        dbPath;
	sqlite3      *db;

	// methods
	PrivateContext(void)
	: db(NULL)
	{
	}

	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};

GMutex            DBAgentSQLite3::PrivateContext::mutex;
DBDomainIdPathMap DBAgentSQLite3::PrivateContext::domainIdPathMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentSQLite3::defineDBPath(DBDomainId domainId, const string &path)
{
	PrivateContext::lock();
	DBDomainIdPathMapIterator it =
	  PrivateContext::domainIdPathMap.find(domainId);
	if (it != PrivateContext::domainIdPathMap.end()) {
		it->second = path;
		PrivateContext::unlock();
		return;
	}

	pair<DBDomainIdPathMapIterator, bool> result =
	  PrivateContext::domainIdPathMap.insert
	    (pair<DBDomainId, string>(domainId, path));
	PrivateContext::unlock();

	ASURA_ASSERT(result.second,
	  "Failed to insert. Probably domain id (%u) is duplicated", domainId);
}

void DBAgentSQLite3::init(void)
{
	DBAgent::addSetupFunction(DefaultDBDomainId, defaultSetupFunc);
}

void DBAgentSQLite3::defaultSetupFunc(DBDomainId domainId)
{
	// We don't lock DB (use transaction) in the existence check of
	// a table and create it, because, this function is called in series.

	// search dbPath
	const string &dbPath = findDBPath(domainId);

	// TODO: check the DB version.
	//       If the DB version is old, update the content
	if (!isTableExisting(dbPath, TABLE_NAME_SYSTEM))
		createTableSystem(dbPath);
	else
		updateDBIfNeeded(dbPath);


	// check the servers table
	if (!isTableExisting(dbPath, TABLE_NAME_SERVERS))
		createTableServers(dbPath);

	// check the trigger table
	if (!isTableExisting(dbPath, TABLE_NAME_TRIGGERS))
		createTableTriggers(dbPath);
}

const string &DBAgentSQLite3::findDBPath(DBDomainId domainId)
{
	PrivateContext::lock();
	DBDomainIdPathMapIterator it =
	   PrivateContext::domainIdPathMap.find(domainId);
	PrivateContext::unlock();
	ASURA_ASSERT(it != PrivateContext::domainIdPathMap.end(),
	             "Not found DBPath: %u", domainId);
	return it->second;
}

bool DBAgentSQLite3::isTableExisting(const string &dbPath,
                                     const string &tableName)
{
	bool exist;
	sqlite3 *db = openDatabase(dbPath);
	try {
		execSql(db, "BEGIN");
		exist = isTableExisting(db, tableName);
	} catch (...) {
		execSql(db, "ROLLBACK");
		sqlite3_close(db);
		throw;
	}
	execSql(db, "COMMIT");
	sqlite3_close(db);
	return exist;
}

void DBAgentSQLite3::createTable(const string &dbPath,
                                 DBAgentTableCreationArg &tableCreationArg)
{
	sqlite3 *db = openDatabase(dbPath);
	try {
		execSql(db, "BEGIN");
		createTable(db, tableCreationArg);
	} catch (...) {
		execSql(db, "ROLLBACK");
		sqlite3_close(db);
		throw;
	}
	execSql(db, "COMMIT");
	sqlite3_close(db);
}

void DBAgentSQLite3::insert(const string &dbPath, DBAgentInsertArg &insertArg)
{
	sqlite3 *db = openDatabase(dbPath);
	try {
		execSql(db, "BEGIN");
		insert(db, insertArg);
	} catch (...) {
		execSql(db, "ROLLBACK");
		sqlite3_close(db);
		throw;
	}
	execSql(db, "COMMIT");
	sqlite3_close(db);
}

void DBAgentSQLite3::update(const string &dbPath, DBAgentUpdateArg &updateArg)
{
	sqlite3 *db = openDatabase(dbPath);
	try {
		execSql(db, "BEGIN");
		update(db, updateArg);
	} catch (...) {
		execSql(db, "ROLLBACK");
		sqlite3_close(db);
		throw;
	}
	execSql(db, "COMMIT");
	sqlite3_close(db);
}

void DBAgentSQLite3::select(const string &dbPath, DBAgentSelectArg &selectArg)
{
	sqlite3 *db = openDatabase(dbPath);
	try {
		execSql(db, "BEGIN");
		select(db, selectArg);
	} catch (...) {
		execSql(db, "ROLLBACK");
		sqlite3_close(db);
		throw;
	}
	execSql(db, "COMMIT");
	sqlite3_close(db);
}

DBAgentSQLite3::DBAgentSQLite3(DBDomainId domainId)
: DBAgent(domainId),
  m_ctx(NULL)
{
	// We don't lock DB (use transaction) in the existence check of
	m_ctx = new PrivateContext();
	m_ctx->dbPath = findDBPath(domainId);
	openDatabase();
}

DBAgentSQLite3::~DBAgentSQLite3()
{
	if (!m_ctx)
		return;

	if (m_ctx->db) {
		int result = sqlite3_close(m_ctx->db);
		if (result != SQLITE_OK) {
			// Should we throw an exception ?
			MLPL_ERR("Failed to close sqlite: %d\n", result);
		}
	}

	delete m_ctx;
}

bool DBAgentSQLite3::isTableExisting(const string &tableName)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	return isTableExisting(m_ctx->db, tableName);
}

bool DBAgentSQLite3::isRecordExisting(const string &tableName,
                                      const string &condition)
{
	int result;
	sqlite3_stmt *stmt;
	string query = StringUtils::sprintf(
	                 "SELECT * FROM %s WHERE %s",
	                 tableName.c_str(), condition.c_str());
	result = sqlite3_prepare(m_ctx->db, query.c_str(), query.size(),
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
	execSql("BEGIN");

	int result;
	sqlite3_stmt *stmt;
	string query = "SELECT id, type, hostname, ip_address, nickname FROM ";
	query += TABLE_NAME_SERVERS;
	result = sqlite3_prepare(m_ctx->db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}
	sqlite3_reset(stmt);
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		monitoringServers.push_back(MonitoringServerInfo());
		MonitoringServerInfo &svInfo = monitoringServers.back();
		svInfo.id   = sqlite3_column_int(stmt, 0);
		svInfo.type = (MonitoringSystemType)sqlite3_column_int(stmt, 1);
		svInfo.hostName  = (const char *)sqlite3_column_text(stmt, 2);
		svInfo.ipAddress = (const char *)sqlite3_column_text(stmt, 3);
		svInfo.nickname  = (const char *)sqlite3_column_text(stmt, 4);
	}
	sqlite3_finalize(stmt);
	if (result != SQLITE_DONE) {
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}

	execSql("COMMIT");
}

void DBAgentSQLite3::addTriggerInfo(TriggerInfo *triggerInfo)
{
	string condition = StringUtils::sprintf("id=%"PRIu64, triggerInfo->id);
	execSql("BEGIN");
	if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
		execSql("INSERT INTO %s VALUES(%lu,%d,%d,%d,%d,%u,%Q,%Q,%Q)",
		        TABLE_NAME_TRIGGERS, triggerInfo->id,
		        triggerInfo->status, triggerInfo->severity,
		        triggerInfo->lastChangeTime.tv_sec, 
		        triggerInfo->lastChangeTime.tv_nsec, 
		        triggerInfo->serverId,
		        triggerInfo->hostId.c_str(),
		        triggerInfo->hostName.c_str(),
		        triggerInfo->brief.c_str());
	} else {
		execSql("UPDATE %s SET status=%d, severity=%d, "
		        "last_change_time_sec=%d, last_change_time_ns=%d, "
		        "server_id=%u, host_id=%Q, host_name=%Q, brief=%Q",
		        TABLE_NAME_TRIGGERS,
		        triggerInfo->status, triggerInfo->severity,
		        triggerInfo->lastChangeTime.tv_sec, 
		        triggerInfo->lastChangeTime.tv_nsec, 
		        triggerInfo->serverId,
		        triggerInfo->hostId.c_str(),
		        triggerInfo->hostName.c_str(),
		        triggerInfo->brief.c_str());
	}
	execSql("COMMIT");
}

void DBAgentSQLite3::getTriggerInfoList(TriggerInfoList &triggerInfoList)
{
	execSql("BEGIN");

	int result;
	sqlite3_stmt *stmt;
	string query = "SELECT id, status, severity, last_change_time_sec, "
	               "last_change_time_ns, server_id, host_id, host_name, "
	               "brief FROM ";
	query += TABLE_NAME_TRIGGERS;
	result = sqlite3_prepare(m_ctx->db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}
	sqlite3_reset(stmt);
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		triggerInfoList.push_back(TriggerInfo());
		TriggerInfo &trigInfo = triggerInfoList.back();
		trigInfo.id   = sqlite3_column_int(stmt, 0);
		trigInfo.status =
		   (TriggerStatusType)sqlite3_column_int(stmt, 1);
		trigInfo.severity =
		   (TriggerSeverityType)sqlite3_column_int(stmt, 2);
		trigInfo.lastChangeTime.tv_sec  = sqlite3_column_int(stmt, 3);
		trigInfo.lastChangeTime.tv_nsec = sqlite3_column_int(stmt, 4);
		trigInfo.serverId = sqlite3_column_int(stmt, 5);
		trigInfo.hostId   = (const char *)sqlite3_column_text(stmt, 6);
		trigInfo.hostName = (const char *)sqlite3_column_text(stmt, 7);
		trigInfo.brief    = (const char *)sqlite3_column_text(stmt, 8);
	}
	sqlite3_finalize(stmt);
	if (result != SQLITE_DONE) {
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}

	execSql("COMMIT");
}

int DBAgentSQLite3::getDBVersion(void)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	return getDBVersion(m_ctx->db);
}

void DBAgentSQLite3::begin(void)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	_execSql(m_ctx->db, "BEGIN");
}

void DBAgentSQLite3::commit(void)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	_execSql(m_ctx->db, "COMMIT");
}

void DBAgentSQLite3::rollback(void)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	_execSql(m_ctx->db, "ROLLBACK");
}

void DBAgentSQLite3::createTable(DBAgentTableCreationArg &tableCreationArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	createTable(m_ctx->db,tableCreationArg);
}

void DBAgentSQLite3::insert(DBAgentInsertArg &insertArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	insert(m_ctx->db, insertArg);
}

void DBAgentSQLite3::update(DBAgentUpdateArg &updateArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	update(m_ctx->db, updateArg);
}

void DBAgentSQLite3::select(DBAgentSelectArg &selectArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	select(m_ctx->db, selectArg);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
sqlite3 *DBAgentSQLite3::openDatabase(const string &dbPath)
{
	sqlite3 *db = NULL;
	int result = sqlite3_open_v2(dbPath.c_str(), &db,
	                             SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,
	                             NULL);
	if (result != SQLITE_OK) {
		THROW_ASURA_EXCEPTION("Failed to open sqlite: %d, %s",
		                      result, dbPath.c_str());
	}
	return db;
}

void DBAgentSQLite3::_execSql(sqlite3 *db, const string &sql)
{
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::execSql(sqlite3 *db, const char *fmt, ...)
{
	// make a query string
	MAKE_SQL_STATEMENT_FROM_VAARG(fmt, sql);

	// execute the query
	_execSql(db, sql);
}

string DBAgentSQLite3::getColumnValueString(const ColumnDef *columnDef,
                                            const ItemData *itemData)
{
	string valueStr;
	switch (columnDef->type) {
	case SQL_COLUMN_TYPE_INT:
	{
		DEFINE_AND_ASSERT(itemData, const ItemInt, item);
		valueStr = StringUtils::sprintf("%d", item->get());
		break;
	}
	case SQL_COLUMN_TYPE_BIGUINT:
	{
		DEFINE_AND_ASSERT(itemData, const ItemUint64, item);
		valueStr = StringUtils::sprintf("%"PRId64, item->get());
		break;
	}
	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
	{
		DEFINE_AND_ASSERT(itemData, const ItemString, item);
		char *str = sqlite3_mprintf("%Q", item->get().c_str());
		valueStr = str;
		sqlite3_free(str);
		break;
	}
	case SQL_COLUMN_TYPE_DOUBLE:
	{
		string fmt;
		DEFINE_AND_ASSERT(itemData, const ItemDouble, item);
		fmt = StringUtils::sprintf("%%%d.%dlf",
		                           columnDef->columnLength,
		                           columnDef->decFracLength);
		valueStr = StringUtils::sprintf(fmt.c_str(), item->get());
		break;
	}
	default:
		ASURA_ASSERT(true, "Unknown column type: %d (%s)",
		             columnDef->type, columnDef->columnName);
	}
	return valueStr;
}

int DBAgentSQLite3::getDBVersion(const string &dbPath)
{
	int version;
	sqlite3 *db = openDatabase(dbPath);
	try {
		version = getDBVersion(db);
	} catch (...) {
		sqlite3_close(db);
		throw;
	}
	sqlite3_close(db);
	return version;
}

int DBAgentSQLite3::getDBVersion(sqlite3 *db)
{
	int result;
	sqlite3_stmt *stmt;
	string query = "SELECT version FROM ";
	query += TABLE_NAME_SYSTEM;
	result = sqlite3_prepare(db, query.c_str(), query.size(),
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

bool DBAgentSQLite3::isTableExisting(sqlite3 *db,
                                     const string &tableName)
{
	int result;
	sqlite3_stmt *stmt;
	const char *query = "SELECT COUNT(*) FROM sqlite_master "
	                    "WHERE type='table' AND name=?";
	result = sqlite3_prepare(db, query, strlen(query), &stmt, NULL);
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

void DBAgentSQLite3::createTable(sqlite3 *db,
                                 DBAgentTableCreationArg &tableCreationArg)
{
	vector<size_t> multipleKeyColumnIndexVector;
	vector<size_t> uniqueKeyColumnIndexVector;

	// make a SQL statement
	string sql = "CREATE TABLE ";
	sql += tableCreationArg.tableName;
	sql += "(";
	for (size_t i = 0; i < tableCreationArg.numColumns; i++) {
		const ColumnDef &columnDef = tableCreationArg.columnDefs[i];

		// set type
		sql += columnDef.columnName;
		sql += " ";
		switch (columnDef.type) {
		case SQL_COLUMN_TYPE_INT:
		case SQL_COLUMN_TYPE_BIGUINT:
			sql += "INTEGER";
			break;
		case SQL_COLUMN_TYPE_VARCHAR:
		case SQL_COLUMN_TYPE_CHAR:
		case SQL_COLUMN_TYPE_TEXT:
			sql += "TEXT";
			break;
		case SQL_COLUMN_TYPE_DOUBLE:
			sql += "REAL";
			break;
		default:
			ASURA_ASSERT(true, "Unknown column type: %d (%s)",
			             columnDef.type, columnDef.columnName);
		}
		sql += " ";

		// set key
		switch (columnDef.keyType) {
		case SQL_KEY_PRI:
			sql += "PRIMARY KEY";
			break;
		case SQL_KEY_MUL:
			multipleKeyColumnIndexVector.push_back(i);
			break;
		case SQL_KEY_UNI:
			uniqueKeyColumnIndexVector.push_back(i);
			break;
		case SQL_KEY_NONE:
			break;
		default:
			ASURA_ASSERT(true, "Unknown column type: %d (%s)",
			             columnDef.keyType, columnDef.columnName);
		}

		if (i < tableCreationArg.numColumns - 1)
			sql += ",";
	}
	sql += ")";

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	// add indexes
	if (!multipleKeyColumnIndexVector.empty()) {
		bool isUniqueKey = false;
		createIndex(db, tableCreationArg.tableName,
		            tableCreationArg.columnDefs, "multiple_index",
		            multipleKeyColumnIndexVector, isUniqueKey);
	}

	if (!uniqueKeyColumnIndexVector.empty()) {
		bool isUniqueKey = true;
		createIndex(db, tableCreationArg.tableName,
		            tableCreationArg.columnDefs, "unique_index",
		            uniqueKeyColumnIndexVector, isUniqueKey);
	}
}

void DBAgentSQLite3::insert(sqlite3 *db, DBAgentInsertArg &insertArg)
{
	size_t numColumns = insertArg.row->getNumberOfItems();
	ASURA_ASSERT(numColumns == insertArg.numColumns,
	             "Invalid number of colums: %zd, %zd",
	             numColumns, insertArg.numColumns);

	// make a SQL statement
	char *_sql;
	string fmt;
	string sql = "INSERT INTO ";
	sql += insertArg.tableName;
	sql += " VALUES (";
	for (size_t i = 0; i < numColumns; i++) {
		const ColumnDef &columnDef = insertArg.columnDefs[i];
		const ItemData *itemData = insertArg.row->getItemAt(i);

		// set type
		switch (columnDef.type) {
		case SQL_COLUMN_TYPE_INT:
		{
			DEFINE_AND_ASSERT(itemData, const ItemInt, item);
			sql += StringUtils::sprintf("%d", item->get());
			break;
		}
		case SQL_COLUMN_TYPE_BIGUINT:
		{
			DEFINE_AND_ASSERT(itemData, const ItemUint64, item);
			sql += StringUtils::sprintf("%"PRId64, item->get());
			break;
		}
		case SQL_COLUMN_TYPE_VARCHAR:
		case SQL_COLUMN_TYPE_CHAR:
		case SQL_COLUMN_TYPE_TEXT:
		{
			DEFINE_AND_ASSERT(itemData, const ItemString, item);
			_sql = sqlite3_mprintf("%Q", item->get().c_str());
			sql += _sql;
			sqlite3_free(_sql);
			break;
		}
		case SQL_COLUMN_TYPE_DOUBLE:
		{
			DEFINE_AND_ASSERT(itemData, const ItemDouble, item);
			fmt = StringUtils::sprintf("%%%d.%dlf",
			                           columnDef.columnLength,
			                           columnDef.decFracLength);
			sql += StringUtils::sprintf(fmt.c_str(), item->get());
			break;
		}
		default:
			ASURA_ASSERT(true, "Unknown column type: %d (%s)",
			             columnDef.type, columnDef.columnName);
		}
		sql += " ";

		if (i < numColumns-1)
			sql += ",";
	}
	sql += ")";

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::update(sqlite3 *db, DBAgentUpdateArg &updateArg)
{
	size_t numColumns = updateArg.row->getNumberOfItems();
	ASURA_ASSERT(numColumns == updateArg.columnIndexes.size(),
	             "Invalid number of colums: %zd, %zd",
	             numColumns, updateArg.columnIndexes.size());

	// make a SQL statement
	string fmt;
	string sql = "UPDATE ";
	sql += updateArg.tableName;
	sql += " SET ";
	for (size_t i = 0; i < numColumns; i++) {
		size_t columnIdx = updateArg.columnIndexes[i];
		const ColumnDef &columnDef = updateArg.columnDefs[columnIdx];
		const ItemData *itemData = updateArg.row->getItemAt(i);
		string valueStr = getColumnValueString(&columnDef, itemData);

		sql += columnDef.columnName;
		sql += "=";
		sql += valueStr;
		if (i < numColumns-1)
			sql += ",";
	}

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::select(sqlite3 *db, DBAgentSelectArg &selectArg)
{
	string sql = "SELECT ";
	for (size_t i = 0; i < selectArg.columnIndexes.size(); i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef = selectArg.columnDefs[idx];
		sql += columnDef.columnName;
		sql += " ";
		if (i < selectArg.columnIndexes.size()- 1)
			sql += ",";
	}
	sql += "FROM ";
	sql += selectArg.tableName;

	// exectute
	int result;
	sqlite3_stmt *stmt;
	result = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}

	sqlite3_reset(stmt);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
		selectGetValuesIteration(selectArg, stmt);
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
}

void DBAgentSQLite3::selectGetValuesIteration(DBAgentSelectArg &selectArg,
                                              sqlite3_stmt *stmt)
{
	sqlite3_int64 int64val;
	const char *str;
	ItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < selectArg.columnIndexes.size(); i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef = selectArg.columnDefs[idx];
		switch (columnDef.type) {
		case SQL_COLUMN_TYPE_INT:
			itemGroup->add(
			  new ItemInt(sqlite3_column_int(stmt, i)), false);
			break;

		case SQL_COLUMN_TYPE_BIGUINT:
			int64val = sqlite3_column_int64(stmt, i);
			itemGroup->add(new ItemUint64(int64val), false);
			break;

		case SQL_COLUMN_TYPE_VARCHAR:
		case SQL_COLUMN_TYPE_CHAR:
		case SQL_COLUMN_TYPE_TEXT:
			str = (const char *)sqlite3_column_text(stmt, i);
			itemGroup->add(new ItemString(str), false);
			break;

		case SQL_COLUMN_TYPE_DOUBLE:
			itemGroup->add(
			  new ItemDouble(sqlite3_column_double(stmt, i)),
			  false);
			break;

		default:
			THROW_ASURA_EXCEPTION("Unknown column type: %d",
		                              columnDef.type);
		}
	}
	selectArg.dataTable->add(itemGroup);
}

void DBAgentSQLite3::updateDBIfNeeded(const string &dbPath)
{
	if (getDBVersion(dbPath) == DB_VERSION)
		return;
	THROW_ASURA_EXCEPTION("Not implemented: %s", __PRETTY_FUNCTION__);
}

void DBAgentSQLite3::createTableSystem(const string &dbPath)
{
	sqlite3 *db = openDatabase(dbPath);

	// make table
	string sql = "CREATE TABLE ";
	sql += TABLE_NAME_SYSTEM;
	sql += "(version INTEGER)";
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		sqlite3_close(db);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	// insert the version
	sql = StringUtils::sprintf("INSERT INTO %s VALUES(%d)",
	                           TABLE_NAME_SYSTEM,  DB_VERSION);
	result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		sqlite3_close(db);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	sqlite3_close(db);
}

void DBAgentSQLite3::createTableServers(const string &dbPath)
{
	sqlite3 *db = openDatabase(dbPath);

	// make table
	string sql = "CREATE TABLE ";
	sql += TABLE_NAME_SERVERS;
	sql += "(id INTEGER PRIMARY KEY, type INTEGER, hostname TEXT, "
	       " ip_address TEXT, nickname TEXT)";
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		sqlite3_close(db);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	sqlite3_close(db);
}

void DBAgentSQLite3::createTableTriggers(const string &dbPath)
{
	sqlite3 *db = openDatabase(dbPath);

	string sql = "CREATE TABLE ";
	sql += TABLE_NAME_TRIGGERS;
	sql += "(id INTEGER PRIMARY KEY, status INTEGER, severity INTEGER, "
	       " last_change_time_sec INTEGER, last_change_time_ns INTERGET, "
	       " server_id INTEGER, host_id TEXT, host_name TEXT, brief TEXT)";
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		sqlite3_close(db);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}

	sqlite3_close(db);
}

void DBAgentSQLite3::createIndex(sqlite3 *db, const string &tableName, 
                                 const ColumnDef *columnDefs,
                                 const string &indexName,
                                 const vector<size_t> &targetIndexes,
                                 bool isUniqueKey)
{
	ASURA_ASSERT(!targetIndexes.empty(), "target indexes vector is empty.");

	// make an SQL statement
	string sql = "CREATE ";
	if (isUniqueKey)
		sql += "UNIQUE ";
	sql += "INDEX ";
	sql += indexName;
	sql += " ON ";
	sql += tableName;
	sql += "(";
	for (size_t i = 0; i < targetIndexes.size(); i++) {
		const ColumnDef &columnDef = columnDefs[i];
		sql += columnDef.columnName;
		if (i < targetIndexes.size() - 1)
			sql += ",";
	}
	sql += ")";

	// execute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_ASURA_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

//
// Non static methods
//
void DBAgentSQLite3::openDatabase(void)
{
	if (m_ctx->db)
		return;

	ASURA_ASSERT(!m_ctx->dbPath.empty(), "dbPath is empty.");
	m_ctx->db = openDatabase(m_ctx->dbPath);
}

void DBAgentSQLite3::execSql(const char *fmt, ...)
{
	// make a query string
	MAKE_SQL_STATEMENT_FROM_VAARG(fmt, sql);

	// execute the query
	_execSql(m_ctx->db, sql);
}

