/*
 * Copyright (C) 2013-2016 Project Hatohol
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

#include <stdexcept>
#include <mutex>
#include <AtomicValue.h>
#include <Reaper.h>
#include "UnifiedDataStore.h"
#include "DBTablesAction.h"
#include "DBTablesConfig.h"
#include "DataStoreManager.h"
#include "ActionManager.h"
#include "ThreadLocalDBCache.h"
#include "ItemFetchWorker.h"
#include "TriggerFetchWorker.h"
#include "DataStoreFactory.h"
#include "ArmIncidentTracker.h"
#include "IncidentSenderManager.h"

using namespace std;
using namespace mlpl;

typedef map<ServerIdType, DataStore *> ServerIdDataStoreMap;
typedef ServerIdDataStoreMap::iterator ServerIdDataStoreMapIterator;

typedef map<IncidentTrackerIdType, ArmIncidentTracker *> ArmIncidentTrackerMap;
typedef ArmIncidentTrackerMap::iterator ArmIncidentTrackerMapIterator;

static ArmInfo getArmInfo(DataStore *dataStore)
{
	return dataStore->getArmStatus().getArmInfo();
}

// ---------------------------------------------------------------------------
// UnifiedDataStore
// ---------------------------------------------------------------------------
struct UnifiedDataStore::Impl
{
	struct UnifiedDataStoreEventProc : public DataStoreEventProc
	{
		Impl *impl;

		UnifiedDataStoreEventProc(
		  Impl *_impl)
		: impl(_impl)
		{
		}

		virtual ~UnifiedDataStoreEventProc()
		{
		}

		virtual void onAdded(DataStore *dataStore) override
		{
			impl->addToDataStoreMap(dataStore);
		}

		virtual void onRemoved(DataStore *dataStore) override
		{
			impl->removeFromDataStoreMap(dataStore);
		}
	};

	static UnifiedDataStore *instance;
	static std::mutex        mutex;

	ItemFetchWorker          itemFetchWorker;
	TriggerFetchWorker       triggerFetchWorker;

	Impl()
	: isStarted(false)
	{
		// TODO: When should the object be freed ?
		UnifiedDataStoreEventProc *evtProc =
		  new Impl::UnifiedDataStoreEventProc(this);
		dataStoreManager.registEventProc(evtProc);
	};

	void addToDataStoreMap(DataStore *dataStore)
	{
		const ServerIdType serverId =
		  dataStore->getMonitoringServerInfo().id;
		pair<ServerIdDataStoreMapIterator, bool> result;
		serverIdDataStoreMapLock.writeLock();
		result = serverIdDataStoreMap.insert(
		  pair<ServerIdType, DataStore *>(serverId, dataStore));
		serverIdDataStoreMapLock.unlock();
		HATOHOL_ASSERT(
		  result.second,
		  "Failed to insert: ServerID: %" FMT_SERVER_ID ", "
		  "DataStore: %p", serverId, dataStore);
	}

	void removeFromDataStoreMap(DataStore *dataStore)
	{
		const ServerIdType serverId =
		  dataStore->getMonitoringServerInfo().id;
		serverIdDataStoreMapLock.writeLock();
		ServerIdDataStoreMapIterator it =
		  serverIdDataStoreMap.find(serverId);
		bool found = (it != serverIdDataStoreMap.end());
		if (found)
			serverIdDataStoreMap.erase(it);
		serverIdDataStoreMapLock.unlock();
		HATOHOL_ASSERT(
		  found,
		  "Failed to found: ServerID: %" FMT_SERVER_ID ", "
		  "DataStore: %p", serverId, dataStore);
	}

	DataStorePtr getDataStore(const ServerIdType &serverId)
	{
		DataStore *dataStore = NULL;
		serverIdDataStoreMapLock.readLock();
		Reaper<ReadWriteLock> unlocker(&serverIdDataStoreMapLock,
		                               ReadWriteLock::unlock);
		ServerIdDataStoreMapIterator it =
		  serverIdDataStoreMap.find(serverId);
		const bool found = (it != serverIdDataStoreMap.end());
		if (found)
			dataStore = it->second;
		return dataStore; // ref() is called in DataStorePtr's C'tor
	}

	HatoholError startDataStore(const MonitoringServerInfo &svInfo,
	                            const bool &autoRun)
	{
		DataStore *dataStore =
		  DataStoreFactory::create(svInfo, autoRun);
		if (!dataStore)
			return HTERR_FAILED_TO_CREATE_DATA_STORE;
		bool successed = dataStoreManager.add(svInfo.id, dataStore);
		 // incremented in the above add() if successed
		dataStore->unref();
		if (!successed)
			return HTERR_FAILED_TO_REGISTER_DATA_STORE;
		return HTERR_OK;
	}

	HatoholError stopDataStore(const ServerIdType &serverId,
	                           bool *isRunning = NULL)
	{
		serverIdDataStoreMapLock.readLock();
		ServerIdDataStoreMapIterator it =
		  serverIdDataStoreMap.find(serverId);
		const bool found = (it != serverIdDataStoreMap.end());
		serverIdDataStoreMapLock.unlock();
		if (!found)
			return HTERR_INVALID_PARAMETER;

		const MonitoringServerInfo &svInfo =
		  it->second->getMonitoringServerInfo();
		HATOHOL_ASSERT(
		  svInfo.id == serverId,
		  "svInfo.id: %" FMT_SERVER_ID ", serverId: %" FMT_SERVER_ID,
		  svInfo.id, serverId);

		if (isRunning)
			*isRunning = getArmInfo(it->second).running;
		dataStoreManager.remove(serverId);
		return HTERR_OK;
	}

	void startAllDataStores(const bool &autoRun)
	{
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		MonitoringServerInfoList monitoringServers;
		ServerQueryOption option(USER_ID_SYSTEM);
		dbConfig.getTargetServers(monitoringServers, option);

		MonitoringServerInfoListConstIterator svInfoItr
			= monitoringServers.begin();
		for (; svInfoItr != monitoringServers.end(); ++svInfoItr)
			startDataStore(*svInfoItr, autoRun);
	}

	void stopAllDataStores(void)
	{
		ServerIdDataStoreMapIterator it;
		while (true) {
			serverIdDataStoreMapLock.readLock();
			it = serverIdDataStoreMap.begin();
			bool found = (it != serverIdDataStoreMap.end());
			// We must unlock before calling stopDataStore()
			// to avoid a deadlock.
			serverIdDataStoreMapLock.unlock();
			if (!found)
				break;
			const ServerIdType serverId = it->first;
			// TODO: serverId is checked again in stopDataStore()
			// This is a little wasteful.
			stopDataStore(serverId);
		}
	}

	DataStoreVector getDataStoreVector(void)
	{
		return dataStoreManager.getDataStoreVector();
	}

	void startArmIncidentTrackerIfNeeded(
	  const IncidentTrackerInfo &trackerInfo)
	{
		lock_guard<std::mutex> lock(armIncidentTrackerMapMutex);
		ArmIncidentTrackerMapIterator it
		  = armIncidentTrackerMap.find(trackerInfo.id);
		ArmIncidentTracker *arm = NULL;
		if (it == armIncidentTrackerMap.end()) {
			arm = ArmIncidentTracker::create(trackerInfo);
			if (arm != NULL) {
				armIncidentTrackerMap[trackerInfo.id] = arm;
			} else {
				return;
			}
		} else {
			arm = it->second;
		}

		arm->startIfNeeded();

		if (!arm->isStarted()) {
			MLPL_DBG("Failed to launch ArmIncidentTracker for "
				 "Tracker ID: %" FMT_INCIDENT_TRACKER_ID ", "
				 "Nickname: %s, URL: %s\n",
				 trackerInfo.id,
				 trackerInfo.nickname.c_str(),
				 trackerInfo.baseURL.c_str());
			armIncidentTrackerMap.erase(trackerInfo.id);
			delete arm;
		}
	}

	void stopArmIncidentTrackerIfNeeded(
	  const IncidentTrackerInfo &trackerInfo)
	{
		stopArmIncidentTrackerIfNeeded(trackerInfo.id);
	}

	void stopArmIncidentTrackerIfNeeded(
	  const IncidentTrackerIdType &trackerId)
	{
		lock_guard<std::mutex> lock(armIncidentTrackerMapMutex);
		ArmIncidentTrackerMapIterator it
		  = armIncidentTrackerMap.find(trackerId);
		if (it == armIncidentTrackerMap.end())
			return;
		ArmIncidentTracker *arm = it->second;
		armIncidentTrackerMap.erase(it);
		delete arm;
	}

	void startAllArmIncidentTrackers(const bool &autoRun)
	{
		if (!autoRun)
			return;

		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		IncidentTrackerInfoVect incidentTrackers;
		IncidentTrackerQueryOption option(USER_ID_SYSTEM);
		dbConfig.getIncidentTrackers(incidentTrackers, option);

		IncidentTrackerInfoVectConstIterator it
			= incidentTrackers.begin();
		for (; it != incidentTrackers.end(); ++it) {
			startArmIncidentTrackerIfNeeded(*it);
		}
	}

	void stopAllArmIncidentTrackers(void)
	{
		lock_guard<std::mutex> lock(armIncidentTrackerMapMutex);
		ArmIncidentTrackerMapIterator it
			= armIncidentTrackerMap.begin();
		while (it != armIncidentTrackerMap.end()) {
			delete it->second;
			armIncidentTrackerMap.erase(it++);
		}
	}

	void start(const bool &autoRun)
	{
		startAllDataStores(autoRun);
		startAllArmIncidentTrackers(autoRun);
		isStarted = true;
	}

	void stop(void)
	{
		stopAllDataStores();
		stopAllArmIncidentTrackers();
		isStarted = false;
	}

	void updateCustomIncidentStatusMap(void)
	{
		customIncidentStatusMapLock.writeLock();
		Reaper<ReadWriteLock> unlocker(&customIncidentStatusMapLock,
		                               ReadWriteLock::unlock);

		customIncidentStatusMap.clear();

		CustomIncidentStatusesQueryOption option(USER_ID_SYSTEM);
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		vector<CustomIncidentStatus> customStatuses;
		dataStore->getCustomIncidentStatuses(customStatuses, option);
		for (auto &customStatus: customStatuses)
			customIncidentStatusMap[customStatus.code] = customStatus;
	}

	void getCustomIncidentStatusMap(map<string, CustomIncidentStatus> &map)
	{
		customIncidentStatusMapLock.readLock();
		Reaper<ReadWriteLock> unlocker(&customIncidentStatusMapLock,
		                               ReadWriteLock::unlock);

		if (customIncidentStatusMap.empty()) {
			customIncidentStatusMapLock.unlock();
			updateCustomIncidentStatusMap();
			customIncidentStatusMapLock.readLock();
		}

		map = customIncidentStatusMap;
	}

	ReadWriteLock            serverIdDataStoreMapLock;
	ServerIdDataStoreMap     serverIdDataStoreMap;
	DataStoreManager         dataStoreManager;
	std::mutex               armIncidentTrackerMapMutex;
	ArmIncidentTrackerMap    armIncidentTrackerMap;
	bool                     isStarted;
	ReadWriteLock            customIncidentStatusMapLock;
	map<string, CustomIncidentStatus> customIncidentStatusMap;
};

UnifiedDataStore *UnifiedDataStore::Impl::instance = NULL;
mutex             UnifiedDataStore::Impl::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_impl(new Impl())
{
}

UnifiedDataStore::~UnifiedDataStore()
{
}

void UnifiedDataStore::reset(void)
{
	stop();
	m_impl->customIncidentStatusMap.clear();
}

UnifiedDataStore *UnifiedDataStore::getInstance(void)
{
	if (Impl::instance)
		return Impl::instance;

	Impl::mutex.lock();
	if (!Impl::instance)
		Impl::instance = new UnifiedDataStore();
	Impl::mutex.unlock();

	return Impl::instance;
}

void UnifiedDataStore::start(const bool &autoRun)
{
	m_impl->start(autoRun);
}

void UnifiedDataStore::stop(void)
{
	m_impl->stop();
}

void UnifiedDataStore::fetchItems(const ServerIdType &targetServerId)
{
	if (!m_impl->itemFetchWorker.updateIsNeeded())
		return;

	bool started = m_impl->itemFetchWorker.start(targetServerId, NULL);
	if (!started)
		return;

	m_impl->itemFetchWorker.waitCompletion();
}

void UnifiedDataStore::getTriggerList(TriggerInfoList &triggerList,
				      const TriggersQueryOption &option)
{
	ThreadLocalDBCache cache;
	cache.getMonitoring().getTriggerInfoList(triggerList, option);
}

void UnifiedDataStore::getTriggerBriefList(
  list<string> &triggerBriefList, const TriggersQueryOption &option)
{
	ThreadLocalDBCache cache;
	cache.getMonitoring().getTriggerBriefList(triggerBriefList, option);
}

SmartTime UnifiedDataStore::getTimestampOfLastTrigger(
  const ServerIdType &serverId)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	const timespec ts =
	  {dbMonitoring.getLastChangeTimeOfTrigger(serverId), 0};
	return SmartTime(ts);
}

HatoholError UnifiedDataStore::getEventList(EventInfoList &eventList,
					    EventsQueryOption &option,
					    IncidentInfoVect *incidentVect)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getEventInfoList(eventList, option, incidentVect);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList,
				   const ItemsQueryOption &option,
				   bool fetchItemsSynchronously)
{
	if (fetchItemsSynchronously)
		fetchItems(option.getTargetServerId());
	ThreadLocalDBCache cache;
	cache.getMonitoring().getItemInfoList(itemList, option);
}

void UnifiedDataStore::getItemCategoryNames(
  vector<string> &categoryNames, const ItemsQueryOption &option)
{
	ThreadLocalDBCache cache;
	cache.getMonitoring().getItemCategoryNames(categoryNames, option);
}

bool UnifiedDataStore::fetchItemsAsync(Closure0 *closure,
                                       const ItemsQueryOption &option)
{
	if (!m_impl->itemFetchWorker.updateIsNeeded())
		return false;

	return m_impl->itemFetchWorker.start(option, closure);
}

bool UnifiedDataStore::fetchTriggerAsync(Closure0 *closure,
					 const TriggersQueryOption &option)
{
	const ServerIdType targetServerId = option.getTargetServerId();
	if (!m_impl->triggerFetchWorker.updateIsNeeded())
		return false;

	return m_impl->triggerFetchWorker.start(targetServerId, closure);
}

HatoholError UnifiedDataStore::getActionList(
  ActionDefList &actionList, const ActionsQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getAction().getActionList(actionList, option);
}

HatoholError UnifiedDataStore::deleteActionList(
  const ActionIdList &actionIdList, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	return cache.getAction().deleteActions(actionIdList, privilege);
}

HatoholError UnifiedDataStore::updateAction(
  ActionDef &actionDef, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	return cache.getAction().updateAction(actionDef, privilege);
}

bool UnifiedDataStore::isIncidentSenderActionEnabled(void)
{
	ThreadLocalDBCache cache;
	return cache.getAction().isIncidentSenderEnabled();
}

HatoholError UnifiedDataStore::upsertHost(const ServerHostDef &serverHostDef,
                                          HostIdType *hostId)
{
	ThreadLocalDBCache cache;
	const HostIdType returnedId = cache.getHost().upsertHost(serverHostDef);
	if (hostId)
		*hostId = returnedId;
	return HTERR_OK;
}

HatoholError UnifiedDataStore::getServerHostDefs(
  ServerHostDefVect &svHostDefVect, const HostsQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getHost().getServerHostDefs(svHostDefVect, option);
}

HatoholError UnifiedDataStore::getHostgroups(
  HostgroupVect &hostgroups, const HostgroupsQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getHost().getHostgroups(hostgroups, option);
}

HatoholError UnifiedDataStore::getServerHostGrpSetMap(
  ServerHostGrpSetMap &serverHostGrpSetMap,
  const HostgroupsQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getHost().getServerHostGrpSetMap(serverHostGrpSetMap, option);
}

HatoholError UnifiedDataStore::upsertHosts(
  const ServerHostDefVect &serverHostDefs, HostHostIdMap *hostHostIdMapPtr,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	cache.getHost().upsertHosts(serverHostDefs, hostHostIdMapPtr, hooks);
	return HTERR_OK;
}

HatoholError UnifiedDataStore::syncHosts(
  const ServerHostDefVect &svHostDefs, const ServerIdType &serverId,
  HostInfoCache &hostInfoCache, DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	HostHostIdMap hostHostIdMap;
	HatoholError err = cache.getHost().syncHosts(svHostDefs, serverId,
	                                             &hostHostIdMap, hooks);
	if (err != HTERR_OK)
		return err;
	hostInfoCache.update(svHostDefs, &hostHostIdMap);
	return HTERR_OK;
}

bool UnifiedDataStore::wasStoredHostsChanged(void)
{
	ThreadLocalDBCache cache;
	return cache.getHost().wasStoredHostsChanged();
}

size_t UnifiedDataStore::getNumberOfHosts(HostsQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getHost().getNumberOfHosts(option);
}

HatoholError UnifiedDataStore::upsertHostgroups(
  const HostgroupVect &hostgroups, DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	cache.getHost().upsertHostgroups(hostgroups, hooks);
	return HTERR_OK;
}

HatoholError UnifiedDataStore::syncHostgroups(
  const HostgroupVect &hostgroups, const ServerIdType &serverId,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	return cache.getHost().syncHostgroups(hostgroups, serverId, hooks);
}

HatoholError UnifiedDataStore::upsertHostgroupMembers(
  const HostgroupMemberVect &hostgroupMembers,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	cache.getHost().upsertHostgroupMembers(hostgroupMembers, hooks);
	return HTERR_OK;
}

HatoholError UnifiedDataStore::syncHostgroupMembers(
  const HostgroupMemberVect &hostgroupMembers,
  const ServerIdType &serverId,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	return cache.getHost().syncHostgroupMembers(hostgroupMembers, serverId,
	                                            hooks);
}

HatoholError UnifiedDataStore::getHostgroupMembers(
  HostgroupMemberVect &hostgroupMembers,
  const HostgroupMembersQueryOption &option)
{
	ThreadLocalDBCache cache;
	cache.getHost().getHostgroupMembers(hostgroupMembers, option);
	return HTERR_OK;
}

size_t UnifiedDataStore::getNumberOfBadTriggers(
  const TriggersQueryOption &option, TriggerSeverityType severity)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getNumberOfBadTriggers(option, severity);
}

size_t UnifiedDataStore::getNumberOfTriggers(const TriggersQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getMonitoring().getNumberOfTriggers(option);
}

void UnifiedDataStore::addTriggers(
  const TriggerInfoList &triggerInfoList,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	return cache.getMonitoring().addTriggerInfoList(triggerInfoList, hooks);
}

HatoholError UnifiedDataStore::syncTriggers(
  const TriggerInfoList &triggerInfoList,
  const ServerIdType &serverId,
  DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	return cache.getMonitoring().syncTriggers(triggerInfoList, serverId,
	                                          hooks);
}

size_t UnifiedDataStore::getNumberOfGoodHosts(const TriggersQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getMonitoring().getNumberOfGoodHosts(option);
}

size_t UnifiedDataStore::getNumberOfBadHosts(const TriggersQueryOption &option)
{
	ThreadLocalDBCache cache;
	return cache.getMonitoring().getNumberOfBadHosts(option);
}

size_t UnifiedDataStore::getNumberOfItems(const ItemsQueryOption &option,
					  bool fetchItemsSynchronously)
{
	if (fetchItemsSynchronously)
		fetchItems(option.getTargetServerId());
	ThreadLocalDBCache cache;
	return cache.getMonitoring().getNumberOfItems(option);
}

HatoholError UnifiedDataStore::getNumberOfMonitoredItemsPerSecond(
  const DataQueryOption &option, MonitoringServerStatus &serverStatus)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getNumberOfMonitoredItemsPerSecond(option, serverStatus);
}

size_t UnifiedDataStore::getNumberOfEvents(const EventsQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getNumberOfEvents(option);
}

size_t UnifiedDataStore::getNumberOfHostsWithSpecifiedEvents(
  const EventsQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getNumberOfHostsWithSpecifiedEvents(option);
}

HatoholError UnifiedDataStore::getEventSeverityStatistics(
  std::vector<DBTablesMonitoring::EventSeverityStatistics> &severityStatisticsVect,
  const EventsQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getEventSeverityStatistics(severityStatisticsVect, option);
}

HatoholError UnifiedDataStore::addAction(ActionDef &actionDef,
                                         const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	return cache.getAction().addAction(actionDef, privilege);
}

void UnifiedDataStore::addEventList(EventInfoList &eventList,
                                    DBAgent::TransactionHooks *hooks)
{
	ThreadLocalDBCache cache;
	ActionManager actionManager;
	cache.getMonitoring().addEventInfoList(eventList, hooks);
	actionManager.checkEvents(eventList);
}

void UnifiedDataStore::addItemList(const ItemInfoList &itemList)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addItemInfoList(itemList);
}

void UnifiedDataStore::syncItems(const ItemInfoList &itemList,
                                 const ServerIdType &serverId)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.syncItems(itemList, serverId);
}

void UnifiedDataStore::addMonitoringServerStatus(
  const MonitoringServerStatus &serverStatus)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addMonitoringServerStatus(serverStatus);
}

void UnifiedDataStore::getUserList(UserInfoList &userList,
                                   const UserQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	dbUser.getUserInfoList(userList, option);
}

HatoholError UnifiedDataStore::addUser(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.addUserInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::updateUser(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.updateUserInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::updateUserFlags(
  OperationPrivilegeFlag &oldUserFlag, OperationPrivilegeFlag &updateUserFlag,
  const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.updateUserInfoFlags(oldUserFlag, updateUserFlag, privilege);
}

HatoholError UnifiedDataStore::deleteUser(
  UserIdType userId, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.deleteUserInfo(userId, privilege);
}

HatoholError UnifiedDataStore::getAccessInfoMap(
  ServerAccessInfoMap &srvAccessInfoMap, const AccessInfoQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.getAccessInfoMap(srvAccessInfoMap, option);
}

HatoholError UnifiedDataStore::addAccessInfo(
  AccessInfo &userInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.addAccessInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::deleteAccessInfo(
  AccessInfoIdType id, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.deleteAccessInfo(id, privilege);
}

void UnifiedDataStore::getUserRoleList(UserRoleInfoList &userRoleList,
				       const UserRoleQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	dbUser.getUserRoleInfoList(userRoleList, option);
}

HatoholError UnifiedDataStore::addUserRole(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.addUserRoleInfo(userRoleInfo, privilege);
}

HatoholError UnifiedDataStore::updateUserRole(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.updateUserRoleInfo(userRoleInfo, privilege);
}

HatoholError UnifiedDataStore::deleteUserRole(
  UserRoleIdType userRoleId, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesUser &dbUser = cache.getUser();
	return dbUser.deleteUserRoleInfo(userRoleId, privilege);
}

void UnifiedDataStore::getServerTypes(ServerTypeInfoVect &serverTypes)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	dbConfig.getServerTypes(serverTypes);
}

void UnifiedDataStore::getTargetServers(
  MonitoringServerInfoList &monitoringServers, ServerQueryOption &option,
  ArmPluginInfoVect *armPluginInfoVect)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	dbConfig.getTargetServers(monitoringServers, option,
	                           armPluginInfoVect);
}

HatoholError UnifiedDataStore::addTargetServer(
  MonitoringServerInfo &svInfo, ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege, const bool &autoRun)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.addTargetServer(svInfo, armPluginInfo,
	                                            privilege);
	if (err != HTERR_OK)
		return err;

	return m_impl->startDataStore(svInfo, autoRun);
}

HatoholError UnifiedDataStore::updateTargetServer(
  MonitoringServerInfo &svInfo, ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.updateTargetServer(svInfo, armPluginInfo,
	                                               privilege);
	if (err != HTERR_OK)
		return err;

	bool isRunning = false;
	err = m_impl->stopDataStore(svInfo.id, &isRunning);
	if (err != HTERR_OK)
		return err;
	return m_impl->startDataStore(svInfo, isRunning);
}

HatoholError UnifiedDataStore::deleteTargetServer(
  const ServerIdType &serverId, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.deleteTargetServer(serverId, privilege);
	if (err != HTERR_OK)
		return err;

	return m_impl->stopDataStore(serverId);
}

void UnifiedDataStore::getServerConnStatusVector(
  ServerConnStatusVector &svConnStatVec, DataQueryContext *dataQueryContext)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	ServerIdSet serverIdSet;
	dbConfig.getServerIdSet(serverIdSet, dataQueryContext);
	svConnStatVec.reserve(serverIdSet.size());

	ServerIdSetIterator serverIdItr = serverIdSet.begin();
	for (; serverIdItr != serverIdSet.end(); ++serverIdItr) {
		DataStorePtr dataStorePtr = getDataStore(*serverIdItr);
		if (!dataStorePtr.hasData())
			continue;
		ServerConnStatus svConnStat;
		svConnStat.serverId = *serverIdItr;
		svConnStat.armInfo = getArmInfo(dataStorePtr);
		svConnStatVec.push_back(svConnStat);
	}
}

void UnifiedDataStore::getIncidentTrackers(
  IncidentTrackerInfoVect &incidentTrackerVect,
  IncidentTrackerQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	dbConfig.getIncidentTrackers(incidentTrackerVect, option);
}

HatoholError UnifiedDataStore::addIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	return dbConfig.addIncidentTracker(incidentTrackerInfo, privilege);
}

HatoholError UnifiedDataStore::updateIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	IncidentSenderManager &senderManager = IncidentSenderManager::getInstance();
	HatoholError err =
	  dbConfig.updateIncidentTracker(incidentTrackerInfo, privilege);
	if (err != HTERR_OK)
		return err;
	stopArmIncidentTrackerIfNeeded(incidentTrackerInfo.id);
	senderManager.setOnChangedIncidentTracker(incidentTrackerInfo.id);
	startArmIncidentTrackerIfNeeded(incidentTrackerInfo.id);
	return HTERR_OK;
}

HatoholError UnifiedDataStore::deleteIncidentTracker(
  const IncidentTrackerIdType &incidentTrackerId,
  const OperationPrivilege &privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	IncidentSenderManager &senderManager = IncidentSenderManager::getInstance();
	HatoholError err =
	  dbConfig.deleteIncidentTracker(incidentTrackerId, privilege);
	if (err != HTERR_OK)
		return err;
	senderManager.deleteIncidentTracker(incidentTrackerId);
	stopArmIncidentTrackerIfNeeded(incidentTrackerId);
	return HTERR_OK;
}

uint64_t UnifiedDataStore::getLastUpdateTimeOfIncidents(
  const IncidentTrackerIdType &trackerId)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getLastUpdateTimeOfIncidents(trackerId);
}

HatoholError UnifiedDataStore::getIncidents(
  IncidentInfoVect &incidentInfoVect, const IncidentsQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getIncidentInfoVect(incidentInfoVect, option);
}

void UnifiedDataStore::addIncidentInfo(IncidentInfo &incidentInfo)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	dbMonitoring.addIncidentInfo(&incidentInfo);
	if (m_impl->isStarted)
		startArmIncidentTrackerIfNeeded(incidentInfo.trackerId);
}

HatoholError UnifiedDataStore::updateIncidentInfo(IncidentInfo &incidentInfo)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.updateIncidentInfo(incidentInfo);
}

HatoholError UnifiedDataStore::addIncidentHistory(
  IncidentHistory &incidentHistory)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.addIncidentHistory(incidentHistory);
}

HatoholError UnifiedDataStore::updateIncidentHistory(
  IncidentHistory &incidentHistory)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.updateIncidentHistory(incidentHistory);
}

HatoholError UnifiedDataStore::getIncidentHistories(
  list<IncidentHistory> &incidentHistoryList,
  const IncidentHistoriesQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	return dbMonitoring.getIncidentHistory(incidentHistoryList,
					       option);
}

DataStoreVector UnifiedDataStore::getDataStoreVector(void)
{
	return m_impl->getDataStoreVector();
}

DataStorePtr UnifiedDataStore::getDataStore(const ServerIdType &serverId)
{
	return m_impl->getDataStore(serverId);
}

void UnifiedDataStore::getSystemInfo(
  SystemInfo &systemInfo, const DataQueryOption &option)
{
	DBTablesMonitoring::getSystemInfo(systemInfo.monitoring, option);
}

bool UnifiedDataStore::getIncidentTrackerInfo(const IncidentTrackerIdType &trackerId,
					      IncidentTrackerInfo &trackerInfo)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	IncidentTrackerInfoVect incidentTrackerVect;
	IncidentTrackerQueryOption option(USER_ID_SYSTEM);
	option.setTargetId(trackerId);
	dbConfig.getIncidentTrackers(incidentTrackerVect, option);

	if (incidentTrackerVect.size() <= 0) {
		MLPL_ERR("Not found IncidentTrackerInfo: %"
			 FMT_INCIDENT_TRACKER_ID "\n", trackerId);
		return false;
	}
	if (incidentTrackerVect.size() > 1) {
		MLPL_ERR("Too many IncidentTrackerInfo for ID:%"
			 FMT_INCIDENT_TRACKER_ID "\n", trackerId);
		return false;
	}

	trackerInfo = *incidentTrackerVect.begin();
	return true;
}

HatoholError UnifiedDataStore::upsertSeverityRank(
  SeverityRankInfo &severityRankInfo, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	return cache.getConfig().upsertSeverityRankInfo(severityRankInfo, privilege);
}

HatoholError UnifiedDataStore::updateSeverityRank(
  SeverityRankInfo &severityRankInfo, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	return cache.getConfig().updateSeverityRankInfo(severityRankInfo, privilege);
}

HatoholError UnifiedDataStore::getSeverityRanks(
  SeverityRankInfoVect &severityRankInfoVect, const SeverityRankQueryOption &option)
{
	ThreadLocalDBCache cache;
	cache.getConfig().getSeverityRanks(severityRankInfoVect, option);

	return HTERR_OK;
}

HatoholError UnifiedDataStore::deleteSeverityRanks(
  std::list<SeverityRankIdType> &idList, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	return cache.getConfig().deleteSeverityRanks(idList, privilege);
}

HatoholError UnifiedDataStore::upsertCustomIncidentStatus(
  CustomIncidentStatus &customIncidentStatus, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.upsertCustomIncidentStatus(customIncidentStatus,
							       privilege);
	if (err == HTERR_OK)
		m_impl->updateCustomIncidentStatusMap();
	return err;
}

HatoholError UnifiedDataStore::updateCustomIncidentStatus(
  CustomIncidentStatus &customIncidentStatus, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.updateCustomIncidentStatus(customIncidentStatus,
							       privilege);
	if (err == HTERR_OK)
		m_impl->updateCustomIncidentStatusMap();
	return err;
}

HatoholError UnifiedDataStore::getCustomIncidentStatuses(
  vector<CustomIncidentStatus> &customIncidentStatusVect,
  const CustomIncidentStatusesQueryOption &option)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	dbConfig.getCustomIncidentStatuses(customIncidentStatusVect, option);
	return HTERR_OK;
}

HatoholError UnifiedDataStore::deleteCustomIncidentStatuses(
  list<CustomIncidentStatusIdType> &idList, const OperationPrivilege privilege)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	HatoholError err = dbConfig.deleteCustomIncidentStatus(idList, privilege);
	m_impl->updateCustomIncidentStatusMap();
	return err;
}

void UnifiedDataStore::getCustomIncidentStatusesCache(
  map<string, CustomIncidentStatus> &customIncidentStatusMap)
{
	m_impl->getCustomIncidentStatusMap(customIncidentStatusMap);
}

void UnifiedDataStore::startArmIncidentTrackerIfNeeded(
  const IncidentTrackerIdType &trackerId)
{
	IncidentTrackerInfo trackerInfo;
	if (getIncidentTrackerInfo(trackerId, trackerInfo))
		m_impl->startArmIncidentTrackerIfNeeded(trackerInfo);
}

void UnifiedDataStore::stopArmIncidentTrackerIfNeeded(
  const IncidentTrackerIdType &trackerId)
{
	m_impl->stopArmIncidentTrackerIfNeeded(trackerId);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
