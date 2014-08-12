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

#include <stdexcept>
#include <Mutex.h>
#include <AtomicValue.h>
#include <Reaper.h>
#include "UnifiedDataStore.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
#include "DataStoreManager.h"
#include "ActionManager.h"
#include "CacheServiceDBClient.h"
#include "ItemFetchWorker.h"
#include "DataStoreFactory.h"
#include "HatoholArmPluginGate.h" // TODO: remove after dynamic_cast is deleted

using namespace std;
using namespace mlpl;

typedef map<ServerIdType, DataStore *> ServerIdDataStoreMap;
typedef ServerIdDataStoreMap::iterator ServerIdDataStoreMapIterator;

static ArmInfo getArmInfo(DataStore *dataStore)
{
	// TODO: Too direct. Be elegant.
	// HatoholArmPluginGate::getArmBase is stub to pass
	// the build. So we can't use it.
	// Our new design suggests that DataStore instance
	// provides getArmStats() directly.
	HatoholArmPluginGate *pluginGate =
	  dynamic_cast<HatoholArmPluginGate *>(dataStore);
	if (pluginGate)
		return pluginGate->getArmStatus().getArmInfo();
	return dataStore->getArmBase().getArmStatus().getArmInfo();
}

// ---------------------------------------------------------------------------
// UnifiedDataStore
// ---------------------------------------------------------------------------
struct UnifiedDataStore::Impl
{
	struct UnifiedDataStoreEventProc : public DataStoreEventProc
	{
		Impl *impl;
		const AtomicValue<bool> &enableCopyOnDemand;

		UnifiedDataStoreEventProc(
		  Impl *_impl,
		  const AtomicValue<bool> &copyOnDemand)
		: impl(_impl),
		  enableCopyOnDemand(copyOnDemand)
		{
		}

		virtual ~UnifiedDataStoreEventProc()
		{
		}

		virtual void onAdded(DataStore *dataStore) override
		{
			dataStore->setCopyOnDemandEnable(enableCopyOnDemand);
			impl->addToDataStoreMap(dataStore);
		}

		virtual void onRemoved(DataStore *dataStore) override
		{
			impl->removeFromDataStoreMap(dataStore);
		}
	};

	static UnifiedDataStore *instance;
	static Mutex             mutex;

	AtomicValue<bool>        isCopyOnDemandEnabled;
	ItemFetchWorker          itemFetchWorker;

	Impl()
	: isCopyOnDemandEnabled(false)
	{ 
		// TODO: When should the object be freed ?
		UnifiedDataStoreEventProc *evtProc =
		  new Impl::UnifiedDataStoreEventProc(
		    this, isCopyOnDemandEnabled);
		dataStoreManager.registEventProc(evtProc);
	};

	void addToDataStoreMap(DataStore *dataStore)
	{
		const ServerIdType serverId =
		  dataStore->getArmBase().getServerInfo().id;
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
		  dataStore->getArmBase().getServerInfo().id;
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
			return HTERR_FAILED_TO_REGIST_DATA_STORE;
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

		ArmBase &armBase = it->second->getArmBase();
		const MonitoringServerInfo &svInfo = armBase.getServerInfo();
		HATOHOL_ASSERT(
		  svInfo.id == serverId,
		  "svInfo.id: %" FMT_SERVER_ID ", serverId: %" FMT_SERVER_ID, 
		  svInfo.id, serverId);

		if (isRunning)
			*isRunning = getArmInfo(it->second).running;
		dataStoreManager.remove(serverId);
		return HTERR_OK;
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

private:
	ReadWriteLock            serverIdDataStoreMapLock;
	ServerIdDataStoreMap     serverIdDataStoreMap;
	DataStoreManager         dataStoreManager;
};

UnifiedDataStore *UnifiedDataStore::Impl::instance = NULL;
Mutex             UnifiedDataStore::Impl::mutex;

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
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	MonitoringServerInfoList monitoringServers;
	ServerQueryOption option(USER_ID_SYSTEM);
	dbConfig->getTargetServers(monitoringServers, option);

