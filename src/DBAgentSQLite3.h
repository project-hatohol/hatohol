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

#ifndef DBAgentSQLite3_h
#define DBAgentSQLite3_h

#include <sqlite3.h>

#include "SQLProcessorTypes.h"
#include "DBAgent.h"

class DBAgentSQLite3 : public DBAgent {
public:
	/**
	 * define database path
	 *
	 * @domainId domain ID. If the domainId has already been set, the path
	 *                      is overwritten.
	 * @path     A path of the database.
	 */
	static string getDefaultDBPath(DBDomainId domainId);
	static void defineDBPath(DBDomainId domainId, const string &path);
	static const string &findDBPath(DBDomainId domainId);
	static void deleteRows(const string &dbPath,
                               DBAgentDeleteArg &deleteArg);

	// constructor and destructor
	DBAgentSQLite3(DBDomainId domainId = DefaultDBDomainId,
	               bool skipSetup = false);
	virtual ~DBAgentSQLite3();

	// virtual methods
	virtual bool isTableExisting(const string &tableName);
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition);
	virtual void begin(void);
	virtual void commit(void);
	virtual void rollback(void);
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg);
	virtual void insert(DBAgentInsertArg &insertArg);
	virtual void update(DBAgentUpdateArg &updateArg);
	virtual void select(DBAgentSelectArg &selectArg);
	virtual void select(DBAgentSelectExArg &selectExArg);
	virtual void deleteRows(DBAgentDeleteArg &deleteArg);

protected:
	static sqlite3 *openDatabase(const string &dbPath);
	static void execSql(sqlite3 *db, const char *fmt, ...);
	static void _execSql(sqlite3 *db, const string &sql);
	static string getColumnValueString(const ColumnDef *columnDef,
	                                   const ItemData *itemData);
	static bool isTableExisting(sqlite3 *db,
	                            const string &tableName);
	static void createTable(sqlite3 *db,
	                        DBAgentTableCreationArg &tableCreationArg);
	static void insert(sqlite3 *db, DBAgentInsertArg &insertArg);
	static void update(sqlite3 *db, DBAgentUpdateArg &updateArg);
	static void select(sqlite3 *db, DBAgentSelectArg &selectArg);
	static void select(sqlite3 *db, DBAgentSelectExArg &selectExArg);
	static void deleteRows(sqlite3 *db, DBAgentDeleteArg &deleteArg);
	static void selectGetValuesIteration(DBAgentSelectArg &selectArg,
	                                     sqlite3_stmt *stmt);
	static ItemDataPtr getValue(sqlite3_stmt *stmt, size_t index,
	                            SQLColumnType columnType);
	static void createIndex(sqlite3 *db,
	                        const string &tableName,
	                        const ColumnDef *columnDefs,
	                        const string &indexName,
	                        const vector<size_t> &targetIndexes,
	                        bool isUniqueKey);

	void openDatabase(void);
	void execSql(const char *fmt, ...);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgentSQLite3_h
