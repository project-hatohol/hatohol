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

#include "RestResourceSummary.h"
#include "UnifiedDataStore.h"

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceSummary>
  RestResourceSummaryFactory;

const char *RestResourceSummary::pathForSummary = "/summary";

void RestResourceSummary::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForSummary,
	  new RestResourceSummaryFactory(
	        faceRest, &RestResourceSummary::handlerSummary));
}

RestResourceSummary::RestResourceSummary(
  FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

void RestResourceSummary::handlerSummary(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	EventsQueryOption option(m_dataQueryContextPtr);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->getNumberOfEvents(option);

	JSONBuilder reply;
	reply.startObject();
	reply.startArray("summary");
	reply.endArray(); // summary

	addHatoholError(reply, HatoholError(HTERR_OK));
	reply.endObject();
	replyJSONData(reply);
}
