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

void RestResourceIssueTracker::handlePost(void)
{
	replyError(HTERR_NOT_IMPLEMENTED);
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
