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
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "Synchronizer.h"

namespace testHatoholThreadBase {

typedef gpointer (*TestFunc)(void *data);
static Synchronizer sync;

static gpointer throwHatoholExceptionFunc(void *data)
{
	THROW_HATOHOL_EXCEPTION("Test Exception");
	return NULL;
}

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
	HatoholThreadTestImpl(void)
	: m_testFunc(defaultTestFunc),
	  m_testData(NULL)
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

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		return (*m_testFunc)(m_testData);
	}

	static gpointer defaultTestFunc(void *data)
	{
		return NULL;
	}

private:
	TestFunc m_testFunc;
	void    *m_testData;
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
void test_exceptionCallback(void)
{
	HatoholThreadTestImpl threadTestee;
	threadTestee.setTestFunc(throwHatoholExceptionFunc, NULL);
	threadTestee.addExceptionCallback(exceptionCallbackFunc, NULL);
	cppcut_assert_equal(true, threadTestee.doTest());
}

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
		MutexLock mutexThreadExit;
		MutexLock mutexWaitRun;
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
	threadTestee.stop();
	cppcut_assert_equal(false, threadTestee.isStarted());
}

} // namespace testHatoholThreadBase


