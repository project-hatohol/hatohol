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

#include <string>
#include <cppcutter.h>
#include <unistd.h>
#include "SessionManager.h"
#include "Helpers.h"
using namespace std;

namespace testSessionManager {

static SessionManager *g_sessionIdMapOwner = NULL;

const SessionIdMap &safeGetSessionIdMap(SessionManager *sessionMgr)
{
	cppcut_assert_equal(static_cast<SessionManager *>(NULL),
	                    g_sessionIdMapOwner);
	g_sessionIdMapOwner = sessionMgr;
	return sessionMgr->getSessionIdMap();
}

void cut_setup(void)
{
	// The reset() clears sessions managed by SessionManager.
	// This implies that each test case doesn't have to remove
	// created sessions explicitly.
	SessionManager::reset();
}

void cut_teardown(void)
{
	if (g_sessionIdMapOwner) {
		g_sessionIdMapOwner->releaseSessionIdMap();
		g_sessionIdMapOwner = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getInstance(void)
{
	cppcut_assert_not_equal(static_cast<SessionManager *>(NULL),
	                        SessionManager::getInstance());
}

void test_isntanceIsSingleton(void)
{
	SessionManager *sessionMgr1 = SessionManager::getInstance();
	SessionManager *sessionMgr2 = SessionManager::getInstance();
	cppcut_assert_equal(sessionMgr1, sessionMgr2);
}

void test_create(void)
{
	const UserIdType userId = 103;
	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);
	cppcut_assert_equal(false, sessionId.empty());

	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(sessionMgr);
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
	cppcut_assert_equal(sessionId, sessionIdMap.begin()->first);
	const Session *session = sessionIdMap.begin()->second;
	cppcut_assert_equal(userId, session->userId);
	assertTimeIsNow(session->loginTime);
	assertTimeIsNow(session->lastAccessTime);
}

void test_createWithoutTimeout(void)
{
	const UserIdType userId = 103;
	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId =
	   sessionMgr->create(userId, SessionManager::NO_TIMEOUT);
	SessionPtr sessionPtr = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, sessionPtr.hasData()); 
	cppcut_assert_equal(SessionManager::NO_TIMEOUT, sessionPtr->timeout);
	cppcut_assert_equal(INVALID_EVENT_ID, sessionPtr->timerId);
}

void test_timeout(void)
{
	const size_t timeout = 1; // 1ms.
	const UserIdType userId = 103;
	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId, timeout);
	SessionPtr sessionPtr = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, sessionPtr.hasData()); 
	cppcut_assert_equal(timeout, sessionPtr->timeout);
	cppcut_assert_not_equal(INVALID_EVENT_ID, sessionPtr->timerId);

	// wait for the session's timeout
	struct : public Watcher
	{
		guint *timerId;
		virtual bool watch(void)
		{
			return *timerId == INVALID_EVENT_ID;
		}
	} watcher;
	watcher.timerId = &sessionPtr->timerId;

	const size_t watcherTimeout = 5*1000; // 5sec
	cppcut_assert_equal(true, watcher.start(watcherTimeout));
	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(sessionMgr);
	cppcut_assert_equal((size_t)0, sessionIdMap.size());
}

void test_getSession(void)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	const UserIdType userId = 103;
	string sessionId = sessionMgr->create(userId);
	{ // Use a block to check the used counter of session.
		SessionPtr session = sessionMgr->getSession(sessionId);
		cppcut_assert_equal(true, session.hasData());
		cppcut_assert_equal(userId, session->userId);
		cppcut_assert_equal(2, session->getUsedCount());
	}

	// check the used count of the session
	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(sessionMgr);
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
	cppcut_assert_equal(sessionId, sessionIdMap.begin()->first);
	const Session *session = sessionIdMap.begin()->second;
	cppcut_assert_equal(userId, session->userId);
	cppcut_assert_equal(1, session->getUsedCount());
}

void test_update(void)
{
	const UserIdType userId = 103;
	SessionManager *sessionMgr = SessionManager::getInstance();
	string sessionId = sessionMgr->create(userId);
	SessionPtr sessionPtr = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, sessionPtr.hasData()); 

	SmartTime prevAccessTime = sessionPtr->lastAccessTime;
	guint     prevTimerId    = sessionPtr->timerId;

	// call getSession a short time later
	const int sleepTimeMSec = 1;
	usleep(sleepTimeMSec * 1000);
	sessionPtr = sessionMgr->getSession(sessionId);
	cppcut_assert_equal(true, sessionPtr.hasData()); 

	// check
	SmartTime diffAccessTime = sessionPtr->lastAccessTime;
	diffAccessTime -= prevAccessTime;
	cppcut_assert_equal(true, diffAccessTime.getAsMSec() > sleepTimeMSec);
	cppcut_assert_not_equal(prevTimerId, sessionPtr->timerId);

	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(sessionMgr);
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
}

void test_getNonExistingSession(void)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	SessionPtr session = sessionMgr->getSession("non-existing-sesion-id");
	cppcut_assert_equal(false, session.hasData());
}

void test_remove(void)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	const UserIdType userId = 103;
	string sessionId = sessionMgr->create(userId);
	cppcut_assert_equal(true, sessionMgr->remove(sessionId));

	// check the session sessionMgr is removed
	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(sessionMgr);
	cppcut_assert_equal((size_t)0, sessionIdMap.size());
}

void test_removeNonExistingSession(void)
{
	SessionManager *sessionMgr = SessionManager::getInstance();
	cppcut_assert_equal(false,
	                    sessionMgr->remove("non-existing-sesion-id"));
}

void test_getInitialTimeout(void)
{
	cppcut_assert_equal(SessionManager::INITIAL_TIMEOUT,
	                    SessionManager::getDefaultTimeout());
}


} // namespace testSessionManager

