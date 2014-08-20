/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <cstring>
#include <cstdio>
#include <stdarg.h>
#include <inttypes.h>
#include <gio/gio.h>
#include <Mutex.h>
#include <Logger.h>
#include <SeparatorInjector.h>
using namespace std;
using namespace mlpl;

#include "DBAgentSQLite3.h"
#include "HatoholException.h"
#include "ConfigManager.h"

const static int TRANSACTION_TIME_OUT_MSEC = 30 * 1000;

class DBTermCodecSQLite3 : public DBTermCodec {
	virtual string enc(const uint64_t &val) const override
	{
		return StringUtils::sprintf("%" PRId64, val);
	}
};

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

typedef map<DBDomainId, string>     DBDomainIdPathMap;
typedef DBDomainIdPathMap::iterator DBDomainIdPathMapIterator;

struct DBAgentSQLite3::Impl {
	static Mutex             mutex;
	static DBDomainIdPathMap domainIdPathMap;
	static DBTermCodecSQLite3 dbTermCodec;

	string        dbPath;
	sqlite3      *db;

	// methods
	Impl(void)
	: db(NULL)
	{
	}

	~Impl(void)
	{
		if (!db)
			return;
		int result = sqlite3_close(db);
		if (result != SQLITE_OK) {
			// Should we throw an exception ?
			MLPL_ERR("Failed to close sqlite: %d\n", result);
		}
	}

	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}

	static bool isDBPathDefined(DBDomainId domainId)
	{
		lock();
		DBDomainIdPathMapIterator it = domainIdPathMap.find(domainId);
		bool defined = (it != domainIdPathMap.end());
		unlock();
		return defined;
	}
};

Mutex             DBAgentSQLite3::Impl::mutex;
DBDomainIdPathMap DBAgentSQLite3::Impl::domainIdPathMap;
DBTermCodecSQLite3 DBAgentSQLite3::Impl::dbTermCodec;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentSQLite3::init(void)
{
	// Currently ConfigManger doesn't have init(), so calling
	// the following functions and getDefaultDBPath() that use
	// ConfigManager is no problem.
	// However, if we add ConfigManager::init() in the future,
	// The order of ConfigManager::init() and DBAgentSQLite3::init() has
	// be well considered.
	ConfigManager *configMgr = ConfigManager::getInstance();
	const string &dbDirectory = configMgr->getDatabaseDirectory();
	checkDBPath(dbDirectory);

	string dbPath = getDefaultDBPath(DEFAULT_DB_DOMAIN_ID);
	defineDBPath(DEFAULT_DB_DOMAIN_ID, dbPath);
}

void DBAgentSQLite3::reset(void)
{
	Impl::domainIdPathMap.clear();
}

bool DBAgentSQLite3::defineDBPath(DBDomainId domainId, const string &path,
                                  bool allowOverwrite)
{
	bool ret = true;
	Impl::lock();
	DBDomainIdPathMapIterator it =
	  Impl::domainIdPathMap.find(domainId);
	if (it != Impl::domainIdPathMap.end()) {
		if (allowOverwrite)
			it->second = path;
		else
			ret = false;
		Impl::unlock();
		return ret;
	}

	pair<DBDomainIdPathMapIterator, bool> result =
	  Impl::domainIdPathMap.insert
	    (pair<DBDomainId, string>(domainId, path));
	Impl::unlock();

	HATOHOL_ASSERT(result.second,
	  "Failed to insert. Probably domain id (%u) is duplicated", domainId);
	return ret;
}

string &DBAgentSQLite3::getDBPath(DBDomainId domainId)
{
	string dbPath;
	Impl::lock();
	DBDomainIdPathMapIterator it =
	   Impl::domainIdPathMap.find(domainId);
	if (it == Impl::domainIdPathMap.end()) {
		string path = getDefaultDBPath(domainId);
		pair<DBDomainIdPathMapIterator, bool> result =
		  Impl::domainIdPathMap.insert
		    (pair<DBDomainId,string>(domainId, path));
		it = result.first;
	}
	Impl::unlock();
	return it->second;
}

