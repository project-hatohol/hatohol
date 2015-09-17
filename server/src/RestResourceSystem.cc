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

#include "RestResourceSystem.h"
#include "UnifiedDataStore.h"

typedef FaceRestResourceHandlerSimpleFactoryTemplate<RestResourceSystem>
  RestResourceSystemFactory;

const char *RestResourceSystem::pathForSystemInfo = "/system-info";

void RestResourceSystem::registerFactories(FaceRest *faceRest)
{
	faceRest->addResourceHandlerFactory(
	  pathForSystemInfo,
	  new RestResourceSystemFactory(
	        faceRest, &RestResourceSystem::handlerSystemInfo));
}

RestResourceSystem::RestResourceSystem(FaceRest *faceRest, HandlerFunc handler)
: RestResourceMemberHandler(faceRest, static_cast<RestMemberHandler>(handler))
{
}

void RestResourceSystem::handlerSystemInfo(void)
{
	if (!httpMethodIs("GET")) {
		MLPL_ERR("Unknown method: %s\n", m_message->method);
		replyHttpStatus(SOUP_STATUS_METHOD_NOT_ALLOWED);
	}

	DataQueryOption option(m_dataQueryContextPtr);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	UnifiedDataStore::SystemInfo systemInfo;
	dataStore->getSystemInfo(systemInfo, option);

	JSONBuilder reply;
	reply.startObject();
	reply.startArray("eventRates");
	for (size_t i = 0; i < DBTablesMonitoring::NUM_EVENTS_COUNTERS; i++) {
		struct {
			const char *label;
			StatisticsCounter::Slot *slot;
		} const params[] = {
		{
			"previous",
			&systemInfo.monitoring.eventsCounterPrevSlots[i],
		}, {
			"current",
			&systemInfo.monitoring.eventsCounterCurrSlots[i],
		},
		};

		reply.startObject();
		for (auto &param : params) {
			reply.startObject(param.label);
			reply.add("startTime", param.slot->startTime);
			reply.add("endTime", param.slot->endTime);
			reply.add("number", param.slot->number);
			reply.endObject(); // label
		}
		reply.endObject();
	}
	reply.endArray(); // eventRates

	addHatoholError(reply, HatoholError(HTERR_OK));
	reply.endObject();
	replyJSONData(reply);
}

