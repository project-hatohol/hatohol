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
#include "HatoholArmPluginGate.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "Hatohol.h"

namespace testHatoholArmPluginGate {

struct StartAndExitArg {
	MonitoringSystemType monitoringSystemType;
	bool                 expectedResultOfStart;

	StartAndExitArg(void)
	: monitoringSystemType(MONITORING_SYSTEM_HAPI_TEST),
	  expectedResultOfStart(false)
	{
	}
};

static void _assertStartAndExit(StartAndExitArg &arg)
{
	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	HatoholArmPluginGatePtr pluginGate(
	  new HatoholArmPluginGate(serverInfo), false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  arg.expectedResultOfStart,
	  pluginGate->start(arg.monitoringSystemType));
	cppcut_assert_equal(
	  arg.expectedResultOfStart, armStatus.getArmInfo().running);

	pluginGate->waitExit();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
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
	StartAndExitArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	arg.expectedResultOfStart = true;
	assertStartAndExit(arg);
}

void test_startWithInvalidPath(void)
{
	StartAndExitArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_NOT_EXIST;
	arg.expectedResultOfStart = false;
	assertStartAndExit(arg);
}

void test_startWithPassivePlugin(void)
{
	StartAndExitArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST_PASSIVE;
	arg.expectedResultOfStart = true;
	assertStartAndExit(arg);
}

} // namespace testHatoholArmPluginGate
