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

string IssueSender::buildTitle(const EventInfo &event)
{
	// TODO: Add server name
        return StringUtils::sprintf("[%"FMT_SERVER_ID" %s] %s",
				    event.serverId,
				    event.hostName.c_str(),
				    event.brief.c_str());
}

string IssueSender::buildDescription(const EventInfo &event)
{
	// TODO: Set full information
        return StringUtils::sprintf("%s", event.brief.c_str());
}
