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

#include "Helpers.h"
#include "HatoholArmPluginGateTest.h"

struct HatoholArmPluginTestPairBase {
	// Definition like 'static const variable = val;' in a tempalate
	// header file such as HatoholArmPluginPair causes the following
	// runtime error when it is built with clang on Ubunu 12.04 and 14.04.
	//
	// can't load a shared object for test case: ./.libs/testHatohol.so: ./.libs/testHatohol.so: undefined symbol: _ZN24HatoholArmPluginTestPairIN24testHatoholArmPluginBase24HatoholArmPluginBaseTestEE17DEFAULT_SERVER_IDE
	//
	// This class is a workaround to it.
	static const ServerIdType DEFAULT_SERVER_ID;
};

template <class HapClass>
struct HatoholArmPluginTestPair : public HatoholArmPluginTestPairBase {

	HapgTestCtx hapgCtx;
	MonitoringServerInfo serverInfo;
	HatoholArmPluginGateTestPtr gate;
	HapClass                   *plugin;

	HatoholArmPluginTestPair(
	  const ServerIdType &serverId = DEFAULT_SERVER_ID,
	  const std::string &serverIpAddr = "127.0.0.1",
	  const int &serverPort = 80)
	: plugin(NULL)
	{
		gate = createHapgTest(hapgCtx, serverInfo, serverId,
		                      serverIpAddr, serverPort);
		loadTestDBTriggers();
		gate->start();
		gate->assertWaitConnected();

		plugin = new HapClass();
		plugin->setQueueAddress(
		  gate->callGenerateBrokerAddress(serverInfo));
		plugin->start();

		gate->assertWaitInitiated();
		plugin->assertWaitInitiated();
	}

	virtual ~HatoholArmPluginTestPair()
	{
		if (plugin)
			delete plugin;
	}

	static HatoholArmPluginGateTestPtr createHapgTest(
	  HapgTestCtx &hapgCtx, MonitoringServerInfo &serverInfo,
	  const ServerIdType &serverId = DEFAULT_SERVER_ID,
	  const std::string &serverIpAddr = "",
	  const int &serverPort = 0)
	{
		hapgCtx.useDefaultReceivedHandler = true;
		hapgCtx.monitoringSystemType =
		  MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
		setupTestDBConfig();
		loadTestDBArmPlugin();
		initServerInfo(serverInfo);
		if (serverId != DEFAULT_SERVER_ID)
			serverInfo.id = serverId;
		serverInfo.ipAddress = serverIpAddr;
		serverInfo.port = serverPort;
		serverInfo.type = hapgCtx.monitoringSystemType;
		HatoholArmPluginGateTest *hapg =
		  new HatoholArmPluginGateTest(serverInfo, hapgCtx);
		return HatoholArmPluginGateTestPtr(hapg, false);
	}
};

