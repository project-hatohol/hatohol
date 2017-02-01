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
#include <mysql.h>
#include "DBAgent.h"

class DBAgentMySQL : public DBAgent {
public:
	struct IndexStruct {
		std::string table;
		bool        nonUnique;
		std::string keyName;
		size_t      seqInIndex;
		std::string columnName;
	};

	static void init(void);

	// constructor and destructor
	DBAgentMySQL(const char *db,            // When the parameter is NULL,
	             const char *user = NULL,   //   current user name is used
	             const char *passwd = NULL, //   passwd is not checked
	             const char *host = NULL,   //   localhost is used
	             unsigned int port = 0);    //   default port is used
	virtual ~DBAgentMySQL();
	std::string getDBName(void) const;
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
	virtual void createTable(const TableProfile &tableProfile); //override
	virtual void insert(const InsertArg &insertArg) override;
	virtual void update(const UpdateArg &updateArg) override;
	virtual void select(const SelectArg &selectArg) override;
	virtual void select(const SelectExArg &selectExArg) override;
	virtual void deleteRows(const DeleteArg &deleteArg) override;
	virtual void addColumns(const AddColumnsArg &addColumnsArg);
	virtual void changeColumnDef(const TableProfile &tableProfile,
				     const std::string &oldColumnName,
				     const size_t &columnIndex) override;
	virtual void dropPrimaryKey(const std::string &tableName) override;
	virtual void renameTable(const std::string &srcName,
				 const std::string &destName);
	virtual uint64_t getLastInsertId(void);
	virtual uint64_t getNumberOfAffectedRows(void);
	virtual bool lastUpsertDidUpdate(void) override;
	virtual bool lastUpsertDidInsert(void) override;
	/**
	 * Dispose DBAgentMySQL object and stop retrying connection to MySQL.
	 *
	 * If the owner thread is calling a method that communicates with
	 * the DB server such as select(), insert(), and update(), a retry
	 * in the method is aborted. In this case, a HatoholException with
	 * an error code HTERR_VALID_DBAGENT_NO_LONGER_EXISTS
	 * is thrown on the thread calling any of their methods.
	 * Note that the instance must not be used after this method is called.
	 */
	void dispose(void);

protected:
	static const char *getCStringOrNullIfEmpty(const std::string &str);
	void connect(void);
	void sleepAndReconnect(unsigned int sleepTimeSec);
	bool throwExceptionIfDisposed(void) const;
	void queryWithRetry(const std::string &statement);

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

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

