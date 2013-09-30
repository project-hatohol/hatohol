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
	typedef bool (*DBUpdater)(DBAgent *, int oldVer, void *data);

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

	static void reset(void);

	/**
	 * set the default parameters to connect the DB.
	 * The parameters set by this function are used for the instance of
	 * DBClientConnectable created with NULL as the first argument.
	 * The paramers are reset to the sysytem default when reset() is called.
	 *
	 * @param domainId A DB domain ID.
	 * @param dbName A database name.
	 * @param user
	 * A user name. If this paramter is "" (empty string), the name of
	 * the current user is used.
	 * @param passowrd
	 * A password. If this parameter is "" (empty string), no passowrd
	 * is used.
	 */
	static void setDefaultDBParams(DBDomainId domainId,
	                               const string &dbName,
	                               const string &user = "",
	                               const string &password = "");
	static DBConnectInfo getDBConnectInfo(DBDomainId domainId);

	DBClient(DBDomainId domainId);
	virtual ~DBClient();
	DBAgent *getDBAgent(void) const;

protected:
	struct DBSetupContext;
	typedef map<DBDomainId, DBSetupContext *> DBSetupContextMap;
	typedef DBSetupContextMap::iterator       DBSetupContextMapIterator;

	// static methods
	static void registerSetupInfo(DBDomainId domainId, const string &dbName,
	                              DBSetupFuncArg *dbSetupFuncArg);
	static void setConnectInfo(DBDomainId domainId,
	                           const DBConnectInfo &connectInfo);
	static void createTable
	  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
	   const ColumnDef *columnDefs,
	   CreateTableInitializer initializer = NULL, void *data = NULL);
	static void insertDBClientVersion(DBAgent *dbAgent,
	                                  DBSetupFuncArg *setupFuncArg);
	static void updateDBIfNeeded(DBAgent *dbAgent,
	                             DBSetupFuncArg *setupFuncArg);
	/**
	 * Get the DB version.
	 * The version is typically used by the sub class
	 * to identify their schema version. This implies that this class
	 * provides a function to manage the DB version.
	 *
	 * @param dbAgent A pointer to a DBAgent instance.
	 */
	static int getDBVersion(DBAgent *dbAgent);

	/**
	 * Set the DB version for the sub class.
	 * See also the description of getDBVersion().
	 *
	 * @param dbAgent A pointer to a DBAgent instance.
	 * @param version
	 * A new version of the DBClient whose name is dbclietName.
	 */
	static void setDBVersion(DBAgent *dbAgent, int version);

	// non-static methods
	static void dbSetupFunc(DBDomainId domainId, void *data);

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
	uint64_t getLastInsertId(void);
	bool updateIfExistElseInsert(
	  const ItemGroup *itemGroup, const string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs, size_t targetIndex);
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
