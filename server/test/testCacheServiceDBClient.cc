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

#include <semaphore.h>
#include <errno.h>
#include <cppcutter.h>
#include "Hatohol.h"
#include "CacheServiceDBClient.h"
#include "HatoholThreadBase.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testCacheServiceDBClient {

class TestCacheServiceThread : public HatoholThreadBase {

	DBTablesMonitoring *m_dbMonitoring;
	sem_t            m_requestSem;
	sem_t            m_completSem;
	bool             m_hasError;
	bool             m_exitRequest;

public:
	TestCacheServiceThread(void)
	: m_dbMonitoring(NULL),
	  m_hasError(false),
	  m_exitRequest(false)
	{
		cppcut_assert_equal(0, sem_init(&m_requestSem, 0, 0),
		                    cut_message("errno: %d", errno));
		cppcut_assert_equal(0, sem_init(&m_completSem, 0, 0),
		                    cut_message("errno: %d", errno));
	}

	DBTablesMonitoring *callGetHatohol(void)
	{
		postSem(&m_requestSem);
		waitSem(&m_completSem);
		return m_dbMonitoring;
	}

	bool hasError(void)
	{
		return m_hasError;
	}

	DBTablesMonitoring *getPrevDBCHatohol(void)
	{
		return m_dbMonitoring;
	}

	virtual void stop(void)
	{
		m_exitRequest = true;
		postSem(&m_requestSem);
		HatoholThreadBase::waitExit();
	}

protected:
	bool postSem(sem_t *sem)
	{
		int ret = sem_post(sem);
		if (ret == -1) {
			MLPL_ERR("Failed to call sem_post(): %d\n", errno);
			m_hasError = true;
			return false;
		}
		return true;
	}

	void waitSem(sem_t *sem)
	{
		while (true) {
			int ret = sem_wait(sem);
			if (ret == -1 && errno == EINTR)
				continue;
			if (ret == -1) {
				MLPL_ERR("Failed to call sem_wait(): %d\n",
				         errno);
				m_hasError = true;
			}
			break;
		}
	}

	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		while (true) {
			waitSem(&m_requestSem);
			if (m_exitRequest)
				break;
			CacheServiceDBClient cache;
			m_dbMonitoring = cache.getMonitoring();
			if (!postSem(&m_completSem))
				break;
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
static void _assertType(DBClient *dbClient)
{
	cppcut_assert_not_null(dbClient);
	const type_info &expect = typeid(T);
	const type_info &actual = typeid(*dbClient);
	cppcut_assert_equal(true, expect == actual,
	                    cut_message("expect: %s, actual: %s\n",
	                      expect.name(), actual.name()));
}
#define assertType(E,DBC) cut_trace(_assertType<E>(DBC))

static vector<TestCacheServiceThread *> g_threads;

void cut_setup(void)
{
	hatoholInit();
	setupTestDBUser();
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
	typedef set<DBTablesMonitoring *>       DBTablesMonitoringSet;
	typedef DBTablesMonitoringSet::iterator DBTablesMonitoringSetIterator;
	const size_t numTestThread = 3;
	set<DBTablesMonitoring *> dbClientAddrs;
	for (size_t i = 0; i < numTestThread; i++) {
		TestCacheServiceThread *thr = new TestCacheServiceThread();
		g_threads.push_back(thr);
		thr->start();
		pair<DBTablesMonitoringSetIterator, bool> result = 
		  dbClientAddrs.insert(thr->callGetHatohol());
		cppcut_assert_equal(true, result.second,
		                    cut_message("i: %zd\n", i));
	}
}

void test_ensureCached(void)
{
	test_hasInstanceByThread();
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		DBTablesMonitoring *prevDBCHatohol = thr->getPrevDBCHatohol();
		DBTablesMonitoring *newDBCHatohol = thr->callGetHatohol();
		cppcut_assert_equal(prevDBCHatohol, newDBCHatohol);
	}
}

void test_cleanupOnThreadExit(void)
{
	// Some thread may be running with the cache when this test is executed
	// So the number of the cached DB may not be zero. We have to
	// take into account it.
	size_t numCached0 = CacheServiceDBClient::getNumberOfDBClientMaps();
	test_hasInstanceByThread();
	size_t numCached = CacheServiceDBClient::getNumberOfDBClientMaps();
	cppcut_assert_equal(g_threads.size() + numCached0, numCached);
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		bool hasError = deleteTestCacheServiceThread(thr);
		g_threads[i] = NULL;
		cppcut_assert_equal(true, hasError);
		size_t newNumCached = 
		  CacheServiceDBClient::getNumberOfDBClientMaps();
		cppcut_assert_equal(numCached - 1, newNumCached);
		numCached = newNumCached;
	}
}

void test_getMonitoring(void)
{
	CacheServiceDBClient cache;
	assertType(DBTablesMonitoring, cache.getMonitoring());
}

void test_getUser(void)
{
	CacheServiceDBClient cache;
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
	CacheServiceDBClient *cache = new CacheServiceDBClient();
}
#endif

} // namespace testCacheServiceDBClient
