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
#include <inttypes.h>

#include "Logger.h"
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "AsuraException.h"
#include "ConfigManager.h"

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

#define TRANSACTION_PREPARE(DB_PATH, SQLITE3_DB_NAME) \
	sqlite3 *SQLITE3_DB_NAME = openDatabase(DB_PATH); \
	try { \
		execSql(SQLITE3_DB_NAME, "BEGIN");

#define TRANSACTION_EXECUTE(SQLITE3_DB_NAME) \
	} catch (...) { \
		execSql(SQLITE3_DB_NAME, "ROLLBACK"); \
		sqlite3_close(SQLITE3_DB_NAME); \
		throw; \
	} \
	execSql(SQLITE3_DB_NAME, "COMMIT"); \
	sqlite3_close(SQLITE3_DB_NAME);

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
string DBAgentSQLite3::getDefaultDBPath(DBDomainId domainId)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	const string &dbDirectory = configMgr->getDatabaseDirectory();
	string dbPath =
	  StringUtils::sprintf("%s/DBAgentSQLite3-%d.db",
	                       dbDirectory.c_str(), domainId);
	return dbPath;
}

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

const string &DBAgentSQLite3::findDBPath(DBDomainId domainId)
{
	string dbPath;
	PrivateContext::lock();
	DBDomainIdPathMapIterator it =
	   PrivateContext::domainIdPathMap.find(domainId);
	if (it == PrivateContext::domainIdPathMap.end()) {
		string path = getDefaultDBPath(domainId);
		pair<DBDomainIdPathMapIterator, bool> result =
		  PrivateContext::domainIdPathMap.insert
		    (pair<DBDomainId,string>(domainId, path));
		it = result.first;
	}
	PrivateContext::unlock();
	return it->second;
}

void DBAgentSQLite3::deleteRows(const string &dbPath,
                                DBAgentDeleteArg &deleteArg)
{
	TRANSACTION_PREPARE(dbPath, db) {
		deleteRows(db, deleteArg);
	} TRANSACTION_EXECUTE(db);
}

DBAgentSQLite3::DBAgentSQLite3(DBDomainId domainId, bool skipSetup)
: DBAgent(domainId, skipSetup),
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

void DBAgentSQLite3::select(DBAgentSelectExArg &selectExArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	select(m_ctx->db, selectExArg);
}

void DBAgentSQLite3::deleteRows(DBAgentDeleteArg &deleteArg)
{
	ASURA_ASSERT(m_ctx->db, "m_ctx->db is NULL");
	deleteRows(m_ctx->db, deleteArg);
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
		string indexName = "mul_index_" + tableCreationArg.tableName;
		createIndex(db, tableCreationArg.tableName,
		            tableCreationArg.columnDefs, indexName,
		            multipleKeyColumnIndexVector, isUniqueKey);
	}

	if (!uniqueKeyColumnIndexVector.empty()) {
		bool isUniqueKey = true;
		string indexName = "uni_index_" + tableCreationArg.tableName;
		createIndex(db, tableCreationArg.tableName,
		            tableCreationArg.columnDefs, indexName,
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
	string sql = "INSERT INTO ";
	sql += insertArg.tableName;
	sql += " VALUES (";
	for (size_t i = 0; i < numColumns; i++) {
		const ColumnDef &columnDef = insertArg.columnDefs[i];
		const ItemData *itemData = insertArg.row->getItemAt(i);
		sql += getColumnValueString(&columnDef, itemData);
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
		THROW_ASURA_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d, %s",
		  result, sql.c_str());
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

void DBAgentSQLite3::select(sqlite3 *db, DBAgentSelectExArg &selectExArg)
{
	size_t numColumns = selectExArg.statements.size();
	ASURA_ASSERT(numColumns > 0, "Vector size must not be zero");
	ASURA_ASSERT(numColumns == selectExArg.columnTypes.size(),
	             "Vector size mismatch: statements (%zd):columnTypes (%zd)",
	             numColumns, selectExArg.columnTypes.size());

	string sql = "SELECT ";
	for (size_t i = 0; i < numColumns; i++) {
		sql += selectExArg.statements[i];
		if (i < numColumns-1)
			sql += ",";
	}
	sql += " FROM ";
	sql += selectExArg.tableName;
	if (!selectExArg.condition.empty()) {
		sql += " WHERE ";
		sql += selectExArg.condition;
	}
	if (!selectExArg.orderBy.empty()) {
		sql += " ORDER BY ";
		sql += selectExArg.orderBy;
	}
	if (selectExArg.limit > 0)
		sql += StringUtils::sprintf(" LIMIT %zd ", selectExArg.limit);
	if (selectExArg.offset > 0)
		sql += StringUtils::sprintf(" OFFSET %zd ", selectExArg.offset);

	// exectute
	int result;
	sqlite3_stmt *stmt;
	result = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d, %s",
		  result, sql.c_str());
	}

	sqlite3_reset(stmt);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		ItemGroupPtr itemGroup = ItemGroupPtr(new ItemGroup(), false);
		for (size_t index = 0; index < numColumns; index++) {
			ItemDataPtr itemDataPtr =
			  getValue(stmt, index, selectExArg.columnTypes[index]);
			itemGroup->add(itemDataPtr);
		}
		selectExArg.dataTable->add(itemGroup);
	}
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_ASURA_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);

	// check the result
	size_t numTableRows = selectExArg.dataTable->getNumberOfRows();
	size_t numTableColumns = selectExArg.dataTable->getNumberOfColumns();
	ASURA_ASSERT((numTableRows == 0) ||
	             ((numTableRows > 0) && (numTableColumns == numColumns)),
	             "Sanity check error: numTableRows: %zd, numTableColumns: "
	             "%zd, numColumns: %zd",
	             numTableRows, numTableColumns, numColumns);
}

void DBAgentSQLite3::deleteRows(sqlite3 *db, DBAgentDeleteArg &deleteArg)
{
	ASURA_ASSERT(!deleteArg.tableName.empty(), "Table name: empty");
	string sql = "DELETE FROM ";
	sql += deleteArg.tableName;
	if (!deleteArg.condition.empty()) {
		sql += " WHERE ";
		sql += deleteArg.condition;
	}
	_execSql(db, sql.c_str());
}

void DBAgentSQLite3::selectGetValuesIteration(DBAgentSelectArg &selectArg,
                                              sqlite3_stmt *stmt)
{
	ItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < selectArg.columnIndexes.size(); i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef = selectArg.columnDefs[idx];
		itemGroup->add(getValue(stmt, i, columnDef.type));
	}
	selectArg.dataTable->add(itemGroup);
}

ItemDataPtr DBAgentSQLite3::getValue(sqlite3_stmt *stmt,
                                     size_t index, SQLColumnType columnType)
{
	sqlite3_int64 int64val;
	const char *str;
	ItemData *itemData;
	switch (columnType) {
	case SQL_COLUMN_TYPE_INT:
		itemData = new ItemInt(sqlite3_column_int(stmt, index));
		break;

	case SQL_COLUMN_TYPE_BIGUINT:
		int64val = sqlite3_column_int64(stmt, index);
		itemData = new ItemUint64(int64val);
		break;

	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
		str = (const char *)sqlite3_column_text(stmt, index);
		itemData  = new ItemString(str);
		break;

	case SQL_COLUMN_TYPE_DOUBLE:
		itemData = new ItemDouble(sqlite3_column_double(stmt, index));
		break;

	default:
		ASURA_ASSERT(false, "Unknown column type: %d", columnType);
	}
	return ItemDataPtr(itemData, false);
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

