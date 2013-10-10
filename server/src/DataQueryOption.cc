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

struct DataQueryOption::PrivateContext {
	UserIdType userId;

	// constuctor
	PrivateContext(void)
	: userId(INVALID_USER_ID)
	{
	}
};

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

void DataQueryOption::setUserId(UserIdType userId)
{
	m_ctx->userId = userId;
}

UserIdType DataQueryOption::getUserId(void) const
{
	return m_ctx->userId;
}

// ---------------------------------------------------------------------------
// SimpleQueryOption
// ---------------------------------------------------------------------------
const string SimpleQueryOption::getServerIdColumnName(void)
{
	MLPL_WARN("This function is just for passing the build and "
	          "should not be called.\n");
	return "InvalidServerIdColumn";
}

const string SimpleQueryOption::getHostGroupIdColumnName(void)
{
	MLPL_WARN("This function is just for passing the build and "
	          "should not be called.\n");
	return "InvaliHostGroupIdColumn";
}
