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
	void stop(void);

	virtual void getTriggerList(TriggerInfoList &triggerList) = 0;

protected:
	template<class T>
	void start(MonitoringSystemType systemType)
	{
		DBClientConfig dbConfig;
		MonitoringServerInfoList monitoringServers;
		dbConfig.getTargetServers(monitoringServers);

		MonitoringServerInfoListIterator it = monitoringServers.begin();
		for (; it != monitoringServers.end(); ++it) {
			MonitoringServerInfo &svInfo = *it;
			if (svInfo.type != systemType)
				continue;
			DataStore *dataStore = new T(svInfo);
			add(svInfo.id, dataStore);
		}
	}
};

#endif // VirtualDataStore_h
