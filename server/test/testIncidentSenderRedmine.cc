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
#include "IncidentSenderRedmine.h"
#include "LabelUtils.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "JSONParserAgent.h"
#include <cppcutter.h>
#include <gcutter.h>

using namespace std;
using namespace mlpl;

namespace testIncidentSenderRedmine {

static Synchronizer g_sync;

class TestRedmineSender : public IncidentSenderRedmine {
public:
	TestRedmineSender(const IncidentTrackerInfo &tracker)
	: IncidentSenderRedmine(tracker)
	{
	}
	virtual ~TestRedmineSender()
	{
	}
	string buildTitle(const EventInfo &event,
			  const MonitoringServerInfo *server)
	{
		return IncidentSender::buildTitle(event, server);
	}
	string buildDescription(const EventInfo &event,
				const MonitoringServerInfo *server)
	{
		return IncidentSender::buildDescription(event, server);
	}
	string buildJSON(const EventInfo &event)
	{
		return IncidentSenderRedmine::buildJSON(event);
	}
	string getIssuesJSONURL(void)
	{
		return IncidentSenderRedmine::getIssuesJSONURL();
	}
	HatoholError parseResponse(IncidentInfo &incidentInfo,const string &response)
	{
		return IncidentSenderRedmine::parseResponse(incidentInfo, response);
	}
};

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
	deleteDBClientHatoholDB();
}

void cut_teardown(void)
{
	g_sync.reset();
	g_redmineEmulator.reset();
}

string expectedJSON(const EventInfo &event)
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
	    "<pre>"
	    "Server ID: %" FMT_SERVER_ID "\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "    IP Address: \\\"%s\\\"\\n"
	    "    Nickname:   \\\"%s\\\"\\n"
	    "\\n"
	    "Host ID: %" FMT_HOST_ID "\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "\\n"
	    "Event ID: %" FMT_EVENT_ID "\\n"
	    "    Time:       \\\"%ld.%09ld (%s)\\\"\\n"
	    "    Type:       \\\"%d (%s)\\\"\\n"
	    "    Brief:      \\\"%s\\\"\\n"
	    "\\n"
	    "Trigger ID: %" FMT_TRIGGER_ID "\\n"
	    "    Status:     \\\"%d (%s)\\\"\\n"
	    "    Severity:   \\\"%d (%s)\\\"\\n"
	    "</pre>\""
	    "}}",
	    // subject
	    server.getDisplayName().c_str(),
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

void test_buildJSON(void)
{
	setupTestDBConfig(true, true);
	IncidentTrackerInfo tracker;
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(expectedJSON(testEventInfo[0]),
			    sender.buildJSON(testEventInfo[0]));
}

void test_getIssuesJSONURL(void)
{
	IncidentTrackerInfo &tracker = testIncidentTrackerInfo[0];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/1/issues.json"),
	  sender.getIssuesJSONURL());
}

void test_getIssuesJSONURLWithStringProjectId(void)
{
	IncidentTrackerInfo &tracker = testIncidentTrackerInfo[1];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/hatohol/issues.json"),
	  sender.getIssuesJSONURL());
}

static void makeExpectedIncidentInfo(IncidentInfo &incident,
				     const IncidentTrackerInfo &tracker,
				     const EventInfo &event,
				     const RedmineIssue &postedIssue)
{
	incident.serverId = event.serverId;
	incident.eventId = event.id;
	incident.triggerId = event.triggerId;
	incident.trackerId = tracker.id;
	incident.identifier = StringUtils::toString((int)postedIssue.id);
	incident.location = tracker.baseURL + "/issues/" + incident.identifier;
	incident.status = postedIssue.getStatusName();
	incident.createdAt.tv_sec = postedIssue.createdOn;
	incident.createdAt.tv_nsec = 0;
	incident.updatedAt.tv_sec = postedIssue.updatedOn;
	incident.updatedAt.tv_nsec = 0;
}

void _assertSend(const HatoholErrorCode &expected,
		 const IncidentTrackerInfo &tracker,
		 const EventInfo &event)
{
	setupTestDBConfig(true, true);
	TestRedmineSender sender(tracker);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	HatoholError result = sender.send(event);
	cppcut_assert_equal(expected, result.getCode());

	if (expected != HTERR_OK)
		return;

	// verify the reply
	MonitoringServerInfo &server = testServerInfo[event.serverId - 1];
	JSONParserAgent agent(g_redmineEmulator.getLastResponse());
	cppcut_assert_equal(true, agent.startObject("issue"));
	string expectedDescription
	  = StringUtils::sprintf("<pre>%s</pre>",
				 sender.buildDescription(event, &server).c_str());
	string title, description, trackerId;
	agent.read("subject", title);
	agent.read("description", description);
	cppcut_assert_equal(sender.buildTitle(event, &server), title);
	cppcut_assert_equal(expectedDescription, description);
	if (!tracker.trackerId.empty()) {
		agent.startObject("tracker");
		int64_t trackerId = 0;
		agent.read("id", trackerId);
		cppcut_assert_equal(tracker.trackerId,
				    StringUtils::toString((int)trackerId));
		agent.endObject();
	}

	// verify the saved information
	IncidentInfo incident;
	makeExpectedIncidentInfo(incident, tracker, event,
				 g_redmineEmulator.getLastIssue());
	DBClientHatohol dbClientHatohol;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from incidents;";
	string expect = makeIncidentOutput(incident);
	assertDBContent(dbAgent, statement, expect);
}
#define assertSend(E,T,V) \
cut_trace(_assertSend(E,T,V))

