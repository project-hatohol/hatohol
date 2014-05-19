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
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";

static SoupSession *getSoupSession(void)
{
	static SoupSession *session = NULL;
	if (session)
		return session;

	session = soup_session_sync_new_with_options(
		    SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
	return session;
}

struct IssueSenderRedmine::PrivateContext
{
	PrivateContext(IssueSenderRedmine &sender)
	: m_sender(sender)
	{
	}
	virtual ~PrivateContext()
	{
	}

	static void authenticateCallback(SoupSession *session,
					 SoupMessage *msg,
					 SoupAuth *auth,
					 gboolean retrying,
					 gpointer user_data);
	void connectSessionSignals(SoupSession *session);
	void disconnectSessionSignals(SoupSession *session);

	IssueSenderRedmine &m_sender;
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
		agent.add("trackerId", trackerInfo.trackerId);
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
	soup_auth_authenticate(auth, tracker.userName.c_str(), tracker.password.c_str());
}

void IssueSenderRedmine::PrivateContext::connectSessionSignals(
  SoupSession *session)
{
	g_signal_connect(session, "authenticate",
			 G_CALLBACK(authenticateCallback), &this->m_sender);
}

void IssueSenderRedmine::PrivateContext::disconnectSessionSignals(
  SoupSession *session)
{
	g_signal_handlers_disconnect_by_func(
	  session, reinterpret_cast<gpointer>(authenticateCallback), this);
}

HatoholError IssueSenderRedmine::send(const EventInfo &event)
{
	string url = getPostURL();

	SoupMessage *msg = soup_message_new(SOUP_METHOD_POST, url.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	string json = buildJson(event);
	soup_message_body_append(msg->request_body, SOUP_MEMORY_TEMPORARY,
	                         json.c_str(), json.size());
	SoupSession *session = getSoupSession();
	m_ctx->connectSessionSignals(session);
	guint sendResult = soup_session_send_message(session, msg);
	m_ctx->disconnectSessionSignals(session);
	g_object_unref(msg);

	return HTERR_NOT_IMPLEMENTED;
}
