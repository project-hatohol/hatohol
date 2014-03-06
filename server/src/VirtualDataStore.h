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
	static VirtualDataStore *getInstance(
	  const MonitoringSystemType &monSysType);

	VirtualDataStore(const MonitoringSystemType &monSysType);
	virtual ~VirtualDataStore(void);

	/**
	 * start all MonitoringServer instances.
	 *
	 * @param autoStart
	 * A flag to run an Arm instance soon after the store is started.
	 */
	virtual void start(const bool &autoRun = true);

	/**
	 * start an MonitoringServer instance.
	 *
	 * @param svInfo A MonitoringServerInfo instance to be started.
	 * @param autoStart
	 * A flag to run an Arm instance soon after the store is started.
	 *
	 * @return An HatoholError instance.
	 */
	virtual HatoholError start(const MonitoringServerInfo &svInfo,
	                           const bool &autoRun = true);

	virtual void stop(void);
	virtual HatoholError stop(const ServerIdType &serverId);

protected:
	virtual DataStore *createDataStore(const MonitoringServerInfo &svInfo,
	                                   const bool &autoRun = true) = 0;

private:
	struct PrivateContext;
	PrivateContext *m_ctx;;
};

typedef std::list<VirtualDataStore *>  VirtualDataStoreList;
typedef VirtualDataStoreList::iterator VirtualDataStoreListIterator;

typedef std::map<MonitoringSystemType, VirtualDataStore *> VirtualDataStoreMap;
typedef VirtualDataStoreMap::iterator  VirtualDataStoreMapIterator;

struct VirtualDataStoreForeachProc
{
	virtual bool operator()(VirtualDataStore *virtDataStore) = 0;
};

#endif // VirtualDataStore_h
