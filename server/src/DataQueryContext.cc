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

#include <cstdio>
#include "DataQueryContext.h"
#include "CacheServiceDBClient.h"

struct DataQueryContext::PrivateContext {
	const OperationPrivilege &privilege;
	ServerHostGrpSetMap      *srvHostGrpSetMap;

	PrivateContext(const OperationPrivilege &_privilege)
	: privilege(_privilege),
	  srvHostGrpSetMap(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		if (srvHostGrpSetMap)
			delete srvHostGrpSetMap;
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataQueryContext::DataQueryContext(const OperationPrivilege &privilege)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(privilege);
}

DataQueryContext::~DataQueryContext()
{
	if (m_ctx)
		delete m_ctx;
}

void DataQueryContext::notifyChangeUserId(void)
{
	if (!m_ctx->srvHostGrpSetMap)
		return;
	delete m_ctx->srvHostGrpSetMap;
	m_ctx->srvHostGrpSetMap = NULL;
	MLPL_INFO("Deleted srvHostGrpSetMap.\n");
}

const ServerHostGrpSetMap &DataQueryContext::getServerHostGrpSetMap(void)
{
	if (!m_ctx->srvHostGrpSetMap) {
		m_ctx->srvHostGrpSetMap = new ServerHostGrpSetMap();
		CacheServiceDBClient cache;
		DBClientUser *dbUser = cache.getUser();
		dbUser->getServerHostGrpSetMap(*m_ctx->srvHostGrpSetMap,
		                               m_ctx->privilege.getUserId());
	}
	return *m_ctx->srvHostGrpSetMap;
}
