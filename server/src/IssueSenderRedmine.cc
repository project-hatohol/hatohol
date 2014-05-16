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

#include "IssueSenderRedmine.h"
#include "JsonBuilderAgent.h"

using namespace std;

struct IssueSenderRedmine::PrivateContext
{
};

IssueSenderRedmine::IssueSenderRedmine(const IssueTrackerInfo &tracker)
: IssueSender(tracker)
{
	m_ctx = new PrivateContext();
}

IssueSenderRedmine::~IssueSenderRedmine()
{
	delete m_ctx;
}

string IssueSenderRedmine::buildJson(const EventInfo &event)
{
	const IssueTrackerInfo &trackerInfo = getIssueTrackerInfo();
	JsonBuilderAgent agent;
	agent.startObject("issue");
	agent.add("subject", buildTitle(event));
	if (!trackerInfo.trackerId.empty())
		agent.add("trackerId", trackerInfo.trackerId);
	agent.add("description", buildDescription(event));
	agent.endObject();
	return agent.generate();
}

HatoholError IssueSenderRedmine::send(const EventInfo &event)
{
	string json = buildJson(event);
	return HTERR_NOT_IMPLEMENTED;
}
