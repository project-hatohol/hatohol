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

#include <SmartTime.h>
#include <algorithm>
#include <mutex>
#include <Reaper.h>
#include "SelfMonitor.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
using namespace std;
using namespace mlpl;

const char *SelfMonitor::DEFAULT_SELF_MONITOR_HOST_NAME = "(self-monitor)";

struct SelfMonitor::Impl
{
	TriggerInfo        triggerInfo;

	/**
	 * The last status other than TRIGGER_STATUS_UNKNOWN.
	 * I.e. TRIGGER_STATUS_OK or TRIGGER_STATUS_PROBLEM.
	 */
	TriggerStatusType  lastKnownStatus;

	/**
	 * The last status set by update(). This can be updated even when
	 * isWorkable() is false, althogh the visible status is
	 * TRIGGER_STATUS_PROBLEM.
	 */
	TriggerStatusType  actualStatus;

	SelfMonitorWeakPtrList listeners;
	EventGenerator eventGenerators[NUM_TRIGGER_STATUS][NUM_TRIGGER_STATUS];
	NotificationHandler notificationHandler;
	bool                inUpdating;
	bool                workable;
	bool                inSetWorkableCall;

	Impl(const ServerIdType &serverId,
	     const TriggerIdType &triggerId,
	     const string &brief,
	     const TriggerSeverityType &severity)
	: lastKnownStatus(TRIGGER_STATUS_UNKNOWN),
	  actualStatus(TRIGGER_STATUS_UNKNOWN),
	  notificationHandler([](
	    SelfMonitor &, const SelfMonitor &,
	    const TriggerStatusType &, const TriggerStatusType &){}),
	  inUpdating(false),
	  workable(true),
	  inSetWorkableCall(false)
	{
		initEventGenerators();
		setupTriggerInfo(serverId, triggerId, brief, severity);
		setupHostForSelfMonitor(serverId);
	}

	TriggerStatusType unworkableVisibleStatus(void)
	{
		// We use NUM_TRIGGER_STATUS for the meaning.
		return TRIGGER_STATUS_UNKNOWN;
	}

	void setupTriggerInfo(const ServerIdType &serverId,
	                      const TriggerIdType &triggerId,
	                      const string &brief,
	                      const TriggerSeverityType &severity)
	{
		triggerInfo.serverId = serverId;
		triggerInfo.id = triggerId;
		triggerInfo.status = TRIGGER_STATUS_UNKNOWN;
		triggerInfo.severity = severity;
		triggerInfo.lastChangeTime =
		  SmartTime::getCurrTime().getAsTimespec();
		triggerInfo.globalHostId = INAPPLICABLE_HOST_ID;
		triggerInfo.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
		triggerInfo.hostName = DEFAULT_SELF_MONITOR_HOST_NAME;

		triggerInfo.brief = brief;
		triggerInfo.extendedInfo.clear();
		triggerInfo.validity = TRIGGER_VALID_SELF_MONITORING;

		updateTrigger();
	}

	void setupHostForSelfMonitor(const ServerIdType &serverId)
	{
		ServerHostDef svHostDef;
		svHostDef.id = AUTO_INCREMENT_VALUE;
		svHostDef.hostId = AUTO_ASSIGNED_ID;
		svHostDef.serverId = serverId;
		svHostDef.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
		svHostDef.name = DEFAULT_SELF_MONITOR_HOST_NAME;
		svHostDef.status = HOST_STAT_SELF_MONITOR;
		ThreadLocalDBCache cache;
		cache.getHost().upsertHost(svHostDef);
	}

	void initEventGenerators(void)
	{
		auto nullFunc = [](SelfMonitor &monitor,
		                   const TriggerStatusType &prevStatus,
		                   const TriggerStatusType &newStatus) {};

		for (auto &generatorArray : eventGenerators) {
			for (auto &generator : generatorArray)
				generator = nullFunc;
		}
	}

	EventType calcEventType(const TriggerStatusType &trigStatus)
	{
		EventType evtType = EVENT_TYPE_UNKNOWN;
		if (trigStatus == TRIGGER_STATUS_OK)
			evtType = EVENT_TYPE_GOOD;
		else
			evtType = EVENT_TYPE_BAD;
		return evtType;
	}

	void updateTrigger(void)
	{
		if (triggerInfo.id == STATELESS_MONITOR)
			return;
		ThreadLocalDBCache cache;
		cache.getMonitoring().addTriggerInfo(&triggerInfo);
	}
};

// TODO: This is easy implementation that uses big lock.
//       We want to replace it with a more efficient way.
static recursive_mutex updateLock;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
SelfMonitor::SelfMonitor(
  const ServerIdType &serverId,
  const TriggerIdType &triggerId,
  const string &brief,
  const TriggerSeverityType &severity)
: m_impl(new Impl(serverId, triggerId, brief, severity))
{
}

SelfMonitor::~SelfMonitor()
{
}

