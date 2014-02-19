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

#include <cstdio>
#include "DataQueryOption.h"

struct DataQueryOption::PrivateContext {
	size_t maxNumber;
	SortDirection sortDirection;
	uint64_t startId;

	// constuctor
	PrivateContext(void)
	: maxNumber(NO_LIMIT),
	  sortDirection(SORT_DONT_CARE),
	  startId(0)
	{
	}
};

const size_t DataQueryOption::NO_LIMIT = 0;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
DataQueryOption::DataQueryOption(UserIdType userId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	setUserId(userId);
}

DataQueryOption::DataQueryOption(const DataQueryOption &src)
{
	m_ctx = new PrivateContext();
	*m_ctx = *src.m_ctx;
}

DataQueryOption::~DataQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

bool DataQueryOption::operator==(const DataQueryOption &rhs)
{
	if (m_ctx->maxNumber != rhs.m_ctx->maxNumber)
		return false;
	if (m_ctx->sortDirection != rhs.m_ctx->sortDirection)
		return false;
	if (m_ctx->startId != rhs.m_ctx->startId)
		return false;
	return true;
}

std::string DataQueryOption::getCondition(void) const
{
	return "";
}

void DataQueryOption::setMaximumNumber(size_t maximum)
{
	m_ctx->maxNumber = maximum;
}

size_t DataQueryOption::getMaximumNumber(void) const
{
	return m_ctx->maxNumber;
}

void DataQueryOption::setSortDirection(SortDirection direction)
{
	m_ctx->sortDirection = direction;
}

DataQueryOption::SortDirection DataQueryOption::getSortDirection(void) const
{
	return m_ctx->sortDirection;
}

void DataQueryOption::setStartId(uint64_t id)
{
	m_ctx->startId = id;
}

uint64_t DataQueryOption::getStartId(void) const
{
	return m_ctx->startId;
}
