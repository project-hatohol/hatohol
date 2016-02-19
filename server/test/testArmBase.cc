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

#include <gcutter.h>
#include <cppcutter.h>
#include <AtomicValue.h>
#include <Mutex.h>
#include <Hatohol.h>
#include <ArmBase.h>
#include <ThreadLocalDBCache.h>
#include "Helpers.h"
#include "DBTablesTest.h"
#include "UnifiedDataStore.h"
using namespace std;
using namespace mlpl;

namespace testArmBase {

class TestArmBase : public ArmBase {
	typedef bool (*OneProcHook)(void *);
	
	OneProcHook m_oneProcHook;
	void       *m_oneProcHookData;
	OneProcHook m_oneProcFetchItemsHook;
	void       *m_oneProcFetchItemsHookData;
	OneProcHook m_oneProcFetchHistoryHook;
	void       *m_oneProcFetchHistoryHookData;
	OneProcHook m_oneProcFetchTriggerHook;
	void       *m_oneProcFetchTriggerHookData;

public:
	TestArmBase(const string name, const MonitoringServerInfo &serverInfo)
	: ArmBase(name, serverInfo),
	  m_oneProcHook(NULL),
	  m_oneProcHookData(NULL),
	  m_oneProcFetchItemsHook(NULL),
	  m_oneProcFetchItemsHookData(NULL),
	  m_oneProcFetchHistoryHookData(NULL),
	  m_oneProcFetchTriggerHook(NULL),
	  m_oneProcFetchTriggerHookData(NULL)
	{
	}

	void callRequestExit(void)
	{
		requestExit();
	}

	void callRequestExitAndWait(void)
	{
		requestExitAndWait();
	}

	void callGetArmStatus(ArmStatus *&armStatus)
	{
		return getArmStatus(armStatus);
	}

	void callSetFailureInfo(const string &comment)
	{
		setFailureInfo(comment);
	}

	void callSetInitialTriggerStatus()
	{
		setInitialTriggerStatus();
	}

	void setOneProcHook(OneProcHook hook, void *data)
	{
		m_oneProcHook = hook;
		m_oneProcHookData = data;
	}

	void setOneProcFetchItemsHook(OneProcHook hook, void *data)
	{
		m_oneProcFetchItemsHook = hook;
		m_oneProcFetchItemsHookData = data;
	}

	void setOneProcFetchHistoryHook(OneProcHook hook, void *data)
	{
		m_oneProcFetchHistoryHook = hook;
		m_oneProcFetchHistoryHookData = data;
	}

	void setOneProcFetchTriggerHook(OneProcHook hook, void *data)
	{
		m_oneProcFetchTriggerHook = hook;
		m_oneProcFetchTriggerHookData = data;
	}

protected:
	ArmPollingResult callHook(OneProcHook hookFunc, void *hookData)
	{
		if (!hookFunc)
			return COLLECT_OK;

		bool succeeded = (*hookFunc)(hookData);
		if (succeeded)
			return COLLECT_OK;
		else
			return COLLECT_NG_INTERNAL_ERROR;
	}

	virtual ArmPollingResult mainThreadOneProc(void) override
	{
		return callHook(m_oneProcHook, m_oneProcHookData);
	}

	virtual ArmPollingResult mainThreadOneProcFetchItems(void) override
	{
		return callHook(m_oneProcFetchItemsHook,
				m_oneProcFetchItemsHookData);
	}

	virtual ArmPollingResult mainThreadOneProcFetchHistory(
	  HistoryInfoVect &historyInfoVect,
	  const ItemInfo &itemInfo,
	  const time_t &beginTime,
	  const time_t &endTime) override
	{
		return callHook(m_oneProcFetchHistoryHook,
				m_oneProcFetchHistoryHookData);
	}

	virtual ArmPollingResult mainThreadOneProcFetchTriggers(void) override
	{
		return callHook(m_oneProcFetchTriggerHook,
				m_oneProcFetchTriggerHookData);
	}
};

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
	loadTestDBAction();

}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getName(void)
{
	const string name = "Bizan";
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(name, serverInfo);
	cppcut_assert_equal(name, armBase.getName());
}