	MonitoringServerInfoListConstIterator svInfoItr
	  = monitoringServers.begin();
	for (; svInfoItr != monitoringServers.end(); ++svInfoItr)
		m_impl->startDataStore(*svInfoItr, autoRun);
}

void UnifiedDataStore::stop(void)
{
	m_impl->stopAllDataStores();
}

void UnifiedDataStore::fetchItems(const ServerIdType &targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return;
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
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList, option);
}

SmartTime UnifiedDataStore::getTimestampOfLastTrigger(
  const ServerIdType &serverId)
{
	CacheServiceDBClient cache;
	DBClientHatohol *dbHatohol = cache.getHatohol();
	const timespec ts =
	  {dbHatohol->getLastChangeTimeOfTrigger(serverId), 0};
	return SmartTime(ts);
}

HatoholError UnifiedDataStore::getEventList(EventInfoList &eventList,
					    EventsQueryOption &option,
					    IncidentInfoVect *incidentVect)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getEventInfoList(eventList, option, incidentVect);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList,
				   const ItemsQueryOption &option,
				   bool fetchItemsSynchronously)
{
	if (fetchItemsSynchronously)
		fetchItems(option.getTargetServerId());
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList, option);
}

bool UnifiedDataStore::fetchItemsAsync(ClosureBase *closure,
                                       const ServerIdType &targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return false;
	if (!m_impl->itemFetchWorker.updateIsNeeded())
		return false;

	return m_impl->itemFetchWorker.start(targetServerId, closure);
}

void UnifiedDataStore::getHostList(HostInfoList &hostInfoList,
				   const HostsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getHostInfoList(hostInfoList, option);
}

HatoholError UnifiedDataStore::getActionList(
  ActionDefList &actionList, const ActionsQueryOption &option)
{
	DBClientAction dbAction;
	return dbAction.getActionList(actionList, option);
}

HatoholError UnifiedDataStore::deleteActionList(
  const ActionIdList &actionIdList, const OperationPrivilege &privilege)
{
	DBClientAction dbAction;
	return dbAction.deleteActions(actionIdList, privilege);
}

bool UnifiedDataStore::isIncidentSenderActionEnabled(void)
{
	DBClientAction dbAction;
	return dbAction.isIncidentSenderEnabled();
}

HatoholError UnifiedDataStore::getHostgroupInfoList
  (HostgroupInfoList &hostgroupInfoList, const HostgroupsQueryOption &option)
{
	DBClientHatohol dbClientHatohol;
	return dbClientHatohol.getHostgroupInfoList(hostgroupInfoList,option);
}

HatoholError UnifiedDataStore::getHostgroupElementList(
  HostgroupElementList &hostgroupElementList,
  const HostgroupElementQueryOption &option)
{
	DBClientHatohol dbClientHatohol;
	return dbClientHatohol.getHostgroupElementList(hostgroupElementList,
	                                               option);
}

size_t UnifiedDataStore::getNumberOfBadTriggers(
  const TriggersQueryOption &option, TriggerSeverityType severity)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfBadTriggers(option, severity);
}

size_t UnifiedDataStore::getNumberOfTriggers(const TriggersQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfTriggers(option);
}

size_t UnifiedDataStore::getNumberOfGoodHosts(const TriggersQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfGoodHosts(option);
}

size_t UnifiedDataStore::getNumberOfBadHosts(const TriggersQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfBadHosts(option);
}

size_t UnifiedDataStore::getNumberOfItems(const ItemsQueryOption &option,
					  bool fetchItemsSynchronously)
{
	if (fetchItemsSynchronously)
		fetchItems(option.getTargetServerId());
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfItems(option);
}

HatoholError UnifiedDataStore::getNumberOfMonitoredItemsPerSecond(
  const DataQueryOption &option, MonitoringServerStatus &serverStatus)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfMonitoredItemsPerSecond(option, serverStatus);
}

bool UnifiedDataStore::getCopyOnDemandEnabled(void) const
{
	return m_impl->isCopyOnDemandEnabled;
}

void UnifiedDataStore::setCopyOnDemandEnabled(bool enable)
{
	m_impl->isCopyOnDemandEnabled = enable;
}

