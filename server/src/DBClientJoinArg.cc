/*
 * Copyright (C) 2014 Project Hatohol
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

#include "DBClientJoinArg.h"

struct DBClientJoinArg::PrivateContext {
	DBAgent::SelectExArg selectExArg;

	PrivateContext(const DBAgent::TableProfile &table)
	: selectExArg(table)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClientJoinArg::DBClientJoinArg(const DBAgent::TableProfile &table)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(table);
}

DBClientJoinArg::~DBClientJoinArg()
{
	delete m_ctx;
}

void DBClientJoinArg::addTable(
  const DBAgent::TableProfile &table, const JoinType &type,
  const size_t &index0, const size_t &index1)
{
	MLPL_BUG("Not implemneted yet: %s\n", __PRETTY_FUNCTION__);
}

void DBClientJoinArg::add(const size_t &index)
{
	MLPL_BUG("Not implemneted yet: %s\n", __PRETTY_FUNCTION__);
}

const DBAgent::SelectExArg &DBClientJoinArg::getSelectExArg(void)
{
	return m_ctx->selectExArg;
}
