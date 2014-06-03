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

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginGate {

static int TIME_OUT = 5000;

struct StartAndExitArg {
	MonitoringSystemType monitoringSystemType;
	bool                 expectedResultOfStart;
	bool                 waitMainSem;
	bool                 checkMessage;
	size_t               numRetry;
	size_t               retrySleepTime; // msec.
	bool                 cancelRetrySleep;
	bool                 checkNumRetry;

	StartAndExitArg(void)
	: monitoringSystemType(MONITORING_SYSTEM_HAPI_TEST),
	  expectedResultOfStart(false),
	  waitMainSem(false),
	  checkMessage(false),
	  numRetry(0),
	  retrySleepTime(1),
	  cancelRetrySleep(false),
	  checkNumRetry(false)
	{
	}
};

class TestHatoholArmPluginGate : public HatoholArmPluginGate {
public:
	static const size_t TIMEOUT_MSEC = 5000;
	const StartAndExitArg &arg;
	bool       abnormalChildTerm;
	string     rcvMessage;
	bool       gotUnexceptedException;
	size_t     retryCount;
	SimpleSemaphore launchedSem;
	bool            launchSucceeded;
	SimpleSemaphore mainSem;

	TestHatoholArmPluginGate(const MonitoringServerInfo &serverInfo,
	                         const StartAndExitArg &_arg)
	: HatoholArmPluginGate(serverInfo),
	  arg(_arg),
	  abnormalChildTerm(false),
	  gotUnexceptedException(false),
	  retryCount(0),
	  launchedSem(0),
	  launchSucceeded(false),
	  mainSem(0)
	{
	}
	
	static string callGenerateBrokerAddress(
	  const MonitoringServerInfo &serverInfo)
	{
		return generateBrokerAddress(serverInfo);
	}

	// We assume these virtual funcitons are called from
	// the plugin's thread.
	// I.e. we must not call cutter's assertions in them.
	virtual void onSessionChanged(Session *session) override
	{
		if (arg.numRetry && session) {
			if (retryCount < arg.numRetry)
				session->close();
			retryCount++;
		}
	}

	virtual void onReceived(SmartBuffer &smbuf) override
	{
		if (arg.numRetry && retryCount <= arg.numRetry)
			return;
		rcvMessage = string(smbuf, smbuf.size());
		mainSem.post();
	}

	virtual void onTerminated(const siginfo_t *siginfo) override
	{
		if (siginfo->si_signo == SIGCHLD &&
		    siginfo->si_code  == CLD_EXITED) {
			return;
		}
		mainSem.post();
		abnormalChildTerm = true;
		MLPL_ERR("si_signo: %d, si_code: %d\n",
		         siginfo->si_signo, siginfo->si_code);
	}

	virtual int onCaughtException(const exception &e) override
	{
		printf("onCaughtException: %s\n", e.what());
		if (arg.numRetry) {
			canncelRetrySleepIfNeeded();
			return arg.retrySleepTime;
		}

		if (rcvMessage.empty())
			gotUnexceptedException = true;
		return HatoholArmPluginGate::NO_RETRY;
	}

	virtual void onLaunchedProcess(
	  const bool &succeeded, const ArmPluginInfo &armPluginInfo) override
	{
		launchSucceeded = succeeded;
		launchedSem.post();
	}

	void canncelRetrySleepIfNeeded(void)
	{
		if (!arg.cancelRetrySleep)
			return;
		mainSem.post();
	}
};

static void _assertStartAndExit(StartAndExitArg &arg)
{
	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.type = arg.monitoringSystemType;
	TestHatoholArmPluginGate *hapg =
	  new TestHatoholArmPluginGate(serverInfo, arg);
	HatoholArmPluginGatePtr pluginGate(hapg, false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	pluginGate->start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  SimpleSemaphore::STAT_OK, hapg->launchedSem.timedWait(TIME_OUT));
	cppcut_assert_equal(arg.expectedResultOfStart, hapg->launchSucceeded);

	SimpleSemaphore::Status status = SimpleSemaphore::STAT_OK;
	if (arg.waitMainSem)
		status = hapg->mainSem.timedWait(TIME_OUT);

	pluginGate->exitSync();
	// These assertions must be after pluginGate->exitSync()
	// to be sure to exit the thread.
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, status);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(false, hapg->abnormalChildTerm);
	if (arg.checkMessage)
		cppcut_assert_equal(string(testMessage), hapg->rcvMessage);
	cppcut_assert_equal(false, hapg->gotUnexceptedException);
	if (arg.checkNumRetry)
		cppcut_assert_equal(arg.numRetry, hapg->retryCount-1);
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
	arg.waitMainSem = true;
	arg.checkMessage = true;
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

void test_generateBrokerAddress(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	serverInfo.id = 530;
	cppcut_assert_equal(
	  string("hatohol-arm-plugin.530"),
	  TestHatoholArmPluginGate::callGenerateBrokerAddress(serverInfo));
}

void test_retryToConnect(void)
{
	StartAndExitArg arg;
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
	StartAndExitArg arg;
	arg.monitoringSystemType = MONITORING_SYSTEM_HAPI_TEST;
	arg.expectedResultOfStart = true;
	arg.checkMessage = false;
	arg.numRetry = 1;
	arg.retrySleepTime = 3600 * 1000; // any value OK if it's long enough
	arg.cancelRetrySleep = true;
	assertStartAndExit(arg);
}

} // namespace testHatoholArmPluginGate
