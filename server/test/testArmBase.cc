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
#include <AtomicValue.h>
#include <MutexLock.h>
#include "ArmBase.h"
using namespace std;
using namespace mlpl;

namespace testArmBase {

class TestArmBase : public ArmBase {
	typedef void (*OneProcHook)(void *);
	
	OneProcHook m_oneProcHook;
	void       *m_oneProcHookData;
public:
	TestArmBase(const string name, const MonitoringServerInfo &serverInfo)
	: ArmBase(name, serverInfo),
	  m_oneProcHook(NULL),
	  m_oneProcHookData(NULL)
	{
	}

	void callRequestExitAndWait(void)
	{
		requestExitAndWait();
	}

	ArmStatus &callGetNonConstArmStatus(void)
	{
		return getNonConstArmStatus();
	}

	void setOneProcHook(OneProcHook hook, void *data)
	{
		m_oneProcHook = hook;
		m_oneProcHookData = data;
	}

protected:
	virtual bool mainThreadOneProc(void) // override
	{
		if (m_oneProcHook)
			(*m_oneProcHook)(m_oneProcHookData);
		return true;
	}
};

static void initServerInfo(MonitoringServerInfo &serverInfo)
{
	serverInfo.id = 12345;
	serverInfo.pollingIntervalSec = 1;
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
		MutexLock         startLock;
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

		static void oneProcHook(void *data)
		{
			Ctx *obj = static_cast<Ctx *>(data);
			if (obj->startLockUnlocked)
				return;
			obj->startLock.unlock();
			obj->startLockUnlocked = true;
		}

		void waitForFirstProc(void)
		{
			const size_t timeout = 5000; // ms
			MutexLock::Status stat = startLock.timedlock(timeout);
			cppcut_assert_equal(MutexLock::STAT_OK, stat);
		}
	} ctx;

	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);

	TestArmBase armBase(__func__, serverInfo);
	armBase.setOneProcHook(ctx.oneProcHook, &ctx);
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

void test_getNonConstArmBase(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	TestArmBase armBase(__func__, serverInfo);
	ArmStatus &armNonConstStatus = armBase.callGetNonConstArmStatus();
	const ArmStatus &armStatus = armBase.getArmStatus();
	cppcut_assert_equal(&armStatus, &armNonConstStatus);
}

} // namespace testArmBase
