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

#include <cutter.h>
#include <cppcutter.h>
#include "IssueSenderManager.h"
#include "Hatohol.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "RedmineAPIEmulator.h"

using namespace std;

namespace testIssueSenderManager {

class TestIssueSenderManager : public IssueSenderManager
{
public:
	// Enable to create a object to test destructor
	TestIssueSenderManager(void)
	{
	}
};

void cut_setup(void)
{
	hatoholInit();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
	deleteDBClientHatoholDB();
}

void cut_teardown(void)
{
	g_redmineEmulator.reset();
}

void test_instanceIsSingleton(void)
{
	IssueSenderManager &manager1 = IssueSenderManager::getInstance();
	IssueSenderManager &manager2 = IssueSenderManager::getInstance();
	cppcut_assert_equal(&manager1, &manager2);
}

void test_sendRedmineIssue(void)
{
	setupTestDBConfig(true, true);
	IssueTrackerIdType trackerId = 3;
	IssueTrackerInfo &tracker = testIssueTrackerInfo[trackerId - 1];
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	TestIssueSenderManager manager;
	manager.queue(trackerId, testEventInfo[0]);
	while (!manager.isIdling())
		usleep(100 * 1000);
	const string &json = g_redmineEmulator.getLastResponse();
	cppcut_assert_equal(false, json.empty());
}

} // namespace testIssueSenderManager
