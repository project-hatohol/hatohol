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

#include <mysql/errmsg.h>
#include <unistd.h>
#include "DBAgentMySQL.h"
#include "SQLUtils.h"
#include "SeparatorInjector.h"
#include "Params.h"
using namespace std;
using namespace mlpl;

static const size_t DEFAULT_NUM_RETRY = 5;
static const size_t RETRY_INTERVAL[DEFAULT_NUM_RETRY] = {
  0, 10, 60, 60, 60 };

struct DBAgentMySQL::Impl {
	static string engineStr;
	static set<unsigned int> retryErrorSet;
	MYSQL mysql;
	bool  connected;
	string dbName;
	string user;
	string password;
	string host;
	unsigned int port;
	bool inTransaction;

	Impl(void)
	: connected(false),
	  port(0),
	  inTransaction(false)
	{
	}

	~Impl(void)
	{
		if (connected) {
			mysql_close(&mysql);
		}
	}
	
	bool shouldRetry(unsigned int errorNumber)
	{
		return retryErrorSet.find(errorNumber) != retryErrorSet.end();
	}
};

string DBAgentMySQL::Impl::engineStr;
set<unsigned int> DBAgentMySQL::Impl::retryErrorSet;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgentMySQL::init(void)
{
	char *env = getenv("HATOHOL_MYSQL_ENGINE_MEMORY");
	if (env && atoi(env) == 1) {
		MLPL_INFO("Use memory engine\n");
		Impl::engineStr = " ENGINE=MEMORY";
	}

	Impl::retryErrorSet.insert(CR_SERVER_GONE_ERROR);
}

DBAgentMySQL::DBAgentMySQL(const char *db, const char *user, const char *passwd,
                           const char *host, unsigned int port)
: m_impl(new Impl())
{
	m_impl->dbName   = db     ? : "";
	m_impl->user     = user   ? : "";
	m_impl->password = passwd ? : "";
	m_impl->host     = host   ? : "";
	m_impl->port     = port;
	connect();
	if (!m_impl->connected) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_CONNECT_MYSQL,
		  "Failed to connect to MySQL: %s: %s\n", 
		  db, mysql_error(&m_impl->mysql));
	}
}

DBAgentMySQL::~DBAgentMySQL()
{
}

string DBAgentMySQL::getDBName(void) const
{
	return m_impl->dbName;
}

void DBAgentMySQL::getIndexes(std::vector<IndexStruct> &indexStructVect,
                              const std::string &tableName)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string query =
	  StringUtils::sprintf("SHOW INDEX FROM %s", tableName.c_str());
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_impl->mysql));
	}

	indexStructVect.reserve(mysql_num_rows(result));
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result))) {
		int col = 0;
		IndexStruct idxStruct;
		idxStruct.table      = row[col++];
		idxStruct.nonUnique  = atoi(row[col++]);
		idxStruct.keyName    = row[col++];
		idxStruct.seqInIndex = atoi(row[col++]);
		idxStruct.columnName = row[col++];
		indexStructVect.push_back(idxStruct);
	}
	mysql_free_result(result);
}

bool DBAgentMySQL::isTableExisting(const string &tableName)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string query =
	  StringUtils::sprintf(
	    "SHOW TABLES FROM %s LIKE '%s'",
	    getDBName().c_str(), tableName.c_str());
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_impl->mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	while ((row = mysql_fetch_row(result))) {
		found = true;
		break;
	}
	mysql_free_result(result);

	return found;
}

bool DBAgentMySQL::isRecordExisting(const string &tableName,
                                    const string &condition)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string query =
	  StringUtils::sprintf(
	    "SELECT * FROM %s WHERE %s",
	    tableName.c_str(), condition.c_str());
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_impl->mysql));
	}

	MYSQL_ROW row;
	bool found = false;
	while ((row = mysql_fetch_row(result))) {
		found = true;
		break;
	}
	mysql_free_result(result);

	return found;
}

void DBAgentMySQL::begin(void)
{
	execSql("START TRANSACTION");
	m_impl->inTransaction = true;
}

void DBAgentMySQL::commit(void)
{
	execSql("COMMIT");
	m_impl->inTransaction = false;
}

void DBAgentMySQL::rollback(void)
{
	execSql("ROLLBACK");
	m_impl->inTransaction = false;
}

void DBAgentMySQL::execSql(const string &statement)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	queryWithRetry(statement);
}

