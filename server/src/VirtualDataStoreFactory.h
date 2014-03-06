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

#ifndef VirtualDataStoreFactory_h
#define VirtualDataStoreFactory_h

#include "DBClientConfig.h"
#include "VirtualDataStore.h"

class VirtualDataStoreFactory
{
public:
	/**
	 * Create a VirtualDataStore instance.
	 *
	 * @param monSysType A type of the created monitoring system.
	 * @return
	 * A new VirtualDataStore instance is returned if successfully. Or
	 * NULL is returned.
	 */
	static VirtualDataStore *create(const MonitoringSystemType &monSysType);

private:
	struct PrivateContext;
};

#endif // VirtualDataStoreFactory_h
