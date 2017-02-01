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

#include <Mutex.h>
#include "SQLUtils.h"
#include "DBAgent.h"
#include "HatoholException.h"
#include "SeparatorInjector.h"
#include "DBTermCStringProvider.h"
using namespace std;
using namespace mlpl;

DBConnectInfo::DBConnectInfo(void)
: host("localhost"),
  port(0)
{
}

DBConnectInfo::~DBConnectInfo()
{
}

void DBConnectInfo::reset(void)
{
	host = "localhost";
	port = 0;

	user.clear();
	password.clear();
	dbName.clear();
}

const char *DBConnectInfo::getHost(void) const
{
	if (host.empty())
		return NULL;
	return host.c_str();
}

const char *DBConnectInfo::getUser(void) const
{
	if (user.empty())
		return NULL;
	return user.c_str();
}

const char *DBConnectInfo::getPassword(void) const
{
	if (password.empty())
		return NULL;
	return password.c_str();
}

struct DBAgent::Impl
{
	static DBTermCodec         dbTermCodec;
};

DBTermCodec    DBAgent::Impl::dbTermCodec;

// ---------------------------------------------------------------------------
// DBAgent::TableProfile
// ---------------------------------------------------------------------------
DBAgent::TableProfile::TableProfile(
  const char *_name,  const ColumnDef *_columnDefs,
  const size_t &numIndexes, const IndexDef *_indexDefArray)
: name(_name),
  columnDefs(_columnDefs),
  numColumns(numIndexes),
  indexDefArray(_indexDefArray)
{
	// We assume there's one (combined) unique key at most every table.
	// This limited is in order to realize upsert on SQLite3.
	size_t numUniqueKeys = 0;
	for (size_t i = 0; i < numIndexes; i++) {
		const ColumnDef &columnDef = columnDefs[i];
		if (columnDef.keyType != SQL_KEY_UNI)
			continue;
		numUniqueKeys++;
		uniqueKeyColumnIndexes.push_back(i);
	}

	const IndexDef *indexDef = indexDefArray;
	for (; indexDef && indexDef->name; indexDef++) {
		if (!indexDef->isUnique)
			continue;
		numUniqueKeys++;
		const int *columnIdx = indexDef->columnIndexes;
		for (; *columnIdx != IndexDef::END; columnIdx++)
			uniqueKeyColumnIndexes.push_back(*columnIdx);
	}

	HATOHOL_ASSERT(numUniqueKeys <= 1,
	               "The number of unique keys must be 0 or 1: %zd\n",
	               numUniqueKeys);
}

std::string DBAgent::TableProfile::getFullColumnName(const size_t &index) const
{
	const ColumnDef &def = columnDefs[index];
	string fullName = name;
	fullName += ".";
	fullName += def.columnName;
	return fullName;
}

// ---------------------------------------------------------------------------
// DBAgent::RowElement
// ---------------------------------------------------------------------------
DBAgent::RowElement::RowElement(const size_t &index, const ItemData *itemData,
                               const bool &doRef)
: columnIndex(index),
  dataPtr(itemData, doRef)
{
}

// ---------------------------------------------------------------------------
// DBAgent::InsertArg
// ---------------------------------------------------------------------------
DBAgent::InsertArg::InsertArg(const TableProfile &profile)
: tableProfile(profile),
  upsertOnDuplicate(false)
{
}

void DBAgent::InsertArg::add(const int &val,
                             const ItemDataNullFlagType &nullFlag)
{
	row->addNewItem(val, nullFlag);
}

void DBAgent::InsertArg::add(const uint64_t &val,
                             const ItemDataNullFlagType &nullFlag)
{
	row->addNewItem(val, nullFlag);
}

void DBAgent::InsertArg::add(const double &val,
                             const ItemDataNullFlagType &nullFlag)
{
	row->addNewItem(val, nullFlag);
}

void DBAgent::InsertArg::add(const string &val,
                             const ItemDataNullFlagType &nullFlag)
{
	row->addNewItem(val, nullFlag);
}

void DBAgent::InsertArg::add(const time_t &val,
                             const ItemDataNullFlagType &nullFlag)
{
	row->addNewItem(val, nullFlag);
}

// ---------------------------------------------------------------------------
// DBAgent::UpdateArg
// ---------------------------------------------------------------------------
DBAgent::UpdateArg::UpdateArg(const TableProfile &profile)
: tableProfile(profile)
{
}

