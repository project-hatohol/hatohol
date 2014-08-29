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

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo),
	  m_session(NULL)
	{
		m_session = soup_session_sync_new_with_options(
			SOUP_SESSION_TIMEOUT, DEFAULT_TIMEOUT_SECONDS, NULL);
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

std::string ArmRedmine::getQuery(void)
{
	// TODO: implement
	return string();
}

std::string ArmRedmine::getURL(void)
{
	string url = m_impl->m_incidentTrackerInfo.baseURL;
	string query = getQuery();
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "issues.json";
	if (!query.empty())
		url += "?" + query;
	return url;
}

gpointer ArmRedmine::mainThread(HatoholThreadArg *arg)
{
	return ArmBase::mainThread(arg);
}

bool ArmRedmine::mainThreadOneProc(void)
{
	string url = getURL();
	SoupMessage *msg = soup_message_new(SOUP_METHOD_GET, url.c_str());
	soup_message_headers_set_content_type(msg->request_headers,
	                                      MIME_JSON, NULL);
	guint sendResult = soup_session_send_message(m_impl->m_session, msg);
	g_object_unref(msg);

	return true;
}