const DBTermCodec *DBAgentSQLite3::getDBTermCodecStatic(void)
{
	return &Impl::dbTermCodec;
}

DBAgentSQLite3::DBAgentSQLite3(const string &dbName,
                               DBDomainId domainId, bool skipSetup)
: DBAgent(domainId, skipSetup),
  m_impl(new Impl())
{
	if (!dbName.empty() && !Impl::isDBPathDefined(domainId)) {
		string dbPath = makeDBPathFromName(dbName);
		const bool allowOverwrite = false;
		defineDBPath(domainId, dbPath, allowOverwrite);
	}
	m_impl->dbPath = getDBPath(domainId);
	openDatabase();
}

DBAgentSQLite3::~DBAgentSQLite3()
{
}

void DBAgentSQLite3::getIndexes(vector<IndexStruct> &indexStructVect,
                                const string &tableName)
{
	string query = StringUtils::sprintf(
	  "SELECT name,tbl_name,sql FROM sqlite_master "
	  "WHERE type='index' and tbl_name='%s';",
	  tableName.c_str());

	sqlite3_stmt *stmt;
	int result;
	result = sqlite3_prepare(m_impl->db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d: %s",
		  result, query.c_str());
	}
	sqlite3_reset(stmt);
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		size_t idx = 0;
		IndexStruct idxStruct;
		idxStruct.name =
		  (const char *)sqlite3_column_text(stmt, idx++);
		idxStruct.tableName =
		  (const char *)sqlite3_column_text(stmt, idx++);
		idxStruct.sql =
		  (const char *)sqlite3_column_text(stmt, idx++);
		indexStructVect.push_back(idxStruct);
	}
	sqlite3_finalize(stmt);
}

bool DBAgentSQLite3::isTableExisting(const string &tableName)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	return isTableExisting(m_impl->db, tableName);
}

bool DBAgentSQLite3::isRecordExisting(const string &tableName,
                                      const string &condition)
{
	int result;
	sqlite3_stmt *stmt;
	string query = StringUtils::sprintf(
	                 "SELECT * FROM %s WHERE %s",
	                 tableName.c_str(), condition.c_str());
	result = sqlite3_prepare(m_impl->db, query.c_str(), query.size(),
	                         &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION(
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
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	return found;
}

void DBAgentSQLite3::begin(void)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	_execSql(m_impl->db, "BEGIN IMMEDIATE");
}

void DBAgentSQLite3::commit(void)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	_execSql(m_impl->db, "COMMIT");
}

void DBAgentSQLite3::rollback(void)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	_execSql(m_impl->db, "ROLLBACK");
}

void DBAgentSQLite3::execSql(const string &sql)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	_execSql(m_impl->db, sql);
}

void DBAgentSQLite3::createTable(const TableProfile &tableProfile)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	createTable(m_impl->db, tableProfile);
}

void DBAgentSQLite3::insert(const DBAgent::InsertArg &insertArg)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	insert(m_impl->db, insertArg);
}

void DBAgentSQLite3::update(const UpdateArg &updateArg)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	update(m_impl->db, updateArg);
}

void DBAgentSQLite3::select(const SelectArg &selectArg)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	select(m_impl->db, selectArg);
}

void DBAgentSQLite3::select(const SelectExArg &selectExArg)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	select(m_impl->db, selectExArg);
}

void DBAgentSQLite3::deleteRows(const DeleteArg &deleteArg)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	deleteRows(m_impl->db, deleteArg);
}

uint64_t DBAgentSQLite3::getLastInsertId(void)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	return getLastInsertId(m_impl->db);
}

