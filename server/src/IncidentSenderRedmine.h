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

#ifndef IncidentSenderRedmine_h
#define IncidentSenderRedmine_h

#include "IncidentSender.h"

class IncidentSenderRedmine : public IncidentSender
{
public:
	IncidentSenderRedmine(const IncidentTrackerInfo &tracker);
	virtual ~IncidentSenderRedmine();

	virtual HatoholError send(const EventInfo &event) override;

protected:
	std::string buildJSON(const EventInfo &event);
	std::string getProjectURL(void);
	std::string getIssuesJSONURL(void);
	std::string getIssuesURL(const std::string &id);
	HatoholError parseResponse(IncidentInfo &incidentInfo,
				   const std::string &response);
	HatoholError buildIncidentInfo(IncidentInfo &incidentInfo,
				       const std::string &response,
				       const EventInfo &event);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // IncidentSenderRedmine_h
