/*
 * Copyright (C) 2016 Project Hatohol
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

#include "RestResourceIncident.h"
#include "UnifiedDataStore.h"
#include "IncidentSenderManager.h"

using namespace std;
using namespace mlpl;

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceIncident>
  RestResourceIncidentFactory;

const char *RestResourceIncident::pathForIncident  = "/incident";

void RestResourceIncident::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForIncident,
	  new RestResourceIncidentFactory(
	    faceRest, &RestResourceIncident::handlerIncident));
}

RestResourceIncident::RestResourceIncident(FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

RestResourceIncident::~RestResourceIncident()
{
}

void RestResourceIncident::handlerIncident(void)
{
	if (httpMethodIs("POST")) {
		handlerPostIncident();
	} else if (httpMethodIs("PUT")) {
		handlerPutIncident();
	} else {
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

static HatoholError parseIncidentParameter(
  GHashTable *query, UnifiedEventIdType &eventId, IncidentTrackerIdType &trackerId)
{
	HatoholError err;
	string key;

	key = "unifiedEventId";
	err = getParam<UnifiedEventIdType>(
		query, key.c_str(), "%" FMT_UNIFIED_EVENT_ID, eventId);
	if (err != HTERR_OK)
		return err;

	key = "incidentTrackerId";
	err = getParam<IncidentTrackerIdType>(
		query, key.c_str(), "%" FMT_INCIDENT_TRACKER_ID, trackerId);
	if (err != HTERR_OK)
		return err;

	return HTERR_OK;
}

static HatoholError parseIncidentParameter(
  IncidentInfo &incidentInfo, string comment,
  GHashTable *query, const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;
	string key;
	const char *value;

	// status
	key = "status";
	value = static_cast<const char *>(
		  g_hash_table_lookup(query, key.c_str()));
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, key);
	if (value)
		incidentInfo.status = value;

	// priority
	key = "priority";
	value = static_cast<const char *>(
		  g_hash_table_lookup(query, key.c_str()));
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, key);
	if (value)
		incidentInfo.priority = value;

	// assignee
	key = "assignee";
	value = static_cast<const char *>(
		  g_hash_table_lookup(query, key.c_str()));
	if (!value && !allowEmpty)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, key);
	if (value)
		incidentInfo.assignee = value;

	// doneRatio
	key = "doneRatio";
	HatoholError err = getParam<int>(
		query, key.c_str(), "%d", incidentInfo.doneRatio);
	if (err != HTERR_OK) {
		if (err != HTERR_NOT_FOUND_PARAMETER && !allowEmpty)
			return err;
	}

	// comment
	key = "comment";
	value = static_cast<const char *>(
		  g_hash_table_lookup(query, key.c_str()));
	if (value)
		comment = value;

	/* Other parameters are frozen */

	return HTERR_OK;
}

static void sendIncidentCallback(const IncidentSender &sender,
				 const UnifiedEventIdType &eventId,
				 const IncidentSender::JobStatus &status,
				 void *userData)
{
	RestResourceIncident *job = static_cast<RestResourceIncident *>(userData);

	// make a response
	switch (status) {
	case IncidentSender::JOB_SUCCEEDED:
	{
		JSONBuilder agent;
		agent.startObject();
		job->addHatoholError(agent, HTERR_OK);
		agent.add("unifiedEventId", eventId);
		agent.endObject();
		job->replyJSONData(agent);
		job->unpauseResponse();
		job->unref();
		break;
	}
	case IncidentSender::JOB_FAILED:
		job->replyError(sender.getLastResult());
		job->unpauseResponse();
		job->unref();
		break;
	default:
		break;
	}
}

static void createIncidentCallback(const IncidentSender &sender,
				   const EventInfo &eventInfo,
				   const IncidentSender::JobStatus &status,
				   void *userData)
{
	const UnifiedEventIdType &eventId = eventInfo.unifiedId;
	sendIncidentCallback(sender, eventId, status, userData);
}

static void updateIncidentCallback(const IncidentSender &sender,
				   const IncidentInfo &incidentInfo,
				   const IncidentSender::JobStatus &status,
				   void *userData)
{
	const UnifiedEventIdType &eventId = incidentInfo.unifiedEventId;
	sendIncidentCallback(sender, eventId, status, userData);
}

void RestResourceIncident::createIncidentAsync(
  const UnifiedEventIdType &eventId, const IncidentTrackerIdType &trackerId)
{
	// Find the specified event in the DB
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventList;
	IncidentInfoVect incidentVect;
	EventsQueryOption eventsQueryOption(m_dataQueryContextPtr);
	eventsQueryOption.setLimitOfUnifiedId(eventId);
	eventsQueryOption.setMaximumNumber(1);
	eventsQueryOption.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
				      DataQueryOption::SORT_DESCENDING);
	dataStore->getEventList(eventList, eventsQueryOption, &incidentVect);
	if (eventList.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
			    "id: %" FMT_UNIFIED_EVENT_ID,
			    eventId);
		return;
	}
	if (incidentVect.size() > 0) {
		IncidentInfo &incidentInfo = *incidentVect.begin();
		if (incidentInfo.trackerId || !incidentInfo.status.empty()) {
			REPLY_ERROR(this, HTERR_RECORD_EXISTS,
				    "id: %" FMT_UNIFIED_EVENT_ID,
				    eventId);
			return;
		}
	}

	EventInfo &eventInfo = *eventList.begin();

	// Post an incident
	IncidentSenderManager &senderManager
	  = IncidentSenderManager::getInstance();
	ref(); // keep myself until the callback function is called
	HatoholError err = senderManager.queue(trackerId, eventInfo,
					       createIncidentCallback, this,
					       m_userId);
	if (err != HTERR_OK) {
		unref();
		replyError(err);
	}
}

void RestResourceIncident::handlerPostIncident(void)
{
	UnifiedEventIdType eventId;
	IncidentTrackerIdType trackerId;
	HatoholError err = parseIncidentParameter(m_query, eventId, trackerId);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	createIncidentAsync(eventId, trackerId);
}

void RestResourceIncident::handlerPutIncident(void)
{
	uint64_t unifiedEventId = getResourceId();
	if (unifiedEventId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	// check the existing record
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	IncidentInfoVect incidents;
	IncidentsQueryOption option(m_dataQueryContextPtr);
	option.setTargetUnifiedEventId(unifiedEventId);
	dataStore->getIncidents(incidents, option);
	if (incidents.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_UNIFIED_EVENT_ID,
			    unifiedEventId);
		return;
	}

	IncidentInfo incidentInfo;
	incidentInfo = *incidents.begin();
	incidentInfo.unifiedEventId = unifiedEventId;

	// check the request
	bool allowEmpty = true;
	string comment;
	HatoholError err = parseIncidentParameter(incidentInfo, comment,
						  m_query, allowEmpty);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// try to update
	ref();
	IncidentSenderManager &senderManager
	  = IncidentSenderManager::getInstance();
	err = senderManager.queue(incidentInfo, comment,
				  updateIncidentCallback, this,
				  m_userId);
	if (err != HTERR_OK) {
		unref();
		replyError(err);
	}
}
