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

#include <errno.h>
#include <gcutter.h>
#include <cppcutter.h>
#include <errno.h>
#include "Hatohol.h"
#include "HapProcessStandard.h"
#include "Helpers.h"
#include "HatoholArmPluginTestPair.h"

using namespace std;
using namespace hfl;
using namespace qpid::messaging;

static int TIMEOUT = 5; // sec

class TestHapProcessStandard :
   public HapProcessStandard, public HapiTestHelper {
public:
	struct CtorParams {
		int argc;
		char **argv;
	};

	SimpleSemaphore           m_acquireSem;
	HatoholError              m_errorOnComplated;
	HatoholArmPluginWatchType m_watchTypeOnComplated;

	// These are input paramters from the test cases
	HatoholError              m_returnValueOfAcquireData;
	bool                      m_throwException;
	bool                      m_calledPipeRdErrCb;
	bool                      m_calledPipeWrErrCb;

	TestHapProcessStandard(void *params)
	: HapProcessStandard(
	    static_cast<CtorParams *>(params)->argc,
	    static_cast<CtorParams *>(params)->argv),
	  m_acquireSem(0),
	  m_errorOnComplated(NUM_HATOHOL_ERROR_CODE),
	  m_watchTypeOnComplated(NUM_COLLECT_NG_KIND),
	  m_returnValueOfAcquireData(HTERR_OK),
	  m_throwException(false),
	  m_calledPipeRdErrCb(false),
	  m_calledPipeWrErrCb(false)
	{
	}

	virtual void onConnected(Connection &conn) override
	{
		HapProcessStandard::onConnected(conn);
		HapiTestHelper::onConnected(conn);
	}

	virtual void onInitiated(void) override
	{
		HapProcessStandard::onInitiated();
		HapiTestHelper::onInitiated();
	}

	virtual HatoholError acquireData(void) override
	{
		m_acquireSem.post();
		if (m_throwException)
			throw 0;
		return m_returnValueOfAcquireData;
	}

	virtual void onCompletedAcquistion(
	  const HatoholError &err,
	  const HatoholArmPluginWatchType &watchType) override
	{
		m_errorOnComplated = err;
		m_watchTypeOnComplated  = watchType;
	}

	virtual gboolean pipeRdErrCb(
	  GIOChannel *source, GIOCondition condition) override
	{
		m_calledPipeRdErrCb = true;
		HapProcessStandard::pipeRdErrCb(source, condition);
		return G_SOURCE_REMOVE;
	}

	virtual gboolean pipeWrErrCb(
	  GIOChannel *source, GIOCondition condition) override
	{
		m_calledPipeWrErrCb = true;
		HapProcessStandard::pipeWrErrCb(source, condition);
		return G_SOURCE_REMOVE;
	}

	void callOnReady(const MonitoringServerInfo &serverInfo)
	{
		onReady(serverInfo);
	}

	const ArmStatus &callGetArmStatus(void)
	{
		return getArmStatus();
	}

	NamedPipe &callGetHapPipeForRead(void)
	{
		return getHapPipeForRead();
	}

	NamedPipe &callGetHapPipeForWrite(void)
	{
		return getHapPipeForWrite();
	}
};

typedef HatoholArmPluginTestPair<TestHapProcessStandard> TestPair;

static TestHapProcessStandard::CtorParams g_ctorParams = {0, NULL};

namespace testHapProcessStandard {

struct StartAcquisitionTester {
	TestHapProcessStandard m_hapProc;
	MonitoringServerInfo   m_serverInfo;

	StartAcquisitionTester(void)
	: m_hapProc(&g_ctorParams)
	{
		initServerInfo(m_serverInfo);

		const ArmStatus &armStatus =  m_hapProc.callGetArmStatus();
		ArmInfo armInfo = armStatus.getArmInfo();
		cppcut_assert_equal(ARM_WORK_STAT_INIT, armInfo.stat);
	}

	void run(void)
	{
		m_hapProc.callOnReady(m_serverInfo);
	}

