/*
 * Copyright (C) 2014 Project Hatohol
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

#include "IncidentSenderRedmine.h"
#include "RedmineAPI.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "LabelUtils.h"
#include <Mutex.h>
#include <JSONBuilder.h>
#include <JSONParser.h>
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";

struct IncidentSenderRedmine::Impl
{
	Impl(IncidentSenderRedmine &sender)
	: m_sender(sender), m_session(NULL)
	{
		m_session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
		connectSessionSignals();
	}
	virtual ~Impl()
	{
		disconnectSessionSignals();
	}

	static void authenticateCallback(SoupSession *session,
					 SoupMessage *msg,
					 SoupAuth *auth,
					 gboolean retrying,
					 gpointer user_data);
	void connectSessionSignals(void);
	void disconnectSessionSignals(void);
	HatoholError parseErrorResponse(const string &response);
	HatoholError handleSendError(int soupStatus,
				     const string &url,
				     const string &response);

	HatoholError send(const string &method,
			  const string &url,
			  const string &json,
			  string &response);

	IncidentSenderRedmine &m_sender;
	SoupSession *m_session;
};

IncidentSenderRedmine::IncidentSenderRedmine(const IncidentTrackerInfo &tracker)
: IncidentSender(tracker),
  m_impl(new Impl(*this))
{
}

IncidentSenderRedmine::~IncidentSenderRedmine()
{
}

string IncidentSenderRedmine::getProjectURL(void)
{
	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	string url = trackerInfo.baseURL;
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "projects/";
	gchar *escapedProjectId =
	  g_uri_escape_string(trackerInfo.projectId.c_str(),
			      NULL, false);
	if (escapedProjectId) {
		url += escapedProjectId;
		url += "/";
	}
	g_free(escapedProjectId);
	return url;
}

string IncidentSenderRedmine::getIssuesJSONURL(void)
{
	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	string url = trackerInfo.baseURL;
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "issues.json";
	return url;
}

string IncidentSenderRedmine::getIssueURL(const string &id)
{
	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	return RedmineAPI::getIssueURL(trackerInfo, id);
}

// TODO: Move to DBTablesMonitoring or IncidentSender or Utils or ...
string IncidentSenderRedmine::buildURLMonitoringServerEvent(
  const EventInfo &event, const MonitoringServerInfo *server)
{
	if (!server)
		return string();

	// TODO: MonitoringServerInfo should have a base URL
	string url;
	switch (server->type) {
	case MONITORING_SYSTEM_ZABBIX:
	{
		url = "http://";
		bool forURI = true;
		url += server->getHostAddress(forURI);
		if (server->port > 0) {
			url += ":";
			url += StringUtils::toString(server->port);
		}
		url += StringUtils::sprintf(
		  "/zabbix/tr_events.php"
		  "?triggerid=%" FMT_TRIGGER_ID
		  "&eventid=%" FMT_EVENT_ID,
		  event.triggerId,
		  event.id);
		break;
	}
	case MONITORING_SYSTEM_NAGIOS:
		// TODO
		break;
	default:
		break;
	}
	return url;
}

string IncidentSenderRedmine::buildDescription(
  const EventInfo &event, const MonitoringServerInfo *server)
{
	string description;
	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);

	if (server) {
		description += 
		  StringUtils::sprintf(
		    "h2. Monitoring server\n"
		    "\n"
		    "|{background:#ddd}. Nickname|%s|\n"
		    "|{background:#ddd}. Hostname|%s|\n"
		    "|{background:#ddd}. IP Address|%s|\n"
		    "\n",
		    server->nickname.c_str(),
		    server->hostName.c_str(),
		    server->ipAddress.c_str());
	}

	description += 
	  StringUtils::sprintf(
	    "h2. Event details\n"
	    "\n"
	    "|{background:#ddd}. Hostname|%s|\n"
	    "|{background:#ddd}. Time|%s|\n"
	    "|{background:#ddd}. Brief|%s|\n"
	    "|{background:#ddd}. Type|%s|\n"
	    "|{background:#ddd}. Trigger Status|%s|\n"
	    "|{background:#ddd}. Trigger Severity|%s|\n"
	    "\n"
	    "{{collapse(ID)\n"
	    "\n",
	    event.hostName.c_str(),
	    timeString,
	    event.brief.c_str(),
	    LabelUtils::getEventTypeLabel(event.type).c_str(),
	    LabelUtils::getTriggerStatusLabel(event.status).c_str(),
	    LabelUtils::getTriggerSeverityLabel(event.severity).c_str());

	if (server) {
		description += 
		  StringUtils::sprintf(
		    "|{background:#ddd}. Server ID|%" FMT_SERVER_ID "|\n",
		    event.serverId);
	}

	description += 
	  StringUtils::sprintf(
	    "|{background:#ddd}. Host ID|%" FMT_HOST_ID "|\n"
	    "|{background:#ddd}. Trigger ID|%" FMT_TRIGGER_ID "|\n"
	    "|{background:#ddd}. Event ID|%" FMT_EVENT_ID "|\n"
	    "}}\n"
	    "\n",
	    event.hostId,
	    event.triggerId,
	    event.id);

	string monitoringServerEventPage
	  = buildURLMonitoringServerEvent(event, server);
	if (!monitoringServerEventPage.empty()) {
		description +=
		  StringUtils::sprintf(
		    "h2. Links\n"
		    "\n"
		    "* Monitoring server's page: %s\n",
		    monitoringServerEventPage.c_str());
	}

	// TODO: Add a link to Hatohol

	return description;
}

string IncidentSenderRedmine::buildJSON(const EventInfo &event)
{
	MonitoringServerInfo serverInfo;
	MonitoringServerInfo *server = NULL;
	if (getServerInfo(event, serverInfo))
		server = &serverInfo;

	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	JSONBuilder agent;
	agent.startObject();
	agent.startObject("issue");
	agent.add("subject", buildTitle(event, server));
	agent.add("project_id", trackerInfo.projectId);
	if (!trackerInfo.trackerId.empty())
		agent.add("tracker_id", trackerInfo.trackerId);
	agent.add("description", buildDescription(event, server));
	agent.endObject();
	agent.endObject();
	return agent.generate();
}

string IncidentSenderRedmine::buildJSON(const IncidentInfo &incident,
					const string &comment)
{
	// TODO: Sending only status_id & notes are supported in this version.

	if (incident.statusCode == IncidentInfo::STATUS_UNKNOWN &&
	    comment.empty()) {
		// nothing to update
		return string();
	}

	JSONBuilder agent;
	int statusId = RedmineAPI::incidentStatus2StatusId(incident.statusCode);
	agent.startObject();
	agent.startObject("issue");
	if (statusId > 0)
		agent.add("status_id", statusId);
	if (!comment.empty())
		agent.add("notes", comment);
	agent.endObject();
	agent.endObject();
	return agent.generate();
}

void IncidentSenderRedmine::Impl::authenticateCallback(
  SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying,
  gpointer user_data)
{
	IncidentSenderRedmine *sender
	  = reinterpret_cast<IncidentSenderRedmine*>(user_data);
	const IncidentTrackerInfo &tracker = sender->getIncidentTrackerInfo();
	soup_auth_authenticate(
	  auth, tracker.userName.c_str(), tracker.password.c_str());
}

void IncidentSenderRedmine::Impl::connectSessionSignals(void)
{
	g_signal_connect(m_session, "authenticate",
			 G_CALLBACK(authenticateCallback), &this->m_sender);
}

void IncidentSenderRedmine::Impl::disconnectSessionSignals(void)
{
	g_signal_handlers_disconnect_by_func(
	  m_session,
	  reinterpret_cast<gpointer>(authenticateCallback),
	  &this->m_sender);
}

HatoholError IncidentSenderRedmine::parseResponse(
  IncidentInfo &incidentInfo, const string &response)
{
	JSONParser agent(response);
	if (agent.hasError()) {
		MLPL_ERR("Failed to parse response.\n");
		return HTERR_FAILED_TO_SEND_INCIDENT;
	}

	if (!agent.isMember("issue"))
		return m_impl->parseErrorResponse(response);

	agent.startObject("issue");
	bool succeeded = RedmineAPI::parseIssue(agent, incidentInfo);
	incidentInfo.location = getIssueURL(incidentInfo.identifier);
	agent.endObject();

	if (!succeeded)
		return HTERR_FAILED_TO_SEND_INCIDENT;

	return HTERR_OK;
}

HatoholError IncidentSenderRedmine::buildIncidentInfo(
  IncidentInfo &incidentInfo, const string &response, const EventInfo &event)
{
	const IncidentTrackerInfo &tracker = getIncidentTrackerInfo();
	incidentInfo.trackerId = tracker.id;
	incidentInfo.serverId = event.serverId;
	incidentInfo.eventId = event.id;
	incidentInfo.triggerId = event.triggerId;
	return parseResponse(incidentInfo, response);
}

HatoholError IncidentSenderRedmine::Impl::parseErrorResponse(
  const string &response)
{
	JSONParser agent(response);
	if (agent.hasError()) {
		MLPL_ERR("Failed to parse error response: %s\n",
			 agent.getErrorMessage());
		return HTERR_FAILED_TO_SEND_INCIDENT;
	}
	if (!agent.isMember("errors"))
		MLPL_ERR("Response: %s\n", response.c_str());
	RedmineAPI::logErrors(agent);
	return HTERR_FAILED_TO_SEND_INCIDENT;
}

HatoholError IncidentSenderRedmine::Impl::handleSendError(
  int soupStatus, const string &url, const string &response)
{
	MLPL_ERR("Failed to send an issue to %s\n", url.c_str());
	if (SOUP_STATUS_IS_TRANSPORT_ERROR(soupStatus)) {
		MLPL_ERR("Transport error: %d %s\n",
			 soupStatus, soup_status_get_phrase(soupStatus));
	} else {
		MLPL_ERR("The server returns an error: %d %s\n",
			 soupStatus, soup_status_get_phrase(soupStatus));
	}

	if (soupStatus == SOUP_STATUS_UNPROCESSABLE_ENTITY)
		return parseErrorResponse(response);
	else
		return HTERR_FAILED_TO_SEND_INCIDENT;
}

HatoholError IncidentSenderRedmine::Impl::send(const string &method, const string &url,
					       const string &json, string &response)
{
	SoupMessage *msg = soup_message_new(method.c_str(), url.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         json.c_str(), json.size());
	guint sendResult = soup_session_send_message(m_session, msg);
	response.assign(msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL(sendResult))
		return handleSendError(sendResult, url, response);

	return HTERR_OK;
}

HatoholError IncidentSenderRedmine::send(const EventInfo &event)
{
	string url = getIssuesJSONURL();
	string json = buildJSON(event);
	string response;

	HatoholError result = m_impl->send(SOUP_METHOD_POST, url, json, response);
	if (result != HTERR_OK)
		return result;

	IncidentInfo incidentInfo;
	result = buildIncidentInfo(incidentInfo, response, event);
	if (result == HTERR_OK) {
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		dataStore->addIncidentInfo(incidentInfo);
	}

	return result;
}

HatoholError IncidentSenderRedmine::send(const IncidentInfo &incident,
					 const std::string &comment)
{
	string url = getIssueURL(incident.identifier) + string(".json");
	string json = buildJSON(incident, comment);
	string response;

	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	if (incident.trackerId != trackerInfo.id)
		return HTERR_FAILED_TO_SEND_INCIDENT;

	// Don't update the incident in DB here.
	// It will be done by ArmRedmine.
	return m_impl->send(SOUP_METHOD_PUT, url, json, response);
}
