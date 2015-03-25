/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <Logger.h>
#include <Mutex.h>
using namespace mlpl;

#include <sstream>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "ConfigManager.h"
#include "ArmZabbixAPI.h"
#include "JSONParser.h"
#include "JSONBuilder.h"
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBTablesMonitoring.h"
#include "UnifiedDataStore.h"
#include "HatoholDBUtils.h"
#include "HostInfoCache.h"
#include "ThreadLocalDBCache.h"

using namespace std;

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;

struct ArmZabbixAPI::Impl
{
	const ServerIdType zabbixServerId;
	HostInfoCache      hostInfoCache;

	// constructors
	Impl(const MonitoringServerInfo &serverInfo)
	: zabbixServerId(serverInfo.id),
	  hostInfoCache(&serverInfo.id)
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

void ArmZabbixAPI::waitExit(void)
{
	freeze();
	abortSession();
	ArmBase::waitExit();
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

ItemTablePtr ArmZabbixAPI::updateTriggerExpandedDescriptions(void)
{
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	SmartTime last = uds->getTimestampOfLastTrigger(m_impl->zabbixServerId);
	const int requestSince = last.getAsTimespec().tv_sec;
	return getTriggerExpandedDescription(requestSince);
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

	return;
}

void ArmZabbixAPI::updateEvents(void)
{
	const uint64_t serverLastEventId = getEndEventId(false);
	if (serverLastEventId == EVENT_ID_NOT_FOUND) {
		MLPL_DBG("Last event ID is not found in Zabbix server\n");
		return;
	}

	ThreadLocalDBCache cache;
	const EventIdType _dbLastEventId =
	  cache.getMonitoring().getMaxEventId(m_impl->zabbixServerId);
	uint64_t dbLastEventId = EVENT_ID_NOT_FOUND;
	if (_dbLastEventId != EVENT_NOT_FOUND)
		Utils::conv(dbLastEventId, _dbLastEventId);
	MLPL_DBG("The last event ID in Hatohol DB: %" PRIu64 "\n",
	         dbLastEventId);
	uint64_t eventIdFrom = dbLastEventId == EVENT_ID_NOT_FOUND ?
				  (ConfigManager::getInstance()->getLoadOldEvents() ?
				   getEndEventId(true) : serverLastEventId) :
	                          dbLastEventId + 1;

	while (eventIdFrom <= serverLastEventId) {
		const uint64_t eventIdTill =
		  eventIdFrom + NUMBER_OF_GET_EVENT_PER_ONCE - 1;
		ItemTablePtr eventsTablePtr =
		  getEvents(eventIdFrom, eventIdTill);
		makeHatoholEvents(eventsTablePtr);
		onGotNewEvents(eventsTablePtr);

		eventIdFrom = eventIdTill + 1;
	}
}

void ArmZabbixAPI::updateApplications(void)
{
	// getHosts() tries to get all hosts when an empty vector is passed.
	static const ItemCategoryIdVector appIdVector;
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
	ArmBase::registerAvailableTrigger(COLLECT_NG_PARSER_ERROR,
					  FAILED_PARSER_JSON_DATA_TRIGGER_ID,
					  HTERR_FAILED_TO_PARSE_JSON_DATA);
	ArmBase::registerAvailableTrigger(COLLECT_NG_DISCONNECT_ZABBIX,
					  FAILED_CONNECT_ZABBIX_TRIGGER_ID,
					  HTERR_FAILED_CONNECT_ZABBIX);
	ArmBase::registerAvailableTrigger(COLLECT_NG_INTERNAL_ERROR,
					  FAILED_INTERNAL_ERROR_TRIGGER_ID,
					  HTERR_INTERNAL_ERROR);
	return ArmBase::mainThread(arg);
}

void ArmZabbixAPI::makeHatoholAllTriggers(void)
{
	TriggerInfoList mergedTriggerInfoList;
	ItemTablePtr triggers, expanded, mergedTriggers;
	triggers = getTrigger(0);
	expanded = getTriggerExpandedDescription(0);
	mergedTriggers =
	  mergePlainTriggersAndExpandedDescriptions(triggers, expanded);
	HatoholDBUtils::transformTriggersToHatoholFormat(
	  mergedTriggerInfoList, mergedTriggers, m_impl->zabbixServerId,
	  m_impl->hostInfoCache);

	ThreadLocalDBCache cache;
	cache.getMonitoring().updateTrigger(mergedTriggerInfoList, m_impl->zabbixServerId);
}

void ArmZabbixAPI::makeHatoholTriggers(ItemTablePtr triggers)
{
	TriggerInfoList mergedTriggerInfoList;
	ItemTablePtr expandedDescriptions, mergedTriggers;
	expandedDescriptions = updateTriggerExpandedDescriptions();
	mergedTriggers =
	  mergePlainTriggersAndExpandedDescriptions(triggers, expandedDescriptions);
	HatoholDBUtils::transformTriggersToHatoholFormat(
	  mergedTriggerInfoList, mergedTriggers, m_impl->zabbixServerId,
	  m_impl->hostInfoCache);

	ThreadLocalDBCache cache;
	cache.getMonitoring().addTriggerInfoList(mergedTriggerInfoList);
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
	  itemInfoList, serverStatus, items, applications,
	  m_impl->zabbixServerId, m_impl->hostInfoCache);
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addItemList(itemInfoList);
	dataStore->addMonitoringServerStatus(serverStatus);
}

void ArmZabbixAPI::makeHatoholHostgroups(ItemTablePtr groups)
{
	HostgroupVect hostgroups;
	HatoholDBUtils::transformGroupsToHatoholFormat(hostgroups, groups,
	                                               m_impl->zabbixServerId);
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  UnifiedDataStore::getInstance()->upsertHostgroups(hostgroups));
}

