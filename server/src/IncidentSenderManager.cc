/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#include <map>
#include <Mutex.h>
#include "Reaper.h"
#include "IncidentSenderManager.h"
#include "IncidentSenderRedmine.h"
#include "IncidentSenderHatohol.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

struct IncidentSenderManager::Impl
{
	static IncidentSenderManager instance;
	map<IncidentTrackerIdType, IncidentSender*> sendersMap;
	Mutex sendersLock;

	~Impl()
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
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		IncidentTrackerInfoVect incidentTrackerVect;
		IncidentTrackerQueryOption option(USER_ID_SYSTEM);
		option.setTargetId(id);
		dbConfig.getIncidentTrackers(incidentTrackerVect, option);

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
		case INCIDENT_TRACKER_HATOHOL:
			sender = new IncidentSenderHatohol(tracker);
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

	void deleteSender(const IncidentTrackerIdType &id)
	{
		AutoMutex autoMutex(&sendersLock);
		map<IncidentTrackerIdType, IncidentSender*>::iterator it;
		it = sendersMap.find(id);
		if (it == sendersMap.end()) {
			MLPL_ERR("Not found IncidentTrackerId: %"
				 FMT_INCIDENT_TRACKER_ID "\n", id);
			return;
		}
		sendersMap.erase(it);
		delete it->second;
	}
};

IncidentSenderManager IncidentSenderManager::Impl::instance;

IncidentSenderManager &IncidentSenderManager::getInstance(void)
{
	return Impl::instance;
}

void IncidentSenderManager::queue(
  const IncidentTrackerIdType &trackerId, const EventInfo &eventInfo,
  IncidentSender::CreateIncidentCallback callback, void *userData)
{
	IncidentSender *sender = m_impl->getSender(trackerId);
	if (!sender) {
		MLPL_ERR("Failed to queue sending an incident"
			 " for the event: %" FMT_EVENT_ID "\n",
			 eventInfo.id.c_str());
		return;
	}
	sender->queue(eventInfo, callback, userData);
}

void IncidentSenderManager::queue(
  const IncidentInfo &incidentInfo, const string &comment,
  IncidentSender::UpdateIncidentCallback callback, void *userData)
{
	IncidentSender *sender = m_impl->getSender(incidentInfo.trackerId);
	if (!sender) {
		MLPL_ERR("Can't find or create IncidentSender for: "
			 "%" FMT_INCIDENT_TRACKER_ID "\n",
			 incidentInfo.trackerId);
		return;
	}
	sender->queue(incidentInfo, comment, callback, userData);
}

IncidentSenderManager::IncidentSenderManager(void)
: m_impl(new Impl())
{
}

IncidentSenderManager::~IncidentSenderManager()
{
}

bool IncidentSenderManager::isIdling(void)
{
	AutoMutex autoMutex(&m_impl->sendersLock);

	map<IncidentTrackerIdType, IncidentSender*>::iterator it
	  = m_impl->sendersMap.begin();
	for (; it != m_impl->sendersMap.end(); ++it) {
		IncidentSender *sender = it->second;
		if (!sender->isIdling())
			return false;
	}

	return true;
}

IncidentSender *IncidentSenderManager::getSender(
  const IncidentTrackerIdType &id, bool autoCreate)
{
	return m_impl->getSender(id, autoCreate);
}

void IncidentSenderManager::setOnChangedIncidentTracker(const IncidentTrackerIdType id)
{
	IncidentSender *sender = m_impl->getSender(id);
	sender->setOnChangedIncidentTracker();
}

void IncidentSenderManager::deleteIncidentTracker(const IncidentTrackerIdType id)
{
	MLPL_DBG("Delete IncidentSender for: "
		 "%" FMT_INCIDENT_TRACKER_ID "\n", id);
	m_impl->deleteSender(id);
}
