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

#ifndef VirtualDataStore_h
#define VirtualDataStore_h

#include <stdint.h>

#include "VirtualDataStore.h"
#include "DataStoreManager.h"
#include "DBClientHatohol.h"
#include "DBClientConfig.h"

class VirtualDataStore : public DataStoreManager
{
public:
	VirtualDataStore(void);
	virtual ~VirtualDataStore(void);

	virtual void start(void);

	/**
	 * start an MonitoringServer instance.
	 *
	 * @param svInfo A MonitoringServerInfo instance to be started.
	 *
	 * @return
	 * true if an monitoring instance is started. Otherwise (typicall,
	 * server type is not matched), false is returned.
	 */
	virtual bool start(const MonitoringServerInfo &svInfo);

	virtual void stop(void);
	virtual bool stop(const ServerIdType &serverId);

protected:
	template<class T>
	void start(MonitoringSystemType systemType)
	{
		DBClientConfig dbConfig;
		MonitoringServerInfoList monitoringServers;
		ServerQueryOption option(USER_ID_SYSTEM);
		dbConfig.getTargetServers(monitoringServers, option);

		MonitoringServerInfoListIterator it = monitoringServers.begin();
		for (; it != monitoringServers.end(); ++it) {
			MonitoringServerInfo &svInfo = *it;
			start<T>(systemType, svInfo);
		}
	}

	template<class T>
	bool start(MonitoringSystemType systemType,
	           MonitoringServerInfo &svInfo)
	{
		if (svInfo.type != systemType)
			return false;
		DataStore *dataStore = new T(svInfo);
		if (add(svInfo.id, dataStore)) {
			dataStore->unref();
			return true;
		}
		return false;
	}
};

typedef std::list<VirtualDataStore *>  VirtualDataStoreList;
typedef VirtualDataStoreList::iterator VirtualDataStoreListIterator;

#endif // VirtualDataStore_h