static string getColumnTypeQuery(const ColumnDef &columnDef)
{
	switch (columnDef.type) {
	case SQL_COLUMN_TYPE_INT:
		return StringUtils::sprintf("INT(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_BIGUINT:
		return StringUtils::sprintf("BIGINT(%zd) UNSIGNED",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_VARCHAR:
		return StringUtils::sprintf("VARCHAR(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_CHAR:
		return StringUtils::sprintf("CHAR(%zd)",
					    columnDef.columnLength);
	case SQL_COLUMN_TYPE_TEXT:
		return "TEXT";
	case SQL_COLUMN_TYPE_DOUBLE:
		return StringUtils::sprintf("DOUBLE(%zd,%zd)",
					    columnDef.columnLength,
					    columnDef.decFracLength);
	case SQL_COLUMN_TYPE_DATETIME:
		return " DATETIME";
	default:
		HATOHOL_ASSERT(false, "Unknown type: %d", columnDef.type);
	}
	return string();
}

static string getColumnDefinitionQuery(const ColumnDef &columnDef)
{
	string query;

	query += columnDef.columnName;
	query += " ";
	query += getColumnTypeQuery(columnDef);

	// nullable
	if (!columnDef.canBeNull)
		query += " NOT NULL";

	// key type
	switch (columnDef.keyType) {
	case SQL_KEY_PRI:
		query += " PRIMARY KEY";
		break;
	case SQL_KEY_UNI:
	case SQL_KEY_IDX: // To be created in createIndexIfNotExists()
	case SQL_KEY_NONE:
		break;
	default:
		HATOHOL_ASSERT(false,
		  "Unknown key type: %d", columnDef.keyType);
	}

	return query;
}

void DBAgentMySQL::createTable(const TableProfile &tableProfile)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string query = StringUtils::sprintf("CREATE TABLE %s (",
	                                    tableProfile.name);
	
	for (size_t i = 0; i < tableProfile.numColumns; i++) {
		const ColumnDef &columnDef = tableProfile.columnDefs[i];
		query += getColumnDefinitionQuery(columnDef);

		// auto increment
		if (columnDef.flags & SQL_COLUMN_FLAG_AUTO_INC)
			query += " AUTO_INCREMENT";

		if (i < tableProfile.numColumns -1)
			query += ",";
	}
	query += ")";
	if (!m_impl->engineStr.empty())
		query += m_impl->engineStr;

	execSql(query);
}

void DBAgentMySQL::insert(const DBAgent::InsertArg &insertArg)
{
	using mlpl::StringUtils::sprintf;
	struct {
		string operator ()(const DBAgent::InsertArg &insertArg,
		                   const size_t &idx, MYSQL *mysql)
		{
			const ColumnDef &columnDef =
			  insertArg.tableProfile.columnDefs[idx];
			const ItemData *itemData = insertArg.row->getItemAt(idx);
			if (itemData->isNull())
				return "NULL";

			switch (columnDef.type) {
			case SQL_COLUMN_TYPE_INT:
			case SQL_COLUMN_TYPE_BIGUINT:
			case SQL_COLUMN_TYPE_DOUBLE:
				return itemData->getString();

			case SQL_COLUMN_TYPE_VARCHAR:
			case SQL_COLUMN_TYPE_CHAR:
			case SQL_COLUMN_TYPE_TEXT:
			{ // bracket is used to avoid an error:
			  // jump to case label
				string src = itemData->getString();
				char *escaped = new char[src.size() * 2 + 1]; 
				mysql_real_escape_string(
				  mysql, escaped, src.c_str(), src.size());
				string val = sprintf("'%s'", escaped);
				delete [] escaped;
				return val;
			}
			case SQL_COLUMN_TYPE_DATETIME:
				return makeDatetimeString(*itemData);
			default:
				HATOHOL_ASSERT(false, "Unknown type: %d",
				               columnDef.type);
			return "";
			}
		}
	} valueMaker;

	const size_t numColumns = insertArg.tableProfile.numColumns;
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	HATOHOL_ASSERT(numColumns == insertArg.row->getNumberOfItems(),
	               "numColumn: %zd != row: %zd",
	               numColumns, insertArg.row->getNumberOfItems());

	SeparatorInjector commaInjector(",");
	string query = StringUtils::sprintf("INSERT INTO %s (",
	                                    insertArg.tableProfile.name);
	for (size_t i = 0; i < numColumns; i++) {
		const ColumnDef &columnDef = insertArg.tableProfile.columnDefs[i];
		commaInjector(query);
		query += columnDef.columnName;
	}

	vector<string> values;
	values.reserve(numColumns);
	struct UpdateParam {
		const char *column;
		const char *value;
	} updateParams[numColumns+1]; // To improve performance, we just having pointers.
	size_t updateParamIdx = 0;

	for (size_t i = 0; i < numColumns; i++) {
		const ColumnDef &columnDef = insertArg.tableProfile.columnDefs[i];
		values.push_back(valueMaker(insertArg, i, &m_impl->mysql));
		if (!insertArg.upsertOnDuplicate)
			continue;

		if (columnDef.keyType == SQL_KEY_PRI)
			continue;
		updateParams[updateParamIdx].column = columnDef.columnName;
		updateParams[updateParamIdx].value  = values.back().c_str();
		updateParamIdx++;
	}
	updateParams[updateParamIdx].column = NULL;

	query += ") VALUES (";
	commaInjector.clear();
	for (size_t i = 0; i < values.size(); i++) {
		commaInjector(query);
		query += values[i];
	}
	query += ")";

	if (insertArg.upsertOnDuplicate) {
		query += " ON DUPLICATE KEY UPDATE ";
		commaInjector.clear();
		UpdateParam *updateParam = updateParams;
		for (; updateParam->column; updateParam++) {
			commaInjector(query);
			query += sprintf("%s=%s", updateParam->column, updateParam->value);
		}
	}
	execSql(query);
}


void DBAgentMySQL::update(const UpdateArg &updateArg)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string sql = makeUpdateStatement(updateArg);
	execSql(sql);
}

void DBAgentMySQL::select(const DBAgent::SelectArg &selectArg)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");

	string query = makeSelectStatement(selectArg);
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION("Failed to call mysql_store_result: %s\n",
		                      mysql_error(&m_impl->mysql));
	}

	MYSQL_ROW row;
	VariableItemTablePtr dataTable;
	size_t numColumns = selectArg.columnIndexes.size();
	while ((row = mysql_fetch_row(result))) {
		VariableItemGroupPtr itemGroup;
		for (size_t i = 0; i < numColumns; i++) {
			size_t idx = selectArg.columnIndexes[i];
			const ColumnDef &columnDef =
			  selectArg.tableProfile.columnDefs[idx];
			ItemDataPtr itemDataPtr =
			  SQLUtils::createFromString(row[i], columnDef.type);
			itemGroup->add(itemDataPtr);
		}
		dataTable->add(itemGroup);
	}
	mysql_free_result(result);
	selectArg.dataTable = dataTable;
}

