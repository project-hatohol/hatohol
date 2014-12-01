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

#include <gcutter.h>
#include <cppcutter.h>
#include <AtomicValue.h>
#include <Mutex.h>
#include "Hatohol.h"
#include "ArmBase.h"
#include "Helpers.h"
#include "DBTablesTest.h"
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

public:
	TestArmBase(const string name, const MonitoringServerInfo &serverInfo)
	: ArmBase(name, serverInfo),
	  m_oneProcHook(NULL),
	  m_oneProcHookData(NULL),
	  m_oneProcFetchItemsHook(NULL),
	  m_oneProcFetchItemsHookData(NULL),
	  m_oneProcFetchHistoryHookData(NULL)
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
	AtomicValue<bool> fetchItemsClosureCalled;
	AtomicValue<bool> fetchItemsClosureDeleted;
	AtomicValue<bool> fetchHistoryClosureCalled;
	AtomicValue<bool> fetchHistoryClosureDeleted;
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

	TestFetchCtx(void)
	: oneProcCount(0),
	  oneProcFetchItemsCount(0),
	  startLockUnlocked(false),
	  fetchItemClosure(NULL),
	  fetchHistoryClosure(NULL)
	{
		fetchItemClosure = new FetchItemClosure(
		  this, &TestFetchCtx::itemFetchedCallback);
		fetchHistoryClosure = new FetchHistoryClosure(
		  this, &TestFetchCtx::historyFetchedCallback);
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
	itemInfo.id = 0;
	itemInfo.serverId = 0;
	itemInfo.hostId = 0;
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

} // namespace testArmBase