void test_requestExitAndWait(void)
{
	struct Ctx {
		AtomicValue<bool> called;
		Mutex             startLock;
		bool              startLockUnlocked;

		Ctx(void)
		: called(false),
		  startLockUnlocked(false)
		{
			startLock.lock();
		}

		static void exitCb(void *data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			obj->called.set(true);
		}

		static bool oneProcHook(void *data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			if (obj->startLockUnlocked)
				return true;
			obj->startLock.unlock();
			obj->startLockUnlocked = true;
			return true;
		}

		void waitForFirstProc(void)
		{
			const size_t timeout = 5000; // ms
			Mutex::Status stat = startLock.timedlock(timeout);
			cppcut_assert_equal(Mutex::STAT_OK, stat);
		}
	} ctx;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(__func__, serverInfo);
	armBase.setOneProcHook(Ctx::oneProcHook, &ctx);
	armBase.addExitCallback(ctx.exitCb, &ctx);
	cppcut_assert_equal(false, ctx.called.get());

	armBase.start();
	ctx.waitForFirstProc();

	armBase.callRequestExitAndWait();
	cppcut_assert_equal(true, ctx.called.get());
}

void test_startStop(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	const ArmStatus &armStatus = armBase.getArmStatus();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
	armBase.start();
	cppcut_assert_equal(true, armStatus.getArmInfo().running);
	armBase.callRequestExitAndWait();
	cppcut_assert_equal(false, armStatus.getArmInfo().running);
}

void test_getArmBasePtr(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	ArmStatus *armNonConstStatus;
	armBase.callGetArmStatus(armNonConstStatus);
	const ArmStatus &armStatus = armBase.getArmStatus();
	cppcut_assert_equal(&armStatus, armNonConstStatus);
}

