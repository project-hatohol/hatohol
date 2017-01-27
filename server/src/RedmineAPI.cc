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

IncidentInfo::Status statusId2IncidentStatus(const int &statusId)
{
	// TODO:
	// Statues of Redmine are customizable so following statuses may not
	// correct. But I don't have good idea how to convert them correctly...
	// Should we detect only "opened" and "closed"? We can known it from
	// http://{redmine}/issue_statuses.json.

	switch(statusId) {
	case 1:
		return IncidentInfo::STATUS_OPENED;
	case 2:
		return IncidentInfo::STATUS_ASSIGNED;
	case 3:
		return IncidentInfo::STATUS_RESOLVED;
	case 4:
		return IncidentInfo::STATUS_REOPENED;
	case 5:
		return IncidentInfo::STATUS_CLOSED;
	case 6:
		return IncidentInfo::STATUS_CLOSED;
	default:
		return IncidentInfo::STATUS_UNKNOWN;
	}
}

int incidentStatus2StatusId(const IncidentInfo::Status &status)
{
	// TODO: same with statusId2IncidentStatus()

	switch(status) {
	case IncidentInfo::STATUS_OPENED:
		return 1;
	case IncidentInfo::STATUS_ASSIGNED:
		return 2;
	case IncidentInfo::STATUS_RESOLVED:
		return 3;
	case IncidentInfo::STATUS_REOPENED:
		return 4;
	case IncidentInfo::STATUS_CLOSED:
		return 5;
	default:
		return 0;
	}
}

bool parseIssue(JSONParser &agent, IncidentInfo &incidentInfo)
{
	int64_t issueId = 0;
	if (!agent.read("id", issueId) || issueId == 0) {
		MLPL_ERR("Failed to parse Incident ID.\n");
		return false;
	}
	incidentInfo.identifier = to_string((uint64_t)issueId);

	agent.startObject("status");
	agent.read("name", incidentInfo.status);
	int64_t statusId = 0;
	agent.read("id", statusId);
	incidentInfo.statusCode = statusId2IncidentStatus(statusId);
	agent.endObject();

	if (agent.isMember("assigned_to")) {
		agent.startObject("assigned_to");
		agent.read("name", incidentInfo.assignee);
		agent.endObject();
	}

	if (agent.isMember("priority")) {
		agent.startObject("priority");
		agent.read("name", incidentInfo.priority);
		agent.endObject();
	}

	int64_t doneRatio = 0;
	agent.read("done_ratio", doneRatio);
	incidentInfo.doneRatio = doneRatio;

	// TODO
	incidentInfo.commentCount = 0;

	if (!parseDateTime(agent, "created_on", incidentInfo.createdAt))
		return false;
	if (!parseDateTime(agent, "updated_on", incidentInfo.updatedAt))
		return false;

	return true;
}

bool parseDateTime(JSONParser &agent, const string &objectName,
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

void logErrors(JSONParser &agent)
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
