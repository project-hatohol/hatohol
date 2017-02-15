/*
 * Copyright (C) 2013-2015 Project Hatohol
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
#include <string>
#include <memory>
#include <glib.h>
#include <stdint.h>
#include "Params.h"
#include "SQLProcessorTypes.h"
#include "DBTermCodec.h"
#include "ItemTable.h"

static const int CURR_DATETIME = -1;

struct DBConnectInfo {
	std::string host;
	size_t      port;
	std::string user;
	std::string password;
	std::string dbName;

	DBConnectInfo(void);
	virtual ~DBConnectInfo();
	void reset(void);
	const char *getHost(void) const;
	const char *getUser(void) const;
	const char *getPassword(void) const;
};

// Initialize DBAgent::TableProfile
// Note: Compile fails if number of coldefs does not match numcols.
#define DBAGENT_TABLEPROFILE_INIT(name, coldefs, numcols, ...)          \
	DBAgent::TableProfile(                                          \
		name, coldefs,                                          \
		HATOHOL_BUILD_EXPECT(ARRAY_SIZE(coldefs), numcols),     \
		##__VA_ARGS__)

class DBAgent {
public:
	struct IndexDef {
		static const int    END = -1;

		const char         *name;
		const int          *columnIndexes; // Last element must be END.
		bool                isUnique;
	};

	struct TableProfile {
		const char         *name;
		const ColumnDef    *columnDefs;
		const size_t        numColumns;
		const IndexDef     *indexDefArray;

		// The following members are initialized in the constructor
		std::vector<int>    uniqueKeyColumnIndexes;

		TableProfile(const char *name,  const ColumnDef *columnDefs,
		             const size_t &numIndexes,
		             const IndexDef *indexDefArray = NULL);

		/**
		 * Get a full name of a column.
		 *
		 * E.g. If table name is 'tbl' and a column name is 'name',
		 * 'tbl.name' is returned.
		 *
		 * @param index      An index of the target column.
		 *
		 * @return A full name of the specified column.
		 */
		std::string getFullColumnName(const size_t &index) const;
	};

	struct IndexInfo {
		std::string name;
		std::string tableName;
		std::string sql;
	};

	struct RowElement {
		size_t      columnIndex;
		ItemDataPtr dataPtr;

		RowElement(const size_t &index, const ItemData *itemData,
		           const bool &doRef = true);
	};

	struct InsertArg {
		const TableProfile   &tableProfile;
		VariableItemGroupPtr  row;
		bool                  upsertOnDuplicate;

		InsertArg(const TableProfile &tableProfile);
		void add(const int         &val,
		         const ItemDataNullFlagType &nullFlag
		                                     = ITEM_DATA_NOT_NULL);
		void add(const uint64_t    &val,
		         const ItemDataNullFlagType &nullFlag
		                                     = ITEM_DATA_NOT_NULL);
		void add(const double      &val,
		         const ItemDataNullFlagType &nullFlag
		                                     = ITEM_DATA_NOT_NULL);
		void add(const std::string &val,
		         const ItemDataNullFlagType &nullFlag
		                                     = ITEM_DATA_NOT_NULL);
		void add(const time_t      &val,
		         const ItemDataNullFlagType &nullFlag
		                                     = ITEM_DATA_NOT_NULL);
	};

	struct UpdateArg {
		const TableProfile             &tableProfile;
		std::string                     condition;
		std::vector<const RowElement *> rows;

		UpdateArg(const TableProfile &tableProfile);
		virtual ~UpdateArg();
		void add(const size_t &columnIndex, const int         &val,
		         const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL);
		void add(const size_t &columnIndex, const uint64_t    &val,
		         const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL);
		void add(const size_t &columnIndex, const double      &val,
		         const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL);
		void add(const size_t &columnIndex, const std::string &val,
		         const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL);
		void add(const size_t &columnIndex, const time_t      &val,
		         const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL);
		void add(const size_t &columnIndex, const ItemGroup  *grp);
	};

	struct SelectArg {
		const TableProfile &tableProfile;
		std::vector<size_t> columnIndexes;
		// output
		mutable std::shared_ptr<const ItemTable> dataTable;

		SelectArg(const TableProfile &tableProfile);
		void add(const size_t &columnIndex);
	};

	struct SelectExArg {
		const TableProfile        *tableProfile;
		std::vector<std::string>   statements;
		std::vector<SQLColumnType> columnTypes;
		std::string                condition;
		std::string                orderBy;
		std::string                groupBy;
		size_t                     limit;
		size_t                     offset;
		std::string                appName;
		std::string                tableField;
		bool                       useFullName;
		bool                       useDistinct;
		// output
		mutable std::shared_ptr<const ItemTable> dataTable;

		SelectExArg(const TableProfile &tableProfile);
		void add(const size_t &columnIndex);
		void add(const std::string &statement,
		         const SQLColumnType &columnType);
	};

	struct SelectMultiTableArg : public SelectExArg {
		const TableProfile **profiles;
		const size_t         numTables;

		/**
		 *
		 * @param profiles
		 * An array of a pointer of TableProfile. It shall be valid
		 * while this object is used.
		 *
		 * @param numTable
		 * A size of the above array.
		 */
		SelectMultiTableArg(const TableProfile **profiles,
		                    const size_t &numTables);
		void setTable(const size_t &index);
		void add(const size_t &columnIndex);
		std::string getFullName(const size_t &tableIndex,
		                        const size_t &columnIndex);
	};

	struct DeleteArg {
		const TableProfile &tableProfile;
		std::string         condition;

		DeleteArg(const TableProfile &tableProfile);
	};

	struct AddColumnsArg {
		const TableProfile &tableProfile;
		std::vector<size_t> columnIndexes;

		AddColumnsArg(const TableProfile &tableProfile);
	};

	DBAgent(void);
	virtual ~DBAgent();

	// virtual methods
	virtual bool isTableExisting(const std::string &tableName) = 0;
	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition) = 0;
	virtual void begin(void) = 0;
	virtual void commit(void) = 0;
	virtual void rollback(void) = 0;
	virtual void execSql(const std::string &sql) = 0;
	virtual void createTable(const TableProfile &tableProfile) = 0;
	virtual void insert(const InsertArg &insertArg) = 0;
	virtual void update(const UpdateArg &updateArg) = 0;
	virtual void select(const SelectArg &selectArg) = 0;
	virtual void select(const SelectExArg &selectExArg) = 0;
	virtual void deleteRows(const DeleteArg &deleteArg) = 0;
	virtual void addColumns(const AddColumnsArg &addColumnsArg) = 0;
	virtual void changeColumnDef(const TableProfile &tableProfile,
				     const std::string &oldColumnName,
				     const size_t &columnIndex) = 0;
	virtual void dropPrimaryKey(const std::string &tableName) = 0;
	virtual void renameTable(const std::string &sourceName,
				 const std::string &destName) = 0;
	virtual void dropTable(const std::string &tableName);
	virtual uint64_t getLastInsertId(void) = 0;
	virtual uint64_t getNumberOfAffectedRows(void) = 0;

	/**
	 * Check wheter the last upsert did update or not.
	 *
	 * @return
	 * true if the last insert() with upsertOnDuplicate = true
	 * did update.
	 */
	virtual bool lastUpsertDidUpdate(void) = 0;

	/**
	 * Check wheter the last upsert did update or not.
	 *
	 * @return
	 * true if the last insert() with upsertOnDuplicate = true
	 * did insert.
	 */
	virtual bool lastUpsertDidInsert(void) = 0;

	/**
	 * Create and drop indexes if needed.
	 *
	 * @param tableProfile
	 * A TableProfile structure concerned with the indexes to be created.
	 *
	 */
	virtual void fixupIndexes(const TableProfile &tableProfile);

	/**
	 * Update a record if there is the record with the same value in the
	 * specified column. Or this function executes an insert operation.
	 * NOTE: the value that belongs to the column with PRIMARY KEY is not
	 * updated.
	 *
	 * @param itemGroup
	 * An ItemGroup instance that has values for the record to be
	 * updated or inserted.
	 *
	 * @param tableProfile
	 * The target table profile.
	 *
	 * @param targetIndex
	 * A column index used for the comparison.
	 *
	 * @return true if updated, otherwise false.
	 */
	virtual bool updateIfExistElseInsert(
	  const ItemGroup *itemGroup, const TableProfile &tableProfile,
	  size_t targetIndex);

	virtual const DBTermCodec *getDBTermCodec(void) const;

	/**
	 * A exception that stops the running transaction and rolls back.
	 * After this exception is thrown in a transaction,
	 * TransactionProc::postproc() is not invoked.
	 */
	class TransactionAbort : public std::exception {
	};

	struct TransactionHooks {
		/**
		 * Called just before TransactionProc::operator() in a
		 * transaction.
		 *
		 * @param dbAgent A DBAgent instance.
		 *
		 * @return
		 * If the false is returned, the transaction is rolled back.
		 * The subsequent TransactionProc::operator() and postAction()
		 * are not carried out.
		 */
		virtual bool preAction(DBAgent &dbAgent);

		/**
		 * Called just after TransactionProc::operator() in a
		 * transaction.
		 *
		 * @param dbAgent A DBAgent instance.
		 *
		 * @return
		 * If the false is returned, the transaction is rolled back.
		 */
		virtual bool postAction(DBAgent &dbAgent);
	};

	struct TransactionProc {
		/**
		 * This method is called before the runTransaction.
		 * @return
		 * If false is returned, runTransaction() immediately returns
		 * without doing a runTransaction and calling postproc().
		 */
		virtual bool preproc(DBAgent &dbAgent);

		/**
		 * This method is called after the runTransaction.
		 */
		virtual void postproc(DBAgent &dbAgent);

		virtual void operator ()(DBAgent &dbAgent) = 0;
	};

	void runTransaction(TransactionProc &proc,
	                    TransactionHooks *hooks = NULL);

	template <typename T, void (DBAgent::*OPERATION)(const T &)>
	void _runTransaction(T &arg)
	{
		struct TrxProc : public DBAgent::TransactionProc {
			T &arg;
			TrxProc(T &_arg)
			: arg(_arg)
			{
			}

			void operator ()(DBAgent &dbAgent) override
			{
				(dbAgent.*OPERATION)(arg);
			}
		} trx(arg);
		runTransaction(trx);
	}
	void runTransaction(const InsertArg &arg)
	{
		_runTransaction<const InsertArg, &DBAgent::insert>(arg);
	}

	void runTransaction(const SelectArg &arg)
	{
		_runTransaction<const SelectArg, &DBAgent::select>(arg);
	}

	void runTransaction(const SelectExArg &arg)
	{
		_runTransaction<const SelectExArg, &DBAgent::select>(arg);
	}

	void runTransaction(const UpdateArg &arg)
	{
		_runTransaction<const UpdateArg, &DBAgent::update>(arg);
	}

	void runTransaction(const DeleteArg &arg)
	{
		_runTransaction<const DeleteArg, &DBAgent::deleteRows>(arg);
	}

	/**
	 * Insert rows in runTransaction.
	 *
	 * @param arg An InsertArg instance.
	 * @param id
	 * the lastly inserted row ID is returned.
	 */
	void runTransaction(const InsertArg &arg, int & id);
	void runTransaction(const InsertArg &arg, uint64_t & id);

	static bool isAutoIncrementValue(const ItemData *item);

protected:
	static std::string makeSelectStatement(const SelectArg &selectArg);
	static std::string makeSelectStatement(const SelectExArg &selectExArg);
	static std::string makeDeleteStatement(const DeleteArg &deleteArg);
	static std::string makeRenameTableStatement(
	  const std::string &srcName,
	  const std::string &destName);
	static std::string makeDatetimeString(int datetime);
	std::string makeUpdateStatement(const UpdateArg &updateArg);

	virtual std::string getColumnValueString(const ColumnDef *columnDef,
						 const ItemData *itemData);

	virtual std::string
	  makeCreateIndexStatement(const TableProfile &tableProfile,
	                           const IndexDef &indexDef) = 0;
	virtual void createIndex(const TableProfile &tableProfile,
	                         const IndexDef &indexDef);

	virtual std::string
	makeDropIndexStatement(const std::string &name,
	                       const std::string &tableName) = 0;
	virtual void dropIndex(
	  const std::string &name, const std::string &tableName);

	virtual void getIndexInfoVect(
	  std::vector<IndexInfo> &indexInfoVect,
	  const TableProfile &tableProfile) = 0;

	virtual std::string makeIndexName(const TableProfile &tableProfile,
	                                  const IndexDef &indexDef);

private:
	struct Impl;
};