uint64_t DBAgentSQLite3::getNumberOfAffectedRows(void)
{
	HATOHOL_ASSERT(m_impl->db, "m_impl->db is NULL");
	return getNumberOfAffectedRows(m_impl->db);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string DBAgentSQLite3::makeDBPathFromName(const string &name)
{
	ConfigManager *configMgr = ConfigManager::getInstance();
	const string &dbDirectory = configMgr->getDatabaseDirectory();
	string dbPath =
	  StringUtils::sprintf("%s/%s.db", dbDirectory.c_str(), name.c_str());
	return dbPath;
}

string DBAgentSQLite3::getDefaultDBPath(DBDomainId domainId)
{
	string name = StringUtils::sprintf("DBAgentSQLite3-%d", domainId);
	return makeDBPathFromName(name);
}

void DBAgentSQLite3::checkDBPath(const string &dbPath)
{
	GFile *gfile = g_file_new_for_path(dbPath.c_str());

	// check if the specified path is directory
	GFileType type =
	   g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, NULL);
	if (type != G_FILE_TYPE_UNKNOWN && type != G_FILE_TYPE_DIRECTORY) {
		g_object_unref(gfile);
		THROW_HATOHOL_EXCEPTION("Specified dir is not directory:%s\n",
		                      dbPath.c_str());
	}

	// try to the directory if it doesn't exist
	if (type == G_FILE_TYPE_UNKNOWN) {
		GError *error = NULL;
		gboolean successed =
		  g_file_make_directory_with_parents(gfile, NULL, &error);
		if (!successed) {
			string msg = error->message;
			g_error_free(error);
			g_object_unref(gfile);
			THROW_HATOHOL_EXCEPTION(
			  "Failed to create dir. for DB: %s: %s\n",
			  dbPath.c_str(), msg.c_str());
		}
	}

	// Should we check if we can write on the directory ?

	g_object_unref(gfile);
}

sqlite3 *DBAgentSQLite3::openDatabase(const string &dbPath)
{
	sqlite3 *db = NULL;
	int result = sqlite3_open_v2(dbPath.c_str(), &db,
	                             SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,
	                             NULL);
	if (result != SQLITE_OK) {
		THROW_HATOHOL_EXCEPTION("Failed to open sqlite: %d, %s",
		                      result, dbPath.c_str());
	}
	sqlite3_busy_timeout(db, TRANSACTION_TIME_OUT_MSEC);
	return db;
}

void DBAgentSQLite3::_execSql(sqlite3 *db, const string &sql)
{
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_HATOHOL_EXCEPTION("Failed to exec: %d, %s, %s",
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
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_prepare(): %d",
		                      result);
	}

	sqlite3_reset(stmt);
	result = sqlite3_bind_text(stmt, 1, tableName.c_str(),
	                           -1, SQLITE_STATIC);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	
	int count = 0;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	}
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
	return count > 0;
}

struct KeyColumnInfo {
	bool   isUnique;
	size_t columnIndex;
};

void DBAgentSQLite3::createTable(sqlite3 *db, const TableProfile &tableProfile)
{
	// make a SQL statement
	string sql = "CREATE TABLE ";
	sql += tableProfile.name;
	sql += "(";
	for (size_t i = 0; i < tableProfile.numColumns; i++) {
		const ColumnDef &columnDef = tableProfile.columnDefs[i];

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
			HATOHOL_ASSERT(true, "Unknown column type: %d (%s)",
			             columnDef.type, columnDef.columnName);
		}
		sql += " ";

		// set key
		switch (columnDef.keyType) {
		case SQL_KEY_PRI:
			sql += "PRIMARY KEY";
			if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC)
				sql += " AUTOINCREMENT";
			break;
		case SQL_KEY_UNI:
		case SQL_KEY_IDX:
		case SQL_KEY_NONE:
			// The index is created in fixupIndexes() for columns
			// with these flags.
			break;
		default:
			HATOHOL_ASSERT(true, "Unknown column type: %d (%s)",
			             columnDef.keyType, columnDef.columnName);
		}

		if (i < tableProfile.numColumns - 1)
			sql += ",";
	}
	sql += ")";

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_HATOHOL_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

