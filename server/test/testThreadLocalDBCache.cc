/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <SimpleSemaphore.h>
#include <errno.h>
#include <cppcutter.h>
#include "Hatohol.h"
#include "ThreadLocalDBCache.h"
#include "HatoholThreadBase.h"
#include "Helpers.h"
#include "DBTablesTest.h"
using namespace std;
using namespace mlpl;

namespace testThreadLocalDBCache {

class TestCacheServiceThread : public HatoholThreadBase {

	DBHatohol       *m_dbHatohol;
	SimpleSemaphore  m_requestSem;
	SimpleSemaphore  m_completSem;
	bool             m_hasError;
	bool             m_exitRequest;

public:
	TestCacheServiceThread(void)
	: m_dbHatohol(NULL),
	  m_requestSem(0),
	  m_completSem(0),
	  m_hasError(false),
	  m_exitRequest(false)
	{
	}

	DBHatohol *callGetHatohol(void)
	{
		m_requestSem.post();
		m_completSem.wait();
		return m_dbHatohol;
	}

	bool hasError(void)
	{
		return m_hasError;
	}

	DBHatohol *getPrevDBCHatohol(void)
	{
		return m_dbHatohol;
	}

	virtual void stop(void)
	{
		m_exitRequest = true;
		m_requestSem.post();
		HatoholThreadBase::waitExit();
	}

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		while (true) {
			m_requestSem.wait();
			if (m_exitRequest)
				break;
			ThreadLocalDBCache cache;
			m_dbHatohol = &cache.getDBHatohol();
			m_completSem.post();
		}
		return NULL;
	}
};

static bool deleteTestCacheServiceThread(TestCacheServiceThread *thr)
{
	bool hasError = thr->hasError();
	thr->stop();
	delete thr;
	return !hasError;
}

template<class T>
static void _assertType(DBTables &dbTables)
{
	const type_info &expect = typeid(T);
	const type_info &actual = typeid(dbTables);
	cppcut_assert_equal(true, expect == actual,
	                    cut_message("expect: %s, actual: %s\n",
	                      expect.name(), actual.name()));
}
#define assertType(E,DBC) cut_trace(_assertType<E>(DBC))

static vector<TestCacheServiceThread *> g_threads;

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

void cut_teardown(void)
{
	bool hasError = false;
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		if (!thr)
			continue;
		if (!deleteTestCacheServiceThread(thr))
			hasError = true;
	}
	g_threads.clear();
	cppcut_assert_equal(false, hasError);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_hasInstanceByThread(void)
{
	typedef set<DBHatohol *>       DBHatoholSet;
	typedef DBHatoholSet::iterator DBHatoholSetIterator;
	const size_t numTestThread = 3;
	set<DBHatohol *> dbAddrs;
	for (size_t i = 0; i < numTestThread; i++) {
		TestCacheServiceThread *thr = new TestCacheServiceThread();
		g_threads.push_back(thr);
		thr->start();
		pair<DBHatoholSetIterator, bool> result =
		  dbAddrs.insert(thr->callGetHatohol());
		cppcut_assert_equal(true, result.second,
		                    cut_message("i: %zd\n", i));
	}
}

void test_ensureCached(void)
{
	test_hasInstanceByThread();
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		DBHatohol *prevDBCHatohol = thr->getPrevDBCHatohol();
		DBHatohol *newDBCHatohol = thr->callGetHatohol();
		cppcut_assert_equal(prevDBCHatohol, newDBCHatohol);
	}
}

void test_cleanupOnThreadExit(void)
{
	// Some thread may be running with the cache when this test is executed
	// So the number of the cached DB may not be zero. We have to
	// take into account it.
	size_t numCached0 = ThreadLocalDBCache::getNumberOfDBClientMaps();
	test_hasInstanceByThread();
	size_t numCached = ThreadLocalDBCache::getNumberOfDBClientMaps();
	cppcut_assert_equal(g_threads.size() + numCached0, numCached);
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		bool hasError = deleteTestCacheServiceThread(thr);
		g_threads[i] = NULL;
		cppcut_assert_equal(true, hasError);
		size_t newNumCached =
		  ThreadLocalDBCache::getNumberOfDBClientMaps();
		cppcut_assert_equal(numCached - 1, newNumCached);
		numCached = newNumCached;
	}
}

void test_getMonitoring(void)
{
	ThreadLocalDBCache cache;
	assertType(DBTablesMonitoring, cache.getMonitoring());
}

void test_getUser(void)
{
	ThreadLocalDBCache cache;
	assertType(DBTablesUser, cache.getUser());
}

#if 0
// The following statement makes a build fail. The behavior is correct,
// because the new operator is defined as a private to avoid it from being
// used. To check the effect, replace the above '#if 0' with '#if 1'.
// Or should we do a test that tries to compile the following function and
// expects a failure ?
void test_new(void)
{
	ThreadLocalDBCache *cache = new DBCache();
}
#endif

} // namespace testThreadLocalDBCache
