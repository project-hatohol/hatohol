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
	OperationPrivilege   privilege;
	ServerHostGrpSetMap *srvHostGrpSetMap;
	ServerIdSet         *serverIdSet;

	PrivateContext(const UserIdType &userId)
	: privilege(userId),
	  srvHostGrpSetMap(NULL),
	  serverIdSet(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		clear();
	}

	void clear(void)
	{
		if (srvHostGrpSetMap) {
			delete srvHostGrpSetMap;
			srvHostGrpSetMap = NULL;
		}

		if (serverIdSet) {
			delete serverIdSet;
			serverIdSet = NULL;
		}
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataQueryContext::DataQueryContext(const UserIdType &userId)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(userId);
}

DataQueryContext::~DataQueryContext()
{
	if (m_ctx)
		delete m_ctx;
}

void DataQueryContext::setUserId(const UserIdType &userId)
{
	m_ctx->privilege.setUserId(userId);
	m_ctx->clear();
}

void DataQueryContext::setFlags(const OperationPrivilegeFlag &flags)
{
	m_ctx->privilege.setFlags(flags);
	m_ctx->clear();
}

const OperationPrivilege &DataQueryContext::getOperationPrivilege(void) const
{
	return m_ctx->privilege;
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

bool DataQueryContext::isValidServer(const ServerIdType &serverId)
{
	const ServerIdSet &svIdSet = getValidServerIdSet();
	return svIdSet.find(serverId) != m_ctx->serverIdSet->end();
}

const ServerIdSet &DataQueryContext::getValidServerIdSet(void)
{
	if (!m_ctx->serverIdSet) {
		m_ctx->serverIdSet = new ServerIdSet();
		CacheServiceDBClient cache;
		DBClientConfig *dbConfig = cache.getConfig();
		dbConfig->getServerIdSet(*m_ctx->serverIdSet, this);
	}
	return *m_ctx->serverIdSet;
}

