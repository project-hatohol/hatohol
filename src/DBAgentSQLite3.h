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
	// static methods
	static const int DB_VERSION;

	static void init(void);

	/**
	 * define database path
	 *
	 * @domainId domain ID. If the domainId has already been set, the path
	 *                      is overwritten.
	 * @path     A path of the database.
	 */
	static void defineDBPath(DBDomainId domainId, const string &path);
	static void defaultSetupFunc(DBDomainId domainId);
	static const string &findDBPath(DBDomainId domainId);
	static bool isTableExisting(const string &dbPath,
	                            const string &tableName);
	static void createTable(const string &dbPath,
	                        TableCreationArg &tableCreationArg);
	static void insert(const string &dbPath, RowInsertArg &rowInsertArg);
	static void select(const string &dbPath, DBAgentSelectArg &selecttArg);

	// constructor and destructor
	DBAgentSQLite3(DBDomainId domainId = DefaultDBDomainId);
	virtual ~DBAgentSQLite3();

	// generic methods
	int getDBVersion(void);

	// virtual methods
	virtual bool isTableExisting(const string &tableName);
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition);
	virtual void
	   addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	virtual void
	   getTargetServers(MonitoringServerInfoList &monitoringServers);
	virtual void
	   addTriggerInfo(TriggerInfo *triggerInfo);
	virtual void getTriggerInfoList(TriggerInfoList &triggerInfoList);
	virtual void createTable(TableCreationArg &tableCreationArg);

protected:
	static sqlite3 *openDatabase(const string &dbPath);
	static int getDBVersion(const string &dbPath);
	static int getDBVersion(sqlite3 *db);
	static bool isTableExisting(sqlite3 *db,
	                            const string &tableName);
	static void createTable(sqlite3 *db,
	                        TableCreationArg &tableCreationArg);
	static void insert(sqlite3 *db, RowInsertArg &rowInsertArg);
	static void select(sqlite3 *db, DBAgentSelectArg &selectArg);
	static void updateDBIfNeeded(const string &dbPath);
	static void createTableSystem(const string &dbPath);
	static void createTableServers(const string &dbPath);
	static void createTableTriggers(const string &dbPath);
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