void test_send(void)
{
	assertSend(HTERR_OK, testIncidentTrackerInfo[2], testEventInfo[0]);
}

void test_sendWithUnknownTracker(void)
{
	IncidentTrackerInfo tracker = testIncidentTrackerInfo[2];
	tracker.trackerId = "100";
	assertSend(HTERR_FAILED_TO_SEND_INCIDENT, tracker, testEventInfo[0]);
}

void test_parseResponse(void)
{
	IncidentTrackerInfo tracker = testIncidentTrackerInfo[2];
	IncidentInfo expected = testIncidentInfo[0], actual;
	RedmineIssue issue;
	issue.id = atoi(expected.identifier.c_str());
	issue.trackerId = atoi(tracker.trackerId.c_str());
	issue.assigneeId = 1;
	issue.assigneeName = expected.assignee;
	issue.startDate = expected.createdAt.tv_sec;
	issue.createdOn = expected.createdAt.tv_sec;
	issue.updatedOn = expected.updatedAt.tv_sec;

	TestRedmineSender sender(tracker);
	actual.trackerId = expected.trackerId;
	actual.serverId = expected.serverId;
	actual.eventId = expected.eventId;
	actual.triggerId = expected.triggerId;
	HatoholError result = sender.parseResponse(actual, issue.toJSON());
	cppcut_assert_equal(HTERR_OK, result.getCode());
	cppcut_assert_equal(makeIncidentOutput(expected),
			    makeIncidentOutput(actual));
}

struct CallbackData
{
	size_t errorsCount;
	bool   succeeded;
	bool   failed;

	CallbackData()
	: errorsCount(0), succeeded(false), failed(false)
	{
	}
};

static void statusCallback(const EventInfo &info,
			   const IncidentSender::JobStatus &status,
			   void *userData)
{
	CallbackData *data = static_cast<CallbackData*>(userData);
	switch (status) {
	case IncidentSender::JOB_RETRYING:
		data->errorsCount++;
		break;
	case IncidentSender::JOB_SUCCEEDED:
		data->succeeded = true;
		break;
	case IncidentSender::JOB_FAILED:
		data->failed = true;
		break;
	default:
		break;
	}
}

void _assertThread(size_t numErrors, bool shouldSuccess = true)
{
	setupTestDBConfig(true, true);
	const IncidentTrackerInfo tracker = testIncidentTrackerInfo[2];
	const EventInfo &event = testEventInfo[0];
	TestRedmineSender sender(tracker);
	CallbackData cbData;
	sender.setRetryInterval(10);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	for (size_t i = 0; i < numErrors; i++)
		g_redmineEmulator.queueDummyResponse(
		  SOUP_STATUS_INTERNAL_SERVER_ERROR);

	sender.start();
	sender.queue(event, statusCallback, (void*)&cbData);
	while (!sender.isIdling())
		usleep(100 * 1000);
	sender.exitSync();

	size_t expectedErrorsCount = shouldSuccess ? numErrors : 3;
	cppcut_assert_equal(expectedErrorsCount, cbData.errorsCount);
	cppcut_assert_equal(shouldSuccess, cbData.succeeded);
	cppcut_assert_equal(!shouldSuccess, cbData.failed);

	// check the posted issue
	string expect;
	if (shouldSuccess) {
		IncidentInfo incident;
		makeExpectedIncidentInfo(incident, tracker, event,
					 g_redmineEmulator.getLastIssue());
		expect = makeIncidentOutput(incident);
	}
	DBClientHatohol dbClientHatohol;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from incidents;";
	assertDBContent(dbAgent, statement, expect);
}
#define assertThread(N, ...) \
cut_trace(_assertThread(N, ##__VA_ARGS__))

void test_thread(void)
{
	assertThread(0);
}

void test_threadWithWithInLimitErrors(void)
{
	const size_t retryLimit = 3;
	assertThread(retryLimit);
}

void test_threadWithLimitOverErrors(void)
{
	const size_t retryLimit = 3;
	bool shouldSuccessSending = true;
	assertThread(retryLimit + 1, !shouldSuccessSending);
}

}
