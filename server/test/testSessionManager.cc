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
	cppcut_assert_equal(true, sessionMgr->remove(sessionId));
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

} // namespace testSessionManager

