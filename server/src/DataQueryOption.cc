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

#include <cstdio>
#include <string>
#include "DataQueryOption.h"
#include "DataQueryContext.h"
#include "Logger.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;

struct DataQueryOption::Impl {
	size_t maxNumber;
	size_t offset;
	SortOrderVect sortOrderVect;
	GroupByVect   groupByVect;
	DataQueryContextPtr dataQueryCtxPtr; // The body is shared
	const DBTermCodec *dbTermCodec;
	bool               tableNameAlways;

	// constuctor
	Impl(const UserIdType &userId)
	: maxNumber(NO_LIMIT),
	  offset(0),
	  dataQueryCtxPtr(new DataQueryContext(userId), false),
	  dbTermCodec(NULL),
	  tableNameAlways(false)
	{
	}

	Impl(DataQueryContext *dataQueryContext)
	: maxNumber(NO_LIMIT),
	  offset(0),
	  dataQueryCtxPtr(dataQueryContext),
	  dbTermCodec(NULL),
	  tableNameAlways(false)
	{
	}

	Impl(const DataQueryOption &masterOption)
	{
		*this = *masterOption.m_impl;
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

DataQueryOption::GroupBy::GroupBy(const std::string &aColumnName)
: columnName(aColumnName)
{
}

DataQueryOption::DataQueryOption(const UserIdType &userId)
: m_impl(new Impl(userId))
{
}

DataQueryOption::DataQueryOption(DataQueryContext *dataQueryContext)
: m_impl(new Impl(dataQueryContext))
{
}

DataQueryOption::DataQueryOption(const DataQueryOption &src)
: m_impl(new Impl(src))
{
}

DataQueryOption::~DataQueryOption()
{
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
	if (m_impl->maxNumber != rhs.m_impl->maxNumber)
		return false;
	if (m_impl->sortOrderVect != rhs.m_impl->sortOrderVect)
		return false;
	return true;
}

std::string DataQueryOption::getCondition(void) const
{
	return "";
}

DataQueryContext &DataQueryOption::getDataQueryContext(void) const
{
	return *m_impl->dataQueryCtxPtr;
}

void DataQueryOption::setDBTermCodec(
  const DBTermCodec *dbTermCodec)
{
	m_impl->dbTermCodec = dbTermCodec;
}

const DBTermCodec *DataQueryOption::getDBTermCodec(void) const
{
	if (m_impl->dbTermCodec)
		return m_impl->dbTermCodec;
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
	m_impl->maxNumber = maximum;
}

size_t DataQueryOption::getMaximumNumber(void) const
{
	return m_impl->maxNumber;
}

void DataQueryOption::setSortOrderVect(const SortOrderVect &sortOrderVect)
{
	m_impl->sortOrderVect = sortOrderVect;
}

void DataQueryOption::setSortOrder(const SortOrder &sortOrder)
{
	m_impl->sortOrderVect.clear();
	m_impl->sortOrderVect.push_back(sortOrder);
}

const DataQueryOption::SortOrderVect &DataQueryOption::getSortOrderVect(void)
  const
{
	return m_impl->sortOrderVect;
}

void DataQueryOption::setGroupByVect(const GroupByVect &groupByVect)
{
	m_impl->groupByVect = groupByVect;
}

void DataQueryOption::setGroupBy(const GroupBy &groupBy)
{
	m_impl->groupByVect.clear();
	m_impl->groupByVect.push_back(groupBy);
}

std::string DataQueryOption::getOrderBy(void) const
{
	SortOrderVectIterator it = m_impl->sortOrderVect.begin();
	std::string orderBy;
	for (; it != m_impl->sortOrderVect.end(); ++it) {
		if (it->columnName.empty()) {
			MLPL_ERR("Empty sort column name\n");
			continue;
		}
		if (it->direction == DataQueryOption::SORT_DONT_CARE)
			continue;
		if (it != m_impl->sortOrderVect.begin())
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

std::string DataQueryOption::getGroupBy(void) const
{
	auto it = begin(m_impl->groupByVect);
	std::string groupBy;
	for (; it != end(m_impl->groupByVect); ++it) {
		if (it->columnName.empty()) {
			MLPL_ERR("Empty group by column name.\n");
			continue;
		}
		if (it != begin(m_impl->groupByVect))
			groupBy += ", ";
		groupBy += it->columnName;
	}
	return groupBy;
}

void DataQueryOption::setTableNameAlways(const bool &enable) const
{
	m_impl->tableNameAlways = enable;
}

bool DataQueryOption::getTableNameAlways(void) const
{
	return m_impl->tableNameAlways;
}

void DataQueryOption::setOffset(size_t offset)
{
	m_impl->offset = offset;
}

size_t DataQueryOption::getOffset(void) const
{
	return m_impl->offset;
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
