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
#include "HatoholArmPluginTestPair.h"
#include "DBTablesTest.h"
#include "Helpers.h"

using namespace std;
using namespace hfl;
using namespace qpid::messaging;

namespace testHatoholArmPluginBase {

class HatoholArmPluginBaseTest :
  public HatoholArmPluginBase, public HapiTestHelper
{
public:
	HatoholArmPluginBaseTest(void *params)
	{
	}

	void callSendArmInfo(const ArmInfo &armInfo)
	{
		sendArmInfo(armInfo);
	}

protected:
	void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}

	void onInitiated(void) override
	{
		HatoholArmPluginBase::onInitiated();
		HapiTestHelper::onInitiated();
	}
};

typedef HatoholArmPluginTestPair<HatoholArmPluginBaseTest> TestPair;

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getMonitoringServerInfo(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	MonitoringServerInfo actual;
	cppcut_assert_equal(true, pair.plugin->getMonitoringServerInfo(actual));
	assertEqual(pair.serverInfo, actual);
}

void test_getTimestampOfLastTrigger(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	SmartTime expect = getTimestampOfLastTestTrigger(arg.serverId);
	SmartTime actual = pair.plugin->getTimestampOfLastTrigger();
	cppcut_assert_equal(expect, actual);
}

void test_getLastEventId(void)
{
	loadTestDBEvents();
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	const EventIdType expect = findLastEventId(arg.serverId);
	const EventIdType actual = pair.plugin->getLastEventId();
	cppcut_assert_equal(expect, actual);
}

void test_getTimeOfLastEvent(void)
{
	loadTestDBEvents();
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	const SmartTime expect(findTimeOfLastEvent(arg.serverId));
	const SmartTime actual = pair.plugin->getTimeOfLastEvent();
	cppcut_assert_equal(expect, actual);
}

void test_sendArmInfo(void)
{
	ArmInfo armInfo;
	setTestValue(armInfo);
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(arg);
	pair.plugin->callSendArmInfo(armInfo);
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_ARM_INFO);
	assertEqual(armInfo, pair.gate->getArmStatus().getArmInfo());
}

} // namespace testHatoholArmPluginBase
