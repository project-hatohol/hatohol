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

#include "RestResourceIssueTracker.h"
#include "DBClientConfig.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

const char *RestResourceIssueTracker::pathForIssueTracker = "/issue-tracker";

void RestResourceIssueTracker::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForIssueTracker, new RestResourceIssueTrackerFactory(faceRest));
}

RestResourceIssueTracker::RestResourceIssueTracker(FaceRest *faceRest)
: FaceRest::ResourceHandler(faceRest, NULL)
{
}

RestResourceIssueTracker::~RestResourceIssueTracker()
{
}

void RestResourceIssueTracker::handle(void)
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

void RestResourceIssueTracker::handleGet(void)
{
	IssueTrackerQueryOption option(m_dataQueryContextPtr);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	IssueTrackerInfoVect issueTrackers;
	dataStore->getIssueTrackers(issueTrackers, option);

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, HTERR_OK);

	IssueTrackerInfoVectIterator it = issueTrackers.begin();
	agent.startArray("issueTrackers");
	for (; it != issueTrackers.end(); ++it) {
		agent.startObject();
		agent.add("id", it->id);
		agent.add("type", it->type);
		agent.add("nickname", it->nickname);
		agent.add("baseURL", it->baseURL);
		agent.add("projectId", it->projectId);
		agent.add("trackerId", it->trackerId);
		agent.endObject();
	}
	agent.endArray();

	agent.endObject();

	replyJsonData(agent);
}

#define PARSE_STRING_VALUE(P)					    \
{								    \
	value = (char *)g_hash_table_lookup(query, #P);		    \
	if (!value && !allowEmpty)				    \
		return HatoholError(HTERR_NOT_FOUND_PARAMETER, #P); \
	issueTrackerInfo.P = value; \
}

static HatoholError parseIssueTrackerParameter(
  IssueTrackerInfo &issueTrackerInfo, GHashTable *query,
  const bool &forUpdate = false)
{
	const bool allowEmpty = forUpdate;
	HatoholError err;
	char *value;

	// required properties
	err = getParam<IssueTrackerType>(
		query, "type", "%d", issueTrackerInfo.type);
	if (err != HTERR_OK) {
		if (!allowEmpty || err != HTERR_NOT_FOUND_PARAMETER)
			return err;
	}
	PARSE_STRING_VALUE(nickname);
	PARSE_STRING_VALUE(baseURL);
	PARSE_STRING_VALUE(projectId);
	PARSE_STRING_VALUE(userName);
	PARSE_STRING_VALUE(password);

	// optional
	value = (char *)g_hash_table_lookup(query, "trackerId");
	issueTrackerInfo.trackerId = value ? : "";

	return HatoholError(HTERR_OK);
}

void RestResourceIssueTracker::handlePost(void)
{
	IssueTrackerInfo issueTrackerInfo;
	HatoholError err = parseIssueTrackerParameter(issueTrackerInfo,
						      m_query);
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	err = dataStore->addIssueTracker(
	  issueTrackerInfo, m_dataQueryContextPtr->getOperationPrivilege());
	if (err != HTERR_OK) {
		replyError(err);
		return;
	}

	JsonBuilderAgent agent;
	agent.startObject();
	addHatoholError(agent, err);
	agent.add("id", issueTrackerInfo.id);
	agent.endObject();
	replyJsonData(agent);
}

void RestResourceIssueTracker::handlePut(void)
{
	replyError(HTERR_NOT_IMPLEMENTED);
}

void RestResourceIssueTracker::handleDelete(void)
{
	replyError(HTERR_NOT_IMPLEMENTED);
}

RestResourceIssueTrackerFactory::RestResourceIssueTrackerFactory(
  FaceRest *faceRest)
: FaceRest::ResourceHandlerFactory(faceRest, NULL)
{
}

FaceRest::ResourceHandler *RestResourceIssueTrackerFactory::createHandler()
{
	return new RestResourceIssueTracker(m_faceRest);
}
