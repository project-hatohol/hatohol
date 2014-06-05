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
#include <string>
#include <qpid/messaging/Message.h>
#include <MutexLock.h>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginGate.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "hapi-test-plugin.h"
#include "HatoholArmPluginGateTest.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginGate {

static void _assertStartAndExit(HapgTestArg &arg)
{
	static const size_t TIMEOUT = 5000;
	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = arg.monitoringSystemType;
	HatoholArmPluginGateTest *hapg =
	  new HatoholArmPluginGateTest(serverInfo, arg);
	HatoholArmPluginGatePtr pluginGate(hapg, false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	pluginGate->start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, arg.launchedSem.timedWait(TIMEOUT));
	cppcut_assert_equal(arg.expectedResultOfStart, arg.launchSucceeded);

	SimpleSemaphore::Status status = SimpleSemaphore::STAT_OK;
	if (arg.waitMainSem)
		status = arg.mainSem.timedWait(TIMEOUT);

	pluginGate->exitSync();
	// These assertions must be after pluginGate->exitSync()
	// to be sure to exit the thread.
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, status);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(false, arg.abnormalChildTerm);
	if (arg.checkMessage)
		cppcut_assert_equal(string(testMessage), arg.rcvMessage);
	cppcut_assert_equal(false, arg.gotUnexceptedException);
	if (arg.checkNumRetry)
		cppcut_assert_equal(arg.numRetry, arg.retryCount-1);
}
#define assertStartAndExit(A) cut_trace(_assertStartAndExit(A))

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	HatoholArmPluginGatePtr pluginGate(
	  new HatoholArmPluginGate(serverInfo), false);
}

void test_startAndWaitExit(void)
{
	HapgTestArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	arg.expectedResultOfStart = true;
	arg.waitMainSem = true;
	arg.checkMessage = true;
	assertStartAndExit(arg);
}

void test_startWithInvalidPath(void)
{
	HapgTestArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
	arg.expectedResultOfStart = false;
	assertStartAndExit(arg);
}

void test_startWithPassivePlugin(void)
{
	HapgTestArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	arg.expectedResultOfStart = true;
	assertStartAndExit(arg);
}

void test_generateBrokerAddress(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.id = 530;
	cppcut_assert_equal(
	  string("hatohol-arm-plugin.530"),
	  HatoholArmPluginGateTest::callGenerateBrokerAddress(serverInfo));
}

void test_retryToConnect(void)
{
	HapgTestArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	arg.expectedResultOfStart = true;
	arg.waitMainSem = true;
	arg.checkMessage = true;
	arg.numRetry = 3;
	arg.checkNumRetry = true;
	assertStartAndExit(arg);
}

void test_abortRetryWait(void)
{
	HapgTestArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	arg.expectedResultOfStart = true;
	arg.checkMessage = false;
	arg.numRetry = 1;
	arg.retrySleepTime = 3600 * 1000; // any value OK if it's long enough
	arg.cancelRetrySleep = true;
	assertStartAndExit(arg);
}

} // namespace testHatoholArmPluginGate
