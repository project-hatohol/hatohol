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

#include <string>
#include <vector>
using namespace std;

#include <glib.h>
#include <cutter.h>
#include <cppcutter.h>

#include "Mutex.h"
using namespace mlpl;

namespace testMutex {

static const int COUNT_MAX = 100;

struct CounterThreadArg {
	volatile int count;
	Mutex lock;
};

static void *counterThread(void *_arg)
{
	CounterThreadArg *arg = static_cast<CounterThreadArg *>(_arg);
	while (arg->count < COUNT_MAX)
		arg->count++;
	arg->lock.unlock();
	return NULL;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_make(void)
{
	Mutex lock;
}

void test_lockUnlock(void)
{
	Mutex lock;
	lock.lock();
	lock.unlock();
}

void test_lockWait(void)
{
	pthread_t thread;
	CounterThreadArg arg;
	arg.count = 0;
	arg.lock.lock();
	cppcut_assert_equal(0,
	                    pthread_create(&thread, NULL, counterThread, &arg));
	cppcut_assert_equal(0, pthread_detach(thread));

	// wait for unlock() by the created thread.
	arg.lock.lock();

	cppcut_assert_equal(COUNT_MAX, (const int)arg.count);
}

void test_trylock(void)
{
	Mutex lock;
	lock.lock();
	cppcut_assert_equal(false, lock.trylock()); 
	lock.unlock();
	cppcut_assert_equal(true, lock.trylock()); 
	cppcut_assert_equal(false, lock.trylock()); 
}

void test_timedlock(void)
{
	Mutex lock;
	cppcut_assert_equal(Mutex::STAT_OK, lock.timedlock(1));
	cppcut_assert_equal(Mutex::STAT_TIMEDOUT, lock.timedlock(1));
}

void test_timedlockWithNormalLock(void)
{
	Mutex lock;
	lock.lock();
	cppcut_assert_equal(Mutex::STAT_TIMEDOUT, lock.timedlock(1));
}

void test_staticUnlock(void)
{
	Mutex lock;
	lock.lock();
	Mutex::unlock(&lock);
}

void test_autoMutex(void)
{
	Mutex lock;
	{
		AutoMutex autoLock(&lock);
		cppcut_assert_equal(false, lock.trylock());
	}
	cppcut_assert_equal(true, lock.trylock());
}

} // namespace testMutex
