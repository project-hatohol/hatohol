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
#include <SimpleSemaphore.h>
#include "Hatohol.h"
#include "HatoholArmPluginBase.h"
#include "HatoholArmPluginGateTest.h"
#include "DBClientTest.h"
#include "Helpers.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginBase {

const ServerIdType DEFAULT_SERVER_ID = -1;

class HatoholArmPluginBaseTest :
  public HatoholArmPluginBase, public HapiTestHelper
{
public:
	HatoholArmPluginBaseTest(void)
	{
	}

protected:
	void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}

	void onInitiated(void) override
	{
		HapiTestHelper::onInitiated();
	}
};

static HatoholArmPluginGateTestPtr createHapgTest(
  HapgTestCtx &hapgCtx, MonitoringServerInfo &serverInfo,
  const ServerIdType &serverId = DEFAULT_SERVER_ID)
{
	hapgCtx.useDefaultReceivedHandler = true;
	hapgCtx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	setupTestDBConfig();
	loadTestDBArmPlugin();
	initServerInfo(serverInfo);
	if (serverId != DEFAULT_SERVER_ID)
		serverInfo.id = serverId;
	serverInfo.type = hapgCtx.monitoringSystemType;
	HatoholArmPluginGateTest *hapg =
	  new HatoholArmPluginGateTest(serverInfo, hapgCtx);
	return HatoholArmPluginGateTestPtr(hapg, false);
}

struct TestPair {
	HapgTestCtx hapgCtx;
	MonitoringServerInfo serverInfo;
	HatoholArmPluginGateTestPtr gate;
	HatoholArmPluginBaseTest   *plugin;

	TestPair(const ServerIdType &serverId = DEFAULT_SERVER_ID)
	: plugin(NULL)
	{
		gate = createHapgTest(hapgCtx, serverInfo, serverId);
		loadTestDBTriggers();
		gate->start();
		gate->assertWaitConnected();

		plugin = new HatoholArmPluginBaseTest();
		plugin->setQueueAddress(
		  gate->callGenerateBrokerAddress(serverInfo));
		plugin->start();

		gate->assertWaitInitiated();
		plugin->assertWaitInitiated();
	}

	virtual ~TestPair()
	{
		if (plugin)
			delete plugin;
	}
};

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getMonitoringServerInfo(void)
{
	TestPair pair;
	MonitoringServerInfo actual;
	cppcut_assert_equal(true, pair.plugin->getMonitoringServerInfo(actual));
	assertEqual(pair.serverInfo, actual);
}

void test_getTimestampOfLastTrigger(void)
{
	TestPair pair;
	SmartTime expect = getTimestampOfLastTestTrigger(pair.serverInfo.id);
	SmartTime actual = pair.plugin->getTimestampOfLastTrigger();
	cppcut_assert_equal(expect, actual);
}

void test_getLastEventId(void)
{
	loadTestDBEvents();
	const ServerIdType serverId = 1;
	TestPair pair(serverId);
	const EventIdType expect = findLastEventId(serverId);
	const EventIdType actual = pair.plugin->getLastEventId();
	cppcut_assert_equal(expect, actual);
}

} // namespace testHatoholArmPluginBase
