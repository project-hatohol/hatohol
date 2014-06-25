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

#include <cppcutter.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "HatoholArmPluginGateTest.h"
#include "ChildProcessManager.h"

namespace test_hatohol_arm_plugin_zabbix {

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	ChildProcessManager::getInstance()->reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_launch(void)
{
	HapgTestCtx ctx;
	ctx.useDefaultReceivedHandler = true;
	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = MONITORING_SYSTEM_HAPI_ZABBIX;
	serverInfo.id = getTestArmPluginInfo(serverInfo.type).serverId;
	HatoholArmPluginGateTestPtr pluginGate(
	  new HatoholArmPluginGateTest(serverInfo, ctx), false);
	pluginGate->start();
	pluginGate->assertWaitConnected();
	pluginGate->assertWaitInitiated();
}

} // namespace test_hatohol_arm_plugin_zabbix