	void assert(const HatoholErrorCode &expectHatoholErrorCode,
	            const HatoholArmPluginWatchType &expectWatchType,
	            const ArmWorkingStatus &expectArmWorkingStat)
	{
		SmartTime t0(SmartTime::INIT_CURR_TIME);
		while (true) {
			int ret = m_hapProc.m_acquireSem.tryWait();
			if (ret == 0)
				break;
			SmartTime t1(SmartTime::INIT_CURR_TIME);
			t1 -= t0;
			if (t1.getAsSec() > TIMEOUT)
				cut_fail("time out\n");
			if (ret == EAGAIN)
				g_main_context_iteration(NULL, TRUE);
			else
				cut_fail("Unexpected result: %d\n", ret);
		}
		assertHatoholError(expectHatoholErrorCode,
		                    m_hapProc.m_errorOnComplated);
		cppcut_assert_equal(expectWatchType,
		                     m_hapProc.m_watchTypeOnComplated);

		const ArmStatus &armStatus =  m_hapProc.callGetArmStatus();
		ArmInfo armInfo = armStatus.getArmInfo();
		cppcut_assert_equal(expectArmWorkingStat, armInfo.stat);
		if (expectArmWorkingStat == ARM_WORK_STAT_FAILURE) {
			cppcut_assert_equal(false,
			                    armInfo.failureComment.empty());
		}
	}
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_startAcquisition(void)
{
	StartAcquisitionTester tester;
	tester.run();
	tester.assert(HTERR_OK, COLLECT_OK, ARM_WORK_STAT_OK);
}

void test_startAcquisitionWithError(void)
{
	StartAcquisitionTester tester;
	tester.m_hapProc.m_returnValueOfAcquireData = HTERR_UNKNOWN_REASON;
	tester.run();
	tester.assert(HTERR_UNKNOWN_REASON, COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	              ARM_WORK_STAT_FAILURE);
}

void test_startAcquisitionWithExcpetion(void)
{
	StartAcquisitionTester tester;
	tester.m_hapProc.m_returnValueOfAcquireData = HTERR_UNKNOWN_REASON;
	tester.run();
	tester.assert(HTERR_UNKNOWN_REASON, COLLECT_NG_HATOHOL_INTERNAL_ERROR,
	              ARM_WORK_STAT_FAILURE);
}

void test_startAcquisitionWithTransaction(void)
{
	StartAcquisitionTester tester;
	tester.m_hapProc.m_throwException = true;
	tester.run();
	tester.assert(HTERR_UNINITIALIZED, COLLECT_NG_PLGIN_INTERNAL_ERROR,
	              ARM_WORK_STAT_FAILURE);
}

} // namespace testHapProcessStandard

// ---------------------------------------------------------------------------
// new namespace
// ---------------------------------------------------------------------------
namespace testHapProcessStandardPair {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

void data_hapPipe(void)
{
	gcut_add_datum("Not passive",
	               "passivePlugin", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Passive",
	               "passivePlugin", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_hapPipe(gconstpointer data)
{
	struct LoopQuiter {
		static gboolean quit(gpointer data)
		{
			GMainLoop *loop = static_cast<GMainLoop *>(data);
			g_main_loop_quit(loop);
			return G_SOURCE_REMOVE;
		}
	};
	const bool passivePlugin = gcut_data_get_string(data, "passivePlugin");

	const MonitoringSystemType monSysType =
	 passivePlugin ?  MONITORING_SYSTEM_HAPI_TEST_PASSIVE :
	                  MONITORING_SYSTEM_HAPI_TEST;
	HatoholArmPluginTestPairArg arg(monSysType);

	TestHapProcessStandard::CtorParams ctorParams;
	string pipeName = StringUtils::sprintf(HAP_PIPE_NAME_FMT,
	                                       arg.serverId);
	const char *argv[] = {"prog", "--" HAP_PIPE_OPT, pipeName.c_str()};
	if (!passivePlugin) {
		ctorParams.argc = ARRAY_SIZE(argv);
		ctorParams.argv = (char **)argv;
		arg.hapClassParameters = &ctorParams;
	} else {
		arg.hapClassParameters = &g_ctorParams;
	}

	arg.autoStartPlugin = false;
	TestPair pair(arg);
	const bool expectValidFd = !passivePlugin;
	cppcut_assert_equal(
	  expectValidFd, pair.gate->callGetHapPipeForRead().getFd() > 0);
	cppcut_assert_equal(
	  expectValidFd, pair.gate->callGetHapPipeForWrite().getFd() > 0);

	Utils::setGLibIdleEvent(LoopQuiter::quit, pair.plugin->getGMainLoop());
	cppcut_assert_equal(EXIT_SUCCESS, pair.plugin->mainLoopRun());
	cppcut_assert_equal(
	  expectValidFd, pair.plugin->callGetHapPipeForRead().getFd() > 0);
	cppcut_assert_equal(
	  expectValidFd, pair.plugin->callGetHapPipeForWrite().getFd() > 0);
}

void test_hapPipeCatchErrorAndExit(void)
{
	struct GateDeleter : public HatoholThreadBase {
		TestPair *pair;
		virtual gpointer mainThread(HatoholThreadArg *arg) override
		{
			pair->gate->exitSync();
			pair->gate = NULL;
			return NULL;
		}
	} gateDeleter;

	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST);

	TestHapProcessStandard::CtorParams ctorParams;
	string pipeName = StringUtils::sprintf(HAP_PIPE_NAME_FMT,
	                                       arg.serverId);
	const char *argv[] = {"prog", "--" HAP_PIPE_OPT, pipeName.c_str()};
	ctorParams.argc = ARRAY_SIZE(argv);
	ctorParams.argv = (char **)argv;
	arg.hapClassParameters = &ctorParams;
	arg.autoStartPlugin = false;
	TestPair pair(arg);
	gateDeleter.pair = &pair;

	cppcut_assert_equal(false, pair.plugin->m_calledPipeRdErrCb);
	cppcut_assert_equal(false, pair.plugin->m_calledPipeWrErrCb);
	gateDeleter.start();
	cppcut_assert_equal(EXIT_SUCCESS, pair.plugin->mainLoopRun());
	g_main_loop_run(pair.plugin->getGMainLoop());
	cppcut_assert_equal(true, pair.plugin->m_calledPipeRdErrCb);
	cppcut_assert_equal(true, pair.plugin->m_calledPipeWrErrCb);
}

} // namespace testHapProcessStandardPair
