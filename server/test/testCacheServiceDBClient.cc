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

namespace testCacheServiceDBClient {

class TestCacheServiceThread : public HatoholThreadBase {

	DBClientHatohol *m_dbHatohol;
	sem_t            m_requestSem;
	sem_t            m_completSem;
	bool             m_hasError;
	bool             m_exitRequest;

public:
	TestCacheServiceThread(void)
	: m_dbHatohol(NULL),
	  m_hasError(false),
	  m_exitRequest(false)
	{
		cppcut_assert_equal(0, sem_init(&m_requestSem, 0, 0),
		                    cut_message("errno: %d", errno));
		cppcut_assert_equal(0, sem_init(&m_completSem, 0, 0),
		                    cut_message("errno: %d", errno));
	}

	DBClientHatohol *callGetHatohol(void)
	{
		postSem(&m_requestSem);
		waitSem(&m_completSem);
		return m_dbHatohol;
	}

	bool hasError(void)
	{
		return m_hasError;
	}

	DBClientHatohol *getPrevDBCHatohol(void)
	{
		return m_dbHatohol;
	}

	virtual void stop(void)
	{
		m_exitRequest = true;
		postSem(&m_requestSem);
		HatoholThreadBase::stop();
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
			m_dbHatohol = cache.getHatohol();
			if (!postSem(&m_completSem))
				break;
		}
		return NULL;
	}
};

static vector<TestCacheServiceThread *> g_threads;

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	bool hasError = false;
	for (size_t i = 0; i < g_threads.size(); i++) {
		TestCacheServiceThread *thr = g_threads[i];
		if (thr->hasError())
			hasError = true;
		thr->stop();
		delete thr;
	}
	g_threads.clear();
	cppcut_assert_equal(false, hasError);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_hasInstanceByThread(void)
{
	typedef set<DBClientHatohol *>       DBClientHatoholSet;
	typedef DBClientHatoholSet::iterator DBClientHatoholSetIterator;
	const size_t numTestThread = 3;
	set<DBClientHatohol *> dbClientAddrs;
	for (size_t i = 0; i < numTestThread; i++) {
		TestCacheServiceThread *thr = new TestCacheServiceThread();
		g_threads.push_back(thr);
		thr->start();
		pair<DBClientHatoholSetIterator, bool> result = 
		  dbClientAddrs.insert(thr->callGetHatohol());
		cppcut_assert_equal(true, result.second,
		                    cut_message("i: %zd\n", i));
	}
}

} // namespace testCacheServiceDBClient
