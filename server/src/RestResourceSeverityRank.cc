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

#include "RestResourceSeverityRank.h"
#include "DBTablesConfig.h"
#include "UnifiedDataStore.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

typedef FaceRestResourceHandlerArg0FactoryTemplate<RestResourceSeverityRank>
  RestResourceSeverityRankFactory;

const char *RestResourceSeverityRank::pathForSeverityRank =
  "/severity-rank";

void RestResourceSeverityRank::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForSeverityRank,
	  new RestResourceSeverityRankFactory(faceRest));
}

RestResourceSeverityRank::RestResourceSeverityRank(FaceRest *faceRest)
: FaceRest::ResourceHandler(faceRest)
{
}

RestResourceSeverityRank::~RestResourceSeverityRank()
{
}

void RestResourceSeverityRank::handle(void)
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

void RestResourceSeverityRank::handleGet(void)
{
	SeverityRankQueryOption option(m_dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	SeverityRankInfoVect severityRanks;
	dataStore->getSeverityRanks(severityRanks, option);

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, HTERR_OK);

	agent.startArray("SeverityRanks");
	for (auto &severityRank : severityRanks) {
		agent.startObject();
		agent.add("id", severityRank.id);
		agent.add("status", severityRank.status);
		agent.add("color", severityRank.color);
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

static HatoholError parseSeverityRankParameter(
  SeverityRankInfo &severityRankInfo, GHashTable *query,
  const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;

	PARSE_VALUE(severityRankInfo, status, SeverityRankStatusType, allowEmpty);
	PARSE_STRING_VALUE(severityRankInfo, color, allowEmpty);

	return HatoholError(HTERR_OK);
}

void RestResourceSeverityRank::handlePost(void)
{
	SeverityRankInfo severityRankInfo;
	HatoholError err = parseSeverityRankParameter(severityRankInfo,
							 m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->upsertSeverityRank(
	  severityRankInfo, m_dataQueryContextPtr->getOperationPrivilege());

	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", severityRankInfo.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceSeverityRank::handlePut(void)
{
	uint64_t severityRankId = getResourceId();
	if (severityRankId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	SeverityRankInfo severityRankInfo;
	severityRankInfo.id = severityRankId;

	SeverityRankInfoVect severityRanks;
	SeverityRankQueryOption option(m_dataQueryContextPtr);
	option.setTargetIdList({severityRankInfo.id});
	ThreadLocalDBCache cache;
	cache.getConfig().getSeverityRanks(severityRanks, option);
	if (severityRanks.empty()) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_TARGET_RECORD,
		            "id: %" FMT_INCIDENT_TRACKER_ID, severityRankInfo.id);
		return;
	}
	severityRankInfo = *severityRanks.begin();

	bool allowEmpty = false;
	HatoholError err = parseSeverityRankParameter(severityRankInfo,
	                                              m_query,
	                                              allowEmpty);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// try to update
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->updateSeverityRank(
	  severityRankInfo, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// make a response
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", severityRankInfo.id);
	agent.endObject();
	replyJSONData(agent);
}

void RestResourceSeverityRank::handleDelete(void)
{
	uint64_t severityRankId = getResourceId();
	if (severityRankId == INVALID_ID) {
		REPLY_ERROR(this, HTERR_NOT_FOUND_ID_IN_URL,
		            "id: %s", getResourceIdString().c_str());
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	std::list<SeverityRankIdType> idList =
		{ static_cast<SeverityRankIdType>(severityRankId) };
	HatoholError err =
	  dataStore->deleteSeverityRanks(
	    idList, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	// reply
	JSONBuilder agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", severityRankId);
	agent.endObject();
	replyJSONData(agent);
}
