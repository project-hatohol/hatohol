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

#include <ReadWriteLock.h>
#include "VirtualDataStore.h"
#include "VirtualDataStoreFactory.h"
#include "CacheServiceDBClient.h"
#include "Reaper.h"

using namespace std;
using namespace mlpl;

struct VirtualDataStore::PrivateContext {
	static ReadWriteLock       virtualDataStoreMapLock;
	static VirtualDataStoreMap virtualDataStoreMap;
	MonitoringSystemType monitoringSystemType;

	static VirtualDataStore *getInstance(
	  const MonitoringSystemType &monSysType)
	{
		VirtualDataStore *virtDataStore = NULL;

		// Try to find an instance from the map.
		virtualDataStoreMapLock.readLock();
		VirtualDataStoreMapIterator it =
		  virtualDataStoreMap.find(monSysType);
		if (it != virtualDataStoreMap.end())
			virtDataStore = it->second;
		virtualDataStoreMapLock.unlock();
		if (virtDataStore)
			return virtDataStore;

		// Create a new instance.
		virtualDataStoreMapLock.writeLock();
		Reaper<ReadWriteLock> unlocker(&virtualDataStoreMapLock,
		                               ReadWriteLock::unlock);
		it = virtualDataStoreMap.find(monSysType);
		if (it != virtualDataStoreMap.end())
			return it->second;
		virtDataStore = VirtualDataStoreFactory::create(monSysType);
		HATOHOL_ASSERT(virtDataStore,
		               "Failed to create: type: %d\n", monSysType);
		MLPL_DBG("created VirtualDataStore: %d\n", monSysType);
		virtualDataStoreMap[monSysType] = virtDataStore;
		return virtDataStore;
	}
};

ReadWriteLock       VirtualDataStore::PrivateContext::virtualDataStoreMapLock;
VirtualDataStoreMap VirtualDataStore::PrivateContext::virtualDataStoreMap;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
VirtualDataStore *VirtualDataStore::getInstance(
  const MonitoringSystemType &monSysType)
{
	return PrivateContext::getInstance(monSysType);
}

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
	bool successed = add(svInfo.id, dataStore);
	dataStore->unref(); // incremented in the above add() if successed
	if (!successed)
		return HTERR_FAILED_TO_REGIST_DATA_STORE;
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