void DBAgentMySQL::select(const SelectExArg &selectExArg)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");

	string query = makeSelectStatement(selectExArg);
	execSql(query);

	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION("Failed to call mysql_store_result: %s\n",
		                      mysql_error(&m_impl->mysql));
	}

	MYSQL_ROW row;
	VariableItemTablePtr dataTable;
	size_t numColumns = selectExArg.statements.size();
	while ((row = mysql_fetch_row(result))) {
		VariableItemGroupPtr itemGroup;
		for (size_t i = 0; i < numColumns; i++) {
			SQLColumnType type = selectExArg.columnTypes[i];
			ItemDataPtr itemDataPtr =
			  SQLUtils::createFromString(row[i], type);
			itemGroup->add(itemDataPtr);
		}
		dataTable->add(itemGroup);
	}
	mysql_free_result(result);
	selectExArg.dataTable = dataTable;

	// check the result
	size_t numTableRows = selectExArg.dataTable->getNumberOfRows();
	size_t numTableColumns = selectExArg.dataTable->getNumberOfColumns();
	HATOHOL_ASSERT((numTableRows == 0) ||
	             ((numTableRows > 0) && (numTableColumns == numColumns)),
	             "Sanity check error: numTableRows: %zd, numTableColumns: "
	             "%zd, numColumns: %zd",
	             numTableRows, numTableColumns, numColumns);
}

void DBAgentMySQL::deleteRows(const DeleteArg &deleteArg)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	string query = makeDeleteStatement(deleteArg);
	execSql(query);
}

uint64_t DBAgentMySQL::getLastInsertId(void)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	execSql("SELECT LAST_INSERT_ID()");
	MYSQL_RES *result = mysql_store_result(&m_impl->mysql);
	if (!result) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call mysql_store_result: %s\n",
		  mysql_error(&m_impl->mysql));
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	HATOHOL_ASSERT(row, "Failed to call mysql_fetch_row.");
	uint64_t id;
	int numScan = sscanf(row[0], "%" PRIu64, &id);
	HATOHOL_ASSERT(numScan == 1, "numScan: %d, %s", numScan, row[0]);
	return id;
}

