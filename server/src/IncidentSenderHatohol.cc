/*
 * Copyright (C) 2015 Project Hatohol
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

#include "IncidentSenderHatohol.h"
#include "UnifiedDataStore.h"
#include "SmartTime.h"

using namespace std;
using namespace mlpl;

enum Status {
	NONE,
	HOLD,
	IN_PROGRESS,
	DONE,
	USER_DEFINED_BEGIN
};

struct StatusDef {
	Status status;
	string indicator;
	string label;
};

static const StatusDef definedStatuses[] = {
	{ NONE,        "",  "NONE" },
	{ HOLD,        "H", "HOLD" },
	{ IN_PROGRESS, "P", "IN PROGRESS" },
	{ DONE,        "*", "DONE" },
};

struct IncidentSenderHatohol::Impl
{
	Impl(IncidentSenderHatohol &sender)
	: m_sender(sender)
	{
	}
	virtual ~Impl()
	{
	}

	void buildNewIncidentInfo(IncidentInfo &incidentInfo,
				  const EventInfo &eventInfo)
	{
		timespec currentTime = SmartTime::getCurrTime().getAsTimespec();
		incidentInfo.trackerId = m_sender.getIncidentTrackerInfo().id;
		incidentInfo.serverId = eventInfo.serverId;
		incidentInfo.eventId = eventInfo.id;
		incidentInfo.triggerId = eventInfo.triggerId;
		incidentInfo.identifier = StringUtils::toString(eventInfo.unifiedId);
		incidentInfo.location = "";
		incidentInfo.status = definedStatuses[0].label;
		incidentInfo.priority = "";
		incidentInfo.assignee = "";
		incidentInfo.doneRatio = 0;
		incidentInfo.createdAt.tv_sec = currentTime.tv_sec;
		incidentInfo.createdAt.tv_nsec = currentTime.tv_nsec;
		incidentInfo.updatedAt = incidentInfo.createdAt;
		incidentInfo.statusCode = IncidentInfo::STATUS_OPENED;
		incidentInfo.unifiedEventId = eventInfo.unifiedId;
	}

	IncidentSenderHatohol &m_sender;
};

IncidentSenderHatohol::IncidentSenderHatohol(const IncidentTrackerInfo &tracker)
: IncidentSender(tracker),
  m_impl(new Impl(*this))
{
}

IncidentSenderHatohol::~IncidentSenderHatohol()
{
}

HatoholError IncidentSenderHatohol::send(const EventInfo &event)
{
	IncidentInfo incidentInfo;
	m_impl->buildNewIncidentInfo(incidentInfo, event);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addIncidentInfo(incidentInfo);
	return HTERR_OK;
}

HatoholError IncidentSenderHatohol::send(const IncidentInfo &incident,
					 const std::string &comment)
{
	return HTERR_NOT_IMPLEMENTED;
}
