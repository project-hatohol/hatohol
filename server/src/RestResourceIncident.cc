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

const char *RestResourceIncident::pathForIncident         = "/incident";
const char *RestResourceIncident::pathForIncidentComment  = "/incident-comment";

void RestResourceIncident::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForIncident,
	  new RestResourceIncidentFactory(
	    faceRest, &RestResourceIncident::handlerIncident));
	faceRest->addResourceHandlerFactory(
	  pathForIncidentComment,
	  new RestResourceIncidentFactory(
	    faceRest, &RestResourceIncident::handlerIncidentComment));
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
	if (httpMethodIs("GET")) {
		handlerGetIncident();
	} else if (httpMethodIs("POST")) {
		handlerPostIncident();
	} else if (httpMethodIs("PUT")) {
		handlerPutIncident();
	} else {
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void RestResourceIncident::handlerGetIncident(void)
{
	uint64_t unifiedEventId = getResourceId();
	if (unifiedEventId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "unifiedEventId: %s",
			    getResourceIdString().c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	IncidentsQueryOption option(m_dataQueryContextPtr);
	option.setTargetUnifiedEventId(unifiedEventId);
	IncidentInfoVect incidentInfoVect;
	HatoholError err = dataStore->getIncidents(incidentInfoVect, option);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}
	if (incidentInfoVect.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "unifiedEventId: %s",
			    getResourceIdString().c_str());
		return;
	}

	int nest = 1;
	string resourceName = getResourceName(nest);
	if (resourceName.empty()) {
		REPLY_ERROR(this, HTERR_NOT_IMPLEMENTED,
			    "Getting an incident isn't implemented yet!");
		return;
	} else if (resourceName == "history") {
		handlerGetIncidentHistory(unifiedEventId);
	} else {
		replyHttpStatus(SOUP_STATUS_NOT_FOUND);
		return;
	}
}

void getUserNameTable(const DataQueryContextPtr &dataQueryContextPtr,
		      map<UserIdType, string> &userNameTable)
{
	UserInfoList userList;
	UserQueryOption option(dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getUserList(userList, option);
	userNameTable.clear();
	userNameTable[USER_ID_SYSTEM] = "System";
	for (auto &userInfo: userList) {
		userNameTable[userInfo.id] = userInfo.name;
	}
}

void RestResourceIncident::handlerGetIncidentHistory(
  const UnifiedEventIdType unifiedEventId)
{
	list<IncidentHistory> incidentHistoryList;
	IncidentHistoriesQueryOption option(m_dataQueryContextPtr);
	option.setTargetUnifiedEventId(unifiedEventId);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getIncidentHistories(incidentHistoryList, option);

	map<UserIdType, string> userNameTable;
	getUserNameTable(m_dataQueryContextPtr, userNameTable);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HatoholError(HTERR_OK));
	agent.startArray("incidentHistory");
	for (auto &history : incidentHistoryList) {
		string userName;
		if (userNameTable.find(history.userId) != userNameTable.end())
			userName = userNameTable[history.userId];

		agent.startObject();
		agent.add("id",             history.id);
		agent.add("unifiedEventId", history.unifiedEventId);
		agent.add("userId",         history.userId);
		agent.add("userName",       userName);
		agent.add("status",         history.status);
		agent.add("time",           history.createdAt.tv_sec);
		if (!history.comment.empty())
			agent.add("comment", history.comment);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();

	replyJSONData(agent);
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

void RestResourceIncident::handlerIncidentComment(void)
{
	if (httpMethodIs("PUT")) {
		handlerPutIncidentComment();
	} else if (httpMethodIs("DELETE")) {
		handlerDeleteIncidentComment();
	} else {
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

bool RestResourceIncident::getRequestedIncidentHistory(
  IncidentHistory &history)
{
	uint64_t id = getResourceId();
	if (id == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return false;
	}

	// get the existing record
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	list<IncidentHistory> incidentHistoryList;
	IncidentHistoriesQueryOption option(m_dataQueryContextPtr);
	option.setTargetId(id);
	dataStore->getIncidentHistories(incidentHistoryList, option);
	if (incidentHistoryList.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_UNIFIED_EVENT_ID, id);
		return false;
	}

	history = *incidentHistoryList.begin();

	if (history.userId != m_userId) {
		replyHttpStatus(SOUP_STATUS_FORBIDDEN);
		return false;
	}

	return true;
}

static HatoholError parseIncidentCommentParameter(
  GHashTable *query, string &comment)
{
	string key = "comment";
	const char *value = static_cast<const char *>(
	  g_hash_table_lookup(query, key.c_str()));
	if (!value)
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, key);
	comment = value;

	return HTERR_OK;
}

void RestResourceIncident::updateIncidentHistory(
  IncidentHistory &history)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->updateIncidentHistory(history);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", history.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceIncident::handlerPutIncidentComment(void)
{
	IncidentHistory history;
	bool succeeded = getRequestedIncidentHistory(history);
	if (!succeeded)
		return;

	string comment;
	HatoholError err = parseIncidentCommentParameter(m_query, comment);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}
	history.comment = comment;
	updateIncidentHistory(history);
}

void RestResourceIncident::handlerDeleteIncidentComment(void)
{
	IncidentHistory history;
	bool succeeded = getRequestedIncidentHistory(history);
	if (!succeeded)
		return;

	history.comment = "";
	updateIncidentHistory(history);
}
