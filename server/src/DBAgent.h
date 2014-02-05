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
#include <glib.h>
#include <stdint.h>
#include "Params.h"
#include "SQLProcessorTypes.h"

static const int      AUTO_INCREMENT_VALUE = 0;
static const uint64_t AUTO_INCREMENT_VALUE_U64 = 0;
static const int CURR_DATETIME = -1;

struct DBAgentSelectExArg {
	std::string                tableName;
	std::vector<std::string>   statements;
	std::vector<SQLColumnType> columnTypes;
	std::string condition;
	std::string orderBy;
	size_t limit;
	size_t offset;

	// output
	ItemTablePtr        dataTable;

	// constructor and methods
	DBAgentSelectExArg(void);
	void pushColumn(const ColumnDef &columnDef,
	                const std::string &varName = "");
};

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

class DBAgent {
public:
	struct TableProfile {
		const char         *name;
		const ColumnDef    *columnDefs;
		const size_t        numColumns;

		TableProfile(const char *name,  const ColumnDef *columnDefs,
		             const size_t &columnDefSize,
		             const size_t &numIndexes);
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

		InsertArg(const TableProfile &tableProfile);
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
	};

	struct SelectExArg {
		const TableProfile        *tableProfile;
		std::vector<std::string>   statements;
		std::vector<SQLColumnType> columnTypes;
		std::string                condition;
		std::string                orderBy;
		size_t                     limit;
		size_t                     offset;
		// output
		mutable ItemTablePtr        dataTable;

		SelectExArg(const TableProfile &tableProfile);
		void add(const size_t &columnIndex,
		         const std::string &varName = "");
		void add(const std::string &statement,
		         const SQLColumnType &columnType);
	};

	struct TableProfileEx {
		const TableProfile *profile;
		const char         *varName;
	};

	struct SelectMultiTableArg : public SelectExArg {
		const TableProfileEx *tableProfiles;
		const size_t          numTables;

		SelectMultiTableArg(const TableProfileEx *tableProfiles,
		                    const size_t &numTables);
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
	virtual void createTable(const TableProfile &tableProfile) = 0;
	virtual void insert(const InsertArg &insertArg) = 0;
	virtual void update(const UpdateArg &updateArg) = 0;
	virtual void select(const SelectArg &selectArg) = 0;
	virtual void select(DBAgentSelectExArg &selectExArg) = 0;
	virtual void select(const SelectExArg &selectExArg) = 0;
	virtual void deleteRows(const DeleteArg &deleteArg) = 0;
	virtual void addColumns(const AddColumnsArg &addColumnsArg) = 0;
	virtual uint64_t getLastInsertId(void) = 0;
	virtual uint64_t getNumberOfAffectedRows(void) = 0;

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

protected:
	static std::string makeSelectStatement(const SelectArg &selectArg);
	static std::string makeSelectStatement(DBAgentSelectExArg &selectExArg);
	static std::string makeSelectStatement(const SelectExArg &selectExArg);
	static std::string getColumnValueString(const ColumnDef *columnDef,
	                                        const ItemData *itemData);
	static std::string makeUpdateStatement(const UpdateArg &updateArg);
	static std::string makeDeleteStatement(const DeleteArg &deleteArg);
	static std::string makeDatetimeString(int datetime);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgent_h
