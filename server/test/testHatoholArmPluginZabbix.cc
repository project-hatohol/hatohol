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

#include <cppcutter.h>
#include "Hatohol.h"
#include "Helpers.h"
#include "HatoholArmPluginGateTest.h"
#include "ChildProcessManager.h"

namespace testHatoholArmPluginZabbix {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
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
	loadTestDBArmPlugin();

	HapgTestCtx ctx;
	ctx.useDefaultReceivedHandler = true;
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

} // namespace testHatoholArmPluginZabbix
