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

#include "DataStoreFactory.h"
#include "DataStoreFake.h"
#include "DataStoreZabbix.h"
#include "DataStoreNagios.h"
#include "HapgZabbixAPI.h"

DataStore *DataStoreFactory::create(const MonitoringServerInfo &svInfo,
                                    const bool &autoStart)
{
	switch (svInfo.type) {
	case MONITORING_SYSTEM_FAKE:
		return new DataStoreFake(svInfo, autoStart);
	case MONITORING_SYSTEM_ZABBIX:
		return new DataStoreZabbix(svInfo, autoStart);
	case MONITORING_SYSTEM_NAGIOS:
		return new DataStoreNagios(svInfo, autoStart);
	case MONITORING_SYSTEM_HAPI_ZABBIX:
	{
		HatoholArmPluginGate *gate = new HatoholArmPluginGate(svInfo);
		if (autoStart)
			gate->start();
                return gate;
	}
	default:
		MLPL_BUG("Invalid monitoring system: %d\n", svInfo.type);
	}
	return NULL;
}




