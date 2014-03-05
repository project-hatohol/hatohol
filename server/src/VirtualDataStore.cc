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

#include "VirtualDataStore.h"
#include "CacheServiceDBClient.h"
using namespace mlpl;

struct VirtualDataStore::PrivateContext {
	MonitoringSystemType monitoringSystemType;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
VirtualDataStore::VirtualDataStore(const MonitoringSystemType &monSysType)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	m_ctx->monitoringSystemType = monSysType;
}

VirtualDataStore::~VirtualDataStore(void)
{
	if (m_ctx)
		delete m_ctx;
}

void VirtualDataStore::start(const bool &autoRun)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(USER_ID_SYSTEM);
	dbConfig->getTargetServers(monitoringServers, option);

	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &svInfo = *it;
		start(svInfo, autoRun);
	}
}

HatoholError VirtualDataStore::start(const MonitoringServerInfo &svInfo,
                                     const bool &autoRun)
{
	if (svInfo.type != m_ctx->monitoringSystemType)
		return HTERR_INVALID_MONITORING_SYSTEM_TYPE;
	DataStore *dataStore = createDataStore(svInfo, autoRun);
	if (!dataStore)
		return HTERR_FAILED_TO_CREATE_DATA_STORE;
	if (!add(svInfo.id, dataStore)) {
		dataStore->unref();
		return HTERR_FAILED_TO_REGIST_DATA_STORE;
	}
	return HTERR_OK;
}

void VirtualDataStore::stop(void)
{
	MLPL_INFO("VirtualDataStore: stop process: started.\n");
	closeAllStores();
}

HatoholError VirtualDataStore::stop(const ServerIdType &serverId)
{
	uint32_t storeId = serverId;
	if (!hasDataStore(storeId))
		return HTERR_NOT_FOUND_PARAMETER;
	remove(storeId);
	return HTERR_OK;
}

