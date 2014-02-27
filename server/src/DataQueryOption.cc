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
#include "DataQueryOption.h"
#include "DataQueryContext.h"
#include "Logger.h"

using namespace mlpl;

struct DataQueryOption::PrivateContext {
	size_t maxNumber;
	size_t offset;
	SortOrderList sortOrderList;
	DataQueryContextPtr dataQueryCtxPtr; // The body is shared

	// constuctor
	PrivateContext(const DataQueryOption &masterOption)
	: maxNumber(NO_LIMIT),
	  offset(0)
	{
		if (!masterOption.m_ctx) {
			dataQueryCtxPtr = new DataQueryContext(masterOption);
			dataQueryCtxPtr->unref();
		} else {
			// copy the member
			*this = *masterOption.m_ctx;
		}
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

DataQueryOption::DataQueryOption(UserIdType userId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(*this);
	setUserId(userId);
}

DataQueryOption::DataQueryOption(const DataQueryOption &src)
{
	m_ctx = new PrivateContext(src);
}

DataQueryOption::~DataQueryOption()
{
	if (m_ctx)
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
	for (; it != m_ctx->sortOrderList.end(); it++) {
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

void DataQueryOption::setOffset(size_t offset)
{
	m_ctx->offset = offset;
}

size_t DataQueryOption::getOffset(void) const
{
	return m_ctx->offset;
}