static bool isPrimaryOrUniqueKeyDuplicated(sqlite3 *db)
{
#if !defined(SQLITE_CONSTRAINT_PRIMARYKEY) || !defined(SQLITE_CONSTRAINT_UNIQUE)
// This is just pass the build. For example, for TravisCI (12.04)
#warning "SQLITE_CONSTRAINT_PRIMARYKEY and/or SQLITE_CONSTRAINT_UNIQUE: not defined. This program may not work properly."
	return true;
#else
	int extErrCode = sqlite3_extended_errcode(db);
	if (extErrCode == SQLITE_CONSTRAINT_PRIMARYKEY ||
	    extErrCode == SQLITE_CONSTRAINT_UNIQUE) {
		return true;
	}
	return false;
#endif
}

void DBAgentSQLite3::insert(sqlite3 *db, const DBAgent::InsertArg &insertArg)
{
	size_t numColumns = insertArg.row->getNumberOfItems();
	HATOHOL_ASSERT(numColumns == insertArg.tableProfile.numColumns,
	               "Invalid number of colums: %zd, %zd",
	               numColumns, insertArg.tableProfile.numColumns);

	// make a SQL statement
	string sql = "INSERT ";
	sql += "INTO ";
	sql += insertArg.tableProfile.name;
	sql += " VALUES (";
	for (size_t i = 0; i < numColumns; i++) {
		if (i > 0)
			sql += ",";
		const ColumnDef &columnDef =
		  insertArg.tableProfile.columnDefs[i];
		const ItemData *itemData = insertArg.row->getItemAt(i);
		string valueStr;
		if (itemData->isNull()) {
			valueStr = "NULL";
		} else {
			valueStr = getColumnValueString(&columnDef, itemData);
			if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC) {
				// Converting 0 to NULL makes the behavior
				// compatible with DBAgentMySQL.
				if (valueStr == "0")
					valueStr = "NULL";
			}
		}
		sql += valueStr;
	}
	sql += ")";

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (insertArg.upsertOnDuplicate && result == SQLITE_CONSTRAINT) {
		// Using 'OR REPLACE', we cannot keep the value in
		// an auto-incremented column since SQLite3 once deletes
		// the duplicated row.
		// So we try to update here if 'insert' fails due to
		// primary or unique key constraint.
		if (isPrimaryOrUniqueKeyDuplicated(db)) {
			update(db, insertArg);
			return ;
		}
	}
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_HATOHOL_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::update(sqlite3 *db, const UpdateArg &updateArg)
{
	string sql = makeUpdateStatement(updateArg);

	// exectute the SQL statement
	char *errmsg;
	int result = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
	if (result != SQLITE_OK) {
		string err = errmsg;
		sqlite3_free(errmsg);
		THROW_HATOHOL_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

void DBAgentSQLite3::update(sqlite3 *db, const DBAgent::InsertArg &insertArg)
{
	struct {
		string operator()(
		  const DBAgent::TableProfile &tableProfile,
		  const ItemGroup *itemGroupPtr, const int &idx)
		{
			using StringUtils::sprintf;
			const ColumnDef *columnDef;
			const ItemData  *itemData;
			string columnName, valStr;

			columnDef = &tableProfile.columnDefs[idx];
			itemData = itemGroupPtr->getItemAt(idx);
			valStr = getColumnValueString(columnDef, itemData);
			return sprintf("%s=%s", columnDef->columnName,
			                        valStr.c_str());
		}
	} makeKeyValueEquation;

	const DBAgent::TableProfile &tableProfile = insertArg.tableProfile;
	DBAgent::UpdateArg arg(tableProfile);
	bool primaryKeyIsAutoIncVal = false;
	int  primaryKeyColumnIndex = -1;
	for (size_t i = 0; i < tableProfile.numColumns; i++) {
		if (tableProfile.columnDefs[i].keyType == SQL_KEY_PRI) {
			const ItemData *item = insertArg.row->getItemAt(i);
			primaryKeyIsAutoIncVal = isValueAutoIncrement(item, i);
			primaryKeyColumnIndex = i;
			if (primaryKeyIsAutoIncVal)
				continue;
		}
		arg.add(i, insertArg.row);
	}

	string cond;
	if (primaryKeyIsAutoIncVal || primaryKeyColumnIndex == -1) {
		SeparatorInjector andInjector(" AND ");
		for (size_t i = 0;
		     i < tableProfile.uniqueKeyColumnIndexes.size(); i++) {
			const int idx = tableProfile.uniqueKeyColumnIndexes[i];
			andInjector(cond);
			cond += makeKeyValueEquation(tableProfile,
			                             insertArg.row, idx);
		}
	} else if (primaryKeyColumnIndex >= 0) {
		cond = makeKeyValueEquation(tableProfile, insertArg.row,
		                            primaryKeyColumnIndex);
	}
	arg.condition = cond;

	update(db, arg);
}

void DBAgentSQLite3::select(sqlite3 *db, const SelectArg &selectArg)
{
	string sql = makeSelectStatement(selectArg);

	// exectute
	int result;
	sqlite3_stmt *stmt;
	result = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d, %s",
		  result, sql.c_str());
	}

	sqlite3_reset(stmt);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	
	VariableItemTablePtr dataTable;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
		selectGetValuesIteration(selectArg, stmt, dataTable);
	selectArg.dataTable = dataTable;
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);
}

