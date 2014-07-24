/*
 * Copyright (C) 2013 Project Hatohol
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
#include <exception>
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "Synchronizer.h"
using namespace std;
using namespace mlpl;

namespace testHatoholThreadBase {

typedef gpointer (*TestFunc)(void *data);
static Synchronizer sync;

void exceptionCallbackFunc(const exception &e, void *data)
{
	sync.unlock();
}

void exitCallbackFunc(void *data)
{
	sync.unlock();
}

class HatoholThreadTestImpl : public HatoholThreadBase {
public:
	static const size_t TIMEOUT_MSEC = 5000;

	struct MainLoop {
		GMainLoop *loop;
		Mutex      lock;
		guint      timerTag;

		MainLoop(void)
		: loop(NULL),
		  timerTag(INVALID_EVENT_ID)
		{
		}

		virtual ~MainLoop()
		{
			if (loop)
				g_main_loop_unref(loop);
			if (timerTag != INVALID_EVENT_ID)
				g_source_remove(timerTag);
		}

		GMainLoop *get(void)
		{
			lock.lock();
			if (!loop)
				loop = g_main_loop_new(NULL, TRUE);
			lock.unlock();
			return loop;
		}
	};

	HatoholThreadTestImpl(void)
	: m_testFunc(defaultTestFunc),
	  m_testData(NULL),
	  m_timedOut(false)
	{
	}

	bool doTest(void) {
		sync.lock();
		start();
		sync.wait();
		return true;
	}

	void setTestFunc(TestFunc func, void *data)
	{
		m_testFunc = func;
		m_testData = data;
	}

	GMainLoop *getMainLoop(void)
	{
		return m_loop.get();
	}

	void loopRun(void)
	{
		m_loop.timerTag = g_timeout_add(TIMEOUT_MSEC, timeOutFunc, this);
		g_main_loop_run(getMainLoop());
	}

	bool isTimedOut(void) const
	{
		return m_timedOut;
	}

	void callRequestExit(void)
	{
		requestExit();
	}

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		return (*m_testFunc)(m_testData);
	}

	static gpointer defaultTestFunc(void *data)
	{
		return NULL;
	}

	static gboolean timeOutFunc(gpointer data)
	{
		HatoholThreadTestImpl *obj =
		  static_cast<HatoholThreadTestImpl *>(data);
		obj->m_loop.timerTag = INVALID_EVENT_ID;
		obj->m_timedOut = true;
		g_main_loop_quit(obj->getMainLoop());
		return G_SOURCE_REMOVE;
	}

private:
	TestFunc m_testFunc;
	void    *m_testData;
	MainLoop m_loop;
	bool     m_timedOut;
};

struct HatoholThreadTestReexec : public HatoholThreadTestImpl {
public:
	int reexecSleepTime;
	int exceptionCount;
	int numReexec;
	bool setupCancelSleep;
	guint cancelTimerTag;

	HatoholThreadTestReexec(void)
	: reexecSleepTime(EXIT_THREAD),
	  exceptionCount(0),
	  numReexec(0),
	  setupCancelSleep(false),
	  cancelTimerTag(INVALID_EVENT_ID)
	{
	}

	virtual ~HatoholThreadTestReexec() override
	{
		if (cancelTimerTag != INVALID_EVENT_ID)
			g_source_remove(cancelTimerTag);
	}

protected:

	virtual gpointer mainThread(HatoholThreadArg *arg) override
	{
		throw exception();
	}

	virtual int onCaughtException(const exception &e) override
	{
		exceptionCount++;
		if (exceptionCount >= numReexec) {
			g_main_loop_quit(getMainLoop());
			return EXIT_THREAD;
		}
		if (setupCancelSleep)
			cancelTimerTag = g_timeout_add(1, cancelTimer, this);
		return reexecSleepTime;
	}

	static gboolean cancelTimer(gpointer data)
	{
		HatoholThreadTestReexec *obj =
		  static_cast<HatoholThreadTestReexec *>(data);
		obj->cancelTimerTag = INVALID_EVENT_ID;
		g_main_loop_quit(obj->getMainLoop());
		return G_SOURCE_REMOVE;
	}
};

void cut_setup(void)
{
	if (!sync.trylock())
		cut_fail("lock is not unlocked.");
	sync.unlock();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_exitCallback(void)
{
	HatoholThreadTestImpl threadTestee;
	threadTestee.addExitCallback(exitCallbackFunc, NULL);
	cppcut_assert_equal(true, threadTestee.doTest());
}

void test_isStartedInitial(void)
{
	HatoholThreadTestImpl threadTestee;
	cppcut_assert_equal(false, threadTestee.isStarted());
}

void test_isStarted(void)
{
	struct Priv {
		Mutex mutexThreadExit;
		Mutex mutexWaitRun;
		static gpointer func(void *data) {
			Priv *obj = static_cast<Priv *>(data);
			obj->mutexWaitRun.unlock();
			obj->mutexThreadExit.lock();
			return NULL;
		}
	} priv;
	HatoholThreadTestImpl threadTestee;
	threadTestee.setTestFunc(priv.func, &priv);
	priv.mutexThreadExit.lock();
	priv.mutexWaitRun.lock();
	threadTestee.start();
	cppcut_assert_equal(true, threadTestee.isStarted());
	priv.mutexWaitRun.lock();
	priv.mutexThreadExit.unlock();
	threadTestee.waitExit();
	cppcut_assert_equal(false, threadTestee.isStarted());
}

void test_reexec(void)
{
	HatoholThreadTestReexec thread;
	thread.numReexec = 3;
	thread.reexecSleepTime = 1;
	thread.start();
	thread.loopRun();
	thread.waitExit();
	cppcut_assert_equal(false, thread.isTimedOut());
	cppcut_assert_equal(thread.numReexec, thread.exceptionCount);
}

void test_cancelReexecSleep(void)
{
	HatoholThreadTestReexec thread;
	thread.numReexec = 2;
	thread.reexecSleepTime = thread.TIMEOUT_MSEC * 1.5;
	thread.setupCancelSleep = true;
	thread.start();
	thread.loopRun();
	thread.exitSync();
	cppcut_assert_equal(false, thread.isTimedOut());
	cppcut_assert_equal(1, thread.exceptionCount);
}

void test_isExitRequested(void)
{
	HatoholThreadTestImpl thread;
	thread.start();
	cppcut_assert_equal(false, thread.isExitRequested());
	thread.exitSync();
	cppcut_assert_equal(true, thread.isExitRequested());
}

void test_requestExit(void)
{
	HatoholThreadTestImpl thread;
	cppcut_assert_equal(false, thread.isExitRequested());
	thread.callRequestExit();
	cppcut_assert_equal(true, thread.isExitRequested());
}

} // namespace testHatoholThreadBase


