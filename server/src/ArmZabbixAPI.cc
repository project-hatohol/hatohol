/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <Logger.h>
#include <Mutex.h>
using namespace mlpl;

#include <sstream>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ArmZabbixAPI.h"
#include "JSONParserAgent.h"
#include "JSONBuilderAgent.h"
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBTablesMonitoring.h"
#include "UnifiedDataStore.h"
#include "HatoholDBUtils.h"
#include "HostInfoCache.h"
#include "ThreadLocalDBCache.h"

using namespace std;

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;
static const guint DEFAULT_IDLE_TIMEOUT = 60;

struct ArmZabbixAPI::Impl
{
	const ServerIdType zabbixServerId;
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring;
	HostInfoCache      hostInfoCache;

	// constructors
	Impl(const MonitoringServerInfo &serverInfo)
	: zabbixServerId(serverInfo.id),
	  dbMonitoring(cache.getMonitoring())
	{
	}
};

class connectionException : public HatoholException {};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(const MonitoringServerInfo &serverInfo)
: ArmBase("ArmZabbixAPI", serverInfo),
  m_impl(new Impl(serverInfo))
{
	setMonitoringServerInfo(serverInfo);
}

ArmZabbixAPI::~ArmZabbixAPI()
{
	requestExitAndWait();
}

void ArmZabbixAPI::onGotNewEvents(const ItemTablePtr &itemPtr)
{
	// This function is used on a test class.
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTablePtr ArmZabbixAPI::updateTriggers(void)
{
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	SmartTime last = uds->getTimestampOfLastTrigger(m_impl->zabbixServerId);
	const int requestSince = last.getAsTimespec().tv_sec;
	return getTrigger(requestSince);
}

void ArmZabbixAPI::updateItems(void)
{
	ItemTablePtr items = getItems();
	ItemTablePtr applications = getApplications(items);
	makeHatoholItems(items, applications);
}

void ArmZabbixAPI::updateHosts(void)
{
	ItemTablePtr hostTablePtr, hostsGroupsTablePtr;
	getHosts(hostTablePtr, hostsGroupsTablePtr);
	makeHatoholHosts(hostTablePtr);
	makeHatoholMapHostsHostgroups(hostsGroupsTablePtr);
}

void ArmZabbixAPI::updateEvents(void)
{
	const EventIdType serverLastEventId = getEndEventId(false);
	if (serverLastEventId == EVENT_ID_NOT_FOUND) {
		MLPL_ERR("Last event ID is not found\n");
		return;
	}

	const EventIdType dbLastEventId =
	  m_impl->dbMonitoring.getLastEventId(m_impl->zabbixServerId);
	MLPL_DBG("The last event ID in Hatohol DB: %" FMT_EVENT_ID "\n", dbLastEventId);
	EventIdType eventIdOffset = 0;

	if (dbLastEventId == EVENT_NOT_FOUND) {
		eventIdOffset = getEndEventId(true);
		if (eventIdOffset == EVENT_ID_NOT_FOUND) {
			MLPL_INFO("First event ID is not found\n");
			return;
		}
	} else {
		eventIdOffset = dbLastEventId + 1;
	}

	while (eventIdOffset < serverLastEventId) {
		const EventIdType eventIdTill =
		  eventIdOffset + NUMBER_OF_GET_EVENT_PER_ONCE;
		ItemTablePtr eventsTablePtr =
		  getEvents(eventIdOffset, eventIdTill);
		makeHatoholEvents(eventsTablePtr);
		onGotNewEvents(eventsTablePtr);

		eventIdOffset = eventIdTill;
	}
}

void ArmZabbixAPI::updateApplications(void)
{
	// getHosts() tries to get all hosts when an empty vector is passed.
	static const vector<uint64_t> appIdVector;
	ItemTablePtr tablePtr = getApplications(appIdVector);
}

void ArmZabbixAPI::updateGroups(void)
{
	ItemTablePtr groupsTablePtr;
	getGroups(groupsTablePtr);
	makeHatoholHostgroups(groupsTablePtr);
}

//
// virtual methods
//
gpointer ArmZabbixAPI::mainThread(HatoholThreadArg *arg)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("started: ArmZabbixAPI (server: %s)\n",
	          svInfo.hostName.c_str());
	return ArmBase::mainThread(arg);
}

void ArmZabbixAPI::makeHatoholTriggers(ItemTablePtr triggers)
{
	TriggerInfoList triggerInfoList;
	HatoholDBUtils::transformTriggersToHatoholFormat(
	  triggerInfoList, triggers, m_impl->zabbixServerId,
	  m_impl->hostInfoCache);
	m_impl->dbMonitoring.addTriggerInfoList(triggerInfoList);
}

void ArmZabbixAPI::makeHatoholEvents(ItemTablePtr events)
{
	EventInfoList eventInfoList;
	HatoholDBUtils::transformEventsToHatoholFormat(
	  eventInfoList, events, m_impl->zabbixServerId);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addEventList(eventInfoList);
}

void ArmZabbixAPI::makeHatoholItems(
  ItemTablePtr items, ItemTablePtr applications)
{
	ItemInfoList itemInfoList;
	MonitoringServerStatus serverStatus;
	serverStatus.serverId = m_impl->zabbixServerId;
	HatoholDBUtils::transformItemsToHatoholFormat(
	  itemInfoList, serverStatus, items, applications);
	m_impl->dbMonitoring.addItemInfoList(itemInfoList);
	m_impl->dbMonitoring.addMonitoringServerStatus(&serverStatus);
}

void ArmZabbixAPI::makeHatoholHostgroups(ItemTablePtr groups)
{
	HostgroupInfoList groupInfoList;
	HatoholDBUtils::transformGroupsToHatoholFormat(groupInfoList, groups,
	                                               m_impl->zabbixServerId);
	m_impl->dbMonitoring.addHostgroupInfoList(groupInfoList);
}

void ArmZabbixAPI::makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups)
{
	HostgroupElementList hostgroupElementList;
	HatoholDBUtils::transformHostsGroupsToHatoholFormat(
	  hostgroupElementList, hostsGroups, m_impl->zabbixServerId);
	m_impl->dbMonitoring.addHostgroupElementList(hostgroupElementList);
}

void ArmZabbixAPI::makeHatoholHosts(ItemTablePtr hosts)
{
	HostInfoList hostInfoList;
	HatoholDBUtils::transformHostsToHatoholFormat(hostInfoList, hosts,
	                                              m_impl->zabbixServerId);
	m_impl->dbMonitoring.addHostInfoList(hostInfoList);

	// TODO: consider if DBClientHatohol should have the cache
	HostInfoListConstIterator hostInfoItr = hostInfoList.begin();
	for (; hostInfoItr != hostInfoList.end(); ++hostInfoItr)
		m_impl->hostInfoCache.update(*hostInfoItr);
}

uint64_t ArmZabbixAPI::getMaximumNumberGetEventPerOnce(void)
{
	return NUMBER_OF_GET_EVENT_PER_ONCE;
}

//
// This function just shows a warning if there is missing host ID.
//
bool ArmZabbixAPI::mainThreadOneProc(void)
{
	if (!updateAuthTokenIfNeeded())
		return false;

	try {
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			updateItems();
			return true;
		}

		ItemTablePtr triggers = updateTriggers();
		updateHosts(); // TODO: Get it only when it's really needed.
		updateGroups();
		makeHatoholTriggers(triggers);
		updateEvents();

		if (!getCopyOnDemandEnabled())
			updateItems();
	} catch (const DataStoreException &dse) {
		MLPL_ERR("Error on update: %s\n", dse.what());
		clearAuthToken();
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
