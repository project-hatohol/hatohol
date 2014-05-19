/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include "Synchronizer.h"
#include "RedmineAPIEmulator.h"
#include "Hatohol.h"
#include "IssueSenderRedmine.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include <cppcutter.h>
#include <gcutter.h>

using namespace std;
using namespace mlpl;

namespace testIssueSenderRedmine {

static const guint EMULATOR_PORT = 44444;
static Synchronizer g_sync;
RedmineAPIEmulator g_redmineEmulator;

class TestRedmineSender : public IssueSenderRedmine {
public:
	TestRedmineSender(const IssueTrackerInfo &tracker)
	: IssueSenderRedmine(tracker)
	{
	}
	virtual ~TestRedmineSender()
	{
	}
	string buildJson(const EventInfo &event)
	{
		return IssueSenderRedmine::buildJson(event);
	}
};

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
}

void cut_teardown(void)
{
	g_sync.reset();
	g_redmineEmulator.reset();
}

string expectedJson(const EventInfo event)
{
	MonitoringServerInfo &server = testServerInfo[event.serverId - 1];
	string expected =
	  StringUtils::sprintf(
	    "{\"issue\":{"
	    "\"subject\":\"[%s %s] %s\","
	    "\"description\":\"%s\""
	    "}}",
	    server.getHostAddress().c_str(),
	    event.hostName.c_str(),
	    event.brief.c_str(),
	    event.brief.c_str());
	return expected;
}

void test_buildJson(void)
{
	setupTestDBConfig(true, true);
	IssueTrackerInfo tracker;
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(expectedJson(testEventInfo[0]),
			    sender.buildJson(testEventInfo[0]));
}

}
