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

#include "StringUtils.h"
#include "RedmineAPI.h"

using namespace std;
using namespace mlpl;

namespace RedmineAPI
{

bool parseIssue(JSONParserAgent &agent, IncidentInfo &incidentInfo)
{
	int64_t issueId = 0;
	if (!agent.read("id", issueId) || issueId == 0) {
		MLPL_ERR("Failed to parse Incident ID.\n");
		return false;
	}
	incidentInfo.identifier = StringUtils::toString((uint64_t)issueId);

	agent.startObject("status");
	agent.read("name", incidentInfo.status);
	agent.endObject();

	if (agent.isMember("assigned_to")) {
		agent.startObject("assigned_to");
		agent.read("name", incidentInfo.assignee);
		agent.endObject();
	}

	if (!parseDateTime(agent, "created_on", incidentInfo.createdAt))
		return false;
	if (!parseDateTime(agent, "updated_on", incidentInfo.updatedAt))
		return false;

	return true;
}

bool parseDateTime(JSONParserAgent &agent, const string &objectName,
		   mlpl::Time &time)
{
	string timeString;
	GTimeVal _time;

	if (!agent.read(objectName, timeString))
		return false;
	
	if (g_time_val_from_iso8601(timeString.c_str(), &_time))
		time.tv_sec = _time.tv_sec;
	else
		time.tv_sec = 0;
	time.tv_nsec = 0;

	return true;	
}

}