DBAgent::UpdateArg::~UpdateArg()
{
	for (size_t i = 0; i < rows.size(); i++)
		delete rows[i];
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const int &val,
                             const ItemDataNullFlagType &nullFlag)
{
	rows.push_back(new RowElement(columnIndex,
	                              new ItemInt(val, nullFlag), false));
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const uint64_t &val,
                             const ItemDataNullFlagType &nullFlag)
{
	rows.push_back(new RowElement(columnIndex,
	                              new ItemUint64(val, nullFlag), false));
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const double &val,
                             const ItemDataNullFlagType &nullFlag)
{
	rows.push_back(new RowElement(columnIndex,
	                              new ItemDouble(val, nullFlag), false));
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const std::string &val,
                             const ItemDataNullFlagType &nullFlag)
{
	rows.push_back(new RowElement(columnIndex,
	                              new ItemString(val, nullFlag), false));
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const time_t &val,
                             const ItemDataNullFlagType &nullFlag)
{
	rows.push_back(new RowElement(columnIndex,
	                              new ItemInt(val, nullFlag), false));
}

void DBAgent::UpdateArg::add(const size_t &columnIndex, const ItemGroup *grp)
{
	rows.push_back(new RowElement(columnIndex,
	                              grp->getItemAt(columnIndex), true));
}

// ---------------------------------------------------------------------------
// DBAgent::SelectArg
// ---------------------------------------------------------------------------
DBAgent::SelectArg::SelectArg(const TableProfile &profile)
: tableProfile(profile)
{
}

void DBAgent::SelectArg::add(const size_t &columnIndex)
{
	columnIndexes.push_back(columnIndex);
}

// ---------------------------------------------------------------------------
// DBAgent::SelectExArg
// ---------------------------------------------------------------------------
DBAgent::SelectExArg::SelectExArg(const TableProfile &profile)
: tableProfile(&profile),
  limit(0),
  offset(0),
  useFullName(false),
  useDistinct(false)
{
}

void DBAgent::SelectExArg::add(const size_t &columnIndex)
{
	string statement;
	const ColumnDef &columnDef = tableProfile->columnDefs[columnIndex];
	if (useFullName) {
		statement = tableProfile->getFullColumnName(columnIndex);
	} else {
		statement = columnDef.columnName;
	}
	statements.push_back(statement);
	columnTypes.push_back(columnDef.type);
}

void DBAgent::SelectExArg::add(
  const std::string &statement, const SQLColumnType &columnType)
{
	statements.push_back(statement);
	columnTypes.push_back(columnType);
}

// ---------------------------------------------------------------------------
// DBAgent::SelectMultiTableArg
// ---------------------------------------------------------------------------
DBAgent::SelectMultiTableArg::SelectMultiTableArg(
  const TableProfile **_profiles, const size_t &_numTables)
: SelectExArg(*_profiles[0]),
  profiles(_profiles),
  numTables(_numTables)
{
	SelectExArg::useFullName = true;
}

void DBAgent::SelectMultiTableArg::setTable(const size_t &index)
{
	HATOHOL_ASSERT(index < numTables, "index (%zd) >= numTables (%zd)",
	               index, numTables);
	SelectExArg::tableProfile = profiles[index];
}

void DBAgent::SelectMultiTableArg::add(const size_t &columnIndex)
{
	SelectExArg::add(columnIndex);
}

string DBAgent::SelectMultiTableArg::getFullName(const size_t &tableIndex,
                                                 const size_t &columnIndex)
{
	HATOHOL_ASSERT(tableIndex < numTables,
	               "tableIndex (%zd) >= numTables (%zd)",
	               tableIndex, numTables);
	const TableProfile *profile = profiles[tableIndex];
	return profile->getFullColumnName(columnIndex);
}

// ---------------------------------------------------------------------------
// DBAgent::DeleteArg
// ---------------------------------------------------------------------------
DBAgent::DeleteArg::DeleteArg(const TableProfile &profile)
: tableProfile(profile)
{
}

// ---------------------------------------------------------------------------
// DBAgent::AddColumnsArg
// ---------------------------------------------------------------------------
DBAgent::AddColumnsArg::AddColumnsArg(const TableProfile &profile)
: tableProfile(profile)
{
}

// ---------------------------------------------------------------------------
// TransactionHooks
// ---------------------------------------------------------------------------
bool DBAgent::TransactionHooks::preAction(DBAgent &dbAgent)
{
	return true;
}

