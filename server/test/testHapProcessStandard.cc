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

#include <errno.h>
#include <gcutter.h>
#include <cppcutter.h>
#include <errno.h>
#include <Reaper.h>
#include "Hatohol.h"
#include "Closure.h"
#include "HapProcessStandard.h"
#include "Helpers.h"
#include "HatoholArmPluginTestPair.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

static int TIMEOUT = 5; // sec
static int TIMEOUT_MS = TIMEOUT * 1000;

class TestHapProcessStandard :
   public HapProcessStandard, public HapiTestHelper {
public:
	struct CtorParams {
		int argc;
		char **argv;
		bool callSyncCommandInAcquireData;

		CtorParams(void)
		: argc(0),
		  argv(NULL),
		  callSyncCommandInAcquireData(false)
		{
		}
	} *m_params;

	SimpleSemaphore           m_acquireSem;
	SimpleSemaphore           m_acquireSyncCommandSem;
	SimpleSemaphore           m_acquireStopSem;
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
	  m_params(static_cast<CtorParams *>(params)),
	  m_acquireSem(0),
	  m_acquireSyncCommandSem(0),
	  m_acquireStopSem(0),
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

	virtual HatoholError acquireData(
	  const MessagingContext &msgCtx,
	  const SmartBuffer &cmdBuf) override
	{
		m_acquireSem.post();
		if (m_params->callSyncCommandInAcquireData) {
			// Before the follwoing semaphore is posted by
			// FetchItemStater, it sends reqFetchItem.
			// The request packet is supposed to be processed
			// prior to the reply of the following
			// getMonitoringServerInfo().
			m_acquireSyncCommandSem.wait();
			MonitoringServerInfo svInfo;
			HatoholArmPluginBase::getMonitoringServerInfo(svInfo);
			m_acquireSem.post();
		}
		if (m_throwException)
			throw 0;
		return m_returnValueOfAcquireData;
	}

	virtual HatoholError fetchItem(const MessagingContext &msgCtx,
				       const SmartBuffer &cmdBuf) override
	{
		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_ITEMS, &msgCtx);
		appendItemTable(resBuf, ItemTablePtr());
		appendItemTable(resBuf, ItemTablePtr()); // Item Category
		reply(msgCtx, resBuf);
		return HTERR_OK;
	}

	virtual HatoholError fetchHistory(const MessagingContext &msgCtx,
					  const SmartBuffer &cmdBuf) override
	{
		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_HISTORY,
					  &msgCtx);
		appendItemTable(resBuf, ItemTablePtr()); // empty history table
		reply(msgCtx, resBuf);
		return HTERR_OK;
	}

	virtual HatoholError fetchTrigger(const MessagingContext &msgCtx,
					  const SmartBuffer &cmdBuf) override
	{
		SmartBuffer resBuf;
		setupResponseBuffer<void>(resBuf, 0, HAPI_RES_TRIGGERS, &msgCtx);
		appendItemTable(resBuf, ItemTablePtr());
		reply(msgCtx, resBuf);
		return HTERR_OK;
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

static TestHapProcessStandard::CtorParams g_ctorParams;

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

struct FetchStarter : public HatoholThreadBase {
	typedef enum {
		INVALID,
		ITEM,
		HISTORY,
		TRIGGER,
		NUM_RESOURCE_TYPES
	} ResourceType;

	TestPair    *pair;
	GMainLoop   *loop;
	bool         succeeded;
	ResourceType fetched;
	ResourceType resourceType;

	FetchStarter(void)
	: loop(NULL),
	  succeeded(false),
	  fetched(INVALID),
	  resourceType(ITEM)
	{
		loop = g_main_loop_new(NULL, TRUE);
	}

	virtual ~FetchStarter()
	{
		g_main_loop_unref(loop);
	}

	void startFetchItem(void) {
		ClosureTemplate0<FetchStarter> *closure =
		  new ClosureTemplate0<FetchStarter>(this,
		    &FetchStarter::fetchItemCb);
		pair->gate->startOnDemandFetchItems(closure);
	}

	void startFetchHistory(void) {
		ClosureTemplate1<FetchStarter, HistoryInfoVect>
		  *closure =
		    new ClosureTemplate1<FetchStarter, HistoryInfoVect>(
		      this, &FetchStarter::fetchHistoryCb);
		ItemInfo itemInfo;
		itemInfo.serverId = ALL_SERVERS; // dummy
		itemInfo.globalHostId = ALL_HOSTS; // dummy
		itemInfo.hostIdInServer = ALL_LOCAL_HOSTS; // dummy
		itemInfo.id = 1;
		itemInfo.valueType = ITEM_INFO_VALUE_TYPE_FLOAT;
		time_t endTime = time(NULL);
		time_t beginTime = endTime - 60 * 60;
		pair->gate->startOnDemandFetchHistory(
		  itemInfo, beginTime, endTime, closure);
	}

	void startFetchTrigger(void) {
		ClosureTemplate0<FetchStarter> *closure =
		  new ClosureTemplate0<FetchStarter>(this,
		    &FetchStarter::fetchTriggerCb);
		pair->gate->startOnDemandFetchTriggers(closure);
	}

	virtual gpointer mainThread(HatoholThreadArg *arg) override
	{
		Reaper<GMainLoop> loopQuiter(loop, g_main_loop_quit);
		if (!wait())
			return NULL;
		HATOHOL_ASSERT(resourceType > INVALID &&
			       resourceType < NUM_RESOURCE_TYPES,
			       "Invalid resourceType");
		if (resourceType == ITEM)
			startFetchItem();
		else if (resourceType == HISTORY)
			startFetchHistory();
		else if (resourceType == TRIGGER)
			startFetchTrigger();
		pair->plugin->m_acquireSyncCommandSem.post();
		if (!wait())
			return NULL;
		// wait for callback of colusreCb()
		g_main_loop_run(m_localLoop.getLoop());
		succeeded = true;
		return NULL;
	}

	bool wait(void)
	{
		SimpleSemaphore::Status waitStat =
		  pair->plugin->m_acquireSem.timedWait(TIMEOUT_MS);
		if (waitStat != SimpleSemaphore::STAT_OK) {
			MLPL_ERR("Failed to wait: %d\n", waitStat);
			return false;
		}
		return true;
	}

	void fetchItemCb(Closure0 *closure)
	{
		fetched = ITEM;
		g_main_loop_quit(m_localLoop.getLoop());
	}

	void fetchHistoryCb(Closure1<HistoryInfoVect> *closure,
			    const HistoryInfoVect &historyInfoVect)
	{
		fetched = HISTORY;
		g_main_loop_quit(m_localLoop.getLoop());
	}

	void fetchTriggerCb(Closure0 *closure)
	{
		fetched = TRIGGER;
		g_main_loop_quit(m_localLoop.getLoop());
	}

private:
	GLibMainLoop m_localLoop;
};

void test_reqFetchItemDuringAcquireData(void)
{
	FetchStarter fetchItemStarter;

	GLibMainLoop glibMainLoopForGate;
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestHapProcessStandard::CtorParams ctorParams;
	ctorParams.callSyncCommandInAcquireData = true;
	arg.hapClassParameters = &ctorParams;
	arg.hapgCtx.glibMainContext = glibMainLoopForGate.getContext();
	glibMainLoopForGate.start();
	TestPair pair(arg);
	fetchItemStarter.pair = &pair;
	fetchItemStarter.start();

	// acquireData() is supposed to be invoked on this event loop.
	g_main_loop_run(fetchItemStarter.loop);
	cppcut_assert_equal(true, fetchItemStarter.succeeded);
	cppcut_assert_equal(FetchStarter::ITEM, fetchItemStarter.fetched);
}

void test_reqFetchHistoryDuringAcquireData(void)
{
	FetchStarter fetchHistoryStarter;
	fetchHistoryStarter.resourceType = FetchStarter::HISTORY;

	GLibMainLoop glibMainLoopForGate;
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestHapProcessStandard::CtorParams ctorParams;
	ctorParams.callSyncCommandInAcquireData = true;
	arg.hapClassParameters = &ctorParams;
	arg.hapgCtx.glibMainContext = glibMainLoopForGate.getContext();
	glibMainLoopForGate.start();
	TestPair pair(arg);
	fetchHistoryStarter.pair = &pair;
	fetchHistoryStarter.start();

	// acquireData() is supposed to be invoked on this event loop.
	g_main_loop_run(fetchHistoryStarter.loop);
	cppcut_assert_equal(true, fetchHistoryStarter.succeeded);
	cppcut_assert_equal(FetchStarter::HISTORY, fetchHistoryStarter.fetched);
}

void test_reqFetchTriggerDuringAcquireData(void)
{
	FetchStarter fetchTriggerStarter;
	fetchTriggerStarter.resourceType = FetchStarter::TRIGGER;

	GLibMainLoop glibMainLoopForGate;
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	TestHapProcessStandard::CtorParams ctorParams;
	ctorParams.callSyncCommandInAcquireData = true;
	arg.hapClassParameters = &ctorParams;
	arg.hapgCtx.glibMainContext = glibMainLoopForGate.getContext();
	glibMainLoopForGate.start();
	TestPair pair(arg);
	fetchTriggerStarter.pair = &pair;
	fetchTriggerStarter.start();

	// acquireData() is supposed to be invoked on this event loop.
	g_main_loop_run(fetchTriggerStarter.loop);
	cppcut_assert_equal(true, fetchTriggerStarter.succeeded);
	cppcut_assert_equal(FetchStarter::TRIGGER, fetchTriggerStarter.fetched);
}

} // namespace testHapProcessStandardPair
