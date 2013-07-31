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

#ifndef DBClient_h
#define DBClient_h

#include "DBAgent.h"

class DBClient {
public:
	typedef void (*CreateTableInitializer)(DBAgent *, void *data);
	typedef void (*DBUpdater)(DBAgent *, int oldVer, void *data);

	struct DBSetupTableInfo {
		const char            *name;
		size_t                 numColumns;
		const ColumnDef       *columnDefs;
		CreateTableInitializer initializer;
		void                  *initializerData;
	};

	struct DBSetupFuncArg {
		int                     version;
		size_t                  numTableInfo;
		const DBSetupTableInfo *tableInfoArray;
		DBUpdater               dbUpdater;
		void                   *dbUpdaterData;
		const DBConnectInfo    *connectInfo;
	};

	static int DBCLIENT_DB_VERSION;

	DBClient(void);
	virtual ~DBClient();
	DBAgent *getDBAgent(void) const;

protected:
	// static methods
	static void createTable
	  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs,
	   CreateTableInitializer initializer = NULL, void *data = NULL);
	static void tableInitializerDBClient(DBAgent *dbAgent, void *data);
	static void updateDBIfNeeded(DBAgent *dbAgent,
	                             DBSetupFuncArg *setupFuncArg);
	/**
	 * Get the DB version.
	 * The version is typically used by the sub class
	 * to identify their schema version. This implies that this class
	 * provides a function to manage the DB version.
	 *
	 * @param dbAgent A pointer to a DBAgent instance.
	 * @param columnDef
	 * A pointer to a ColumnDef structure for _dbclient.version.
	 * Note that this is not the head of a ColumnDef structure array,
	 * but points the address of the "version" definition such as
	 * &COLUMN_DEF_DBCLIENT[IDX_DBCLIENT_VERSION].
	 */
	static int getDBVersion(DBAgent *dbAgent, const ColumnDef *columnDef);

	/**
	 * Set the DB version for the sub class.
	 * See also the description of getDBVersion().
	 *
	 * @param dbAgent A pointer to a DBAgent instance.
	 * @param columnDef
	 * A pointer to a ColumnDef structure for _dbclient.version.
	 */
	static void setDBVersion(DBAgent *dbAgent, const ColumnDef *columnDef,
	                         int version);

	// non-static methods
	static void dbSetupFunc(DBDomainId domainId, void *data);
	void setDBAgent(DBAgent *dbAgent);

	void begin(void);
	void rollback(void);
	void commit(void);

	void insert(DBAgentInsertArg &insertArg);
	void update(DBAgentUpdateArg &updateArg);
	void select(DBAgentSelectArg &selectArg);
	void select(DBAgentSelectExArg &selectExArg);
	void deleteRows(DBAgentDeleteArg &deleteArg);
	void addColumns(DBAgentAddColumnsArg &addColumnsArg);
	bool isRecordExisting(const string &tableName,
	                      const string &condition);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#define DBCLIENT_TRANSACTION_BEGIN() \
	begin(); \
	try

#define DBCLIENT_TRANSACTION_END() \
	catch (...) { \
		rollback(); \
		throw; \
	} \
	commit()

#endif // DBClient_h
