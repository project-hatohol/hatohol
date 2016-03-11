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


#include "RestResourceTriggerBriefs.h"
#include "UnifiedDataStore.h"

using namespace std;

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceTriggerBriefs>
  RestResourceTriggerBriefsFactory;

const char *RestResourceTriggerBriefs::pathForTriggerBriefs = "/trigger/briefs";

void RestResourceTriggerBriefs::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForTriggerBriefs,
	  new RestResourceTriggerBriefsFactory(
	        faceRest, &RestResourceTriggerBriefs::handlerGetTriggerBriefs));
}

RestResourceTriggerBriefs::RestResourceTriggerBriefs(
  FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}


void RestResourceTriggerBriefs::handlerGetTriggerBriefs(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}
}
