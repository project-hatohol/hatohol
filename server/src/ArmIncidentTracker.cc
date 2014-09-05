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

#include "ArmIncidentTracker.h"
#include "ArmRedmine.h"
#include <libsoup/soup.h>

using namespace std;
using namespace mlpl;

static MonitoringServerInfo toMonitoringServerInfo(
  const IncidentTrackerInfo &trackerInfo)
{
	MonitoringServerInfo monitoringServer;
	SoupURI *uri = soup_uri_new(trackerInfo.baseURL.c_str());
	//TODO: define a new MONITORING_SYSTEM?
	monitoringServer.type = MONITORING_SYSTEM_UNKNOWN;
	monitoringServer.nickname = trackerInfo.nickname;
	monitoringServer.hostName = uri->host;
	monitoringServer.port = uri->port;
	//TODO: should be customizable
	monitoringServer.pollingIntervalSec = 60;
	monitoringServer.retryIntervalSec = 30;
	soup_uri_free(uri);
	return monitoringServer;
}

struct ArmIncidentTracker::Impl
{
	IncidentTrackerInfo m_incidentTrackerInfo;

	Impl(const IncidentTrackerInfo &trackerInfo)
	: m_incidentTrackerInfo(trackerInfo)
	{
	}

	virtual ~Impl()
	{
	}
};

ArmIncidentTracker::ArmIncidentTracker(const string &name,
				       const IncidentTrackerInfo &trackerInfo)
: ArmBase(name, toMonitoringServerInfo(trackerInfo)),
  m_impl(new Impl(trackerInfo))
{
}

ArmIncidentTracker::~ArmIncidentTracker()
{
}

const IncidentTrackerInfo &ArmIncidentTracker::getIncidentTrackerInfo(void)
{
	return m_impl->m_incidentTrackerInfo;
}

bool ArmIncidentTracker::isFetchItemsSupported(void) const
{
	return false;
}


// TODO: should be pluggable
ArmIncidentTracker *ArmIncidentTracker::create(
  const IncidentTrackerInfo &trackerInfo)
{
	switch (trackerInfo.type) {
	case INCIDENT_TRACKER_REDMINE:
		return new ArmRedmine(trackerInfo);
	default:
		MLPL_BUG("Invalid incident tracking system: %d\n",
			 trackerInfo.type);
	}
	return NULL;
}
