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

#include "IssueSender.h"
#include "StringUtils.h"
#include "LabelUtils.h"
#include "CacheServiceDBClient.h"
#include <time.h>

using namespace std;
using namespace mlpl;

struct IssueSender::PrivateContext
{
	IssueTrackerInfo issueTrackerInfo;
};

IssueSender::IssueSender(const IssueTrackerInfo &tracker)
{
	m_ctx = new PrivateContext();
	m_ctx->issueTrackerInfo = tracker;
}

IssueSender::~IssueSender()
{
	delete m_ctx;
}

const IssueTrackerInfo &IssueSender::getIssueTrackerInfo(void)
{
	return m_ctx->issueTrackerInfo;
}

bool IssueSender::getServerInfo(const EventInfo &event,
				MonitoringServerInfo &server)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	ServerQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(event.serverId);
	MonitoringServerInfoList servers;
	dbConfig->getTargetServers(servers, option);
	if (servers.empty())
		return false;
	server = *(servers.begin());
	return true;
}

static string getServerLabel(const EventInfo &event,
			     const MonitoringServerInfo *server = NULL)
{
	if (server)
		return server->getHostAddress();
	else
		return StringUtils::sprintf("Unknown:%"FMT_SERVER_ID,
					    event.serverId);
}

string IssueSender::buildTitle(const EventInfo &event,
			       const MonitoringServerInfo *server)
{
        return StringUtils::sprintf("[%s %s] %s",
				    getServerLabel(event, server).c_str(),
				    event.hostName.c_str(),
				    event.brief.c_str());
}

string IssueSender::buildDescription(const EventInfo &event,
				     const MonitoringServerInfo *server)
{
	string desc;
	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);
	desc += StringUtils::sprintf(
		  "Server ID: %"FMT_SERVER_ID"\n",
		  event.serverId);
	if (server)
		desc += StringUtils::sprintf(
			  "    Hostname:   \"%s\"\n"
			  "    IP Address: \"%s\"\n"
			  "    Nickname:   \"%s\"\n",
			  server->hostName.c_str(),
			  server->ipAddress.c_str(),
			  server->nickname.c_str());
	desc += StringUtils::sprintf(
		  "Host ID: %"FMT_HOST_ID"\n"
		  "    Hostname:   \"%s\"\n",
		  event.hostId, event.hostName.c_str());
	desc += StringUtils::sprintf(
		  "Event ID: %"FMT_EVENT_ID"\n"
		  "    Time:       \"%ld.%09ld (%s)\"\n"
		  "    Type:       \"%d (%s)\"\n"
		  "    Brief:      \"%s\"\n",
		  event.id,
		  event.time.tv_sec, event.time.tv_nsec, timeString,
		  event.type,
		  LabelUtils::getEventTypeLabel(event.type).c_str(),
		  event.brief.c_str());
	desc += StringUtils::sprintf(
		  "Trigger ID: %"FMT_TRIGGER_ID"\n"
		  "    Status:     \"%d (%s)\"\n"
		  "    Severity:   \"%d (%s)\"\n",
		  event.triggerId,
		  event.status,
		  LabelUtils::getTriggerStatusLabel(event.status).c_str(),
		  event.severity,
		  LabelUtils::getTriggerSeverityLabel(event.severity).c_str());
	return desc;
}
