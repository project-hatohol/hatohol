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
#include "HatoholArmPluginGate.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "Hatohol.h"
#include "hapi-test-plugin.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

namespace testHatoholArmPluginGate {

struct StartAndExitArg {
	MonitoringSystemType monitoringSystemType;
	bool                 expectedResultOfStart;
	bool                 checkMessage;
	size_t               numRetry;

	StartAndExitArg(void)
	: monitoringSystemType(MONITORING_SYSTEM_HAPI_TEST),
	  expectedResultOfStart(false),
	  checkMessage(false),
	  numRetry(0)
	{
	}
};

class TestHatoholArmPluginGate : public HatoholArmPluginGate {
public:
	static const size_t TIMEOUT_MSEC = 5000;
	const StartAndExitArg &arg;
	bool       timedOut;
	guint      timerTag;
	bool       abnormalChildTerm;
	string     rcvMessage;
	bool       gotUnexceptedException;
	size_t     retryCount;

	class Loop {
	public:
		Loop(void)
		: m_loop(NULL)
		{
		}

		virtual ~Loop()
		{
			if (m_loop)
				g_main_loop_unref(m_loop);
		}

		GMainLoop *get(void)
		{
			m_lock.lock();
			if (!m_loop)
				m_loop = g_main_loop_new(NULL, TRUE);
			m_lock.unlock();
			return m_loop;
		}

	private:
		MutexLock m_lock;
		GMainLoop *m_loop;
	} loop;

	TestHatoholArmPluginGate(const MonitoringServerInfo &serverInfo,
	                         const StartAndExitArg &_arg)
	: HatoholArmPluginGate(serverInfo),
	  arg(_arg),
	  timedOut(false),
	  timerTag(INVALID_EVENT_ID),
	  abnormalChildTerm(false),
	  gotUnexceptedException(false),
	  retryCount(0)
	{
	}
	
	virtual ~TestHatoholArmPluginGate()
	{
		if (timerTag != INVALID_EVENT_ID)
			g_source_remove(timerTag);
	}

	static string callGenerateBrokerAddress(
	  const MonitoringServerInfo &serverInfo)
	{
		return generateBrokerAddress(serverInfo);
	}

	// We assume these virtual funcitons are called from
	// the plugin's thread.
	// I.e. we must not call cutter's assertions in them.
	virtual void onSessionChanged(Session *session) // override
	{
		if (arg.numRetry && session) {
			if (retryCount < arg.numRetry)
				session->close();
			retryCount++;
		}
	}

	virtual void onReceived(Message &message) // override
	{
		rcvMessage = message.getContent();
		g_main_loop_quit(loop.get());
	}

	virtual void onTerminated(const siginfo_t *siginfo) // override
	{
		g_main_loop_quit(loop.get());
		if (siginfo->si_signo == SIGCHLD &&
		    siginfo->si_code  == CLD_EXITED) {
			return;
		}
		abnormalChildTerm = true;
		MLPL_ERR("si_signo: %d, si_code: %d\n",
		         siginfo->si_signo, siginfo->si_code);
	}

	virtual int onCaughtException(const exception &e) // override
	{
		printf("onCaughtException: %s\n", e.what());
		if (arg.numRetry)
			return 1;

		if (rcvMessage.empty())
			gotUnexceptedException = true;
		return HatoholArmPluginGate::NO_RETRY;
	}

	// We assume this funciton is called from the main test thread.
	void mainLoopRun(void)
	{
		timerTag = g_timeout_add(TIMEOUT_MSEC, timeOutFunc, this);
		g_main_loop_run(loop.get());
	}

	static gboolean timeOutFunc(gpointer data)
	{
		TestHatoholArmPluginGate *obj =
		  static_cast<TestHatoholArmPluginGate *>(data);
		obj->timerTag = INVALID_EVENT_ID;
		obj->timedOut = true;
		g_main_loop_quit(obj->loop.get());
		return G_SOURCE_REMOVE;
	}
};

static void _assertStartAndExit(StartAndExitArg &arg)
{
	setupTestDBConfig();
	loadTestDBArmPlugin();
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestHatoholArmPluginGate *hapg =
	  new TestHatoholArmPluginGate(serverInfo, arg);
	HatoholArmPluginGatePtr pluginGate(hapg, false);
	const ArmStatus &armStatus = pluginGate->getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(
	  arg.expectedResultOfStart,
	  pluginGate->start(arg.monitoringSystemType));
	cppcut_assert_equal(
	  arg.expectedResultOfStart, armStatus.getArmInfo().running);

	if (arg.checkMessage)
		hapg->mainLoopRun();

	pluginGate->waitExit();
	// These assertions must be after pluginGate->waitExit()
	// to be sure to exit the thread.
	cppcut_assert_equal(false, hapg->timedOut);
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	cppcut_assert_equal(false, hapg->abnormalChildTerm);
	if (arg.checkMessage)
		cppcut_assert_equal(string(testMessage), hapg->rcvMessage);
	cppcut_assert_equal(false, hapg->gotUnexceptedException);
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
	arg.checkMessage = true;
	arg.numRetry = 3;
	assertStartAndExit(arg);
}

} // namespace testHatoholArmPluginGate
