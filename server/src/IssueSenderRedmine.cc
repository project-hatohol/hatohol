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

#include "IssueSenderRedmine.h"
#include "JsonBuilderAgent.h"
#include "JsonParserAgent.h"
#include "MutexLock.h"
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";

static SoupSession *getSoupSession(void)
{
	static SoupSession *session = NULL;
	static MutexLock mutex;
	mutex.lock();
	if (!session) {
		session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
	}
	mutex.unlock();
	return session;
}

struct IssueSenderRedmine::PrivateContext
{
	PrivateContext(IssueSenderRedmine &sender)
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
	HatoholError parseResponse(const string &response);

	IssueSenderRedmine &m_sender;
	SoupSession *m_session;
};

IssueSenderRedmine::IssueSenderRedmine(const IssueTrackerInfo &tracker)
: IssueSender(tracker)
{
	m_ctx = new PrivateContext(*this);
}

IssueSenderRedmine::~IssueSenderRedmine()
{
	delete m_ctx;
}

string IssueSenderRedmine::getPostURL(void)
{
	const IssueTrackerInfo &trackerInfo = getIssueTrackerInfo();
	string url = trackerInfo.baseURL;
	if (url[url.size() - 1] != '/')
		url += "/";
	url += "projects/";
	gchar *escapedProjectId =
	  g_uri_escape_string(trackerInfo.projectId.c_str(),
			      NULL, false);
	if (escapedProjectId)
		url += escapedProjectId;
	g_free(escapedProjectId);
	url += "/issues.json";
	return url;
}

string IssueSenderRedmine::buildJson(const EventInfo &event)
{
	MonitoringServerInfo serverInfo;
	MonitoringServerInfo *server = NULL;
	if (getServerInfo(event, serverInfo))
		server = &serverInfo;

	const IssueTrackerInfo &trackerInfo = getIssueTrackerInfo();
	JsonBuilderAgent agent;
	agent.startObject();
	agent.startObject("issue");
	agent.add("subject", buildTitle(event, server));
	if (!trackerInfo.trackerId.empty())
		agent.add("tracker_id", trackerInfo.trackerId);
	agent.add("description", buildDescription(event, server));
	agent.endObject();
	agent.endObject();
	return agent.generate();
}

void IssueSenderRedmine::PrivateContext::authenticateCallback(
  SoupSession *session, SoupMessage *msg, SoupAuth *auth, gboolean retrying,
  gpointer user_data)
{
	IssueSenderRedmine *sender
	  = reinterpret_cast<IssueSenderRedmine*>(user_data);
	const IssueTrackerInfo &tracker = sender->getIssueTrackerInfo();
	soup_auth_authenticate(
	  auth, tracker.userName.c_str(), tracker.password.c_str());
}

void IssueSenderRedmine::PrivateContext::connectSessionSignals(void)
{
	g_signal_connect(m_session, "authenticate",
			 G_CALLBACK(authenticateCallback), &this->m_sender);
}

void IssueSenderRedmine::PrivateContext::disconnectSessionSignals(void)
{
	g_signal_handlers_disconnect_by_func(
	  m_session,
	  reinterpret_cast<gpointer>(authenticateCallback),
	  &this->m_sender);
}

HatoholError IssueSenderRedmine::PrivateContext::parseResponse(
  const string &response)
{
	JsonParserAgent agent(response);
	if (agent.hasError()) {
		MLPL_ERR("Failed to parse response.\n");
		return HTERR_FAILED_TO_SEND_ISSUE;
	}

	if (!agent.isMember("issue")) {
		if (agent.isMember("errors")) {
			MLPL_ERR("Redmine returns errors.\n");
		} else {
			MLPL_ERR("Failed to parse issue.\n");
		}
		return HTERR_FAILED_TO_SEND_ISSUE;
	}

	agent.startObject("issue");
	int64_t issueId = 0;
	if (!agent.read("id", issueId) || issueId == 0) {
		MLPL_ERR("Failed to parse Issue ID.\n");
		return HTERR_FAILED_TO_SEND_ISSUE;
	}
	agent.endObject();

	return HTERR_OK;
}

HatoholError IssueSenderRedmine::send(const EventInfo &event)
{
	string url = getPostURL();
	string json = buildJson(event);
	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, url.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         json.c_str(), json.size());
	guint sendResult = soup_session_send_message(m_ctx->m_session, msg);
	string response(msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL(sendResult)) {
		MLPL_ERR("The server returns an error: %d\n", sendResult);
		return HTERR_FAILED_TO_SEND_ISSUE;
	}

	return m_ctx->parseResponse(response);
}
