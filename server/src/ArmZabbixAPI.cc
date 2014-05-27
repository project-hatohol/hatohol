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
#include "ArmZabbixAPI-template.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DataStoreException.h"
#include "ItemEnum.h"
#include "DBClientZabbix.h"
#include "DBClientHatohol.h"
#include "UnifiedDataStore.h"

using namespace std;

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;
static const guint DEFAULT_IDLE_TIMEOUT = 60;

struct ArmZabbixAPI::PrivateContext
{
	string         authToken;
	const ServerIdType zabbixServerId;
	DBClientZabbix  *dbClientZabbix;
	DBClientHatohol  dbClientHatohol;
	UnifiedDataStore *dataStore;

	// constructors
	PrivateContext(const MonitoringServerInfo &serverInfo)
	: zabbixServerId(serverInfo.id),
	  dbClientZabbix(NULL),
	  dataStore(NULL)
	{
		dbClientZabbix = DBClientZabbix::create(serverInfo.id);

		// TODO: use serverInfo.ipAddress if it is given.
		dataStore = UnifiedDataStore::getInstance();
	}

	~PrivateContext()
	{
		if (dbClientZabbix)
			delete dbClientZabbix;
	}
};

class connectionException : public HatoholException {};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmZabbixAPI::ArmZabbixAPI(const MonitoringServerInfo &serverInfo)
: ZabbixAPI(serverInfo),
  ArmBase("ArmZabbixAPI", serverInfo),
  m_ctx(NULL)
{
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
template<typename T>
void ArmZabbixAPI::updateOnlyNeededItem
  (const ItemTable *primaryTable,
   const ItemId pickupItemId, const ItemId checkItemId,
   ArmZabbixAPI::DataGetter dataGetter,
   DBClientZabbix::AbsentItemPicker absentItemPicker,
   ArmZabbixAPI::TableSaver tableSaver)
{
	if (primaryTable->getNumberOfRows() == 0)
		return;

	// make a vector that has items used in the table.
	vector<T> usedItemVector;
	makeItemVector<T>(usedItemVector, primaryTable, pickupItemId);
	if (usedItemVector.empty())
		return;

	// extract items that are not in the replication DB.
	vector<T> absentItemVector;
	(m_ctx->dbClientZabbix->*absentItemPicker)(absentItemVector,
	                                          usedItemVector);
	if (absentItemVector.empty())
		return;

	// get needed data via ZABBIX API
	ItemTablePtr tablePtr = (this->*dataGetter)(absentItemVector);
	(this->*tableSaver)(tablePtr);

	// check the result
	checkObtainedItems<uint64_t>(tablePtr, absentItemVector, checkItemId);
}

ItemTablePtr ArmZabbixAPI::updateTriggers(void)
{
	int requestSince;
	int lastChange = m_ctx->dbClientZabbix->getTriggerLastChange();

	// TODO: to be considered that we may leak triggers.
	if (lastChange == DBClientZabbix::TRIGGER_CHANGE_TIME_NOT_FOUND)
		requestSince = 0;
	else
		requestSince = lastChange;
	ItemTablePtr tablePtr = getTrigger(requestSince);
	m_ctx->dbClientZabbix->addTriggersRaw2_0(tablePtr);
	return tablePtr;
}

void ArmZabbixAPI::updateFunctions(void)
{
	ItemTablePtr tablePtr = getFunctions();
	m_ctx->dbClientZabbix->addFunctionsRaw2_0(tablePtr);
}

ItemTablePtr ArmZabbixAPI::updateItems(void)
{
	ItemTablePtr tablePtr = getItems();
	m_ctx->dbClientZabbix->addItemsRaw2_0(tablePtr);
	return tablePtr;
}

void ArmZabbixAPI::updateHosts(void)
{
	ItemTablePtr hostTablePtr, hostsGroupsTablePtr;
	getHosts(hostTablePtr, hostsGroupsTablePtr);
	addHostsDataToDB(hostTablePtr);
	m_ctx->dbClientZabbix->addHostsGroupsRaw2_0(hostsGroupsTablePtr);
	makeHatoholMapHostsHostgroups(hostsGroupsTablePtr);
}

void ArmZabbixAPI::updateEvents(void)
{
	uint64_t eventIdOffset, eventIdTill;
	uint64_t dbLastEventId = m_ctx->dbClientZabbix->getLastEventId();
	uint64_t serverLastEventId = getLastEventId();
	ItemTablePtr tablePtr;
	while (dbLastEventId != serverLastEventId) {
		if (dbLastEventId == DBClientZabbix::EVENT_ID_NOT_FOUND) {
			eventIdOffset = 0;
			eventIdTill = NUMBER_OF_GET_EVENT_PER_ONCE;
		} else {
			eventIdOffset = dbLastEventId + 1;
			eventIdTill = dbLastEventId + NUMBER_OF_GET_EVENT_PER_ONCE;
		}
		tablePtr = getEvents(eventIdOffset, eventIdTill);
		m_ctx->dbClientZabbix->addEventsRaw2_0(tablePtr);
		makeHatoholEvents(tablePtr);
		onGotNewEvents(tablePtr);
		dbLastEventId = m_ctx->dbClientZabbix->getLastEventId();
	}
}

void ArmZabbixAPI::updateApplications(void)
{
	// getHosts() tries to get all hosts when an empty vector is passed.
	static const vector<uint64_t> appIdVector;
	ItemTablePtr tablePtr = getApplications(appIdVector);
	m_ctx->dbClientZabbix->addApplicationsRaw2_0(tablePtr);
}

void ArmZabbixAPI::updateApplications(const ItemTable *items)
{
	updateOnlyNeededItem<uint64_t>(
	  items,
	  ITEM_ID_ZBX_ITEMS_APPLICATIONID,
	  ITEM_ID_ZBX_APPLICATIONS_APPLICATIONID,
	  &ArmZabbixAPI::getApplications,
	  &DBClientZabbix::pickupAbsentApplcationIds,
	  &ArmZabbixAPI::addApplicationsDataToDB);
}

void ArmZabbixAPI::updateGroups(void)
{
	ItemTablePtr groupsTablePtr;
	getGroups(groupsTablePtr);
	m_ctx->dbClientZabbix->addGroupsRaw2_0(groupsTablePtr);
	makeHatoholHostgroups(groupsTablePtr);
}

void ArmZabbixAPI::addApplicationsDataToDB(ItemTablePtr &applications)
{
	m_ctx->dbClientZabbix->addApplicationsRaw2_0(applications);
}

void ArmZabbixAPI::addHostsDataToDB(ItemTablePtr &hosts)
{
	m_ctx->dbClientZabbix->addHostsRaw2_0(hosts);
	makeHatoholHosts(hosts);
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

void ArmZabbixAPI::makeHatoholTriggers(void)
{
	TriggerInfoList triggerInfoList;
	m_ctx->dbClientZabbix->getTriggersAsHatoholFormat(triggerInfoList);
	m_ctx->dbClientHatohol.setTriggerInfoList(triggerInfoList,
	                                          m_ctx->zabbixServerId);
}

void ArmZabbixAPI::makeHatoholEvents(ItemTablePtr events)
{
	EventInfoList eventInfoList;
	DBClientZabbix::transformEventsToHatoholFormat(eventInfoList, events,
	                                               m_ctx->zabbixServerId);
	m_ctx->dataStore->addEventList(eventInfoList);
}

void ArmZabbixAPI::makeHatoholItems(ItemTablePtr items)
{
	ItemInfoList itemInfoList;
	MonitoringServerStatus serverStatus;
	serverStatus.serverId = m_ctx->zabbixServerId;
	DBClientZabbix::transformItemsToHatoholFormat(itemInfoList,
	                                              serverStatus,
	                                              items);
	m_ctx->dbClientHatohol.addItemInfoList(itemInfoList);
	m_ctx->dbClientHatohol.addMonitoringServerStatus(&serverStatus);
}

void ArmZabbixAPI::makeHatoholHostgroups(ItemTablePtr groups)
{
	HostgroupInfoList groupInfoList;
	DBClientZabbix::transformGroupsToHatoholFormat(groupInfoList, groups,
	                                               m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostgroupInfoList(groupInfoList);
}

void ArmZabbixAPI::makeHatoholMapHostsHostgroups(ItemTablePtr hostsGroups)
{
	HostgroupElementList hostgroupElementList;
	DBClientZabbix::transformHostsGroupsToHatoholFormat(hostgroupElementList,
	                                                    hostsGroups,
	                                                    m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostgroupElementList(hostgroupElementList);
}

void ArmZabbixAPI::makeHatoholHosts(ItemTablePtr hosts)
{
	HostInfoList hostInfoList;
	DBClientZabbix::transformHostsToHatoholFormat(hostInfoList, hosts,
	                                              m_ctx->zabbixServerId);
	m_ctx->dbClientHatohol.addHostInfoList(hostInfoList);
}

uint64_t ArmZabbixAPI::getMaximumNumberGetEventPerOnce(void)
{
	return NUMBER_OF_GET_EVENT_PER_ONCE;
}

//
// This function just shows a warning if there is missing host ID.
//
template<typename T>
void ArmZabbixAPI::checkObtainedItems(const ItemTable *obtainedItemTable,
                                      const vector<T> &requestedItemVector,
                                      const ItemId itemId)
{
	size_t numRequested = requestedItemVector.size();
	size_t numObtained = obtainedItemTable->getNumberOfRows();
	if (numRequested != numObtained) {
		MLPL_WARN("requested: %zd, obtained: %zd\n",
		          numRequested, numObtained);
	}

	// make the set of obtained items
	set<T> obtainedItemSet;
	const ItemGroupList &grpList = obtainedItemTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		const ItemData *itemData = (*it)->getItem(itemId);
		const T &item = *itemData;
		obtainedItemSet.insert(item);
	}

	// check the requested ID is in the obtained
	for (size_t i = 0; i < numRequested; i++) {
		const T &reqItem = requestedItemVector[i];
		typename set<T>::iterator it = obtainedItemSet.find(reqItem);
		if (it == obtainedItemSet.end()) {
			ostringstream ss;
			ss << reqItem;
			MLPL_WARN(
			  "Not found in the obtained items: %s "
			  "(%"PRIu64")\n",
		          ss.str().c_str(), itemId);
		} else {
			obtainedItemSet.erase(it);
		}
	}
}

bool ArmZabbixAPI::mainThreadOneProc(void)
{
	if (!updateAuthTokenIfNeeded())
		return false;

	try
	{
		if (getUpdateType() == UPDATE_ITEM_REQUEST) {
			ItemTablePtr items = updateItems();
			makeHatoholItems(items);
			updateApplications(items);
			return true;
		}

		// get triggers
		updateTriggers();

		// TODO: Change retrieve interval.
		//       Or, Hatohol gets in the event-driven.
		updateHosts();

		updateGroups();
		// Currently functions are no longer updated, because ZABBIX
		// API can return host ID directly (If we use DBs as exactly
		// the same as those in Zabbix Server, we have to join
		// triggers, functions, and items to get the host ID).
		//
		// updateFunctions();

		makeHatoholTriggers();

		updateEvents();

		if (!getCopyOnDemandEnabled()) {
			ItemTablePtr items = updateItems();
			makeHatoholItems(items);
			updateApplications(items);
		}
	} catch (const DataStoreException &dse) {
		MLPL_ERR("Error on update: %s\n", dse.what());
		clearAuthToken();
		return false;
	}

	return true;
}

void ArmZabbixAPI::onUpdatedAuthToken(const string &authToken)
{
	m_ctx->authToken = authToken;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
