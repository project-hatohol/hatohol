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
#include "UnifiedDataStore.h"
#include <time.h>
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

// TODO: should share with other classes such as IncidentSenderRedmine
static const guint DEFAULT_TIMEOUT_SECONDS = 60;
static const char *MIME_JSON = "application/json";

typedef enum {
	PARSE_RESULT_OK,
	PARSE_RESULT_ERROR,
	PARSE_RESULT_NEED_NEXT_PAGE,
} ParseResult;

struct ArmRedmine::Impl
{
	IncidentTrackerInfo m_incidentTrackerInfo;
	SoupSession *m_session;
	string m_url;
	GHashTable *m_query;
	int m_page;
	time_t m_lastUpdateTime;
	time_t m_lastUpdateTimePending;

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo),
	  m_session(NULL),
	  m_query(NULL),
	  m_page(0),
	  m_lastUpdateTime(0),
	  m_lastUpdateTimePending(0)
	{
		IncidentTrackerIdType trackerId = m_incidentTrackerInfo.id;
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		m_lastUpdateTime
			= dataStore->getLastUpdateTimeOfIncidents(trackerId);

		m_session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
		connectSessionSignals();

		m_query = g_hash_table_new_full(soup_str_case_hash,
						soup_str_case_equal,
						g_free, g_free);
		buildURL();
		buildBaseQuery();
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

	void setQuery(const char *key, const char *value)
	{
		g_hash_table_insert(m_query, g_strdup(key), g_strdup(value));
	}

	void removeQuery(const char *key)
	{
		g_hash_table_remove(m_query, key);
	}

	void setPage(const int &page)
	{
		m_page = page;
		if (page <= 0) {
			removeQuery("page");
		} else {
			string pageString = StringUtils::toString(m_page);
			g_hash_table_insert(m_query, g_strdup("page"),
					    g_strdup(pageString.c_str()));
		}
	}

	void buildBaseQuery(void)
	{
		g_hash_table_remove_all(m_query);

		string &projectId = m_incidentTrackerInfo.projectId;
		string &trackerId = m_incidentTrackerInfo.trackerId;
		setQuery("project_id", projectId.c_str());
		if (!trackerId.empty())
			setQuery("tracker_id", trackerId.c_str());
		setQuery("limit", "100"); // 100 is max
		setQuery("sort", "updated_on:desc");

		// Filter by status_id
		setQuery("f[]", "status_id");
		setQuery("op[status_id]", "*"); // all
	}

	void updateQuery(void)
	{
		// Filter by created_on
		if (m_lastUpdateTime > 0) {
			setQuery("f[]", "updated_on");
			setQuery("op[updated_on]", ">=");
			// Redmine doesn't accept time string, use date instead
			setQuery("v[updated_on][]",
				 getLastUpdateDate().c_str());
		}
	}

	string getLastUpdateDate()
	{
		struct tm localTime;
		localtime_r(&m_lastUpdateTime, &localTime);
		char buf[16];
		strftime(buf, sizeof(buf), "%Y-%m-%d", &localTime);
		return string(buf);
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

	ParseResult parseResponse(const string &response)
	{
		JSONParserAgent agent(response);
		if (agent.hasError()) {
			MLPL_ERR("Failed to parse response.\n");
			return PARSE_RESULT_ERROR;
		}
		if (agent.isMember("errors")) {
			RedmineAPI::logErrors(agent);
			return PARSE_RESULT_ERROR;
		}

		bool succeeded = agent.startObject("issues");
		if (!succeeded) {
			MLPL_ERR("Failed to parse issues.\n");
			return PARSE_RESULT_ERROR;
		}
		size_t i, num = agent.countElements();
		if (m_lastUpdateTime > m_lastUpdateTimePending)
			m_lastUpdateTimePending = m_lastUpdateTime;
		for (i = 0; i < num; i++) {
			agent.startElement(i);
			IncidentInfo incident;
			succeeded = succeeded && parseIssue(agent, incident);
			agent.endObject();

			time_t updateTime = incident.updatedAt.tv_sec;
			if (updateTime > m_lastUpdateTime)
				m_lastUpdateTime = updateTime;
			if (updateTime >= m_lastUpdateTimePending) {
				//TODO: update the incident in DB
			} else {
				// All incidents are updated
				break;
			}
		}
		agent.endObject();

		if (num != 0 && i == num)
			return PARSE_RESULT_NEED_NEXT_PAGE;

		m_lastUpdateTime = m_lastUpdateTimePending;
		return PARSE_RESULT_OK;
	}

	bool parseIssue(JSONParserAgent &agent,	IncidentInfo &incident)
	{
		bool succeeded = RedmineAPI::parseIssue(agent, incident);
		incident.trackerId = m_incidentTrackerInfo.id;
		incident.location
			= RedmineAPI::getIssueURL(m_incidentTrackerInfo,
						  incident.identifier);
		return succeeded;
	}
};

static MonitoringServerInfo toMonitoringServerInfo(
  const IncidentTrackerInfo &trackerInfo)
{
	MonitoringServerInfo monitoringServer;
	SoupURI *uri = soup_uri_new(trackerInfo.baseURL.c_str());
	//TODO: define a new MONITORING_SYSTEM?
	monitoringServer.type = MONITORING_SYSTEM_UNKNOWN;
	monitoringServer.nickname = trackerInfo.nickname;
	monitoringServer.hostName = soup_uri_get_host(uri);
	monitoringServer.port = soup_uri_get_port(uri);
	//TODO: should be customizable
	monitoringServer.pollingIntervalSec = 60;
	monitoringServer.retryIntervalSec = 30;
	soup_uri_free(uri);
	return monitoringServer;
}

ArmRedmine::ArmRedmine(const IncidentTrackerInfo &trackerInfo)
: ArmBase("ArmRedmine", toMonitoringServerInfo(trackerInfo)),
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
	m_impl->updateQuery();
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
	if (m_impl->m_lastUpdateTime == 0) {
		// There is no incident to update.
		return COLLECT_OK;
	}

	m_impl->setPage(0);

RETRY:
	m_impl->updateQuery();
	SoupMessage *msg = soup_form_request_new_from_hash(
		SOUP_METHOD_GET, m_impl->m_url.c_str(), m_impl->m_query);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	guint soupStatus = soup_session_send_message(m_impl->m_session, msg);
	string response(msg->response_body->data, msg->response_body->length);
	g_object_unref(msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL(soupStatus)) {
		m_impl->handleError(soupStatus, response);
		return COLLECT_NG_DISCONNECT_REDMINE;
	}

	ParseResult result = m_impl->parseResponse(response);

	switch (result) {
	case PARSE_RESULT_NEED_NEXT_PAGE:
		m_impl->setPage(m_impl->m_page + 1);
		//TODO: check inifinite loop
		goto RETRY;
	case PARSE_RESULT_OK:
		return COLLECT_OK;
	case PARSE_RESULT_ERROR:
	default:
		return COLLECT_NG_PERSER_ERROR;
	}
}
