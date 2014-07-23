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
#include <MutexLock.h>
using namespace mlpl;

#include <sstream>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ArmZabbixAPI.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBClientHatohol.h"
#include "UnifiedDataStore.h"
#include "HatoholDBUtils.h"
#include "HostInfoCache.h"

using namespace std;

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;
static const guint DEFAULT_IDLE_TIMEOUT = 60;

struct ArmZabbixAPI::PrivateContext
{
	const ServerIdType zabbixServerId;
	DBClientHatohol    dbClientHatohol;
	HostInfoCache      hostInfoCache;

	// constructors
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: zabbixServerId(serverInfo.id)
	{
	}
};

class connectionException : public HatoholException {};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(const MonitoringServerInfo &serverInfo)
: ArmBase("ArmZabbixAPI", serverInfo),
  m_ctx(NULL)
{
	setMonitoringServerInfo(serverInfo);
	m_ctx = new PrivateContext(serverInfo);
}

ArmZabbixAPI::~ArmZabbixAPI()
{
	requestExitAndWait();
	if (m_ctx)
		delete m_ctx;
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
	SmartTime last = uds->getTimestampOfLastTrigger(m_ctx->zabbixServerId);
	const int requestSince = last.getAsTimespec().tv_sec;
	return getTrigger(requestSince);
}

ItemTablePtr ArmZabbixAPI::updateItems(void)
{
	ItemTablePtr tablePtr = getItems();
	return tablePtr;
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
	const uint64_t serverLastEventId = getEndEventId(false);
	if (serverLastEventId == EVENT_ID_NOT_FOUND) {
		MLPL_ERR("Last event ID is not found\n");
		return;
	}

	const uint64_t dbLastEventId =
	  m_ctx->dbClientHatohol.getLastEventId(m_ctx->zabbixServerId);
	uint64_t eventIdOffset = 0;

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
		const uint64_t eventIdTill =
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
	  triggerInfoList, triggers, m_ctx->zabbixServerId,
	  m_ctx->hostInfoCache);
	m_ctx->dbClientHatohol.setTriggerInfoList(triggerInfoList,
	                                          m_ctx->zabbixServerId);
}

void ArmZabbixAPI::makeHatoholEvents(ItemTablePtr events)
{
	EventInfoList eventInfoList;
	HatoholDBUtils::transformEventsToHatoholFormat(
	  eventInfoList, events, m_ctx->zabbixServerId);

	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addEventList(eventInfoList);
}

void ArmZabbixAPI::makeHatoholItems(
  ItemTablePtr items, ItemTablePtr applications)
{
	ItemInfoList itemInfoList;
	MonitoringServerStatus serverStatus;
	serverStatus.serverId = m_ctx->zabbixServerId;
	HatoholDBUtils::transformItemsToHatoholFormat(
	  itemInfoList, serverStatus, items, applications);
	m_ctx->dbClientHatohol.addItemInfoList(itemInfoList);
	m_ctx->dbClientHatohol.addMonitoringServerStatus(&serverStatus);
}

void ArmZabbixAPI::makeHatoholHostgroups(ItemTablePtr groups)
{
	HostgroupInfoList groupInfoList;
	HatoholDBUtils::transformGroupsToHatoholFormat(groupInfoList, groups,
	                                               m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostgroupInfoList(groupInfoList);
}

void ArmZabbixAPI::makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups)
{
	HostgroupElementList hostgroupElementList;
	HatoholDBUtils::transformHostsGroupsToHatoholFormat(
	  hostgroupElementList, hostsGroups, m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostgroupElementList(hostgroupElementList);
}

void ArmZabbixAPI::makeHatoholHosts(ItemTablePtr hosts)
{
	HostInfoList hostInfoList;
	HatoholDBUtils::transformHostsToHatoholFormat(hostInfoList, hosts,
	                                              m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostInfoList(hostInfoList);

	// TODO: consider if DBClientHatohol should have the cache
	HostInfoListConstIterator hostInfoItr = hostInfoList.begin();
	for (; hostInfoItr != hostInfoList.end(); ++hostInfoItr)
		m_ctx->hostInfoCache.update(*hostInfoItr);
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

	try
	{
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			ItemTablePtr items = updateItems();
			ItemTablePtr applications = getApplications(items);
			makeHatoholItems(items, applications);
			return true;
		}

		// get triggers
		ItemTablePtr triggers = updateTriggers();

		// TODO: Change retrieve interval.
		//       Or, Hatohol gets in the event-driven.
		updateHosts();

		updateGroups();
		makeHatoholTriggers(triggers);

		updateEvents();

		if (!getCopyOnDemandEnabled()) {
			ItemTablePtr items = updateItems();
			ItemTablePtr applications = getApplications(items);
			makeHatoholItems(items, applications);
		}
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