void data_statusLog(void)
{
	gcut_add_datum("Success", "result", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("Failure", "result", G_TYPE_BOOLEAN, FALSE,
	               "comment", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Failure (comment)", "result", G_TYPE_BOOLEAN, FALSE,
	               "comment", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_statusLog(gconstpointer data)
{
	struct Ctx {
		string comment;
		TestArmBase *armBase;
		bool result;
		bool setComment;

		bool oneProcHook(void)
		{
			if (!result && setComment)
				armBase->callSetFailureInfo(comment);
			armBase->callRequestExit();
			return result;
		}

		static bool oneProcHook(void *data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			return obj->oneProcHook();
		}

	} ctx;

	ctx.result = gcut_data_get_boolean(data, "result");
	ctx.setComment = false;
	if (!ctx.result)
		ctx.setComment = gcut_data_get_boolean(data, "comment");
	if (ctx.setComment)
		ctx.comment = "She sells sea shells by the seashore.";
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	ctx.armBase = &armBase;
	armBase.setOneProcHook(Ctx::oneProcHook, &ctx);
	armBase.start();
	armBase.waitExit(); // TODO: May blocks forever

	ArmInfo armInfo = armBase.getArmStatus().getArmInfo();
	cppcut_assert_equal((size_t)1, armInfo.numUpdate);
	if (ctx.result) {
		cppcut_assert_equal(ARM_WORK_STAT_OK, armInfo.stat);
		cppcut_assert_equal((size_t)0, armInfo.numFailure);
	} else {
		cppcut_assert_equal(ARM_WORK_STAT_FAILURE, armInfo.stat);
		cppcut_assert_equal((size_t)1, armInfo.numFailure);
	}
	if (ctx.setComment)
		cppcut_assert_equal(ctx.comment, armInfo.failureComment);
}

struct TestFetchCtx {
	AtomicValue<int>  oneProcCount;
	AtomicValue<int>  oneProcFetchItemsCount;
	AtomicValue<int>  oneProcFetchHistoryCount;
	AtomicValue<int>  oneProcFetchTriggerCount;
	AtomicValue<bool> fetchItemsClosureCalled;
	AtomicValue<bool> fetchItemsClosureDeleted;
	AtomicValue<bool> fetchHistoryClosureCalled;
	AtomicValue<bool> fetchHistoryClosureDeleted;
	AtomicValue<bool> fetchTriggerClosureCalled;
	AtomicValue<bool> fetchTriggerClosureDeleted;
	Mutex             startLock;
	bool              startLockUnlocked;
	struct FetchItemClosure : ClosureTemplate0<TestFetchCtx>
	{
		FetchItemClosure(TestFetchCtx *receiver, callback func)
		: ClosureTemplate0<TestFetchCtx>(receiver, func)
		{
		}
		virtual ~FetchItemClosure()
		{
			m_receiver->fetchItemsClosureDeleted.set(true);
		}
	} *fetchItemClosure;
	struct FetchHistoryClosure
	  : ClosureTemplate1<TestFetchCtx, HistoryInfoVect>
	{
		FetchHistoryClosure(TestFetchCtx *receiver, callback func)
		: ClosureTemplate1<TestFetchCtx, HistoryInfoVect>(
		    receiver, func)
		{
		}
		virtual ~FetchHistoryClosure()
		{
			m_receiver->fetchHistoryClosureDeleted.set(true);
		}
	} *fetchHistoryClosure;
	struct FetchTriggerClosure : ClosureTemplate0<TestFetchCtx>
	{
		FetchTriggerClosure(TestFetchCtx *receiver, callback func)
		: ClosureTemplate0<TestFetchCtx>(receiver, func)
		{
		}
		virtual ~FetchTriggerClosure()
		{
			m_receiver->fetchTriggerClosureDeleted.set(true);
		}
	} *fetchTriggerClosure;

	TestFetchCtx(void)
	: oneProcCount(0),
	  oneProcFetchItemsCount(0),
	  startLockUnlocked(false),
	  fetchItemClosure(NULL),
	  fetchHistoryClosure(NULL),
	  fetchTriggerClosure(NULL)
	{
		fetchItemClosure = new FetchItemClosure(
		  this, &TestFetchCtx::itemFetchedCallback);
		fetchHistoryClosure = new FetchHistoryClosure(
		  this, &TestFetchCtx::historyFetchedCallback);
		fetchTriggerClosure = new FetchTriggerClosure(
		  this, &TestFetchCtx::triggerFetchedCallback);
		startLock.lock();
	}

	virtual ~TestFetchCtx(void)
	{
		delete fetchItemClosure;
		delete fetchHistoryClosure;
	}

	void unlock(void)
	{
		if (startLockUnlocked)
			return;
		startLock.unlock();
		startLockUnlocked = true;
	}

	static bool oneProcHook(void *data)
	{
		TestFetchCtx *obj
		  = static_cast<TestFetchCtx *>(data);
		obj->oneProcCount.set(obj->oneProcCount.get() + 1);
		obj->unlock();
		return true;
	}

	static bool oneProcFetchItemsHook(void *data)
	{
		TestFetchCtx *obj
		  = static_cast<TestFetchCtx *>(data);
		obj->oneProcFetchItemsCount.set(
		  obj->oneProcFetchItemsCount.get() + 1);
		obj->unlock();
		return true;
	}

	static bool oneProcFetchHistoryHook(void *data)
	{
		TestFetchCtx *obj
		  = static_cast<TestFetchCtx *>(data);
		obj->oneProcFetchHistoryCount.set(
		  obj->oneProcFetchHistoryCount.get() + 1);
		obj->unlock();
		return true;
	}

	static bool oneProcFetchTriggerHook(void *data)
	{
		TestFetchCtx *obj
		  = static_cast<TestFetchCtx *>(data);
		obj->oneProcFetchTriggerCount.set(
		  obj->oneProcFetchTriggerCount.get() + 1);
		obj->unlock();
		return true;
	}

	void waitForFirstProc(void)
	{
		const size_t timeout = 5000; // ms
		Mutex::Status stat = startLock.timedlock(timeout);
		cppcut_assert_equal(Mutex::STAT_OK, stat);
	}

	void itemFetchedCallback(Closure0 *_closure)
	{
		fetchItemsClosureCalled.set(true);
		// will be deleted by ArmBae
		fetchItemClosure = NULL;
	}

	void historyFetchedCallback(Closure1<HistoryInfoVect> *_closure,
				    const HistoryInfoVect &historyInfoVect)
	{
		fetchHistoryClosureCalled.set(true);
		// will be deleted by ArmBae
		fetchHistoryClosure = NULL;
	}

	void triggerFetchedCallback(Closure0 *_closure)
	{
		fetchTriggerClosureCalled.set(true);
		// will be deleted by ArmBae
		fetchTriggerClosure = NULL;
	}
};

void test_fetchItems(void)
{
	TestFetchCtx ctx;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(__func__, serverInfo);
	armBase.setOneProcHook(TestFetchCtx::oneProcHook, &ctx);
	armBase.setOneProcFetchItemsHook(
	  TestFetchCtx::oneProcFetchItemsHook, &ctx);

	armBase.fetchItems(ctx.fetchItemClosure);
	armBase.start();
	ctx.waitForFirstProc();
	armBase.callRequestExitAndWait();

	cppcut_assert_equal(1, ctx.oneProcFetchItemsCount.get());
	cppcut_assert_equal(0, ctx.oneProcCount.get());
	cppcut_assert_equal(true, ctx.fetchItemsClosureCalled.get());
	cppcut_assert_equal(true, ctx.fetchItemsClosureDeleted.get());
}

void test_fetchHistory(void)
{
	TestFetchCtx ctx;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(__func__, serverInfo);
	armBase.setOneProcHook(TestFetchCtx::oneProcHook, &ctx);
	armBase.setOneProcFetchHistoryHook(
	  TestFetchCtx::oneProcFetchHistoryHook, &ctx);

	ItemInfo itemInfo;
	itemInfo.id = "0";
	itemInfo.serverId = 0;
	itemInfo.globalHostId = 0;
	itemInfo.valueType = ITEM_INFO_VALUE_TYPE_FLOAT;
	armBase.fetchHistory(itemInfo, 0, 0, ctx.fetchHistoryClosure);
	armBase.start();
	ctx.waitForFirstProc();
	armBase.callRequestExitAndWait();

	cppcut_assert_equal(1, ctx.oneProcFetchHistoryCount.get());
	cppcut_assert_equal(0, ctx.oneProcCount.get());
	cppcut_assert_equal(true, ctx.fetchHistoryClosureCalled.get());
	cppcut_assert_equal(true, ctx.fetchHistoryClosureDeleted.get());
}

void test_fetchTriggers(void)
{
	TestFetchCtx ctx;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(__func__, serverInfo);
	armBase.setOneProcHook(TestFetchCtx::oneProcHook, &ctx);
	armBase.setOneProcFetchTriggerHook(
	  TestFetchCtx::oneProcFetchTriggerHook, &ctx);

	armBase.fetchTriggers(ctx.fetchTriggerClosure);
	armBase.start();
	ctx.waitForFirstProc();
	armBase.callRequestExitAndWait();

	cppcut_assert_equal(1, ctx.oneProcFetchTriggerCount.get());
	cppcut_assert_equal(0, ctx.oneProcCount.get());
	cppcut_assert_equal(true, ctx.fetchTriggerClosureCalled.get());
	cppcut_assert_equal(true, ctx.fetchTriggerClosureDeleted.get());
}

void test_hasTriggerWithNoTrigger(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	cppcut_assert_equal(
	  false, armBase.hasTrigger(ArmBase::COLLECT_NG_PARSER_ERROR));
}

void test_hasTrigger(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	armBase.registerAvailableTrigger(ArmBase::COLLECT_NG_PARSER_ERROR,
					 FAILED_PARSER_JSON_DATA_TRIGGER_ID,
					 HTERR_FAILED_TO_PARSE_JSON_DATA);
	cppcut_assert_equal(
	  true, armBase.hasTrigger(ArmBase::COLLECT_NG_PARSER_ERROR));
}

void test_setServerConnectStatusWithNoTrigger(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	armBase.setServerConnectStatus(ArmBase::COLLECT_NG_INTERNAL_ERROR);

	ThreadLocalDBCache cache;
	DBAgent &dbAgent = cache.getMonitoring().getDBAgent();
	assertDBContent(&dbAgent, "SELECT * FROM triggers", "");
	assertDBContent(&dbAgent, "SELECT * FROM events", "");
	assertDBContent(&dbAgent, "SELECT * FROM hosts", "");
}

void test_setServerConnectStatus(void)
{
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	MonitoringServerInfo serverInfo;
	ArmPluginInfo armInfo;
	initServerInfo(serverInfo);
	ArmPluginInfo::initialize(armInfo);
	serverInfo.id = 1;
	ThreadLocalDBCache cache;
	OperationPrivilege privilege(USER_ID_SYSTEM);
	cache.getConfig().addTargetServer(serverInfo, armInfo, privilege);
	TestArmBase armBase(__func__, serverInfo);
	armBase.registerAvailableTrigger(ArmBase::COLLECT_NG_INTERNAL_ERROR,
					 FAILED_INTERNAL_ERROR_TRIGGER_ID,
					 HTERR_INTERNAL_ERROR);
	armBase.callSetInitialTriggerStatus();
	armBase.setServerConnectStatus(ArmBase::COLLECT_NG_INTERNAL_ERROR);

	// check the generated trigger
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggers;
	dbMonitoring.getTriggerInfoList(triggers, TriggersQueryOption(USER_ID_SYSTEM));
	cppcut_assert_equal((size_t)1, (size_t)triggers.size());

	TriggerInfo triggerInfo;
	triggerInfo.serverId = serverInfo.id;
	triggerInfo.id = FAILED_INTERNAL_ERROR_TRIGGER_ID;
	triggerInfo.status = TRIGGER_STATUS_PROBLEM;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.lastChangeTime = triggers.begin()->lastChangeTime;
	triggerInfo.globalHostId = 1;
	triggerInfo.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
	triggerInfo.hostName = "_SELF";
	triggerInfo.brief = HatoholError(HTERR_INTERNAL_ERROR).getMessage();
	triggerInfo.validity = TRIGGER_VALID_SELF_MONITORING;
	cppcut_assert_equal(makeTriggerOutput(triggerInfo),
			    makeTriggerOutput(*triggers.begin()));

	// check the generated event
	EventInfoList events;
	dbMonitoring.getEventInfoList(events, EventsQueryOption(USER_ID_SYSTEM));
	cppcut_assert_equal((size_t)1, (size_t)events.size());

	EventInfo eventInfo;
	eventInfo.unifiedId = 1;
	eventInfo.serverId = serverInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = events.begin()->time;
	eventInfo.type = EVENT_TYPE_BAD;
	eventInfo.triggerId = FAILED_INTERNAL_ERROR_TRIGGER_ID;
	eventInfo.status = TRIGGER_STATUS_PROBLEM;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.globalHostId = 1;
	eventInfo.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
	eventInfo.hostName = "_SELF";
	eventInfo.brief = HatoholError(HTERR_INTERNAL_ERROR).getMessage();
	cppcut_assert_equal(makeEventOutput(eventInfo),
			    makeEventOutput(*events.begin()));

	// check the generated host
	ServerHostDefVect hosts;
	HostsQueryOption option(USER_ID_SYSTEM);
	uds->getServerHostDefs(hosts, option);
	cppcut_assert_equal((size_t)1, (size_t)hosts.size());
	const ServerHostDef &actualHost = *hosts.begin();
	cppcut_assert_equal(serverInfo.id, actualHost.serverId);
	cppcut_assert_equal(
	  MONITORING_SELF_LOCAL_HOST_ID, actualHost.hostIdInServer);
	cppcut_assert_equal(string("_SELF"), actualHost.name);
	cppcut_assert_equal(HOST_STAT_SELF_MONITOR, actualHost.status);
}

} // namespace testArmBase
