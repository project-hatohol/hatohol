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
#include "DBClientTest.h"
#include "Helpers.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginBase {

class HatoholArmPluginBaseTest :
  public HatoholArmPluginBase, public HapiTestHelper
{
public:
	HatoholArmPluginBaseTest(void)
	{
	}

	void callSendArmInfo(const ArmInfo &armInfo)
	{
		sendArmInfo(armInfo);
	}

	void callEnableWaitInitiatedAck(const bool &enable = true)
	{
		enableWaitInitiatedAck(enable);
	}

	bool callSleepInitiatedExceptionThrowable(const size_t timeoutInMS)
	{
		return sleepInitiatedExceptionThrowable(timeoutInMS);
	}

	void callAckInitiated(void)
	{
		ackInitiated();
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

void test_getMonitoringServerInfoAsync(void)
{
	MonitoringServerInfo serverInfo;
	struct Arg :
	  public HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg
	{
		SimpleSemaphore sem;

		Arg(MonitoringServerInfo *serverInfo)
		: GetMonitoringServerInfoAsyncArg(serverInfo),
		  sem(0)
		{
		}

		virtual void doneCb(const bool &succeeded) override
		{
			sem.post();
		}
	} arg(&serverInfo);

	HatoholArmPluginTestPairArg
	  testPairArg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestPair pair(testPairArg);
	MonitoringServerInfo actual;
	pair.plugin->getMonitoringServerInfoAsync(&arg);
	pair.plugin->assertWaitSemaphore(arg.sem);
	assertEqual(pair.serverInfo, serverInfo);
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

void test_waitInitiatedException(void)
{
	struct Arg : public HatoholArmPluginTestPairArg {

		bool wakedUp;
		bool gotInitiatedException;

		Arg(void)
		: HatoholArmPluginTestPairArg(
		    MONITORING_SYSTEM_HAPI_TEST_PASSIVE),
		  wakedUp(false),
		  gotInitiatedException(false)
		{
		}

		HatoholArmPluginBaseTest *dcast(HatoholArmPluginBase *_plugin)
		{
			HatoholArmPluginBaseTest *plugin =
			  dynamic_cast<HatoholArmPluginBaseTest *>(_plugin);
			return plugin;
		}

		virtual void
		  onCreatedPlugin(HatoholArmPluginBase *_plugin) override
		{
			HatoholArmPluginBaseTest *plugin = dcast(_plugin);
			plugin->callEnableWaitInitiatedAck(true);
		}
	
		virtual void
		  preAssertWaitInitiated(HatoholArmPluginBase *_plugin) override
		{
			static const size_t TIMEOUT = 5000;
			HatoholArmPluginBaseTest *plugin = dcast(_plugin);
			try {
				wakedUp =
				  plugin->callSleepInitiatedExceptionThrowable(TIMEOUT);
			} catch (const HapInitiatedException &e) {
				gotInitiatedException = true;
				plugin->callAckInitiated();
			}
		}
	} arg;

	TestPair pair(arg);
	cppcut_assert_equal(false, arg.wakedUp);
	cppcut_assert_equal(true,  arg.gotInitiatedException);
}

} // namespace testHatoholArmPluginBase
