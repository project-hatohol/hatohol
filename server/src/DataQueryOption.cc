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
#include "DBClientUser.h"
#include "DataQueryOption.h"
#include "CacheServiceDBClient.h"

struct DataQueryOption::PrivateContext {
	UserIdType userId;
	size_t maxNumber;
	SortOrder sortOrder;

	// constuctor
	PrivateContext(void)
	: userId(INVALID_USER_ID),
	  maxNumber(NO_LIMIT),
	  sortOrder(SORT_DONT_CARE)
	{
	}
};

const size_t DataQueryOption::NO_LIMIT = 0;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
DataQueryOption::DataQueryOption(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DataQueryOption::~DataQueryOption()
{
}

string DataQueryOption::getCondition(void) const
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

DataQueryOption::SortOrder DataQueryOption::getSortOrder(void) const
{
	return m_ctx->sortOrder;
}

void DataQueryOption::setUserId(UserIdType userId)
{
	m_ctx->userId = userId;
	if (m_ctx->userId == USER_ID_ADMIN) {
		setFlags(ALL_PRIVILEGES);
		return;
	}

	UserInfo userInfo;
	CacheServiceDBClient cache;
	if (!cache.getUser()->getUserInfo(userInfo, userId)) {
		MLPL_ERR("Failed to getUserInfo(): userId: "
		         "%"FMT_USER_ID"\n", userId);
		m_ctx->userId = INVALID_USER_ID;
		return;
	}
	setFlags(userInfo.flags);
}

UserIdType DataQueryOption::getUserId(void) const
{
	return m_ctx->userId;
}
