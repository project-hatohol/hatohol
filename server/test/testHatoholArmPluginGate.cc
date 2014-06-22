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

static void _assertStartAndExit(HapgTestCtx &ctx)
{
	static const size_t TIMEOUT = 5000;
	if (ctx.checkMessage)
		ctx.expectRcvMessage = testMessage;

	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = ctx.monitoringSystemType;
	HatoholArmPluginGateTest *hapg =
	  new HatoholArmPluginGateTest(serverInfo, ctx);
	HatoholArmPluginGatePtr pluginGate(hapg, false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	pluginGate->start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, ctx.launchedSem.timedWait(TIMEOUT));
	cppcut_assert_equal(ctx.expectedResultOfStart, ctx.launchSucceeded);

	SimpleSemaphore::Status status = SimpleSemaphore::STAT_OK;
	if (ctx.waitMainSem)
		status = ctx.mainSem.timedWait(TIMEOUT);

	pluginGate->exitSync();
	// These assertions must be after pluginGate->exitSync()
	// to be sure to exit the thread.
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, status);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(false, ctx.abnormalChildTerm);
	if (ctx.checkMessage)
		cppcut_assert_equal(string(testMessage), ctx.rcvMessage);
	cppcut_assert_equal(false, ctx.gotUnexceptedException);
	if (ctx.checkNumRetry)
		cppcut_assert_equal(ctx.numRetry, ctx.retryCount-1);
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
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
}

void test_startWithInvalidPath(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
	ctx.expectedResultOfStart = false;
	assertStartAndExit(ctx);
}

void test_startWithPassivePlugin(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	ctx.expectedResultOfStart = true;
	assertStartAndExit(ctx);
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
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	ctx.numRetry = 3;
	ctx.checkNumRetry = true;
	assertStartAndExit(ctx);
}

void test_abortRetryWait(void)
{
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.checkMessage = false;
	ctx.numRetry = 1;
	ctx.retrySleepTime = 3600 * 1000; // any value OK if it's long enough
	ctx.cancelRetrySleep = true;
	assertStartAndExit(ctx);
}

} // namespace testHatoholArmPluginGate
