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

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo),
	  m_session(NULL),
	  m_query(NULL)
	{
		m_session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
		m_query = g_hash_table_new_full(soup_str_case_hash,
						soup_str_case_equal,
						g_free, g_free);
		buildURL();
		buildQuery();
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
		// TODO: build date string from the last updated incident
		/*
		addQuery("f[]", "updated_on");
		addQuery("op[updated_on]", ">=");
		addQuery("v[updated_on][]", "2014-08-25");
		*/
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

bool ArmRedmine::mainThreadOneProc(void)
{
	string url = getURL();
	SoupMessage *msg = soup_form_request_new_from_hash(SOUP_METHOD_GET,
							   url.c_str(),
							   m_impl->m_query);
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	guint sendResult = soup_session_send_message(m_impl->m_session, msg);
	g_object_unref(msg);

	return true;
}
