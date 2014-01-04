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
using namespace std;

namespace testSessionManager {

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
	UserIdType userId = 103;
	SessionManager *instance = SessionManager::getInstance();
	string sessionId = instance->create(userId);
	cppcut_assert_equal(false, sessionId.empty());

	const SessionIdMap &sessionIdMap = instance->getSessionIdMap();
	cppcut_assert_equal((size_t)1, sessionIdMap.size());
	cppcut_assert_equal(sessionId, sessionIdMap.begin()->first);
	instance->releaseSessionIdMap();
}

} // namespace testSessionManager

