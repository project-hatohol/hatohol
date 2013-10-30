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

#include "DBAgentMySQL.h"
#include "SQLUtils.h"
#include "Params.h"

struct DBAgentMySQL::PrivateContext {
	static string engineStr;
	MYSQL mysql;
	bool  connected;
	string dbName;

	PrivateContext(void)
	: connected(false)
	{
	}
};

string DBAgentMySQL::PrivateContext::engineStr;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentMySQL::init(void)
{
	char *env = getenv("HATOHOL_MYSQL_ENGINE_MEMORY");
	if (env && atoi(env) == 1) {
		MLPL_INFO("Use memory engine\n");
		PrivateContext::engineStr = " ENGINE=MEMORY";
	}
}

DBAgentMySQL::DBAgentMySQL(const char *db, const char *user, const char *passwd,
                           const char *host, unsigned int port,
                           DBDomainId domainId, bool skipSetup)
: DBAgent(domainId, skipSetup),
  m_ctx(NULL)
{
	const char *unixSocket = NULL;
	unsigned long clientFlag = 0;
	m_ctx = new PrivateContext();
	mysql_init(&m_ctx->mysql);
	MYSQL *result = mysql_real_connect(&m_ctx->mysql, host, user, passwd,
	                                   db, port, unixSocket, clientFlag);
	if (!result) {
		THROW_HATOHOL_EXCEPTION("Failed to connect to MySQL: %s: %s\n",
		                      db, mysql_error(&m_ctx->mysql));
	}
	m_ctx->connected = true;
	m_ctx->dbName = db;
}

DBAgentMySQL::~DBAgentMySQL()
{
	if (m_ctx) {
		mysql_close(&m_ctx->mysql);
		delete m_ctx;
	}
}

string DBAgentMySQL::getDBName(void) const
{
	return m_ctx->dbName;
}

bool DBAgentMySQL::isTableExisting(const string &tableName)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string query =
	  StringUtils::sprintf(
	    "SHOW TABLES FROM %s LIKE '%s'",
	    getDBName().c_str(), tableName.c_str());
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_ctx->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_ctx->mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	while ((row = mysql_fetch_row(result))) {
		found = true;
		break;
	}
	mysql_free_result(result);

	return found;
}

bool DBAgentMySQL::isRecordExisting(const string &tableName,
                                    const string &condition)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string query =
	  StringUtils::sprintf(
	    "SELECT * FROM %s WHERE %s",
	    tableName.c_str(), condition.c_str());
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_ctx->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_ctx->mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	while ((row = mysql_fetch_row(result))) {
		found = true;
		break;
	}
	mysql_free_result(result);

	return found;
}

void DBAgentMySQL::begin(void)
{
	execSql("START TRANSACTION");
}

void DBAgentMySQL::commit(void)
{
	execSql("COMMIT");
}

void DBAgentMySQL::rollback(void)
{
	execSql("ROLLBACK");
}

