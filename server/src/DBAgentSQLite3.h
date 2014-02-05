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

#ifndef DBAgentSQLite3_h
#define DBAgentSQLite3_h

#include <sqlite3.h>
#include "SQLProcessorTypes.h"
#include "DBAgent.h"

class DBAgentSQLite3 : public DBAgent {
public:
	static void init(void);
	static void reset(void);

	/**
	 * define database path
	 *
	 * @domainId domain ID.
	 * @path     A path of the database.
	 * @allowOverwrite
	 * A flag that indicates if redefinition of the path is allowed.
	 *
	 * @return
	 * When allowOverwrite is true and the path for domainId is already
	 * defined, false is retruened. Otherwise true is returned.
	 */
	static bool defineDBPath(DBDomainId domainId, const std::string &path,
	                         bool allowOverwrite = true);
	static std::string &getDBPath(DBDomainId domainId);

	// constructor and destructor
	DBAgentSQLite3(const std::string &dbName = "",
	               DBDomainId domainId = DEFAULT_DB_DOMAIN_ID,
	               bool skipSetup = false);
	virtual ~DBAgentSQLite3();

	// virtual methods
	virtual bool isTableExisting(const std::string &tableName);
	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition);
	virtual void begin(void);
	virtual void commit(void);
	virtual void rollback(void);
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg);
	virtual void insert(const InsertArg &insertArg); // override
	virtual void update(const UpdateArg &updateArg); // override
	virtual void select(const SelectArg &selectArg); // override
	virtual void select(DBAgentSelectExArg &selectExArg);
	virtual void deleteRows(const DeleteArg &deleteArg); // override
	virtual void addColumns(DBAgentAddColumnsArg &addColumnsArg);
	virtual uint64_t getLastInsertId(void);
	virtual uint64_t getNumberOfAffectedRows(void);
	std::string getDBPath(void) const;

protected:
	static std::string makeDBPathFromName(const std::string &name);
	static std::string getDefaultDBPath(DBDomainId domainId);
	static void checkDBPath(const std::string &dbPath);
	static sqlite3 *openDatabase(const std::string &dbPath);
	static void execSql(sqlite3 *db, const char *fmt, ...);
	static void _execSql(sqlite3 *db, const std::string &sql);
	static bool isTableExisting(sqlite3 *db,
	                            const std::string &tableName);
	static void createTable(sqlite3 *db,
	                        DBAgentTableCreationArg &tableCreationArg);
	static void insert(sqlite3 *db, const InsertArg &insertArg);
	static void update(sqlite3 *db, const UpdateArg &updateArg);
	static void select(sqlite3 *db, const SelectArg &selectArg);
	static void select(sqlite3 *db, DBAgentSelectExArg &selectExArg);
	static void deleteRows(sqlite3 *db, const DeleteArg &deleteArg);
	static void selectGetValuesIteration(const SelectArg &selectArg,
	                                     sqlite3_stmt *stmt,
	                                     VariableItemTablePtr &dataTable);
	static uint64_t getLastInsertId(sqlite3 *db);
	static uint64_t getNumberOfAffectedRows(sqlite3 *db);
	static ItemDataPtr getValue(sqlite3_stmt *stmt, size_t index,
	                            SQLColumnType columnType);
	static void createIndex(sqlite3 *db,
	                        const std::string &tableName,
	                        const ColumnDef *columnDefs,
	                        const std::string &indexName,
	                        const std::vector<size_t> &targetIndexes,
	                        bool isUniqueKey);

	void openDatabase(void);
	void execSql(const char *fmt, ...);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgentSQLite3_h
