/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "IncidentSender.h"

class IncidentSenderRedmine : public IncidentSender
{
public:
	IncidentSenderRedmine(const IncidentTrackerInfo &tracker,
			      bool shouldRecordIncidentHistory = false);
	virtual ~IncidentSenderRedmine();

	virtual HatoholError send(const EventInfo &event,
				  IncidentInfo *incident = NULL) override;

	virtual HatoholError send(const IncidentInfo &incident,
				  const std::string &comment) override;

protected:
	std::string buildJSON(const EventInfo &event);
	std::string buildJSON(const IncidentInfo &incident,
			      const std::string &comment);
	std::string getProjectURL(void);
	std::string buildDescription(const EventInfo &event,
				     const MonitoringServerInfo *server) override;
	std::string getIssuesJSONURL(void);
	std::string getIssueURL(const std::string &id);
	HatoholError parseResponse(IncidentInfo &incidentInfo,
				   const std::string &response);
	HatoholError buildIncidentInfo(IncidentInfo &incidentInfo,
				       const std::string &response,
				       const EventInfo &event);

	// TODO: Move to MonitoringServer?
	static std::string buildURLMonitoringServerEvent(
	  const EventInfo &event,
	  const MonitoringServerInfo *server);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