uint64_t DBAgentMySQL::getNumberOfAffectedRows(void)
{
	HATOHOL_ASSERT(m_impl->connected, "Not connected.");
	my_ulonglong num = mysql_affected_rows(&m_impl->mysql);
	// According to the referece manual, mysql_affected_rows()
	// doesn't return an error.
	//   http://dev.mysql.com/doc/refman/5.1/en/mysql-affected-rows.html
	return num;
}

void DBAgentMySQL::addColumns(const AddColumnsArg &addColumnsArg)
{
	string query = "ALTER TABLE ";
	query += addColumnsArg.tableProfile.name;
	vector<size_t>::const_iterator it = addColumnsArg.columnIndexes.begin();
	vector<size_t>::const_iterator lastElemIt =
	  --addColumnsArg.columnIndexes.end();

	for (; it != addColumnsArg.columnIndexes.end(); ++it) {
		const size_t index = *it;
		const ColumnDef &columnDef =
		  addColumnsArg.tableProfile.columnDefs[index];
		query += " ADD COLUMN ";
		query += getColumnDefinitionQuery(columnDef);
		if (it != lastElemIt)
			query += ",";
	}
	execSql(query);
}

void DBAgentMySQL::renameTable(const string &srcName, const string &destName)
{
	string query = makeRenameTableStatement(srcName, destName);
	execSql(query);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
const char *DBAgentMySQL::getCStringOrNullIfEmpty(const string &str)
{
	return str.empty() ? NULL : str.c_str();
}

void DBAgentMySQL::connect(void)
{
	const char *unixSocket = NULL;
	unsigned long clientFlag = 0;
	const char *host   = getCStringOrNullIfEmpty(m_impl->host);
	const char *user   = getCStringOrNullIfEmpty(m_impl->user);
	const char *passwd = getCStringOrNullIfEmpty(m_impl->password);
	const char *db     = getCStringOrNullIfEmpty(m_impl->dbName);
	mysql_init(&m_impl->mysql);
	MYSQL *result = mysql_real_connect(&m_impl->mysql, host, user, passwd,
	                                   db, m_impl->port,
	                                   unixSocket, clientFlag);
	if (!result) {
		MLPL_ERR("Failed to connect to MySQL: %s: (error: %u) %s\n",
		         db, mysql_errno(&m_impl->mysql),
		         mysql_error(&m_impl->mysql));
	}
	m_impl->connected = result;
	m_impl->inTransaction = false;
}

void DBAgentMySQL::sleepAndReconnect(unsigned int sleepTimeSec)
{
	// TODO:
	// add mechanism to wake up immediately if the program is
	// going to exit. We should make an interrputible sleep object
	// which is similar to ArmBase::sleepInterruptible().
	while (sleepTimeSec) {
		// If a signal happens during the sleep(), it returns
		// the remaining time.
		sleepTimeSec = sleep(sleepTimeSec);
	}

	mysql_close(&m_impl->mysql);
	m_impl->connected = false;
	connect();
}

void DBAgentMySQL::queryWithRetry(const string &statement)
{
	unsigned int errorNumber = 0;
	size_t numRetry = DEFAULT_NUM_RETRY;
	for (size_t i = 0; i < numRetry; i++) {
		if (mysql_query(&m_impl->mysql, statement.c_str()) == 0) {
			if (i >= 1) {
				MLPL_INFO("Recoverd: %s (retry #%zd).\n",
				          statement.c_str(), i);
			}
			return;
		}
		errorNumber = mysql_errno(&m_impl->mysql);
		if (!m_impl->shouldRetry(errorNumber))
			break;
		if (m_impl->inTransaction)
			break;
		MLPL_ERR("Failed to query: %s: (%u) %s.\n",
		         statement.c_str(), errorNumber,
		         mysql_error(&m_impl->mysql));
		if (i == numRetry - 1)
			break;

		// retry repeatedly until the connection is established or
		// the maximum retry count.
		for (; i < numRetry; i++) {
			size_t sleepTimeSec = RETRY_INTERVAL[i];
			MLPL_INFO("Try to connect after %zd sec. (%zd/%zd)\n",
			          sleepTimeSec, i+1, numRetry);
			sleepAndReconnect(sleepTimeSec);
			if (m_impl->connected)
				break;
		}
	}
	if (errorNumber == CR_SERVER_GONE_ERROR || errorNumber == CR_SERVER_LOST ) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_FAILED_CONNECT_MYSQL,
		  "Failed to connect to MySQL: %s: (%u) %s\n",
		  m_impl->dbName.c_str(), errorNumber,
		  mysql_error(&m_impl->mysql));
	} else {
		THROW_HATOHOL_EXCEPTION("Failed to query: %s: (%u) %s\n",
					statement.c_str(), errorNumber,
					mysql_error(&m_impl->mysql));
	}
}

