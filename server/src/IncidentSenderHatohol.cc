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

const string IncidentSenderHatohol::STATUS_NONE("NONE");
const string IncidentSenderHatohol::STATUS_HOLD("HOLD");
const string IncidentSenderHatohol::STATUS_IN_PROGRESS("IN PROGRESS");
const string IncidentSenderHatohol::STATUS_DONE("DONE");

static const string systemDefinedStatuses[] = {
	IncidentSenderHatohol::STATUS_NONE,
	IncidentSenderHatohol::STATUS_HOLD,
	IncidentSenderHatohol::STATUS_IN_PROGRESS,
	IncidentSenderHatohol::STATUS_DONE,
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

	bool isKnownStatus(const string &status)
	{
		for (auto &definedStatus: systemDefinedStatuses) {
			if (status == definedStatus)
				return true;
		}

		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		map<string, CustomIncidentStatus> customStatuses;
		dataStore->getCustomIncidentStatusesCache(customStatuses);
		return customStatuses.find(status) != customStatuses.end();
	}

	HatoholError validate(const IncidentInfo &incident)
	{
		const IncidentTrackerInfo &tracker
		  = m_sender.getIncidentTrackerInfo();
		if (incident.trackerId != tracker.id) {
			string message(StringUtils::sprintf(
			  "Unmatched tarcker ID: %" FMT_INCIDENT_TRACKER_ID,
			  incident.trackerId));
			return HatoholError(HTERR_INVALID_PARAMETER, message);
		}

		if (!isKnownStatus(incident.status)) {
			string message(StringUtils::sprintf(
			  "Unknown status: %s", incident.status.c_str()));
			return HatoholError(HTERR_INVALID_PARAMETER, message);
		}

		return HTERR_OK;
	}

	void buildNewIncidentInfo(IncidentInfo &incidentInfo,
				  const EventInfo &eventInfo)
	{
		timespec currentTime = SmartTime::getCurrTime().getAsTimespec();
		incidentInfo.trackerId = m_sender.getIncidentTrackerInfo().id;
		incidentInfo.serverId = eventInfo.serverId;
		incidentInfo.eventId = eventInfo.id;
		incidentInfo.triggerId = eventInfo.triggerId;
		incidentInfo.identifier = to_string(eventInfo.unifiedId);
		incidentInfo.location = "";
		incidentInfo.status = STATUS_NONE;
		incidentInfo.priority = "";
		incidentInfo.assignee = "";
		incidentInfo.doneRatio = 0;
		incidentInfo.commentCount = 0;
		incidentInfo.createdAt.tv_sec = currentTime.tv_sec;
		incidentInfo.createdAt.tv_nsec = currentTime.tv_nsec;
		incidentInfo.updatedAt = incidentInfo.createdAt;
		incidentInfo.statusCode = IncidentInfo::STATUS_OPENED;
		incidentInfo.unifiedEventId = eventInfo.unifiedId;
	}

	IncidentSenderHatohol &m_sender;
};

IncidentSenderHatohol::IncidentSenderHatohol(
  const IncidentTrackerInfo &tracker, bool shouldRecordIncidentHistory)
: IncidentSender(tracker, shouldRecordIncidentHistory),
  m_impl(new Impl(*this))
{
}

IncidentSenderHatohol::~IncidentSenderHatohol()
{
}

HatoholError IncidentSenderHatohol::send(const EventInfo &event,
					 IncidentInfo *incident)
{
	IncidentInfo incidentInfo;
	m_impl->buildNewIncidentInfo(incidentInfo, event);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addIncidentInfo(incidentInfo);
	if (incident)
		*incident = incidentInfo;
	return HTERR_OK;
}

HatoholError IncidentSenderHatohol::send(const IncidentInfo &incident,
					 const std::string &comment)
{
	// IncidentSenderHatohol doesn't support "comment", ignore it.
	IncidentInfo incidentInfo = incident;
	timespec currentTime = SmartTime::getCurrTime().getAsTimespec();
	incidentInfo.updatedAt.tv_sec = currentTime.tv_sec;
	incidentInfo.updatedAt.tv_nsec = currentTime.tv_nsec;
	HatoholError err = m_impl->validate(incidentInfo);
	if (err != HTERR_OK)
		return err;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	return dataStore->updateIncidentInfo(incidentInfo);
}