static string getColumnTypeQuery(const ColumnDef &columnDef)
{
	switch (columnDef.type) {
	case SQL_COLUMN_TYPE_INT:
		return StringUtils::sprintf("INT(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_BIGUINT:
		return StringUtils::sprintf("BIGINT(%zd) UNSIGNED",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_VARCHAR:
		return StringUtils::sprintf("VARCHAR(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_CHAR:
		return StringUtils::sprintf("CHAR(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_TEXT:
		return "TEXT";
	case SQL_COLUMN_TYPE_DOUBLE:
		return StringUtils::sprintf("DOUBLE(%zd,%zd)",
					    columnDef.columnLength,
					    columnDef.decFracLength);
	case SQL_COLUMN_TYPE_DATETIME:
		return " DATETIME";
	default:
		HATOHOL_ASSERT(false, "Unknown type: %d", columnDef.type);
	}
	return string();
}

static string getColumnDefinitionQuery(const ColumnDef &columnDef)
{
	string query;

	query += columnDef.columnName;
	query += " ";
	query += getColumnTypeQuery(columnDef);

	// nullable
	if (!columnDef.canBeNull)
		query += " NOT NULL";

	// key type
	switch (columnDef.keyType) {
	case SQL_KEY_PRI:
		query += " PRIMARY KEY";
	case SQL_KEY_UNI:
		query += " UNIQUE";
	case SQL_KEY_MUL:
	case SQL_KEY_NONE:
		break;
	default:
		HATOHOL_ASSERT(false,
		  "Unknown key type: %d", columnDef.keyType);
	}

	return query;
}

void DBAgentMySQL::createTable(DBAgentTableCreationArg &tableCreationArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string query = StringUtils::sprintf("CREATE TABLE %s (",
	                                    tableCreationArg.tableName.c_str());
	
	for (size_t i = 0; i < tableCreationArg.numColumns; i++) {
		const ColumnDef &columnDef = tableCreationArg.columnDefs[i];
		query += getColumnDefinitionQuery(columnDef);

		// auto increment
		if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC)
			query += " AUTO_INCREMENT";

		if (i < tableCreationArg.numColumns -1)
			query += ",";
	}
	query += ")";
	if (!m_ctx->engineStr.empty())
		query += m_ctx->engineStr;

	execSql(query);
}

void DBAgentMySQL::insert(DBAgentInsertArg &insertArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	HATOHOL_ASSERT(insertArg.numColumns == insertArg.row->getNumberOfItems(),
	             "numColumn: %zd != row: %zd",
	             insertArg.numColumns, insertArg.row->getNumberOfItems());

	string query = StringUtils::sprintf("INSERT INTO %s (",
	                                    insertArg.tableName.c_str());
	for (size_t i = 0; i < insertArg.numColumns; i++) {
		const ColumnDef &columnDef = insertArg.columnDefs[i];
		query += columnDef.columnName;
		if (i < insertArg.numColumns -1)
			query += ",";
	}
	query += ") VALUES (";
	for (size_t i = 0; i < insertArg.numColumns; i++) {
		if (i > 0)
			query += ",";

		const ColumnDef &columnDef = insertArg.columnDefs[i];
		const ItemData *itemData = insertArg.row->getItemAt(i);
		if (itemData->isNull()) {
			query += "NULL";
			continue;
		}

		switch (columnDef.type) {
		case SQL_COLUMN_TYPE_INT:
		case SQL_COLUMN_TYPE_BIGUINT:
		case SQL_COLUMN_TYPE_DOUBLE:
			query += itemData->getString();
			break;
		case SQL_COLUMN_TYPE_VARCHAR:
		case SQL_COLUMN_TYPE_CHAR:
		case SQL_COLUMN_TYPE_TEXT:
		{ // bracket is used to avoid an error: jump to case label
			string src =  itemData->getString();
			char *escaped = new char[src.size() * 2 + 1]; 
			mysql_real_escape_string(&m_ctx->mysql, escaped,
			                         src.c_str(), src.size());
			query += "'";
			query += escaped,
			query += "'";
			delete [] escaped;
			break;
		}
		case SQL_COLUMN_TYPE_DATETIME:
		{ // bracket is used to avoid an error: jump to case label
			DEFINE_AND_ASSERT(itemData, ItemInt, item);
			query += makeDatetimeString(item->get());
			break;
		}
		default:
			HATOHOL_ASSERT(false, "Unknown type: %d", columnDef.type);
		}
	}
	query += ")";

	execSql(query);
}

void DBAgentMySQL::update(DBAgentUpdateArg &updateArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string sql = makeUpdateStatement(updateArg);
	execSql(sql);
}

void DBAgentMySQL::select(DBAgentSelectArg &selectArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");

	string query = makeSelectStatement(selectArg);
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_ctx->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION("Failed to call mysql_store_result: %s\n",
		                      mysql_error(&m_ctx->mysql));
	}

	MYSQL_ROW row;
	VariableItemTablePtr dataTable;
	size_t numColumns = selectArg.columnIndexes.size();
	while ((row = mysql_fetch_row(result))) {
		VariableItemGroupPtr itemGroup;
		for (size_t i = 0; i < numColumns; i++) {
			size_t idx = selectArg.columnIndexes[i];
			const ColumnDef &columnDef = selectArg.columnDefs[idx];
			ItemDataPtr itemDataPtr =
			  SQLUtils::createFromString(row[i], columnDef.type);
			itemGroup->add(itemDataPtr);
		}
		dataTable->add(itemGroup);
	}
	mysql_free_result(result);
	selectArg.dataTable = dataTable;
}

void DBAgentMySQL::select(DBAgentSelectExArg &selectExArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");

	string query = makeSelectStatement(selectExArg);
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_ctx->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION("Failed to call mysql_store_result: %s\n",
		                      mysql_error(&m_ctx->mysql));
	}

	MYSQL_ROW row;
	VariableItemTablePtr dataTable;
	size_t numColumns = selectExArg.statements.size();
	while ((row = mysql_fetch_row(result))) {
		VariableItemGroupPtr itemGroup;
		for (size_t i = 0; i < numColumns; i++) {
			SQLColumnType type = selectExArg.columnTypes[i];
			ItemDataPtr itemDataPtr =
			  SQLUtils::createFromString(row[i], type);
			itemGroup->add(itemDataPtr);
		}
		dataTable->add(itemGroup);
	}
	mysql_free_result(result);
	selectExArg.dataTable = dataTable;

	// check the result
	size_t numTableRows = selectExArg.dataTable->getNumberOfRows();
	size_t numTableColumns = selectExArg.dataTable->getNumberOfColumns();
	HATOHOL_ASSERT((numTableRows == 0) ||
	             ((numTableRows > 0) && (numTableColumns == numColumns)),
	             "Sanity check error: numTableRows: %zd, numTableColumns: "
	             "%zd, numColumns: %zd",
	             numTableRows, numTableColumns, numColumns);
}

void DBAgentMySQL::deleteRows(DBAgentDeleteArg &deleteArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string query = makeDeleteStatement(deleteArg);
	execSql(query);
}

uint64_t DBAgentMySQL::getLastInsertId(void)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	execSql("SELECT LAST_INSERT_ID()");
	MYSQL_RES *result = mysql_store_result(&m_ctx->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_ctx->mysql));
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	HATOHOL_ASSERT(row, "Failed to call mysql_fetch_row.");
	uint64_t id;
	int numScan = sscanf(row[0], "%"PRIu64, &id);
	HATOHOL_ASSERT(numScan == 1, "numScan: %d, %s", numScan, row[0]);
	return id;
}

void DBAgentMySQL::addColumns(DBAgentAddColumnsArg &addColumnsArg)
{
	string query = "ALTER TABLE ";
	query += addColumnsArg.tableName;
	vector<size_t>::iterator it = addColumnsArg.columnIndexes.begin();
	for (; it != addColumnsArg.columnIndexes.end(); it++) {
		size_t index = *it;
		const ColumnDef &columnDef = addColumnsArg.columnDefs[index];
		query += " ADD COLUMN ";
		query += getColumnDefinitionQuery(columnDef);
		if (index < addColumnsArg.columnIndexes.size() - 1)
			query += ",";
	}
	execSql(query);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBAgentMySQL::execSql(const string &statement)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	if (mysql_query(&m_ctx->mysql, statement.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: %s\n",
		                        statement.c_str(),
		                        mysql_error(&m_ctx->mysql));
	}
}