string DBAgentMySQL::makeCreateIndexStatement(const TableProfile &tableProfile,
                                              const IndexDef &indexDef)
{
	string sql = StringUtils::sprintf(
	  "CREATE %sINDEX %s ON %s (",
	  indexDef.isUnique ? "UNIQUE " : "",
	  indexDef.name, tableProfile.name);

	const int *columnIdxPtr = indexDef.columnIndexes;
	while (true) {
		const ColumnDef &columnDef =
		  tableProfile.columnDefs[*columnIdxPtr];
		sql += columnDef.columnName;

		// get the next column index and check if it's the end.
		columnIdxPtr++;
		if (*columnIdxPtr != IndexDef::END) {
			sql += ",";
		} else {
			sql += ")";
			break;
		}
	}
	return sql;
}

std::string DBAgentMySQL::makeDropIndexStatement(
  const std::string &name, const std::string &tableName)
{
	return StringUtils::sprintf("DROP INDEX %s ON %s",
	                            name.c_str(), tableName.c_str());
}

void DBAgentMySQL::getIndexInfoVect(vector<IndexInfo> &indexInfoVect,
                                    const TableProfile &tableProfile)
{
	struct ColumnIndexDict {
		map<string, int> dict;

		ColumnIndexDict(const TableProfile &tableProfile)
		{
			for (size_t i = 0; i < tableProfile.numColumns; i++) {
				const ColumnDef &columnDef = 
				  tableProfile.columnDefs[i];
				dict[columnDef.columnName] = i;
			}
		}

		int find(const string &columnName)
		{
			map<string, int>::iterator it = dict.find(columnName);
			HATOHOL_ASSERT(it != dict.end(),
			               "Not found: %s", columnName.c_str());
			return it->second;
		}
	} columnIdxDict(tableProfile);

	typedef map<size_t, const IndexStruct *>  IndexStructSeqMap;
	typedef IndexStructSeqMap::const_iterator IndexStructSeqMapConstIterator;
	typedef map<string, IndexStructSeqMap>    IndexStructMap;
	typedef IndexStructMap::iterator          IndexStructMapIterator;

	vector<IndexStruct> indexStructVect;
	getIndexes(indexStructVect, tableProfile.name);
	
	// Group the same index
	IndexStructMap indexStructMap;
	for (size_t i = 0; i < indexStructVect.size(); i++) {
		const IndexStruct &idxStruct = indexStructVect[i];
		if (idxStruct.keyName == "PRIMARY")
			continue;
		const size_t seq = idxStruct.seqInIndex;
		indexStructMap[idxStruct.keyName][seq] = &idxStruct;
	}

	// Make an ouput vector
	IndexStructMapIterator it = indexStructMap.begin();
	for (; it != indexStructMap.end(); ++it) {
		const IndexStructSeqMap seqMap = it->second;
		IndexStructSeqMapConstIterator seqIt = seqMap.begin();
		const IndexStruct &firstElem = *seqIt->second;
		const bool isUnique = !firstElem.nonUnique;

		int columnIndexes[seqMap.size() + 1];
		columnIndexes[seqMap.size()] = IndexDef::END;
		for (int cnt = 0; seqIt != seqMap.end(); ++seqIt, cnt++) {
			const string &columnName = seqIt->second->columnName;
			columnIndexes[cnt] = columnIdxDict.find(columnName);
		}

		IndexInfo idxInfo;
		idxInfo.name      = firstElem.keyName;
		idxInfo.tableName = firstElem.table;

		const IndexDef indexDef = {
		  idxInfo.name.c_str(), columnIndexes, isUnique
		};
		idxInfo.sql = makeCreateIndexStatement(tableProfile, indexDef);
		indexInfoVect.push_back(idxInfo);
	}
}

