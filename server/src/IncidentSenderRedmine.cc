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

#include "IncidentSenderRedmine.h"
#include "JSONBuilderAgent.h"
#include "JSONParserAgent.h"
#include <Mutex.h>
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";
static Mutex soupSessionMutex;

static SoupSession *getSoupSession(void)
{
	static SoupSession *session = NULL;
	soupSessionMutex.lock();
	if (!session) {
		session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
	}
	soupSessionMutex.unlock();
	return session;
}

struct IncidentSenderRedmine::PrivateContext
{
	PrivateContext(IncidentSenderRedmine &sender)
	: m_sender(sender), m_session(getSoupSession())
	{
		connectSessionSignals();
	}
	virtual ~PrivateContext()
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
				     const string &response);

	IncidentSenderRedmine &m_sender;
	SoupSession *m_session;
};

IncidentSenderRedmine::IncidentSenderRedmine(const IncidentTrackerInfo &tracker)
: IncidentSender(tracker), m_ctx(NULL)
{
	m_ctx = new PrivateContext(*this);
}

IncidentSenderRedmine::~IncidentSenderRedmine()
{
	delete m_ctx;
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
	string url = getProjectURL();
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "issues.json";
	return url;
}

string IncidentSenderRedmine::getIssuesURL(const string &id)
{
	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	string url = trackerInfo.baseURL;
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "issues/";
	url += id;
	return url;
}

string IncidentSenderRedmine::buildJSON(const EventInfo &event)
{
	MonitoringServerInfo serverInfo;
	MonitoringServerInfo *server = NULL;
	if (getServerInfo(event, serverInfo))
		server = &serverInfo;

	const IncidentTrackerInfo &trackerInfo = getIncidentTrackerInfo();
	JSONBuilderAgent agent;
	agent.startObject();
	agent.startObject("issue");
	agent.add("subject", buildTitle(event, server));
	if (!trackerInfo.trackerId.empty())
		agent.add("tracker_id", trackerInfo.trackerId);
	string description = "<pre>";
	description += buildDescription(event, server);
	description += "</pre>";
	agent.add("description", description);
	agent.endObject();
	agent.endObject();
	return agent.generate();
}

void IncidentSenderRedmine::PrivateContext::authenticateCallback(
  SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying,
  gpointer user_data)
{
	IncidentSenderRedmine *sender
	  = reinterpret_cast<IncidentSenderRedmine*>(user_data);
	const IncidentTrackerInfo &tracker = sender->getIncidentTrackerInfo();
	soup_auth_authenticate(
	  auth, tracker.userName.c_str(), tracker.password.c_str());
}

void IncidentSenderRedmine::PrivateContext::connectSessionSignals(void)
{
	g_signal_connect(m_session, "authenticate",
			 G_CALLBACK(authenticateCallback), &this->m_sender);
}

void IncidentSenderRedmine::PrivateContext::disconnectSessionSignals(void)
{
	g_signal_handlers_disconnect_by_func(
	  m_session,
	  reinterpret_cast<gpointer>(authenticateCallback),
	  &this->m_sender);
}

HatoholError IncidentSenderRedmine::parseResponse(
  IncidentInfo &incidentInfo, const string &response)
{
	JSONParserAgent agent(response);
	if (agent.hasError()) {
		MLPL_ERR("Failed to parse response.\n");
		return HTERR_FAILED_TO_SEND_INCIDENT;
	}

	if (!agent.isMember("issue"))
		return m_ctx->parseErrorResponse(response);

	agent.startObject("issue");
	int64_t issueId = 0;
	if (!agent.read("id", issueId) || issueId == 0) {
		MLPL_ERR("Failed to parse Incident ID.\n");
		return HTERR_FAILED_TO_SEND_INCIDENT;
	}
	incidentInfo.identifier = StringUtils::toString((uint64_t)issueId);
	incidentInfo.location = getIssuesURL(incidentInfo.identifier);

	agent.startObject("status");
	agent.read("name", incidentInfo.status);
	agent.endObject();

	if (agent.isMember("assigned_to")) {
		agent.startObject("assigned_to");
		agent.read("name", incidentInfo.assignee);
		agent.endObject();
	}

	string timeString;
	GTimeVal _time;

	agent.read("created_on", timeString);
	if (g_time_val_from_iso8601(timeString.c_str(), &_time))
		incidentInfo.createdAt.tv_sec = _time.tv_sec;
	else
		incidentInfo.createdAt.tv_sec = 0;
	incidentInfo.createdAt.tv_nsec = 0;

	agent.read("updated_on", timeString);
	if (g_time_val_from_iso8601(timeString.c_str(), &_time))
		incidentInfo.updatedAt.tv_sec = _time.tv_sec;
	else
		incidentInfo.updatedAt.tv_sec = 0;
	incidentInfo.updatedAt.tv_nsec = 0;

	agent.endObject();

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

HatoholError IncidentSenderRedmine::PrivateContext::parseErrorResponse(
  const string &response)
{
	JSONParserAgent agent(response);
	if (!agent.startObject("errors")) {
		MLPL_ERR("Failed to parse errors.\n");
		return HTERR_FAILED_TO_SEND_INCIDENT;
	}

	int numErrors = agent.countElements();
	if (numErrors <= 0)
		return HTERR_FAILED_TO_SEND_INCIDENT;

	string message = "Redmine errors:\n";
	for (int i = 0; i < numErrors; i++) {
		string error;
		agent.read(i, error);
		message += StringUtils::sprintf("    * %s\n", error.c_str());
	}
	MLPL_ERR("%s", message.c_str());

	return HTERR_FAILED_TO_SEND_INCIDENT;
}

HatoholError IncidentSenderRedmine::PrivateContext::handleSendError(
  int soupStatus, const string &response)
{
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

HatoholError IncidentSenderRedmine::send(const EventInfo &event)
{
	string url = getIssuesJSONURL();
	string json = buildJSON(event);
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, url.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         json.c_str(), json.size());
	guint sendResult = soup_session_send_message(m_ctx->m_session, msg);
	string response(msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL(sendResult))
		return m_ctx->handleSendError(sendResult, response);

	IncidentInfo incidentInfo;
	HatoholError result = buildIncidentInfo(incidentInfo, response, event);
	if (result == HTERR_OK) {
		DBClientHatohol dbHatohol;
		dbHatohol.addIncidentInfo(&incidentInfo);
	}

	return result;
}
