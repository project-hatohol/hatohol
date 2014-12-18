/*
 * Copyright (C) 2014 Project Hatohol
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
#include "DataQueryContext.h"
#include "ThreadLocalDBCache.h"

struct DataQueryContext::Impl {
	OperationPrivilege   privilege;
	ServerHostGrpSetMap *srvHostGrpSetMap;
	ServerIdSet         *serverIdSet;

	Impl(const UserIdType &userId)
	: privilege(userId),
	  srvHostGrpSetMap(NULL),
	  serverIdSet(NULL)
	{
	}

	virtual ~Impl()
	{
		clear();
	}

	void clear(void)
	{
		delete srvHostGrpSetMap;
		srvHostGrpSetMap = NULL;

		delete serverIdSet;
		serverIdSet = NULL;
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataQueryContext::DataQueryContext(const UserIdType &userId)
: m_impl(new Impl(userId))
{
}

DataQueryContext::~DataQueryContext()
{
}

void DataQueryContext::setUserId(const UserIdType &userId)
{
	m_impl->privilege.setUserId(userId);
	m_impl->clear();
}

void DataQueryContext::setFlags(const OperationPrivilegeFlag &flags)
{
	m_impl->privilege.setFlags(flags);
	m_impl->clear();
}

const OperationPrivilege &DataQueryContext::getOperationPrivilege(void) const
{
	return m_impl->privilege;
}

const ServerHostGrpSetMap &DataQueryContext::getServerHostGrpSetMap(void)
{
	if (!m_impl->srvHostGrpSetMap) {
		m_impl->srvHostGrpSetMap = new ServerHostGrpSetMap();
		ThreadLocalDBCache cache;
		DBTablesUser &dbUser = cache.getUser();
		dbUser.getServerHostGrpSetMap(*m_impl->srvHostGrpSetMap,
		                              m_impl->privilege.getUserId());
	}
	return *m_impl->srvHostGrpSetMap;
}

bool DataQueryContext::isValidServer(const ServerIdType &serverId)
{
	const ServerIdSet &svIdSet = getValidServerIdSet();
	return svIdSet.find(serverId) != m_impl->serverIdSet->end();
}

const ServerIdSet &DataQueryContext::getValidServerIdSet(void)
{
	if (!m_impl->serverIdSet) {
		m_impl->serverIdSet = new ServerIdSet();
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		dbConfig.getServerIdSet(*m_impl->serverIdSet, this);
	}
	return *m_impl->serverIdSet;
}

