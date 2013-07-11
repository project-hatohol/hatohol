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

#include <MutexLock.h>
using namespace mlpl;

#include "DBAgent.h"

struct DBSetupInfo {
	DBSetupFunc func;
	void       *data;
};

typedef multimap<DBDomainId, DBSetupInfo> DBSetupInfoMap;
typedef DBSetupInfoMap::iterator          DBSetupInfoMapIterator;

DBAgentSelectExArg::DBAgentSelectExArg(void)
: limit(0),
  offset(0)
{
}

void DBAgentSelectExArg::pushColumn
  (const ColumnDef &columnDef, const string &varName)
{
	string statement;
	if (!varName.empty()) {
		statement = varName;
		statement += ".";
	}
	statement += columnDef.columnName;
	statements.push_back(statement);
	columnTypes.push_back(columnDef.type);
}

DBConnectInfo::DBConnectInfo(void)
: host("localhost"),
  port(0)
{
}

DBConnectInfo::~DBConnectInfo()
{
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

struct DBAgent::PrivateContext
{
	static MutexLock          mutex;
	static DBSetupInfoMap     setupInfoMap;

	// methods
	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};

MutexLock      DBAgent::PrivateContext::mutex;
DBSetupInfoMap DBAgent::PrivateContext::setupInfoMap;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBAgent::addSetupFunction(DBDomainId domainId,
                               DBSetupFunc setupFunc, void *data)
{
	DBSetupInfo setupInfo;
	setupInfo.func = setupFunc;
	setupInfo.data = data;
	PrivateContext::lock();
	PrivateContext::setupInfoMap.insert
	  (pair<DBDomainId, DBSetupInfo>(domainId, setupInfo));
	PrivateContext::unlock();
}

DBAgent::DBAgent(DBDomainId domainId, bool skipSetup)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	if (skipSetup)
		return;

	PrivateContext::lock();
	pair<DBSetupInfoMapIterator, DBSetupInfoMapIterator> matchedRange = 
	  PrivateContext::setupInfoMap.equal_range(domainId);
	DBSetupInfoMapIterator it = matchedRange.first;
	for (; it != matchedRange.second; ++it) {
		DBSetupInfo &setupInfo = it->second;
		DBSetupFunc &setupFunc = setupInfo.func;
		void        *data      = setupInfo.data;
		try {
			(*setupFunc)(domainId, data);
		} catch (...) {
			// Note: contetns in DBSetupInfoMap remains.
			PrivateContext::unlock();
			throw;
		}
	}
	PrivateContext::setupInfoMap.erase
	  (matchedRange.first, matchedRange.second);
	PrivateContext::unlock();
}

DBAgent::~DBAgent()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
string DBAgent::makeSelectStatement(DBAgentSelectArg &selectArg)
{
	size_t numColumns = selectArg.columnIndexes.size();
	string sql = "SELECT ";
	for (size_t i = 0; i < numColumns; i++) {
		size_t idx = selectArg.columnIndexes[i];
		const ColumnDef &columnDef = selectArg.columnDefs[idx];
		sql += columnDef.columnName;
		sql += " ";
		if (i < selectArg.columnIndexes.size()- 1)
			sql += ",";
	}
	sql += "FROM ";
	sql += selectArg.tableName;
	return sql;
}

string DBAgent::makeSelectStatement(DBAgentSelectExArg &selectExArg)
{
	size_t numColumns = selectExArg.statements.size();
	HATOHOL_ASSERT(numColumns > 0, "Vector size must not be zero");
	HATOHOL_ASSERT(numColumns == selectExArg.columnTypes.size(),
	             "Vector size mismatch: statements (%zd):columnTypes (%zd)",
	             numColumns, selectExArg.columnTypes.size());

	string sql = "SELECT ";
	for (size_t i = 0; i < numColumns; i++) {
		sql += selectExArg.statements[i];
		if (i < numColumns-1)
			sql += ",";
	}
	sql += " FROM ";
	sql += selectExArg.tableName;
	if (!selectExArg.condition.empty()) {
		sql += " WHERE ";
		sql += selectExArg.condition;
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
	string valueStr;
	switch (columnDef->type) {
	case SQL_COLUMN_TYPE_INT:
	{
		DEFINE_AND_ASSERT(itemData, ItemInt, item);
		valueStr = StringUtils::sprintf("%d", item->get());
		break;
	}
	case SQL_COLUMN_TYPE_BIGUINT:
	{
		DEFINE_AND_ASSERT(itemData, ItemUint64, item);
		valueStr = StringUtils::sprintf("%"PRId64, item->get());
		break;
	}
	case SQL_COLUMN_TYPE_VARCHAR:
	case SQL_COLUMN_TYPE_CHAR:
	case SQL_COLUMN_TYPE_TEXT:
	{
		DEFINE_AND_ASSERT(itemData, ItemString, item);
		if (item->isNull()) {
			valueStr = "NULL";
		} else {
			string escaped =
			   StringUtils::replace(item->get(), "'", "''");
			valueStr =
			   StringUtils::sprintf("'%s'", escaped.c_str());
		}
		break;
	}
	case SQL_COLUMN_TYPE_DOUBLE:
	{
		string fmt;
		DEFINE_AND_ASSERT(itemData, ItemDouble, item);
		fmt = StringUtils::sprintf("%%.%zdlf", columnDef->decFracLength);
		valueStr = StringUtils::sprintf(fmt.c_str(), item->get());
		break;
	}
	default:
		HATOHOL_ASSERT(true, "Unknown column type: %d (%s)",
		             columnDef->type, columnDef->columnName);
	}
	return valueStr;
}

string DBAgent::makeUpdateStatement(DBAgentUpdateArg &updateArg)
{
	size_t numColumns = updateArg.row->getNumberOfItems();
	HATOHOL_ASSERT(numColumns == updateArg.columnIndexes.size(),
	             "Invalid number of colums: %zd, %zd",
	             numColumns, updateArg.columnIndexes.size());

	// make a SQL statement
	string statement = "UPDATE ";
	statement += updateArg.tableName;
	statement += " SET ";
	for (size_t i = 0; i < numColumns; i++) {
		size_t columnIdx = updateArg.columnIndexes[i];
		const ColumnDef &columnDef = updateArg.columnDefs[columnIdx];
		const ItemData *itemData = updateArg.row->getItemAt(i);
		string valueStr = getColumnValueString(&columnDef, itemData);

		statement += columnDef.columnName;
		statement += "=";
		statement += valueStr;
		if (i < numColumns-1)
			statement += ",";
	}

	// condition
	if (!updateArg.condition.empty()) {
		statement += " WHERE ";
		statement += updateArg.condition;
	}
	return statement;
}
