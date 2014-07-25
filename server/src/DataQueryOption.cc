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

#include <cstdio>
#include <string>
#include "DataQueryOption.h"
#include "DataQueryContext.h"
#include "Logger.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

struct DataQueryOption::PrivateContext {
	size_t maxNumber;
	size_t offset;
	SortOrderList sortOrderList;
	DataQueryContextPtr dataQueryCtxPtr; // The body is shared
	const DBTermCodec *dbTermCodec;
	bool               tableNameAlways;

	// constuctor
	PrivateContext(const UserIdType &userId)
	: maxNumber(NO_LIMIT),
	  offset(0),
	  dataQueryCtxPtr(new DataQueryContext(userId), false),
	  dbTermCodec(NULL),
	  tableNameAlways(false)
	{
	}

	PrivateContext(DataQueryContext *dataQueryContext)
	: maxNumber(NO_LIMIT),
	  offset(0),
	  dataQueryCtxPtr(dataQueryContext),
	  dbTermCodec(NULL),
	  tableNameAlways(false)
	{
	}

	PrivateContext(const DataQueryOption &masterOption)
	{
		*this = *masterOption.m_ctx;
	}


};

const size_t DataQueryOption::NO_LIMIT = 0;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
DataQueryOption::SortOrder::SortOrder(
  const std::string &aColumnName, const SortDirection &aDirection)
: columnName(aColumnName), direction(aDirection)
{
}

DataQueryOption::DataQueryOption(const UserIdType &userId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(userId);
}

DataQueryOption::DataQueryOption(DataQueryContext *dataQueryContext)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(dataQueryContext);
}

DataQueryOption::DataQueryOption(const DataQueryOption &src)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(src);
}

DataQueryOption::~DataQueryOption()
{
	delete m_ctx;
}

static inline bool operator ==(
  const DataQueryOption::SortOrder &lhs, const DataQueryOption::SortOrder &rhs)
{
	if (lhs.columnName != rhs.columnName)
		return false;
	if (lhs.direction != rhs.direction)
		return false;
	return true;
}

bool DataQueryOption::operator==(const DataQueryOption &rhs)
{
	if (m_ctx->maxNumber != rhs.m_ctx->maxNumber)
		return false;
	if (m_ctx->sortOrderList != rhs.m_ctx->sortOrderList)
		return false;
	return true;
}

std::string DataQueryOption::getCondition(void) const
{
	return "";
}

DataQueryContext &DataQueryOption::getDataQueryContext(void) const
{
	return *m_ctx->dataQueryCtxPtr;
}

void DataQueryOption::setDBTermCodec(
  const DBTermCodec *dbTermCodec)
{
	m_ctx->dbTermCodec = dbTermCodec;
}

const DBTermCodec *DataQueryOption::getDBTermCodec(void) const
{
	if (m_ctx->dbTermCodec)
		return m_ctx->dbTermCodec;
	static DBTermCodec dbTermCodec;
	return &dbTermCodec;
}

bool DataQueryOption::isValidServer(const ServerIdType &serverId) const
{
	return getDataQueryContext().isValidServer(serverId);
}

void DataQueryOption::setUserId(const UserIdType &userId)
{
	getDataQueryContext().setUserId(userId);
}

void DataQueryOption::setFlags(const OperationPrivilegeFlag &flags)
{
	getDataQueryContext().setFlags(flags);
}

UserIdType DataQueryOption::getUserId(void) const
{
	return getDataQueryContext().getOperationPrivilege().getUserId();
}

bool DataQueryOption::has(const OperationPrivilegeType &type) const
{
	return getDataQueryContext().getOperationPrivilege().has(type);
}

DataQueryOption::operator const OperationPrivilege &() const
{
	return getDataQueryContext().getOperationPrivilege();
}

void DataQueryOption::setMaximumNumber(size_t maximum)
{
	m_ctx->maxNumber = maximum;
}

size_t DataQueryOption::getMaximumNumber(void) const
{
	return m_ctx->maxNumber;
}

void DataQueryOption::setSortOrderList(const SortOrderList &sortOrderList)
{
	m_ctx->sortOrderList = sortOrderList;
}

void DataQueryOption::setSortOrder(const SortOrder &sortOrder)
{
	m_ctx->sortOrderList.clear();
	m_ctx->sortOrderList.push_back(sortOrder);
}

const DataQueryOption::SortOrderList &DataQueryOption::getSortOrderList(void)
  const
{
	return m_ctx->sortOrderList;
}

std::string DataQueryOption::getOrderBy(void) const
{
	SortOrderListIterator it = m_ctx->sortOrderList.begin();
	std::string orderBy;
	for (; it != m_ctx->sortOrderList.end(); ++it) {
		if (it->columnName.empty()) {
			MLPL_ERR("Empty sort column name\n");
			continue;
		}
		if (it->direction == DataQueryOption::SORT_DONT_CARE)
			continue;
		if (it != m_ctx->sortOrderList.begin())
			orderBy += ", ";
		if (it->direction == DataQueryOption::SORT_ASCENDING) {
			orderBy += it->columnName + " ASC";
		} else if (it->direction == DataQueryOption::SORT_DESCENDING) {
			orderBy += it->columnName + " DESC";
		} else {
			MLPL_ERR("Unknown sort direction: %d\n",
				 it->direction);
		}
	}
	return orderBy;
}

void DataQueryOption::setTableNameAlways(const bool &enable) const
{
	m_ctx->tableNameAlways = enable;
}

bool DataQueryOption::getTableNameAlways(void) const
{
	return m_ctx->tableNameAlways;
}

void DataQueryOption::setOffset(size_t offset)
{
	m_ctx->offset = offset;
}

size_t DataQueryOption::getOffset(void) const
{
	return m_ctx->offset;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DataQueryOption::addCondition(
  string &currCondition, const string &addedCondition,
  const AddConditionType &type, const bool &useParenthesis)
{
	if (addedCondition.empty())
		return;

	if (currCondition.empty()) {
		currCondition = addedCondition;
		return;
	}

	switch (type) {
	case ADD_TYPE_AND:
		currCondition += " AND ";
		break;
	case ADD_TYPE_OR:
		currCondition += " OR ";
		break;
	default:
		HATOHOL_ASSERT(false, "Unknown condition: %d\n", type);
	}

	if (useParenthesis)
		currCondition += "(";
	currCondition += addedCondition;
	if (useParenthesis)
		currCondition += ")";
}