bool DBAgent::TransactionHooks::postAction(DBAgent &dbAgent)
{
	return true;
}

// ---------------------------------------------------------------------------
// TransactionProc
// ---------------------------------------------------------------------------
bool DBAgent::TransactionProc::preproc(DBAgent &dbAgent)
{
	return true;
}

void DBAgent::TransactionProc::postproc(DBAgent &dbAgent)
{
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBAgent::DBAgent(void)
{
}

DBAgent::~DBAgent()
{
}

void DBAgent::dropTable(const std::string &tableName)
{
	string sql = "DROP TABLE ";
	sql += tableName;
	execSql(sql);
}

void DBAgent::fixupIndexes(const TableProfile &tableProfile)
{
	typedef map<string, IndexInfo *>   IndexNameInfoMap;
	typedef IndexNameInfoMap::iterator IndexNameInfoMapIterator;

	struct {
		DBAgent        *obj;
		IndexNameInfoMap existingIndexMap;

		void proc(const TableProfile &tableProfile,
		          const IndexDef &indexDef)
		{
			// If there's no index with the same name,
			// the new one should be created.
			const string indexName =
			  obj->makeIndexName(tableProfile, indexDef);
			IndexNameInfoMapIterator it =
			  existingIndexMap.find(indexName);
			if (it == existingIndexMap.end()) {
				obj->createIndex(tableProfile, indexDef);
				return;
			}

			// We create the index only when the requested
			// SQL is different from the existing one.
			const IndexInfo &indexInfo = *it->second;
			const string &existingSql = indexInfo.sql;
			const string reqSql =
			  obj->makeCreateIndexStatement(tableProfile, indexDef);
			if (reqSql != existingSql) {
				obj->dropIndex(indexName, tableProfile.name);
				obj->createIndex(tableProfile, indexDef);
			}
			existingIndexMap.erase(it);
		}
	} ctx;
	ctx.obj = this;

	// Gather existing indexes
	IndexNameInfoMap existingIndexNameInfoMap;
	vector<IndexInfo> indexInfoVect;
	getIndexInfoVect(indexInfoVect, tableProfile);
	for (size_t i = 0; i < indexInfoVect.size(); i++) {
		IndexInfo &idxInfo = indexInfoVect[i];
		ctx.existingIndexMap[idxInfo.name] = &idxInfo;
	}

	// Create needed indexes with ColumnDef
	for (size_t i = 0; i < tableProfile.numColumns; i++) {
		const ColumnDef &columnDef = tableProfile.columnDefs[i];
		bool isUnique = false;
		switch (columnDef.keyType) {
		case SQL_KEY_UNI:
			isUnique = true;
		case SQL_KEY_IDX:
			break;
		default:
			continue;
		}
		const int columnIndexes[] = {(int)i, IndexDef::END};
		const IndexDef indexDef = {
		  columnDef.columnName, columnIndexes, isUnique
		};
		ctx.proc(tableProfile, indexDef);
	}

	// Select really necessary indexes for the creation.
	const IndexDef *indexDefPtr = tableProfile.indexDefArray;
	for (; indexDefPtr && indexDefPtr->name; indexDefPtr++)
		ctx.proc(tableProfile, *indexDefPtr);

	// Drop remaining (unnecessary) indexes
	while (!ctx.existingIndexMap.empty()) {
		IndexNameInfoMapIterator it = ctx.existingIndexMap.begin();
		const IndexInfo &indexInfo = *it->second;
		dropIndex(indexInfo.name, indexInfo.tableName);
		ctx.existingIndexMap.erase(it);
	}
}

void DBAgent::runTransaction(TransactionProc &proc, TransactionHooks *hooks)
{
	auto preAction = [&] {
		if (hooks && !hooks->preAction(*this))
			throw TransactionAbort();
	};

	auto postAction = [&] {
		if (hooks && !hooks->postAction(*this))
			throw TransactionAbort();
	};

	if (!proc.preproc(*this))
		return;
	begin();
	try {
		preAction();
		proc(*this);
		postAction();
	} catch (const TransactionAbort &e) {
		rollback();
		return;
	} catch (...) {
		rollback();
		throw;
	};
	commit();
	proc.postproc(*this);
}

template <typename T>
struct TrxInsert : public DBAgent::TransactionProc {
	const DBAgent::InsertArg &arg;
	T & id;

	TrxInsert(const DBAgent::InsertArg &_arg, T & _id)
	: arg(_arg),
	  id(_id)
	{
	}

	void operator ()(DBAgent &dbAgent) override
	{
		dbAgent.insert(arg);
		id = dbAgent.getLastInsertId();
	}
};

void DBAgent::runTransaction(const InsertArg &arg, int & id)
{
	TrxInsert<int> trx(arg, id);
	runTransaction(trx);
}

void DBAgent::runTransaction(const InsertArg &arg, uint64_t & id)
{
	TrxInsert<uint64_t> trx(arg, id);
	runTransaction(trx);
}

bool DBAgent::isAutoIncrementValue(const ItemData *item)
{
	const ItemDataType type = item->getItemType();
	if (type == ITEM_TYPE_INT) {
		const int &val = static_cast<const int &>(*item);
		return (val == AUTO_INCREMENT_VALUE);
	 } else if (type == ITEM_TYPE_UINT64) {
		const uint64_t &val = static_cast<const uint64_t &>(*item);
		return (val == AUTO_INCREMENT_VALUE_U64);
	}
	return false;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string DBAgent::makeSelectStatement(const SelectArg &selectArg)
{
	size_t numColumns = selectArg.columnIndexes.size();
	string sql = "SELECT ";
	for (size_t i = 0; i < numColumns; i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef =
		  selectArg.tableProfile.columnDefs[idx];
		sql += columnDef.columnName;
		sql += " ";
		if (i < selectArg.columnIndexes.size()- 1)
			sql += ",";
	}
	sql += "FROM ";
	sql += selectArg.tableProfile.name;
	return sql;
}

string DBAgent::makeSelectStatement(const SelectExArg &selectExArg)
{
	size_t numColumns = selectExArg.statements.size();
	HATOHOL_ASSERT(numColumns > 0, "Vector size must not be zero");
	HATOHOL_ASSERT(numColumns == selectExArg.columnTypes.size(),
	             "Vector size mismatch: statements (%zd):columnTypes (%zd)",
	             numColumns, selectExArg.columnTypes.size());

	string sql = "SELECT ";
	if (selectExArg.useDistinct)
		sql += "DISTINCT ";
	for (size_t i = 0; i < numColumns; i++) {
		sql += selectExArg.statements[i];
		if (i < numColumns-1)
			sql += ",";
	}
	sql += " FROM ";
	if (!selectExArg.tableField.empty())
		sql += selectExArg.tableField;
	else
		sql += selectExArg.tableProfile->name;
	if (!selectExArg.condition.empty()) {
		sql += " WHERE ";
		sql += selectExArg.condition;
	}
	if (!selectExArg.groupBy.empty()) {
		sql += " GROUP BY ";
		sql += selectExArg.groupBy;
	}
	if (!selectExArg.orderBy.empty()) {
		sql += " ORDER BY ";
		sql += selectExArg.orderBy;
	}
	if (selectExArg.limit > 0)
		sql += StringUtils::sprintf(" LIMIT %zd ", selectExArg.limit);
	if (selectExArg.offset > 0)
		sql += StringUtils::sprintf(" OFFSET %zd ", selectExArg.offset);
	return sql;
}


string DBAgent::getColumnValueString(const ColumnDef *columnDef,
                                     const ItemData *itemData)
{
	if (itemData->isNull())
		return "NULL";

	string valueStr;
	switch (columnDef->type) {
	case SQL_COLUMN_TYPE_INT:
	{
		valueStr = StringUtils::sprintf("%d", (int)*itemData);
		break;
	}
	case SQL_COLUMN_TYPE_BIGUINT:
	{
		valueStr = StringUtils::sprintf("%" PRIu64,
						(uint64_t)*itemData);
		break;
	}
	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
	{
		DBTermCStringProvider rhs(*getDBTermCodec());
		valueStr = rhs(static_cast<string>(*itemData));
		break;
	}
	case SQL_COLUMN_TYPE_DOUBLE:
	{
		string fmt
		  = StringUtils::sprintf("%%.%zdlf", columnDef->decFracLength);
		valueStr = StringUtils::sprintf(fmt.c_str(), (double)*itemData);
		break;
	}
	case SQL_COLUMN_TYPE_DATETIME:
	{
		valueStr = makeDatetimeString(*itemData);
		break;
	}
	default:
		HATOHOL_ASSERT(true, "Unknown column type: %d (%s)",
		             columnDef->type, columnDef->columnName);
	}
	return valueStr;
}

string DBAgent::makeUpdateStatement(const UpdateArg &updateArg)
{
	// make a SQL statement
	string statement = StringUtils::sprintf("UPDATE %s SET ",
	                                        updateArg.tableProfile.name);
	const size_t numColumns = updateArg.rows.size();
	for (size_t i = 0; i < numColumns; i++) {
		const RowElement *elem = updateArg.rows[i];
		const ColumnDef &columnDef =
		  updateArg.tableProfile.columnDefs[elem->columnIndex];
		const string valueStr =
		  getColumnValueString(&columnDef, elem->dataPtr);

		statement += StringUtils::sprintf("%s=%s",
		                                  columnDef.columnName,
		                                  valueStr.c_str());
		if (i < numColumns - 1)
			statement += ",";
	}

	// condition
	if (!updateArg.condition.empty()) {
		statement += StringUtils::sprintf(" WHERE %s",
		                                  updateArg.condition.c_str());
	}
	return statement;
}

string DBAgent::makeDeleteStatement(const DeleteArg &deleteArg)
{
	string statement = "DELETE FROM ";
	statement += deleteArg.tableProfile.name;
	if (!deleteArg.condition.empty()) {
		statement += " WHERE ";
		statement += deleteArg.condition;
	}
	return statement;
}

string DBAgent::makeRenameTableStatement(const string &srcName,
					 const string &destName)
{
	return StringUtils::sprintf("ALTER TABLE %s RENAME TO %s",
				    srcName.c_str(),
				    destName.c_str());
}

string DBAgent::makeDatetimeString(int datetime)
{
	time_t clock;
	if (datetime == CURR_DATETIME)
		time(&clock);
	else
		clock = (time_t)datetime;
	struct tm tm;
	localtime_r(&clock, &tm);
	string str = StringUtils::sprintf(
	               "'%04d-%02d-%02d %02d:%02d:%02d'",
	               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	               tm.tm_hour, tm.tm_min, tm.tm_sec);
	return str;
}

bool DBAgent::updateIfExistElseInsert(
  const ItemGroup *itemGroup, const TableProfile &tableProfile,
  size_t targetIndex)
{
	size_t numElemInItemGrp = itemGroup->getNumberOfItems();
	HATOHOL_ASSERT(targetIndex < numElemInItemGrp,
	               "targetIndex: %zd, number of items: %zd",
	               targetIndex, numElemInItemGrp);
	HATOHOL_ASSERT(tableProfile.numColumns == numElemInItemGrp,
	               "numColumns: %zd, number of items: %zd",
	               tableProfile.numColumns, numElemInItemGrp);
	const ItemData *item = itemGroup->getItemAt(targetIndex);
	const char *columnName =
	  tableProfile.columnDefs[targetIndex].columnName;
	string condition = StringUtils::sprintf("%s=%s", columnName,
	                                        item->getString().c_str());
	bool exist = isRecordExisting(tableProfile.name, condition);
	if (exist) {
		// update
		DBAgent::UpdateArg arg(tableProfile);
		VariableItemGroupPtr row;
		for (size_t i = 0; i < tableProfile.numColumns; i++) {
			// exclude a primary key
			if (tableProfile.columnDefs[i].keyType == SQL_KEY_PRI)
				continue;
			arg.rows.push_back(
			  new RowElement(i, itemGroup->getItemAt(i)));
		}
		arg.condition = condition;
		update(arg);
	} else {
		// insert
		DBAgent::InsertArg arg(tableProfile);
		for (size_t i = 0; i < tableProfile.numColumns; i++)
			arg.row->add(itemGroup->getItemAt(i));
		insert(arg);
	}
	return exist;
}

const DBTermCodec *DBAgent::getDBTermCodec(void) const
{
	return &Impl::dbTermCodec;
}

void DBAgent::createIndex(const TableProfile &tableProfile,
                          const IndexDef &indexDef)
{
	execSql(makeCreateIndexStatement(tableProfile, indexDef));
	MLPL_INFO("Created index: %s (table: %s)\n",
	          indexDef.name, tableProfile.name);
}

void DBAgent::dropIndex(const std::string &name, const std::string &tableName)
{
	execSql(makeDropIndexStatement(name, tableName));
	MLPL_INFO("Deleted index: %s (table: %s)\n",
	          name.c_str(), tableName.c_str());
}

string DBAgent::makeIndexName(const TableProfile &tableProfile,
                              const IndexDef &indexDef)
{
	return indexDef.name;
}

