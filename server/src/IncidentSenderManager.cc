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

#include <map>
#include <Mutex.h>
#include "Reaper.h"
#include "IncidentSenderManager.h"
#include "IncidentSenderRedmine.h"
#include "CacheServiceDBClient.h"

using namespace std;
using namespace mlpl;

struct IncidentSenderManager::PrivateContext
{
	static IncidentSenderManager instance;
	map<IncidentTrackerIdType, IncidentSender*> sendersMap;
	Mutex sendersLock;

	~PrivateContext()
	{
		AutoMutex autoMutex(&sendersLock);
		map<IncidentTrackerIdType, IncidentSender*>::iterator it;
		for (it = sendersMap.begin(); it != sendersMap.end(); ++it) {
			IncidentSender *sender = it->second;
			delete sender;
		}
		sendersMap.clear();
	}

	IncidentSender *createSender(const IncidentTrackerIdType &id)
	{
		CacheServiceDBClient cache;
		DBClientConfig *dbConfig = cache.getConfig();
		IncidentTrackerInfoVect incidentTrackerVect;
		IncidentTrackerQueryOption option(USER_ID_SYSTEM);
		option.setTargetId(id);
		dbConfig->getIncidentTrackers(incidentTrackerVect, option);

		if (incidentTrackerVect.size() <= 0) {
			MLPL_ERR("Not found IncidentTrackerInfo: %"
				 FMT_INCIDENT_TRACKER_ID "\n", id);
			return NULL;
		}
		if (incidentTrackerVect.size() > 1) {
			MLPL_ERR("Too many IncidentTrackerInfo for ID:%"
				 FMT_INCIDENT_TRACKER_ID "\n", id);
			return NULL;
		}

		IncidentTrackerInfo &tracker = *incidentTrackerVect.begin();
		IncidentSender *sender = NULL;
		switch (tracker.type) {
		case INCIDENT_TRACKER_REDMINE:
			sender = new IncidentSenderRedmine(tracker);
			break;
		default:
			MLPL_ERR("Invalid IncidentTracker type: %d\n",
				 tracker.type);
			break;
		}
		return sender;
	}

	IncidentSender *getSender(const IncidentTrackerIdType &id,
				  bool autoCreate = true)
	{
		AutoMutex autoMutex(&sendersLock);
		if (sendersMap.find(id) != sendersMap.end())
			return sendersMap[id];

		if (!autoCreate)
			return NULL;

		IncidentSender *sender = createSender(id);
		if (sender) {
			sendersMap[id] = sender;
			sender->start();
		}

		return sender;
	}
};

IncidentSenderManager IncidentSenderManager::PrivateContext::instance;

IncidentSenderManager &IncidentSenderManager::getInstance(void)
{
	return PrivateContext::instance;
}

void IncidentSenderManager::queue(
  const IncidentTrackerIdType &trackerId, const EventInfo &eventInfo,
  IncidentSender::StatusCallback callback, void *userData)
{
	IncidentSender *sender = m_ctx->getSender(trackerId);
	if (!sender) {
		MLPL_ERR("Failed to queue sending an incident"
			 " for the event: %" FMT_EVENT_ID "\n",
			 eventInfo.id);
		return;
	}
	sender->queue(eventInfo, callback, userData);
}

IncidentSenderManager::IncidentSenderManager(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

IncidentSenderManager::~IncidentSenderManager()
{
	delete m_ctx;
}

bool IncidentSenderManager::isIdling(void)
{
	AutoMutex autoMutex(&m_ctx->sendersLock);

	map<IncidentTrackerIdType, IncidentSender*>::iterator it
	  = m_ctx->sendersMap.begin();
	for (; it != m_ctx->sendersMap.end(); ++it) {
		IncidentSender *sender = it->second;
		if (!sender->isIdling())
			return false;
	}

	return true;
}

IncidentSender *IncidentSenderManager::getSender(
  const IncidentTrackerIdType &id, bool autoCreate)
{
	return m_ctx->getSender(id, autoCreate);
}

