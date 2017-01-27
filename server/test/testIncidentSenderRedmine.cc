/*
 * Copyright (C) 2013-2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <Synchronizer.h>
#include <Hatohol.h>
#include <IncidentSenderRedmine.h>
#include <LabelUtils.h>
#include <JSONParser.h>
#include <ThreadLocalDBCache.h>
#include <RedmineAPI.h>
#include <cppcutter.h>
#include <gcutter.h>
#include "RedmineAPIEmulator.h"
#include "DBTablesTest.h"
#include "Helpers.h"

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
		return IncidentSenderRedmine::buildDescription(event, server);
	}
	string buildJSON(const EventInfo &event)
	{
		return IncidentSenderRedmine::buildJSON(event);
	}
	string buildJSON(const IncidentInfo &incident, const string &comment)
	{
		return IncidentSenderRedmine::buildJSON(incident, comment);
	}
	string getIssuesJSONURL(void)
	{
		return IncidentSenderRedmine::getIssuesJSONURL();
	}
	HatoholError parseResponse(IncidentInfo &incidentInfo,
				   const string &response)
	{
		return IncidentSenderRedmine::parseResponse(incidentInfo,
							    response);
	}
	static std::string callBuildURLMonitoringServerEvent(
	  const EventInfo &event,
	  const MonitoringServerInfo *server)
	{
		return buildURLMonitoringServerEvent(event, server);
	}
};

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	setupTestDB();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
}

void cut_teardown(void)
{
	g_sync.reset();
	g_redmineEmulator.reset();
}

string expectedJSON(const EventInfo &event, const IncidentTrackerInfo &tracker)
{
	const MonitoringServerInfo &server = testServerInfo[event.serverId - 1];
	string eventsURL =
	  TestRedmineSender::callBuildURLMonitoringServerEvent(event, &server);
	string linkPart;
	if (!eventsURL.empty()) {
		linkPart = StringUtils::sprintf(
		             "h2. Links\\n"
		             "\\n"
		             "* Monitoring server's page: %s\\n",
		             eventsURL.c_str());
	}

	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);

	string expected =
	  StringUtils::sprintf(
	    "{\"issue\":{"
	    "\"subject\":\"[%s %s] %s\","
	    "\"project_id\":\"%s\","
	    "\"description\":\""
	    "h2. Monitoring server\\n"
	    "\\n"
	    "|{background:#ddd}. Nickname|%s|\\n"
	    "|{background:#ddd}. Hostname|%s|\\n"
	    "|{background:#ddd}. IP Address|%s|\\n"
	    "\\n"
	    "h2. Event details\\n"
	    "\\n"
	    "|{background:#ddd}. Hostname|%s|\\n"
	    "|{background:#ddd}. Time|%s|\\n"
	    "|{background:#ddd}. Brief|%s|\\n"
	    "|{background:#ddd}. Type|%s|\\n"
	    "|{background:#ddd}. Trigger Status|%s|\\n"
	    "|{background:#ddd}. Trigger Severity|%s|\\n"
	    "\\n"
	    "{{collapse(ID)\\n"
	    "\\n"
	    "|{background:#ddd}. Server ID|%" FMT_SERVER_ID "|\\n"
	    "|{background:#ddd}. Host ID|'%" FMT_LOCAL_HOST_ID "'|\\n"
	    "|{background:#ddd}. Trigger ID|%" FMT_TRIGGER_ID "|\\n"
	    "|{background:#ddd}. Event ID|%" FMT_EVENT_ID "|\\n}}\\n"
	    "\\n"
	    "%s" // linkPart
	    "\"}}",
	    // subject
	    server.getDisplayName().c_str(),
	    event.hostName.c_str(),
	    event.brief.c_str(),
	    // projectId
	    tracker.projectId.c_str(),
	    // description
	    server.nickname.c_str(),
	    server.hostName.c_str(),
	    server.ipAddress.c_str(),

	    event.hostName.c_str(),
	    timeString,
	    event.brief.c_str(),
	    LabelUtils::getEventTypeLabel(event.type).c_str(),
	    LabelUtils::getTriggerStatusLabel(event.status).c_str(),
	    LabelUtils::getTriggerSeverityLabel(event.severity).c_str(),
	    server.id,
	    event.hostIdInServer.c_str(),
	    event.triggerId.c_str(),
	    event.id.c_str(),
	    linkPart.c_str());
	return expected;
}

string expectedJSON(const IncidentInfo &incident, const string &comment)
{
	string expected = "{\"issue\":{";
	string contents;
	if (incident.statusCode != IncidentInfo::STATUS_UNKNOWN) {
		int statusId = RedmineAPI::incidentStatus2StatusId(
			incident.statusCode);
		contents += StringUtils::sprintf("\"status_id\":%d",
						 statusId);
	}
	if (!comment.empty()) {
		if (!contents.empty())
			contents += ",";
		// TODO: It doesn't support escaping UTF-8
		char *escaped = g_strescape(comment.c_str(), "");
		contents += StringUtils::sprintf("\"notes\":\"%s\"",
						 escaped);
		g_free(escaped);
	}
	expected += contents;
	expected += "}}";
	return expected;
}

void test_buildJSON(void)
{
	loadTestDBTablesConfig();
	IncidentTrackerInfo tracker;
	tracker.projectId = "hatoholtest";
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(expectedJSON(testEventInfo[0], tracker),
			    sender.buildJSON(testEventInfo[0]));
}

void test_buildJSONForUpdate(void)
{
	IncidentTrackerInfo tracker;
	tracker.id = testIncidentInfo[0].trackerId;
	tracker.projectId = "hatoholtest";
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(expectedJSON(testIncidentInfo[0], "abc\n"),
			    sender.buildJSON(testIncidentInfo[0], "abc\n"));
}

void test_getIssuesJSONURL(void)
{
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[0];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/issues.json"),
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
	incident.identifier = to_string((int)postedIssue.id);
	incident.location = tracker.baseURL + "/issues/" + incident.identifier;
	incident.status = postedIssue.getStatusName();
	incident.assignee = "";
	incident.priority = postedIssue.getPriorityName();
	incident.doneRatio = postedIssue.doneRatio;
	incident.createdAt.tv_sec = postedIssue.createdOn;
	incident.createdAt.tv_nsec = 0;
	incident.updatedAt.tv_sec = postedIssue.updatedOn;
	incident.updatedAt.tv_nsec = 0;
	incident.unifiedEventId = event.unifiedId;
	incident.commentCount = 0;
}

void _assertSend(const HatoholErrorCode &expected,
		 const IncidentTrackerInfo &tracker,
		 const EventInfo &event)
{
	loadTestDBTablesConfig();
	TestRedmineSender sender(tracker);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);

	HatoholError result = sender.send(event);

	assertHatoholError(expected, result);
	cppcut_assert_equal(string("/issues.json"),
			    g_redmineEmulator.getLastRequestPath());
	cppcut_assert_equal(string("POST"),
			    g_redmineEmulator.getLastRequestMethod());

	if (expected != HTERR_OK)
		return;

	// verify the reply
	const MonitoringServerInfo &server = testServerInfo[event.serverId - 1];
	JSONParser agent(g_redmineEmulator.getLastResponseBody());
	cppcut_assert_equal(true, agent.startObject("issue"));
	string expectedDescription
	  = StringUtils::sprintf("%s",
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
				    to_string((int)trackerId));
		agent.endObject();
	}

	// verify the saved information
	IncidentInfo incident;
	makeExpectedIncidentInfo(incident, tracker, event,
				 g_redmineEmulator.getLastIssue());
	ThreadLocalDBCache cache;
	DBAgent &dbAgent = cache.getMonitoring().getDBAgent();
	string statement = "select * from incidents;";
	string expect = makeIncidentOutput(incident);
	assertDBContent(&dbAgent, statement, expect);
}
#define assertSend(E,T,V) cut_trace(_assertSend(E,T,V))

void test_send(void)
{
	assertSend(HTERR_OK, testIncidentTrackerInfo[2], testEventInfo[0]);
}

void _assertSendForUpdate(const HatoholErrorCode &expected,
			  const IncidentInfo &incident,
			  const string &comment)
{
	loadTestDBTablesConfig();
	int trackerIndex = incident.trackerId - 1;
	const IncidentTrackerInfo &tracker =
	  testIncidentTrackerInfo[trackerIndex];
	TestRedmineSender sender(tracker);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);

	HatoholError result = sender.send(incident, comment);

	assertHatoholError(expected, result);
	cppcut_assert_equal(string("/issues/" + incident.identifier + ".json"),
			    g_redmineEmulator.getLastRequestPath());
	cppcut_assert_equal(string("PUT"),
			    g_redmineEmulator.getLastRequestMethod());

	if (expected != HTERR_OK)
		return;

	cppcut_assert_equal(expectedJSON(incident, comment),
			    g_redmineEmulator.getLastRequestBody());
}
#define assertSendForUpdate(E,I,C) cut_trace(_assertSendForUpdate(E,I,C))

void test_sendForUpdate(void)
{
	IncidentInfo incident = testIncidentInfo[0];
	assertSendForUpdate(HTERR_OK, incident, "Updated");
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
	actual.unifiedEventId = expected.unifiedEventId;
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

static void statusCallback(const IncidentSender &sender,
			   const EventInfo &info,
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
	loadTestDBTablesConfig();
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
	ThreadLocalDBCache cache;
	DBAgent &dbAgent = cache.getMonitoring().getDBAgent();
	string statement = "select * from incidents;";
	assertDBContent(&dbAgent, statement, expect);
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
