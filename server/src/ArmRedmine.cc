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

#include "ArmRedmine.h"
#include "RedmineAPI.h"
#include "JSONParserAgent.h"
#include <time.h>
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

// TODO: should share with other classes such as IncidentSenderRedmine
static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";

struct ArmRedmine::Impl
{
	IncidentTrackerInfo m_incidentTrackerInfo;
	SoupSession *m_session;
	string m_url;
	GHashTable *m_query;
	time_t lastUpdateTime;

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo),
	  m_session(NULL),
	  m_query(NULL),
	  lastUpdateTime(0)
	{
		// TODO: init lastUpdateTime (find it from incidents table)

		m_session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
		connectSessionSignals();

		m_query = g_hash_table_new_full(soup_str_case_hash,
						soup_str_case_equal,
						g_free, g_free);
		buildURL();
		buildQuery();
	}

	virtual ~Impl()
	{
		g_object_unref(m_session);
		g_hash_table_unref(m_query);
	}

	void connectSessionSignals(void)
	{
		g_signal_connect(m_session, "authenticate",
				 G_CALLBACK(authenticateCallback),
				 this);
	}

	static void authenticateCallback(SoupSession *session,
					 SoupMessage *msg, SoupAuth *auth,
					 gboolean retrying,
					 gpointer user_data)
	{
		ArmRedmine::Impl *impl
			= reinterpret_cast<ArmRedmine::Impl*>(user_data);
		const IncidentTrackerInfo &tracker
			= impl->m_incidentTrackerInfo;
		soup_auth_authenticate(auth,
				       tracker.userName.c_str(),
				       tracker.password.c_str());
	}

	void buildURL(void)
	{
		m_url = m_incidentTrackerInfo.baseURL;
		if (!StringUtils::hasSuffix(m_url, "/"))
			m_url += "/";
		m_url += "issues.json";
	}

	void addQuery(const char *key, const char *value)
	{
		g_hash_table_insert(m_query, g_strdup(key), g_strdup(value));
	}

	string getLastUpdateDate()
	{
		struct tm localTime;
		localtime_r(&lastUpdateTime, &localTime);
		char buf[16];
		strftime(buf, sizeof(buf), "%Y-%m-%d", &localTime);
		return string(buf);
	}

	void buildQuery(void)
	{
		g_hash_table_remove_all(m_query);

		string &projectId = m_incidentTrackerInfo.projectId;
		string &trackerId = m_incidentTrackerInfo.trackerId;
		addQuery("project_id", projectId.c_str());
		if (!trackerId.empty())
			addQuery("tracker_id", trackerId.c_str());
		addQuery("limit", "100"); // 100 is max
		addQuery("sort", "updated_on:desc");

		// Filter by status_id
		addQuery("f[]", "status_id");
		addQuery("op[status_id]", "*"); // all

		// Filter by created_on
		if (lastUpdateTime > 0) {
			addQuery("f[]", "updated_on");
			addQuery("op[updated_on]", ">=");
			// Redmine doesn't accept time string, use date instead
			addQuery("v[updated_on][]",
				 getLastUpdateDate().c_str());
		}
	}

	void handleError(int soupStatus, const string &response)
	{
		if (SOUP_STATUS_IS_TRANSPORT_ERROR(soupStatus)) {
			MLPL_ERR("Transport error: %d %s\n",
				 soupStatus,
				 soup_status_get_phrase(soupStatus));
		} else {
			MLPL_ERR("The server returns an error: %d %s\n",
				 soupStatus,
				 soup_status_get_phrase(soupStatus));
		}
	}

	bool parseResponse(const string &response)
	{
		JSONParserAgent agent(response);
		if (agent.hasError()) {
			MLPL_ERR("Failed to parse response.\n");
			return false;
		}

		bool succeeded = agent.startObject("issues");
		if (!succeeded) {
			MLPL_ERR("Failed to parse issues.\n");
			return false;
		}
		size_t num = agent.countElements();
		for (size_t i = 0; i < num; i++) {
			agent.startElement(i);
			IncidentInfo incident;
			succeeded = succeeded && parseIssue(agent, incident);
			//TODO: check outdated incidents in DB
			agent.endObject();
		}
		agent.endObject();

		return succeeded;
	}

	bool parseIssue(JSONParserAgent &agent,	IncidentInfo &incident)
	{
		bool succeeded = RedmineAPI::parseIssue(agent, incident);
		incident.trackerId = m_incidentTrackerInfo.id;
		incident.location
			= RedmineAPI::getIssueURL(m_incidentTrackerInfo,
						  incident.identifier);
		if (incident.updatedAt.tv_sec > lastUpdateTime)
			lastUpdateTime = incident.updatedAt.tv_sec;
		return succeeded;
	}
};

ArmRedmine::ArmRedmine(const IncidentTrackerInfo &trackerInfo)
: ArmBase("ArmRedmine", MonitoringServerInfo()), // TODO: fill server info
  m_impl(new Impl(trackerInfo))
{
}

ArmRedmine::~ArmRedmine()
{
}

std::string ArmRedmine::getURL(void)
{
	return m_impl->m_url;
}

std::string ArmRedmine::getQuery(void)
{
	char *query = soup_form_encode_hash(m_impl->m_query);
	string retval = query;
	g_free(query);
	return retval;
}

gpointer ArmRedmine::mainThread(HatoholThreadArg *arg)
{
	return ArmBase::mainThread(arg);
}

ArmBase::ArmPollingResult ArmRedmine::mainThreadOneProc(void)
{
	// TODO: update lastUpdateTime
	if (m_impl->lastUpdateTime == 0) {
		// There is no incident to update.
		return COLLECT_OK;
	}

	SoupMessage *msg = soup_form_request_new_from_hash(
		SOUP_METHOD_GET, m_impl->m_url.c_str(), m_impl->m_query);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	guint soupStatus = soup_session_send_message(m_impl->m_session, msg);
	string response(msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL(soupStatus)) {
		m_impl->handleError(soupStatus, response);
		//TODO: Is a server error also "disconnect"?
		return COLLECT_NG_DISCONNECT_REDMINE;
	}

	if (m_impl->parseResponse(response))
		return COLLECT_OK;
	else
		return COLLECT_NG_PERSER_ERROR;
}
