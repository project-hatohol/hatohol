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

#include "RestResourceIncidentTracker.h"
#include "DBClientConfig.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

const char *RestResourceIncidentTracker::pathForIncidentTracker =
  "/incident-tracker";

void RestResourceIncidentTracker::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForIncidentTracker,
	  new RestResourceIncidentTrackerFactory(faceRest));
}

RestResourceIncidentTracker::RestResourceIncidentTracker(FaceRest *faceRest)
: FaceRest::ResourceHandler(faceRest, NULL)
{
}

RestResourceIncidentTracker::~RestResourceIncidentTracker()
{
}

void RestResourceIncidentTracker::handle(void)
{
	if (httpMethodIs("GET")) {
		handleGet();
	} else if (httpMethodIs("POST")) {
		handlePost();
	} else if (httpMethodIs("PUT")) {
		handlePut();
	} else if (httpMethodIs("DELETE")) {
		handleDelete();
	} else {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}

void RestResourceIncidentTracker::handleGet(void)
{
	IncidentTrackerQueryOption option(m_dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	IncidentTrackerInfoVect incidentTrackers;
	dataStore->getIncidentTrackers(incidentTrackers, option);

	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HTERR_OK);

	IncidentTrackerInfoVectIterator it = incidentTrackers.begin();
	agent.startArray("incidentTrackers");
	for (; it != incidentTrackers.end(); ++it) {
		agent.startObject();
		agent.add("id", it->id);
		agent.add("type", it->type);
		agent.add("nickname", it->nickname);
		agent.add("baseURL", it->baseURL);
		agent.add("projectId", it->projectId);
		agent.add("trackerId", it->trackerId);
		if (option.has(OPPRVLG_UPDATE_INCIDENT_SETTING))
			agent.add("userName", it->userName);
		agent.endObject();
	}
	agent.endArray();

	agent.endObject();

	replyJSONData(agent);
}

#define PARSE_VALUE(STRUCT,PROPERTY,TYPE,ALLOW_EMPTY)			      \
{									      \
	HatoholError err = getParam<TYPE>(				      \
		query, #PROPERTY, "%d", STRUCT.PROPERTY);		      \
	if (err != HTERR_OK) {						      \
		if (!ALLOW_EMPTY || err != HTERR_NOT_FOUND_PARAMETER)	      \
			return err;					      \
	}								      \
}

#define PARSE_STRING_VALUE(STRUCT,PROPERTY,ALLOW_EMPTY)			      \
{									      \
	char *value = (char *)g_hash_table_lookup(query, #PROPERTY);	      \
	if (!value && !ALLOW_EMPTY)					      \
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, #PROPERTY);    \
	if (value)							      \
		STRUCT.PROPERTY = value;				      \
}

static HatoholError parseIncidentTrackerParameter(
  IncidentTrackerInfo &incidentTrackerInfo, GHashTable *query,
  const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;

	PARSE_VALUE(incidentTrackerInfo, type, IncidentTrackerType, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, nickname, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, baseURL, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, projectId, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, userName, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, password, allowEmpty);
	PARSE_STRING_VALUE(incidentTrackerInfo, trackerId, true);

	return HatoholError(HTERR_OK);
}

void RestResourceIncidentTracker::handlePost(void)
{
	IncidentTrackerInfo incidentTrackerInfo;
	HatoholError err = parseIncidentTrackerParameter(incidentTrackerInfo,
							 m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addIncidentTracker(
	  incidentTrackerInfo, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", incidentTrackerInfo.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceIncidentTracker::handlePut(void)
{
	uint64_t incidentTrackerId = getResourceId();
	if (incidentTrackerId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	IncidentTrackerInfo incidentTrackerInfo;
	incidentTrackerInfo.id = incidentTrackerId;

	DBClientConfig dbConfig;
	IncidentTrackerInfoVect incidentTrackers;
	IncidentTrackerQueryOption option(m_dataQueryContextPtr);
	option.setTargetId(incidentTrackerInfo.id);
	dbConfig.getIncidentTrackers(incidentTrackers, option);
	if (incidentTrackers.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_INCIDENT_TRACKER_ID, incidentTrackerInfo.id);
		return;
	}
	incidentTrackerInfo = incidentTrackers[0];

	bool allowEmpty = true;
	HatoholError err = parseIncidentTrackerParameter(incidentTrackerInfo,
							 m_query,
							 allowEmpty);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// try to update
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateIncidentTracker(
	  incidentTrackerInfo, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// make a response
	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", incidentTrackerInfo.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceIncidentTracker::handleDelete(void)
{
	uint64_t incidentTrackerId = getResourceId();
	if (incidentTrackerId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err =
	  dataStore->deleteIncidentTracker(
	    incidentTrackerId, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// replay
	JSONBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", incidentTrackerId);
	agent.endObject();
	replyJSONData(agent);
}

RestResourceIncidentTrackerFactory::RestResourceIncidentTrackerFactory(
  FaceRest *faceRest)
: FaceRest::ResourceHandlerFactory(faceRest, NULL)
{
}

FaceRest::ResourceHandler *RestResourceIncidentTrackerFactory::createHandler()
{
	return new RestResourceIncidentTracker(m_faceRest);
}
