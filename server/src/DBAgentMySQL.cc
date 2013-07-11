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
#include "ConfigManager.h"

struct DBAgentMySQL::PrivateContext {
	MYSQL mysql;
	bool  connected;
	string dbName;

	PrivateContext(void)
	: connected(false)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBAgentMySQL::DBAgentMySQL(const char *db, const char *user, const char *passwd,
                           const char *host, unsigned int port, bool skipSetup)
: DBAgent(DB_DOMAIN_ID_CONFIG, skipSetup),
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

	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to query: %s: %s\n",
		  query.c_str(), mysql_error(&m_ctx->mysql));
	}

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

	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to query: %s: %s\n",
		  query.c_str(), mysql_error(&m_ctx->mysql));
	}

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
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::commit(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::rollback(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::createTable(DBAgentTableCreationArg &tableCreationArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");
	string query = StringUtils::sprintf("CREATE TABLE %s (",
	                                    tableCreationArg.tableName.c_str());
	
	for (size_t i = 0; i < tableCreationArg.numColumns; i++) {
		const ColumnDef &columnDef = tableCreationArg.columnDefs[i];
		query += columnDef.columnName;
		switch (columnDef.type) {
		case SQL_COLUMN_TYPE_INT:
			query += StringUtils::sprintf(" INT(%zd)",
			                              columnDef.columnLength);
			break;
		case SQL_COLUMN_TYPE_BIGUINT:
			query += StringUtils::sprintf(" BIGINT(%zd) UNSIGNED",
			                              columnDef.columnLength);
			break;
		case SQL_COLUMN_TYPE_VARCHAR:
			query += StringUtils::sprintf(" VARCHAR(%zd)",
			                              columnDef.columnLength);
			break;
		case SQL_COLUMN_TYPE_CHAR:
			query += StringUtils::sprintf(" CHAR(%zd)",
			                              columnDef.columnLength);
			break;
		case SQL_COLUMN_TYPE_TEXT:
			query += " TEXT";
			break;
		case SQL_COLUMN_TYPE_DOUBLE:
			query += StringUtils::sprintf(" DOUBLE(%zd,%zd)",
			                              columnDef.columnLength,
			                              columnDef.decFracLength);
			break;
		default:
			HATOHOL_ASSERT(false, "Unknown type: %d", columnDef.type);
		}

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

		if (i < tableCreationArg.numColumns -1)
			query += ",";
	}
	query += ")";
	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: %s\n",
		                      query.c_str(),
		                      mysql_error(&m_ctx->mysql));
	}
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
		const ColumnDef &columnDef = insertArg.columnDefs[i];
		const ItemData *itemData = insertArg.row->getItemAt(i);
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
			delete escaped;
			break;
		}
		default:
			HATOHOL_ASSERT(false, "Unknown type: %d", columnDef.type);
		}
		if (i < insertArg.numColumns -1)
			query += ",";
	}
	query += ")";

	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: %s\n",
		                      query.c_str(),
		                      mysql_error(&m_ctx->mysql));
	}
}

void DBAgentMySQL::update(DBAgentUpdateArg &updateArg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

void DBAgentMySQL::select(DBAgentSelectArg &selectArg)
{
	HATOHOL_ASSERT(m_ctx->connected, "Not connected.");

	string query = makeSelectStatement(selectArg);
	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: %s\n",
		                      query.c_str(),
		                      mysql_error(&m_ctx->mysql));
	}

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
			const ColumnDef &columnDef = selectArg.columnDefs[i];
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
	if (mysql_query(&m_ctx->mysql, query.c_str()) != 0) {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: %s\n",
		                      query.c_str(),
		                      mysql_error(&m_ctx->mysql));
	}

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
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
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