void DBAgentSQLite3::select(sqlite3 *db, const SelectExArg &selectExArg)
{
	string sql = makeSelectStatement(selectExArg);

	// exectute
	int result;
	sqlite3_stmt *stmt;
	result = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, NULL);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call sqlite3_prepare(): %d, %s",
		  result, sql.c_str());
	}

	sqlite3_reset(stmt);
	if (result != SQLITE_OK) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_bind(): %d",
		                      result);
	}
	size_t numColumns = selectExArg.statements.size();
	VariableItemTablePtr dataTable;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		VariableItemGroupPtr itemGroup;
		for (size_t index = 0; index < numColumns; index++) {
			ItemDataPtr itemDataPtr =
			  getValue(stmt, index, selectExArg.columnTypes[index]);
			itemGroup->add(itemDataPtr);
		}
		dataTable->add(itemGroup);
	}
	selectExArg.dataTable = dataTable;
	if (result != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		THROW_HATOHOL_EXCEPTION("Failed to call sqlite3_step(): %d",
		                      result);
	}
	sqlite3_finalize(stmt);

	// check the result
	size_t numTableRows = selectExArg.dataTable->getNumberOfRows();
	size_t numTableColumns = selectExArg.dataTable->getNumberOfColumns();
	HATOHOL_ASSERT((numTableRows == 0) ||
	             ((numTableRows > 0) && (numTableColumns == numColumns)),
	             "Sanity check error: numTableRows: %zd, numTableColumns: "
	             "%zd, numColumns: %zd",
	             numTableRows, numTableColumns, numColumns);
}

void DBAgentSQLite3::deleteRows(sqlite3 *db, const DeleteArg &deleteArg)
{
	string sql = makeDeleteStatement(deleteArg);
	_execSql(db, sql.c_str());
}

void DBAgentSQLite3::addColumns(const AddColumnsArg &addColumnsArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentSQLite3::renameTable(const string &srcName, const string &destName)
{
	string query = makeRenameTableStatement(srcName, destName);
	execSql(query);
}

const DBTermCodec *DBAgentSQLite3::getDBTermCodec(void) const
{
	return &m_impl->dbTermCodec;
}

void DBAgentSQLite3::selectGetValuesIteration(const SelectArg &selectArg,
                                              sqlite3_stmt *stmt,
                                              VariableItemTablePtr &dataTable)
{
	VariableItemGroupPtr itemGroup;
	for (size_t i = 0; i < selectArg.columnIndexes.size(); i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef =
		  selectArg.tableProfile.columnDefs[idx];
		itemGroup->add(getValue(stmt, i, columnDef.type));
	}
	dataTable->add(itemGroup);
}

uint64_t DBAgentSQLite3::getLastInsertId(sqlite3 *db)
{
	return sqlite3_last_insert_rowid(db);
}

uint64_t DBAgentSQLite3::getNumberOfAffectedRows(sqlite3 *db)
{
	return sqlite3_changes(db);
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
		if (!str)
			str = "";
		itemData = new ItemString(str);
		break;

	case SQL_COLUMN_TYPE_DOUBLE:
		itemData = new ItemDouble(sqlite3_column_double(stmt, index));
		break;

	case SQL_COLUMN_TYPE_DATETIME:
		itemData = new ItemInt(sqlite3_column_int(stmt, index));
		break;

	default:
		HATOHOL_ASSERT(false, "Unknown column type: %d", columnType);
	}

	// check null
	if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
		itemData->setNull();

	return ItemDataPtr(itemData, false);
}

