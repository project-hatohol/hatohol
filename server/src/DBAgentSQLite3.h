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
	struct IndexStruct {
		std::string name;
		std::string tableName;
		std::string sql;
	};

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
	static const DBTermCodec *getDBTermCodecStatic(void);

	// constructor and destructor
	DBAgentSQLite3(const std::string &dbName = "",
	               DBDomainId domainId = DEFAULT_DB_DOMAIN_ID,
	               bool skipSetup = false);
	virtual ~DBAgentSQLite3();

	void getIndexes(std::vector<IndexStruct> &indexStructVect,
	                const std::string &tableName);

	// virtual methods
	virtual bool isTableExisting(const std::string &tableName);
	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition);
	virtual void begin(void);
	virtual void commit(void);
	virtual void rollback(void);
	virtual void execSql(const std::string &sql) override;
	virtual void createTable(const TableProfile &tableProfile) override;
	virtual void insert(const InsertArg &insertArg) override;
	virtual void update(const UpdateArg &updateArg) override;
	virtual void select(const SelectArg &selectArg) override;
	virtual void select(const SelectExArg &selectExArg) override;
	virtual void deleteRows(const DeleteArg &deleteArg) override;
	virtual void addColumns(const AddColumnsArg &addColumnsArg) override;
	virtual void renameTable(const std::string &srcName,
				 const std::string &destName);
	virtual const
	  DBTermCodec *getDBTermCodec(void) const override;

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
	static void createTable(sqlite3 *db, const TableProfile &tableProfile);
	static void insert(sqlite3 *db, const InsertArg &insertArg);
	static void update(sqlite3 *db, const UpdateArg &updateArg);
	static void select(sqlite3 *db, const SelectArg &selectArg);
	static void select(sqlite3 *db, const SelectExArg &selectExArg);
	static void deleteRows(sqlite3 *db, const DeleteArg &deleteArg);
	static void selectGetValuesIteration(const SelectArg &selectArg,
	                                     sqlite3_stmt *stmt,
	                                     VariableItemTablePtr &dataTable);
	static uint64_t getLastInsertId(sqlite3 *db);
	static uint64_t getNumberOfAffectedRows(sqlite3 *db);
	static ItemDataPtr getValue(sqlite3_stmt *stmt, size_t index,
	                            SQLColumnType columnType);
	static void createIndexIfNotExistsEach(
	  sqlite3 *db, const TableProfile &tableProfile,
	  const std::string &indexName,
	  const std::vector<size_t> &targetIndexes, const bool &isUniqueKey);

	void openDatabase(void);
	void execSql(const char *fmt, ...);

	// virtual methods
	virtual std::string
	makeCreateIndexStatement(const IndexDef &indexDef) override;

	virtual std::string
	makeDropIndexStatement(const std::string &name,
	                       const std::string &tableName) override;

	virtual void getIndexInfoVect(
	  std::vector<IndexInfo> &indexInfoVect,
	  const TableProfile &tableProfile) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBAgentSQLite3_h