HatoholError UnifiedDataStore::addAction(ActionDef &actionDef,
                                         const OperationPrivilege &privilege)
{
	DBClientAction dbAction;
	return dbAction.addAction(actionDef, privilege);
}

void UnifiedDataStore::addEventList(const EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	ActionManager actionManager;
	actionManager.checkEvents(eventList);
	dbHatohol.addEventInfoList(eventList);
}

void UnifiedDataStore::getUserList(UserInfoList &userList,
                                   const UserQueryOption &option)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	dbUser->getUserInfoList(userList, option);
}

HatoholError UnifiedDataStore::addUser(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->addUserInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::updateUser(
  UserInfo &userInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->updateUserInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::deleteUser(
  UserIdType userId, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->deleteUserInfo(userId, privilege);
}

HatoholError UnifiedDataStore::getAccessInfoMap(
  ServerAccessInfoMap &srvAccessInfoMap, const AccessInfoQueryOption &option)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->getAccessInfoMap(srvAccessInfoMap, option);
}

HatoholError UnifiedDataStore::addAccessInfo(
  AccessInfo &userInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->addAccessInfo(userInfo, privilege);
}

HatoholError UnifiedDataStore::deleteAccessInfo(
  AccessInfoIdType id, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->deleteAccessInfo(id, privilege);
}

void UnifiedDataStore::getUserRoleList(UserRoleInfoList &userRoleList,
				       const UserRoleQueryOption &option)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	dbUser->getUserRoleInfoList(userRoleList, option);
}

HatoholError UnifiedDataStore::addUserRole(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->addUserRoleInfo(userRoleInfo, privilege);
}

HatoholError UnifiedDataStore::updateUserRole(
  UserRoleInfo &userRoleInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->updateUserRoleInfo(userRoleInfo, privilege);
}

HatoholError UnifiedDataStore::deleteUserRole(
  UserRoleIdType userRoleId, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->deleteUserRoleInfo(userRoleId, privilege);
}

void UnifiedDataStore::getTargetServers(
  MonitoringServerInfoList &monitoringServers, ServerQueryOption &option,
  ArmPluginInfoVect *armPluginInfoVect)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	dbConfig->getTargetServers(monitoringServers, option,
	                           armPluginInfoVect);
}

HatoholError UnifiedDataStore::addTargetServer(
  MonitoringServerInfo &svInfo, ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege, const bool &autoRun)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->addTargetServer(&svInfo, privilege,
	                                             &armPluginInfo);
	if (err != HTERR_OK)
		return err;

	return m_impl->startDataStore(svInfo, autoRun);
}

HatoholError UnifiedDataStore::updateTargetServer(
  MonitoringServerInfo &svInfo, ArmPluginInfo &armPluginInfo,
  const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->updateTargetServer(&svInfo, privilege,
	                                                &armPluginInfo);
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
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->deleteTargetServer(serverId, privilege);
	if (err != HTERR_OK)
		return err;

	return m_impl->stopDataStore(serverId);
}

void UnifiedDataStore::getServerConnStatusVector(
  ServerConnStatusVector &svConnStatVec, DataQueryContext *dataQueryContext)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	ServerIdSet serverIdSet;
	dbConfig->getServerIdSet(serverIdSet, dataQueryContext);
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
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	dbConfig->getIncidentTrackers(incidentTrackerVect, option);
}

HatoholError UnifiedDataStore::addIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	return dbConfig->addIncidentTracker(incidentTrackerInfo, privilege);
}

HatoholError UnifiedDataStore::updateIncidentTracker(
  IncidentTrackerInfo &incidentTrackerInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	return dbConfig->updateIncidentTracker(incidentTrackerInfo, privilege);
}

HatoholError UnifiedDataStore::deleteIncidentTracker(
  const IncidentTrackerIdType &incidentTrackerId,
  const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	return dbConfig->deleteIncidentTracker(incidentTrackerId, privilege);
}

DataStoreVector UnifiedDataStore::getDataStoreVector(void)
{
	return m_impl->getDataStoreVector();
}

DataStorePtr UnifiedDataStore::getDataStore(const ServerIdType &serverId)
{
	return m_impl->getDataStore(serverId);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
