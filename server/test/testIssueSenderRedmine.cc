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
#include "LabelUtils.h"
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
	string getPostURL(void)
	{
		return IssueSenderRedmine::getPostURL();
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

	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);

	string expected =
	  StringUtils::sprintf(
	    "{\"issue\":{"
	    "\"subject\":\"[%s %s] %s\","
	    "\"description\":\""
	    "Server ID: %"FMT_SERVER_ID"\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "    IP Address: \\\"%s\\\"\\n"
	    "    Nickname:   \\\"%s\\\"\\n"
	    "Host ID: %"FMT_HOST_ID"\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "Event ID: %"FMT_EVENT_ID"\\n"
	    "    Time:       \\\"%ld.%09ld (%s)\\\"\\n"
	    "    Type:       \\\"%d (%s)\\\"\\n"
	    "    Brief:      \\\"%s\\\"\\n"
	    "Trigger ID: %"FMT_TRIGGER_ID"\\n"
	    "    Status:     \\\"%d (%s)\\\"\\n"
	    "    Severity:   \\\"%d (%s)\\\"\\n"
	    "\""
	    "}}",
	    // subject
	    server.getHostAddress().c_str(),
	    event.hostName.c_str(),
	    event.brief.c_str(),
	    // description
	    server.id,
	    server.hostName.c_str(),
	    server.ipAddress.c_str(),
	    server.nickname.c_str(),
	    event.hostId,
	    event.hostName.c_str(),
	    event.id,
	    event.time.tv_sec, event.time.tv_nsec, timeString,
	    event.type,
	    LabelUtils::getEventTypeLabel(event.type).c_str(),
	    event.brief.c_str(),
	    event.triggerId,
	    event.status,
	    LabelUtils::getTriggerStatusLabel(event.status).c_str(),
	    event.severity,
	    LabelUtils::getTriggerSeverityLabel(event.severity).c_str());
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

void test_getPostURL(void)
{
	IssueTrackerInfo &tracker = testIssueTrackerInfo[0];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/1/issues.json"),
	  sender.getPostURL());
}

void test_getPostURLWithStringProjectId(void)
{
	IssueTrackerInfo &tracker = testIssueTrackerInfo[1];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/hatohol/issues.json"),
	  sender.getPostURL());
}

void test_send(void)
{
	IssueTrackerInfo &tracker = testIssueTrackerInfo[2];
	setupTestDBConfig(true, true);
	TestRedmineSender sender(tracker);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	// TODO: not completed yet
	HatoholErrorCode expected = HTERR_NOT_IMPLEMENTED;
	HatoholError result = sender.send(testEventInfo[0]);
	cppcut_assert_equal(expected, result.getCode());
}

}
