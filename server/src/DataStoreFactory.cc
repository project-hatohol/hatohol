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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "DataStoreFactory.h"
#include "DataStoreFake.h"
#include "HatoholArmPluginGateJSON.h"
#include "HatoholArmPluginGateHAPI2.h"

using namespace std;

shared_ptr<DataStore> DataStoreFactory::create(const MonitoringServerInfo &svInfo,
                                    const bool &autoStart)
{
	switch (svInfo.type) {
	case MONITORING_SYSTEM_FAKE:
		return make_shared<DataStoreFake>(svInfo, autoStart);
	case MONITORING_SYSTEM_HAPI_JSON:
	{
		return make_shared<HatoholArmPluginGateJSON>(svInfo, autoStart);
	}
	case MONITORING_SYSTEM_HAPI2:
	{
		return make_shared<HatoholArmPluginGateHAPI2>(svInfo, autoStart);
	}
	default:
		MLPL_BUG("Invalid monitoring system: %d\n", svInfo.type);
	}
	return nullptr;
}




