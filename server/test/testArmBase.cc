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
using namespace std;
using namespace mlpl;

namespace testArmBase {

class TestArmBase : public ArmBase {
	typedef bool (*OneProcHook)(void *);
	
	OneProcHook m_oneProcHook;
	void       *m_oneProcHookData;
public:
	TestArmBase(const string name, const MonitoringServerInfo &serverInfo)
	: ArmBase(name, serverInfo),
	  m_oneProcHook(NULL),
	  m_oneProcHookData(NULL)
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

protected:
	virtual OneProcEndType mainThreadOneProc(void) override
	{
		if (m_oneProcHook){
			bool succeeded = (*m_oneProcHook)(m_oneProcHookData);
			if (succeeded)
				return COLLECT_OK;
			else
				return COLLECT_NG_INTERNAL_ERROR;
		}
		return COLLECT_OK;
	}
};

void cut_setup(void)
{
	hatoholInit();
	deleteDBClientHatoholDB();
	setupTestDBUser(true, true);
	setupTestDBAction(true, true);
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

} // namespace testArmBase