void ArmZabbixAPI::makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups)
{
	HostgroupMemberVect hostgroupMembers;
	HatoholDBUtils::transformHostsGroupsToHatoholFormat(
	  hostgroupMembers, hostsGroups, m_impl->zabbixServerId,
	  m_impl->hostInfoCache);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  uds->upsertHostgroupMembers(hostgroupMembers));
}

void ArmZabbixAPI::makeHatoholHosts(ItemTablePtr hosts)
{
	ServerHostDefVect svHostDefs;
	HatoholDBUtils::transformHostsToHatoholFormat(svHostDefs, hosts,
	                                              m_impl->zabbixServerId);
	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	THROW_HATOHOL_EXCEPTION_IF_NOT_OK(
	  uds->syncHosts(svHostDefs, m_impl->zabbixServerId,
	                 m_impl->hostInfoCache));
}

uint64_t ArmZabbixAPI::getMaximumNumberGetEventPerOnce(void)
{
	return NUMBER_OF_GET_EVENT_PER_ONCE;
}

HostInfoCache &ArmZabbixAPI::getHostInfoCache(void)
{
	return m_impl->hostInfoCache;
}

ArmBase::ArmPollingResult ArmZabbixAPI::handleHatoholException(
  const HatoholException &he)
{
	if (he.getErrCode() == HTERR_FAILED_CONNECT_ZABBIX) {
		clearAuthToken();
		MLPL_ERR("Error Connection: %s %d\n",
			 he.what(), he.getErrCode());
		return COLLECT_NG_DISCONNECT_ZABBIX;
	} else if (he.getErrCode() == HTERR_FAILED_TO_PARSE_JSON_DATA) {
		MLPL_ERR("Error Message parse: %s %d\n",
			 he.what(), he.getErrCode());
		return COLLECT_NG_PARSER_ERROR;
	}
	MLPL_ERR("Error on update: %s %d\n", he.what(), he.getErrCode());
	return COLLECT_NG_INTERNAL_ERROR;;
}

//
// This function just shows a warning if there is missing host ID.
//
ArmBase::ArmPollingResult ArmZabbixAPI::mainThreadOneProc(void)
{
	if (!updateAuthTokenIfNeeded())
		return COLLECT_NG_DISCONNECT_ZABBIX;

	try {
		updateHosts();
		updateGroups();
		if (UnifiedDataStore::getInstance()->wasStoredHostsChanged()){
			ItemTablePtr triggers = updateTriggers();
			makeHatoholTriggers(triggers);
		} else {
			makeHatoholAllTriggers();
		}
		updateEvents();

		if (!getCopyOnDemandEnabled())
			updateItems();
	} catch (const HatoholException &he) {
		return handleHatoholException(he);
	}

	return COLLECT_OK;
}

ArmBase::ArmPollingResult ArmZabbixAPI::mainThreadOneProcFetchItems(void)
{
	if (!updateAuthTokenIfNeeded())
		return COLLECT_NG_DISCONNECT_ZABBIX;

	try {
		updateItems();
	} catch (const HatoholException &he) {
		return handleHatoholException(he);
	}
	return COLLECT_OK;
}

ArmBase::ArmPollingResult ArmZabbixAPI::mainThreadOneProcFetchHistory(
  HistoryInfoVect &historyInfoVect,
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime)
{
	if (!updateAuthTokenIfNeeded())
		return COLLECT_NG_DISCONNECT_ZABBIX;

	try {
		ItemTablePtr itemTablePtr = getHistory(
		  itemInfo.id,
		  ZabbixAPI::fromItemValueType(itemInfo.valueType),
		  beginTime, endTime);
		HatoholDBUtils::transformHistoryToHatoholFormat(
		  historyInfoVect, itemTablePtr, m_impl->zabbixServerId);
	} catch (const HatoholException &he) {
		return handleHatoholException(he);
	}
	return COLLECT_OK;
}

ArmBase::ArmPollingResult ArmZabbixAPI::mainThreadOneProcFetchTriggers(void)
{
	if (!updateAuthTokenIfNeeded())
		return COLLECT_NG_DISCONNECT_ZABBIX;

	try {
		makeHatoholAllTriggers();
	} catch (const HatoholException &he) {
		return handleHatoholException(he);
	}
	return COLLECT_OK;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