void DBAgentSQLite3::createIndexIfNotExistsEach(
  sqlite3 *db, const TableProfile &tableProfile, const string &indexName,
  const vector<size_t> &targetIndexes, const bool &isUniqueKey)
{
	HATOHOL_ASSERT(!targetIndexes.empty(), "target indexes vector is empty.");

	// make an SQL statement
	string sql = "CREATE ";
	if (isUniqueKey)
		sql += "UNIQUE ";
	sql += "INDEX IF NOT EXISTS ";
	sql += indexName;
	sql += " ON ";
	sql += tableProfile.name;
	sql += "(";
	for (size_t i = 0; i < targetIndexes.size(); i++) {
		const size_t targetIdx = targetIndexes[i];
		const ColumnDef &columnDef = tableProfile.columnDefs[targetIdx];
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
		THROW_HATOHOL_EXCEPTION("Failed to exec: %d, %s, %s",
		                      result, err.c_str(), sql.c_str());
	}
}

string DBAgentSQLite3::makeCreateIndexStatement(
  const TableProfile &tableProfile, const IndexDef &indexDef)
{
	string sql = StringUtils::sprintf(
	  "CREATE %sINDEX i_%s_%s ON %s(",
	  indexDef.isUnique ? "UNIQUE " : "",
	  tableProfile.name, indexDef.name, tableProfile.name);

	const int *columnIdxPtr = indexDef.columnIndexes;
	while (true) {
		const ColumnDef &columnDef =
		  tableProfile.columnDefs[*columnIdxPtr];
		sql += columnDef.columnName;

		// get the next column index and check if it's the end.
		columnIdxPtr++;
		if (*columnIdxPtr != IndexDef::END) {
			sql += ",";
		} else {
			sql += ")";
			break;
		}
	}
	return sql;
}

std::string DBAgentSQLite3::makeDropIndexStatement(
  const std::string &name, const std::string &tableName)
{
	return StringUtils::sprintf("DROP INDEX %s", name.c_str());
}

void DBAgentSQLite3::getIndexInfoVect(
  vector<IndexInfo> &indexInfoVect, const TableProfile &tableProfile)
{
	// TODO: Consider if we should merge this method with getIndexes().
	vector<IndexStruct> indexStructVect;
	getIndexes(indexStructVect, tableProfile.name);
	indexInfoVect.reserve(indexStructVect.size());
	for (size_t i = 0; i < indexStructVect.size(); i++) {
		const IndexStruct idxStruct = indexStructVect[i];
		IndexInfo idxInfo;
		idxInfo.name      = idxStruct.name;
		idxInfo.tableName = idxStruct.tableName;
		idxInfo.sql       = idxStruct.sql;
		indexInfoVect.push_back(idxInfo);
	}
}

string DBAgentSQLite3::getDBPath(void) const
{
	return m_impl->dbPath;
}

//
// Non static methods
//
void DBAgentSQLite3::openDatabase(void)
{
	if (m_impl->db)
		return;

	HATOHOL_ASSERT(!m_impl->dbPath.empty(), "dbPath is empty.");
	m_impl->db = openDatabase(m_impl->dbPath);
}

void DBAgentSQLite3::execSql(const char *fmt, ...)
{
	// make a query string
	MAKE_SQL_STATEMENT_FROM_VAARG(fmt, sql);

	// execute the query
	_execSql(m_impl->db, sql);
}
