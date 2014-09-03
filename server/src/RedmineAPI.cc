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

string getIssueURL(const IncidentTrackerInfo &trackerInfo,
		   const string &id)
{
	string url = trackerInfo.baseURL;
	if (!StringUtils::hasSuffix(url, "/"))
		url += "/";
	url += "issues/";
	url += id;
	return url;
}


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

	// Redmine returns UTC
	if (g_time_val_from_iso8601(timeString.c_str(), &_time))
		time.tv_sec = _time.tv_sec;
	else
		time.tv_sec = 0;
	time.tv_nsec = 0;

	return true;	
}

void logErrors(JSONParserAgent &agent)
{
	if (!agent.startObject("errors"))
		MLPL_ERR("Failed to parse errors\n");

	int numErrors = agent.countElements();
	if (numErrors <= 0) {
		MLPL_ERR("Empty error message\n");
		return;
	}

	string message = "Redmine errors:\n";
	for (int i = 0; i < numErrors; i++) {
		string error;
		agent.read(i, error);
		message += StringUtils::sprintf("    * %s\n", error.c_str());
	}
	MLPL_ERR("%s", message.c_str());
	agent.endObject();
}

}
