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

#include <fstream>
#include <errno.h>
#include <cppcutter.h>
#include <string>
#include <qpid/messaging/Message.h>
#include <Mutex.h>
#include <SimpleSemaphore.h>
#include "HatoholArmPluginGate.h"
#include "DBTablesTest.h"
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

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = ctx.monitoringSystemType;
	serverInfo.id = getTestArmPluginInfo(ctx.monitoringSystemType).serverId;
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
	ctx.onReleasedMainSem();

	pluginGate->exitSync();
	// These assertions must be after pluginGate->exitSync()
	// to be sure to exit the thread.
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, status);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	if (!ctx.expectAbnormalChildTerm)
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
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBServer();
	loadTestDBArmPlugin();
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

void test_getPid(void)
{
	// TOOD: Share the code with test_startAndWaitExit().
	HapgTestCtx ctx;
	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
	cppcut_assert_not_equal(0, ctx.pluginPid);
}

void test_launchPluginProcessWithMLPLLoggerLevel(void)
{
	// TOOD: Share the code with test_startAndWaitExit().
	struct Ctx : public HapgTestCtx {
		bool   hasError;
		string currLoggerEnv;
		string targetEnvLine;
		string paramFilePath;
		const char *TEST_ENV_WORD;

		Ctx(void)
		: hasError(false),
		  TEST_ENV_WORD("TEST_LEVEL")
		{
			const char *env = getenv(Logger::LEVEL_ENV_VAR_NAME);
			if (env)
				currLoggerEnv = env;
			setEnv(Logger::LEVEL_ENV_VAR_NAME, TEST_ENV_WORD);
			targetEnvLine = StringUtils::sprintf("%s=%s",
			  Logger::LEVEL_ENV_VAR_NAME, TEST_ENV_WORD);
			createParamFile();
		}

		virtual ~Ctx()
		{
			// Restore the environment variable
			setEnv(Logger::LEVEL_ENV_VAR_NAME, currLoggerEnv);

			errno = 0;
			remove(paramFilePath.c_str());
			cut_assert_errno();
		}

		void createParamFile(void)
		{
			paramFilePath = StringUtils::sprintf(
			                  HAPI_TEST_PLUGIN_PARAM_FILE_FMT,
			                  getpid());
			ofstream ofs(paramFilePath.c_str(), ios::trunc);
			if (!ofs) {
				cut_notify("Failed to open: %s\n",
				           paramFilePath.c_str());
				hasError = true;
				return;
			}
			ofs << "dontExit\t1" << endl;
		}

		void setEnv(const string &name, const string &value)
		{
			if (setenv(name.c_str(), value.c_str(), 1) != 0) {
				cut_notify("Failed to call setenv(%s): %d\n",
				           name.c_str(), errno);
				hasError = true;
			}
		}

		virtual void onReleasedMainSem(void) override
		{
			readoutTargetEnvVar();
			if (pluginPid)
				kill(pluginPid, SIGKILL);
		}

		void readoutTargetEnvVar(void)
		{
			string path = StringUtils::sprintf("/proc/%d/environ",
			                                   pluginPid);
			ifstream ifs(path.c_str());
			if (!ifs) {
				cut_notify("Failed to open: %s\n",
				           path.c_str());
				hasError = true;
				return;
			}

			string line;
			while (getline(ifs, line, '\0')) {
				if (line == targetEnvLine)
					return;
			}
			cut_notify("Not found the target env line: %s in %s\n",
			           targetEnvLine.c_str(), path.c_str());
			hasError = true;
		}
	} ctx;

	ctx.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	ctx.expectedResultOfStart = true;
	ctx.waitMainSem = true;
	ctx.expectAbnormalChildTerm = true;
	ctx.checkMessage = true;
	assertStartAndExit(ctx);
	cppcut_assert_equal(false, ctx.hasError);
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

// TODO: implement
/*
void test_terminateCommand(void)
{
}
*/

} // namespace testHatoholArmPluginGate
