/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
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
	static const char *DEFAULT_DB_NAME;

	static void init(void);

	static const DBTermCodec *getDBTermCodecStatic(void);

	// constructor and destructor
	DBAgentSQLite3(const std::string &name = DEFAULT_DB_NAME,
	               const std::string &dbDir = "");
	virtual ~DBAgentSQLite3();

	void getIndexes(std::vector<IndexStruct> &indexStructVect,
	                const std::string &tableName);

	// virtual methods
	virtual bool isTableExisting(const std::string &tableName) override;
	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition) override;
	virtual void begin(void) override;
	virtual void commit(void) override;
	virtual void rollback(void) override;
	virtual void execSql(const std::string &sql) override;
	virtual void createTable(const TableProfile &tableProfile) override;
	virtual void insert(const InsertArg &insertArg) override;
	virtual void update(const UpdateArg &updateArg) override;
	virtual void select(const SelectArg &selectArg) override;
	virtual void select(const SelectExArg &selectExArg) override;
	virtual void deleteRows(const DeleteArg &deleteArg) override;
	virtual void addColumns(const AddColumnsArg &addColumnsArg) override;
	virtual void changeColumnDef(const TableProfile &tableProfile,
				     const std::string &oldColumnName,
				     const size_t &columnIndex) override;
	virtual void dropPrimaryKey(const std::string &tableName) override;
	virtual void renameTable(const std::string &srcName,
				 const std::string &destName) override;
	virtual const
	  DBTermCodec *getDBTermCodec(void) const override;

	virtual uint64_t getLastInsertId(void) override;
	virtual uint64_t getNumberOfAffectedRows(void) override;
	virtual bool lastUpsertDidUpdate(void) override;
	virtual bool lastUpsertDidInsert(void) override;

	std::string getDBPath(void) const;

protected:
	static std::string makeDBPathFromName(
	  const std::string &name = DEFAULT_DB_NAME,
	  const std::string &dbDir = "");
	static void checkDBPath(const std::string &dbPath);
	static sqlite3 *openDatabase(const std::string &dbPath);
	static void execSql(sqlite3 *db, const char *fmt, ...);
	static void _execSql(sqlite3 *db, const std::string &sql);
	static bool isTableExisting(sqlite3 *db,
	                            const std::string &tableName);
	static void createTable(sqlite3 *db, const TableProfile &tableProfile);
	static std::string getColumnValueStringStatic(const ColumnDef *columnDef,
						      const ItemData *itemData);
	static std::string makeUpdateStatementStatic(const UpdateArg &updateArg);
	static void insert(sqlite3 *db, const InsertArg &insertArg);
	static void update(sqlite3 *db, const UpdateArg &updateArg);
	static void update(sqlite3 *db, const InsertArg &updateArg);
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
	virtual std::string getColumnValueString(
	  const ColumnDef *columnDef, const ItemData *itemData) override;

	virtual std::string
	  makeCreateIndexStatement(const TableProfile &tableProfile,
	                           const IndexDef &indexDef) override;

	virtual std::string
	makeDropIndexStatement(const std::string &name,
	                       const std::string &tableName) override;

	virtual void getIndexInfoVect(
	  std::vector<IndexInfo> &indexInfoVect,
	  const TableProfile &tableProfile) override;

	virtual std::string makeIndexName(const TableProfile &tableProfile,
	                                  const IndexDef &indexDef) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

