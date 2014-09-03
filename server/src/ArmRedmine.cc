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
static const int DEFAULT_PAGE_LIMIT = 100;
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
	// Don't use hash table to allow duplicated keys
	string m_baseQuery;
	int m_page;
	int m_pageLimit;
	time_t m_lastUpdateTime;
	time_t m_lastUpdateTimePending;

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo),
	  m_session(NULL),
	  m_page(1),
	  m_pageLimit(DEFAULT_PAGE_LIMIT),
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

		buildURL();
		buildBaseQuery();
	}

	virtual ~Impl()
	{
		g_object_unref(m_session);
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

	void setPage(const int &page)
	{
		m_page = page;
	}

	void buildBaseQuery(void)
	{
		m_baseQuery.clear();
		string &projectId = m_incidentTrackerInfo.projectId;
		string &trackerId = m_incidentTrackerInfo.trackerId;
		char *query = soup_form_encode("project_id", projectId.c_str(),
					       "tracker_id", trackerId.c_str(),
					       "limit", "100",
					       "sort", "updated_on:desc",
					       "f[]", "status_id",
					       "op[status_id]", "*",
					       NULL);
		m_baseQuery = query;
		g_free(query);
	}

	string getLastUpdateDate()
	{
		// TODO:
		// The timezone isn't stored in the DB (Redmine returns UTC).
		// How should we know it?
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
			MLPL_ERR("Failed to parse error response: %s\n",
				 agent.getErrorMessage());
			return PARSE_RESULT_ERROR;
		}
		if (agent.isMember("errors")) {
			RedmineAPI::logErrors(agent);
			return PARSE_RESULT_ERROR;
		}

		bool succeeded = agent.startObject("issues");
		if (!succeeded) {
			MLPL_ERR("Failed to parse issues.\n");
			MLPL_DBG("Response: %s\n", response.c_str());
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
			if (updateTime > m_lastUpdateTimePending)
				m_lastUpdateTimePending = updateTime;
			if (updateTime >= m_lastUpdateTime) {
				//TODO: update the incident in DB
			} else {
				// All incidents are updated
				break;
			}
		}
		agent.endObject();

		if (num != 0 && i == num) {
			hasNextPage(agent, num);
			return PARSE_RESULT_NEED_NEXT_PAGE;
		}

		m_lastUpdateTime = m_lastUpdateTimePending;
		return PARSE_RESULT_OK;
	}

	bool hasNextPage(JSONParserAgent &agent, const int &numIssues)
	{
		int64_t offset = 0;
		int64_t total = 0;
		agent.read("offset", offset);
		agent.read("total_count", total);
		if (offset + numIssues >= total)
			return false;
		return true;
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
	string query = m_impl->m_baseQuery;
	if (m_impl->m_lastUpdateTime > 0) {
		char *updatedOnQuery = soup_form_encode(
			"f[]", "updated_on",
			"op[updated_on]", ">=",
			"v[updated_on][]", m_impl->getLastUpdateDate().c_str(),
			NULL);
		query += "&";
		query += updatedOnQuery;
		g_free(updatedOnQuery);
	}
	if (m_impl->m_page > 1) {
		query += "&page=";
		query += StringUtils::toString(m_impl->m_page);
	}
	return query;
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

	m_impl->setPage(1);

RETRY:
	string url = m_impl->m_url;
	url += "?";
	url += getQuery();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, url.c_str());
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
		if (m_impl->m_page >= m_impl->m_pageLimit) {
			MLPL_ERR("Too many updated records "
				 "or something is wrong!\n");
			return COLLECT_NG_INTERNAL_ERROR;
		}
		m_impl->setPage(m_impl->m_page + 1);
		goto RETRY;
	case PARSE_RESULT_OK:
		return COLLECT_OK;
	case PARSE_RESULT_ERROR:
	default:
		return COLLECT_NG_PARSER_ERROR;
	}
}
