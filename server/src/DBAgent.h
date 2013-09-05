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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
using namespace std;

#include <glib.h>
#include <stdint.h>

#include "Params.h"
#include "SQLProcessorTypes.h"

#define CURR_DATETIME -1

struct DBAgentTableCreationArg {
	string              tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
};

struct DBAgentInsertArg {
	string tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
	ItemGroupPtr        row;
};

struct DBAgentUpdateArg {
	string tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;
	ItemGroupPtr        row;
	string condition;
};

struct DBAgentSelectArg {
	string tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;

	// output
	ItemTablePtr        dataTable;
};

struct DBAgentSelectExArg {
	string tableName;
	vector<string>        statements;
	vector<SQLColumnType> columnTypes;
	string condition;
	string orderBy;
	size_t limit;
	size_t offset;

	// output
	ItemTablePtr        dataTable;

	// constructor and methods
	DBAgentSelectExArg(void);
	void pushColumn(const ColumnDef &columnDef, const string &varName = "");
};

struct DBAgentDeleteArg {
	string tableName;
	string condition;
};

struct DBAgentAddColumnsArg {
	string              tableName;
	const ColumnDef    *columnDefs;
	vector<size_t>      columnIndexes;
};

struct DBConnectInfo {
	string host;
	size_t port;
	string user;
	string password;
	string dbName;

	DBConnectInfo(void);
	virtual ~DBConnectInfo();
	const char *getHost(void) const;
	const char *getUser(void) const;
	const char *getPassword(void) const;
};

typedef void (*DBSetupFunc)(DBDomainId domainId, void *data);
static const DBDomainId DEFAULT_DB_DOMAIN_ID = 0;

class DBAgent {
public:
	static void addSetupFunction(DBDomainId domainId,
	                             DBSetupFunc setupFunc, void *data = NULL);

	DBAgent(DBDomainId = DEFAULT_DB_DOMAIN_ID, bool skipSetup = false);
	virtual ~DBAgent();
	DBDomainId getDBDomainId(void) const;

	// virtual methods
	virtual bool isTableExisting(const string &tableName) = 0;
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition) = 0;
	virtual void begin(void) = 0;
	virtual void commit(void) = 0;
	virtual void rollback(void) = 0;
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg) = 0;
	virtual void insert(DBAgentInsertArg &insertArg) = 0;
	virtual void update(DBAgentUpdateArg &updateArg) = 0;
	virtual void select(DBAgentSelectArg &selectArg) = 0;
	virtual void select(DBAgentSelectExArg &selectExArg) = 0;
	virtual void deleteRows(DBAgentDeleteArg &deleteArg) = 0;
	virtual void addColumns(DBAgentAddColumnsArg &addColumnsArg) = 0;
	virtual uint64_t getLastInsertId(void) = 0;

protected:
	static string makeSelectStatement(DBAgentSelectArg &selectArg);
	static string makeSelectStatement(DBAgentSelectExArg &selectExArg);
	static string getColumnValueString(const ColumnDef *columnDef,
	                                   const ItemData *itemData);
	static string makeUpdateStatement(DBAgentUpdateArg &updateArg);
	static string makeDeleteStatement(DBAgentDeleteArg &deleteArg);
	static string makeDatetimeString(int datetime);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgent_h
