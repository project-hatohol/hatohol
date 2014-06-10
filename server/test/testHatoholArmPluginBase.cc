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

static const size_t TIMEOUT = 5000;

class HatoholArmPluginBaseTest :
  public HatoholArmPluginBase, public HapiTestHelper
{
public:
	HatoholArmPluginBaseTest(const string queueAddr)
	{
		setQueueAddress(queueAddr);
	}

protected:
	void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}
};

static HatoholArmPluginGateTestPtr createHapgTest(
  HapgTestCtx &hapgCtx, MonitoringServerInfo &serverInfo)
{
	hapgCtx.useDefaultReceivedHandler = true;
	hapgCtx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	setupTestDBConfig();
	loadTestDBArmPlugin();
	initServerInfo(serverInfo);
	serverInfo.type = hapgCtx.monitoringSystemType;
	HatoholArmPluginGateTest *hapg =
	  new HatoholArmPluginGateTest(serverInfo, hapgCtx);
	return HatoholArmPluginGateTestPtr(hapg, false);
}

struct TestPair {
	HapgTestCtx hapgCtx;
	MonitoringServerInfo serverInfo;
	HatoholArmPluginGateTestPtr pluginGate;
	HatoholArmPluginBaseTest   *plugin;

	TestPair(void)
	: plugin(NULL)
	{
		pluginGate = createHapgTest(hapgCtx, serverInfo);
		loadTestDBTriggers();
		pluginGate->start();
		cppcut_assert_equal(
		  SimpleSemaphore::STAT_OK,
		  pluginGate->getConnectedSem().timedWait(TIMEOUT));

		plugin = new HatoholArmPluginBaseTest(
		  pluginGate->callGenerateBrokerAddress(serverInfo));
		plugin->start();
		cppcut_assert_equal(
		  SimpleSemaphore::STAT_OK,
		  plugin->getConnectedSem().timedWait(TIMEOUT));
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

} // namespace testHatoholArmPluginBase