void SelfMonitor::setEventGenerator(
  const TriggerStatusType &prevStatus, const TriggerStatusType &newStatus,
  SelfMonitor::EventGenerator generator)
{
	auto setupRange = [](const TriggerStatusType &st,
	                     TriggerStatusType &begin, TriggerStatusType &end)
	{
		if (st == TRIGGER_STATUS_ALL) {
			begin = static_cast<TriggerStatusType>(0);
			end = NUM_TRIGGER_STATUS;
		} else {
			begin = st;
			end = static_cast<TriggerStatusType>(st + 1);
		}
	};

	TriggerStatusType prevBegin, prevEnd, newBegin, newEnd;
	setupRange(prevStatus, prevBegin, prevEnd);
	setupRange(newStatus, newBegin, newEnd);

	for (int pst = prevBegin; pst != prevEnd; ++pst) {
		for (int nst = newBegin; nst != newEnd; ++nst)
			m_impl->eventGenerators[pst][nst] = generator;
	}
}

void SelfMonitor::setNotificationHandler(NotificationHandler handler)
{
	m_impl->notificationHandler = handler;
}

void SelfMonitor::update(const TriggerStatusType &_status)
{
	// Prevent other threads from running this method concurrently
	lock_guard<recursive_mutex> lock(updateLock);

	HATOHOL_ASSERT(!m_impl->inUpdating, "Detetec a circular listening.");
	Reaper<bool> eraser(&m_impl->inUpdating, [](bool *bp){*bp = false;});
	m_impl->inUpdating = true;

	TriggerStatusType status = _status;
	if (!m_impl->inSetWorkableCall)
		m_impl->actualStatus = status;
	if (!m_impl->workable)
		status = m_impl->unworkableVisibleStatus();

	TriggerStatusType prevStatus = m_impl->triggerInfo.status;
	m_impl->triggerInfo.status = status;
	onUpdated(status, prevStatus);

	m_impl->updateTrigger();

	generateEvent(prevStatus, status);

	SelfMonitorWeakPtrList::iterator listenerIt = m_impl->listeners.begin();
	while (listenerIt != m_impl->listeners.end()) {
		auto listener = listenerIt->lock();
		if (listener) {
			listener->onNotified(*this, prevStatus, status);
			++listenerIt;
		} else {
			// Delete a deleleted listener.
			listenerIt = m_impl->listeners.erase(listenerIt);
		}
	}

	if (status != TRIGGER_STATUS_UNKNOWN)
		m_impl->lastKnownStatus = status;
}

TriggerStatusType SelfMonitor::getLastKnownStatus(void) const
{
	return m_impl->lastKnownStatus;
}

void SelfMonitor::saveEvent(const string &brief, const EventType &eventType)
{
	// TODO: Set once for members that never change
	EventInfoList eventList;
	eventList.push_back(EventInfo());
	EventInfo &eventInfo = eventList.back();
	initEventInfo(eventInfo);

	const TriggerInfo &triggerInfo = m_impl->triggerInfo;
	eventInfo.serverId = triggerInfo.serverId;
	eventInfo.id = "";
	eventInfo.time =
	  SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();

	if (eventType != EVENT_TYPE_AUTO)
		eventInfo.type = eventType;
	else
		eventInfo.type = m_impl->calcEventType(triggerInfo.status);

	eventInfo.triggerId = triggerInfo.id;

	eventInfo.status = triggerInfo.status;
	eventInfo.severity = triggerInfo.severity;
	eventInfo.globalHostId = triggerInfo.globalHostId;
	eventInfo.hostIdInServer = triggerInfo.hostIdInServer;
	eventInfo.hostName = triggerInfo.hostName;
	if (!brief.empty())
		eventInfo.brief = brief;
	else
		eventInfo.brief = triggerInfo.brief;
	eventInfo.extendedInfo = triggerInfo.extendedInfo;

	// We have to use UnifiedDataStore::addEventList
	// (not DBTablesMonitoring method directly) to work
	// Action mechanism. Therefore, we use EventInfoList
	// although we save only one event.
	UnifiedDataStore::getInstance()->addEventList(eventList);
}

void SelfMonitor::setWorkable(const bool &workable)
{
	Reaper<bool> clean(&m_impl->inSetWorkableCall,
	                   [](bool *b){ *b = false;});
	m_impl->inSetWorkableCall = true;
	m_impl->workable = workable;
	if (workable)
		update(m_impl->actualStatus);
	else
		update(m_impl->unworkableVisibleStatus());
}

void SelfMonitor::addNotificationListener(SelfMonitorPtr monitor)
{
	m_impl->listeners.push_back(monitor);
}

void SelfMonitor::generateEvent(const TriggerStatusType &prevStatus,
                                const TriggerStatusType &newStatus)
{
	auto generator = m_impl->eventGenerators[prevStatus][newStatus];
	generator(*this, prevStatus, newStatus);
}

void SelfMonitor::onUpdated(const TriggerStatusType &prevStatus,
                            const TriggerStatusType &newStatus)
{
}

void SelfMonitor::onNotified(const SelfMonitor &srcMonitor,
                             const TriggerStatusType &prevStatus,
                             const TriggerStatusType &newStatus)
{
	auto handler = m_impl->notificationHandler;
	handler(*this, srcMonitor, prevStatus, newStatus);
}
