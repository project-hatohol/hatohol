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
	SessionManager *instance1 = SessionManager::getInstance();
	SessionManager *instance2 = SessionManager::getInstance();
	cppcut_assert_equal(instance1, instance2);
}

void test_create(void)
{
	const UserIdType userId = 103;
	SessionManager *instance = SessionManager::getInstance();
	string sessionId = instance->create(userId);
	cppcut_assert_equal(false, sessionId.empty());

	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(instance);
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
	cppcut_assert_equal(sessionId, sessionIdMap.begin()->first);
	const Session *session = sessionIdMap.begin()->second;
	cppcut_assert_equal(userId, session->userId);
	assertTimeIsNow(session->loginTime);
	assertTimeIsNow(session->lastAccessTime);
}

void test_getSession(void)
{
	SessionManager *instance = SessionManager::getInstance();
	const UserIdType userId = 103;
	string sessionId = instance->create(userId);
	{ // Use a block to check the used counter of session.
		SessionPtr session = instance->getSession(sessionId);
		cppcut_assert_equal(true, session.hasData());
		cppcut_assert_equal(userId, session->userId);
		cppcut_assert_equal(2, session->getUsedCount());
	}

	// check the used count of the session
	const SessionIdMap &sessionIdMap = safeGetSessionIdMap(instance);
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
	cppcut_assert_equal(sessionId, sessionIdMap.begin()->first);
	const Session *session = sessionIdMap.begin()->second;
	cppcut_assert_equal(userId, session->userId);
	cppcut_assert_equal(1, session->getUsedCount());
}

} // namespace testSessionManager

