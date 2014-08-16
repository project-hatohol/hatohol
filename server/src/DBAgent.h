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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
#include <memory>
#include <glib.h>
#include <stdint.h>
#include "Params.h"
#include "SQLProcessorTypes.h"
#include "DBTermCodec.h"

static const int      AUTO_INCREMENT_VALUE = 0;
static const uint64_t AUTO_INCREMENT_VALUE_U64 = 0;
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

typedef void (*DBSetupFunc)(DBDomainId domainId, void *data);
static const DBDomainId DEFAULT_DB_DOMAIN_ID = 0;

// Initialize DBAgent::TableProfile
// Note: Compile fails if number of coldefs does not match numcols.
#define DBAGENT_TABLEPROFILE_INIT(name,coldefs,numcols)			\
	DBAgent::TableProfile(name, coldefs,				\
			     HATOHOL_BUILD_EXPECT(ARRAY_SIZE(coldefs), 	\
						  numcols)	\
	)

class DBAgent {
public:
	struct TableProfile {
		const char         *name;
		const ColumnDef    *columnDefs;
		const size_t        numColumns;

		TableProfile(const char *name,  const ColumnDef *columnDefs,
		             const size_t &numIndexes);
	};

	struct IndexDef {
		static const int    END = -1;

		const char         *name;
		const TableProfile *tableProfile;
		const int          *columnIndexes; // Last element must be END.
		bool                isUnique;
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
		void add(const size_t &columnIndex, const int         &val);
		void add(const size_t &columnIndex, const uint64_t    &val);
		void add(const size_t &columnIndex, const double      &val);
		void add(const size_t &columnIndex, const std::string &val);
		void add(const size_t &columnIndex, const time_t      &val);
	};

	struct SelectArg {
		const TableProfile &tableProfile;
		std::vector<size_t> columnIndexes;
		// output
		mutable ItemTablePtr dataTable;

		SelectArg(const TableProfile &tableProfile);
		void add(const size_t &columnIndex);
	};

	struct SelectExArg {
		const TableProfile        *tableProfile;
		std::vector<std::string>   statements;
		std::vector<SQLColumnType> columnTypes;
		std::string                condition;
		std::string                orderBy;
		size_t                     limit;
		size_t                     offset;
		std::string                tableField;
		bool                       useFullName;
		bool                       useDistinct;
		// output
		mutable ItemTablePtr        dataTable;

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

	static void addSetupFunction(DBDomainId domainId,
	                             DBSetupFunc setupFunc, void *data = NULL);

	DBAgent(DBDomainId = DEFAULT_DB_DOMAIN_ID, bool skipSetup = false);
	virtual ~DBAgent();
	DBDomainId getDBDomainId(void) const;

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
	virtual void renameTable(const std::string &sourceName,
				 const std::string &destName) = 0;
	virtual uint64_t getLastInsertId(void) = 0;
	virtual uint64_t getNumberOfAffectedRows(void) = 0;

	/**
	 * Create and drop indexes if needed.
	 *
	 * @param tableProfile
	 * A TableProfile structure concerned with the indexes to be created.
	 *
	 * @param indexesDefArray
	 * An array of IndexDef. A member: 'name' of the final element shall
	 * be NULL for a mark of the termination.
	 */
	virtual void fixupIndexes(const TableProfile &tableProfile,
	                          const IndexDef *indexDefArray);

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

	struct TransactionProc {
		/**
		 * This method is called before the transaction.
		 * @return
		 * If false is returned, transaction() immediately returns
		 * without doing a transaction and calling postproc().
		 */
		virtual bool preproc(DBAgent &dbAgent);

		/**
		 * This method is called after the transaction.
		 */
		virtual void postproc(DBAgent &dbAgent);

		virtual void operator ()(DBAgent &dbAgent) = 0;
	};

	void transaction(TransactionProc &proc);

	template <typename T, void (DBAgent::*OPERATION)(const T &)>
	void _transaction(T &arg)
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
		transaction(trx);
	}

	void transaction(const SelectArg &arg)
	{
		_transaction<const SelectArg, &DBAgent::select>(arg);
	}

	void transaction(const SelectExArg &arg)
	{
		_transaction<const SelectExArg, &DBAgent::select>(arg);
	}

	void transaction(const UpdateArg &arg)
	{
		_transaction<const UpdateArg, &DBAgent::update>(arg);
	}

	void transaction(const DeleteArg &arg)
	{
		_transaction<const DeleteArg, &DBAgent::deleteRows>(arg);
	}

	/**
	 * Insert rows in transaction.
	 *
	 * @param arg An InsertArg instance.
	 * @param id
	 * If this is not NULL, the lastly inserted row ID is returned.
	 */
	void transaction(const InsertArg &arg, int *id = NULL);
	void transaction(const InsertArg &arg, uint64_t *id = NULL);

protected:
	static std::string makeSelectStatement(const SelectArg &selectArg);
	static std::string makeSelectStatement(const SelectExArg &selectExArg);
	static std::string getColumnValueString(const ColumnDef *columnDef,
	                                        const ItemData *itemData);
	static std::string makeUpdateStatement(const UpdateArg &updateArg);
	static std::string makeDeleteStatement(const DeleteArg &deleteArg);
	static std::string makeRenameTableStatement(
	  const std::string &srcName,
	  const std::string &destName);
	static std::string makeDatetimeString(int datetime);

	virtual std::string
	makeCreateIndexStatement(const IndexDef &indexDef) = 0;
	virtual void createIndex(const IndexDef &indexDef);

	virtual std::string
	makeDropIndexStatement(const std::string &name,
	                       const std::string &tableName) = 0;
	virtual void dropIndex(
	  const std::string &name, const std::string &tableName);

	virtual void getIndexInfoVect(
	  std::vector<IndexInfo> &indexInfoVect,
	  const TableProfile &tableProfile) = 0;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

#endif // DBAgent_h
