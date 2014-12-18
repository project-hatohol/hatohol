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

#ifndef DBAgentMySQL_h
#define DBAgentMySQL_h

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
	virtual void renameTable(const std::string &srcName,
				 const std::string &destName);
	virtual uint64_t getLastInsertId(void);
	virtual uint64_t getNumberOfAffectedRows(void);

protected:
	static const char *getCStringOrNullIfEmpty(const std::string &str);
	void connect(void);
	void sleepAndReconnect(unsigned int sleepTimeSec);
	void queryWithRetry(const std::string &statement);

	// virtual methods
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

#endif // DBAgentMySQL_h


