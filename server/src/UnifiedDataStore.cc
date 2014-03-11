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
#include <MutexLock.h>
#include <AtomicValue.h>
#include <Reaper.h>
#include "UnifiedDataStore.h"
#include "VirtualDataStoreZabbix.h"
#include "VirtualDataStoreNagios.h"
#include "VirtualDataStoreFake.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
#include "ActionManager.h"
#include "CacheServiceDBClient.h"
#include "ItemFetchWorker.h"
using namespace std;
using namespace mlpl;

typedef map<ServerIdType, DataStore *> ServerIdDataStoreMap;
typedef ServerIdDataStoreMap::iterator ServerIdDataStoreMapIterator;

// ---------------------------------------------------------------------------
// UnifiedDataStore
// ---------------------------------------------------------------------------
struct UnifiedDataStore::PrivateContext
{
	struct UnifiedDataStoreEventProc : public DataStoreEventProc
	{
		PrivateContext *ctx;
		const AtomicValue<bool> &enableCopyOnDemand;

		UnifiedDataStoreEventProc(
		  PrivateContext *_ctx,
		  const AtomicValue<bool> &copyOnDemand)
		: ctx(_ctx),
		  enableCopyOnDemand(copyOnDemand)
		{
		}

		virtual ~UnifiedDataStoreEventProc()
		{
		}

		virtual void onAdded(DataStore *dataStore) // override
		{
			dataStore->setCopyOnDemandEnable(enableCopyOnDemand);
			ctx->addToDataStoreMap(dataStore);
		}

		virtual void onRemoved(DataStore *dataStore) // override
		{
			ctx->removeFromDataStoreMap(dataStore);
		}
	};

	static UnifiedDataStore *instance;
	static MutexLock         mutex;

	ReadWriteLock            serverIdDataStoreMapLock;
	ServerIdDataStoreMap     serverIdDataStoreMap;
	AtomicValue<bool>        isCopyOnDemandEnabled;
	ItemFetchWorker          itemFetchWorker;

	PrivateContext()
	: isCopyOnDemandEnabled(false)
	{ 
		virtualDataStoreMap[MONITORING_SYSTEM_FAKE] =
		  VirtualDataStore::getInstance(MONITORING_SYSTEM_FAKE);

		virtualDataStoreMap[MONITORING_SYSTEM_ZABBIX] =
		  VirtualDataStore::getInstance(MONITORING_SYSTEM_ZABBIX);

		virtualDataStoreMap[MONITORING_SYSTEM_NAGIOS] =
		  VirtualDataStore::getInstance(MONITORING_SYSTEM_NAGIOS);

		// TODO: When should the object be freed ?
		UnifiedDataStoreEventProc *evtProc =
		  new PrivateContext::UnifiedDataStoreEventProc(
		    this, isCopyOnDemandEnabled);

		// make a list
		VirtualDataStoreMapIterator it = virtualDataStoreMap.begin();
		for (; it != virtualDataStoreMap.end(); ++it) {
			VirtualDataStore *virtDataStore = it->second;
			virtDataStore->registEventProc(evtProc);
			virtualDataStoreList.push_back(virtDataStore);
		}
	};

	void virtualDataStoreForeach(VirtualDataStoreForeachProc *vdsProc)
	{
		// We assume that virtualDataStoreList is not changed
		// after the initialization. So we don't need to take a lock.
		VirtualDataStoreListIterator it = virtualDataStoreList.begin();
		for (; it != virtualDataStoreList.end(); ++it) {
			bool breakFlag = (*vdsProc)(*it);
			if (breakFlag)
				break;
		}
	}

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
		  "Failed to insert: ServerID: %"FMT_SERVER_ID", DataStore: %p",
		  serverId, dataStore);
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
		  "Failed to found: ServerID: %"FMT_SERVER_ID", DataStore: %p",
		  serverId, dataStore);
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

	VirtualDataStore *
	  findVirtualDataStore(const MonitoringSystemType &type)
	{
		VirtualDataStoreMapIterator it = virtualDataStoreMap.find(type);
		if (it == virtualDataStoreMap.end())
			return NULL;
		return it->second;
	}

	HatoholError startDataStore(const MonitoringServerInfo &svInfo,
	                            const bool &autoRun)
	{
		VirtualDataStore *virtDataStore =
		  findVirtualDataStore(svInfo.type);
		if (!virtDataStore)
			return HTERR_INVALID_MONITORING_SYSTEM_TYPE;
		return virtDataStore->start(svInfo, autoRun);
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
		  "svInfo.id: %"FMT_SERVER_ID", serverId: %"FMT_SERVER_ID, 
		  svInfo.id, serverId);

		if (isRunning) {
			ArmInfo armInfo = armBase.getArmStatus().getArmInfo();
			*isRunning = armInfo.running;
		}

		VirtualDataStore *virtDataStore =
		  findVirtualDataStore(svInfo.type);
		if (!virtDataStore)
			return HTERR_INVALID_MONITORING_SYSTEM_TYPE;
		return virtDataStore->stop(serverId);
	}

private:
	// We assume that the following list is initialized once at
	// the constructor. This is a limitation to realize a lock-free
	// structure.
	VirtualDataStoreList virtualDataStoreList;
	VirtualDataStoreMap  virtualDataStoreMap;
};

UnifiedDataStore *UnifiedDataStore::PrivateContext::instance = NULL;
MutexLock UnifiedDataStore::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

UnifiedDataStore::~UnifiedDataStore()
{
	if (m_ctx)
		delete m_ctx;
}

void UnifiedDataStore::reset(void)
{
	stop();
}

void UnifiedDataStore::parseCommandLineArgument(CommandLineArg &cmdArg)
{
	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--enable-copy-on-demand")
			setCopyOnDemandEnabled(true);
		else if (cmd == "--disable-copy-on-demand")
			setCopyOnDemandEnabled(false);
	}
}

UnifiedDataStore *UnifiedDataStore::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::mutex.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new UnifiedDataStore();
	PrivateContext::mutex.unlock();

	return PrivateContext::instance;
}

void UnifiedDataStore::start(const bool &autoRun)
{
	struct : public VirtualDataStoreForeachProc
	{
		bool autoRun;
		virtual bool operator()(VirtualDataStore *virtDataStore)
		{
			virtDataStore->start(autoRun);
			return false;
		}
	} starter;
	starter.autoRun = autoRun;
	m_ctx->virtualDataStoreForeach(&starter);
}

void UnifiedDataStore::stop(void)
{
	struct : public VirtualDataStoreForeachProc
	{
		virtual bool operator()(VirtualDataStore *virtDataStore)
		{
			virtDataStore->stop();
			return false;
		}
	} stopper;

	m_ctx->virtualDataStoreForeach(&stopper);
}

void UnifiedDataStore::fetchItems(const ServerIdType &targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return;
	if (!m_ctx->itemFetchWorker.updateIsNeeded())
		return;

	bool started = m_ctx->itemFetchWorker.start(targetServerId, NULL);
	if (!started)
		return;

	m_ctx->itemFetchWorker.waitCompletion();
}

void UnifiedDataStore::getTriggerList(TriggerInfoList &triggerList,
				      const TriggersQueryOption &option)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList, option);
}

HatoholError UnifiedDataStore::getEventList(EventInfoList &eventList,
					    EventsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getEventInfoList(eventList, option);
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
	if (!m_ctx->itemFetchWorker.updateIsNeeded())
		return false;

	return m_ctx->itemFetchWorker.start(targetServerId, closure);
}

void UnifiedDataStore::getHostList(HostInfoList &hostInfoList,
				   const HostsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getHostInfoList(hostInfoList, option);
}

HatoholError UnifiedDataStore::getActionList(
  ActionDefList &actionList, const OperationPrivilege &privilege)
{
	DBClientAction dbAction;
	return dbAction.getActionList(actionList, privilege);
}

HatoholError UnifiedDataStore::deleteActionList(
  const ActionIdList &actionIdList, const OperationPrivilege &privilege)
{
	DBClientAction dbAction;
	return dbAction.deleteActions(actionIdList, privilege);
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

size_t UnifiedDataStore::getNumberOfTriggers(const TriggersQueryOption &option,
					     TriggerSeverityType severity)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfTriggers(option, severity);
}

size_t UnifiedDataStore::getNumberOfGoodHosts(const HostsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfGoodHosts(option);
}

size_t UnifiedDataStore::getNumberOfBadHosts(const HostsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfBadHosts(option);
}

bool UnifiedDataStore::getCopyOnDemandEnabled(void) const
{
	return m_ctx->isCopyOnDemandEnabled;
}

void UnifiedDataStore::setCopyOnDemandEnabled(bool enable)
{
	m_ctx->isCopyOnDemandEnabled = enable;
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
  MonitoringServerInfoList &monitoringServers, ServerQueryOption &option)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	dbConfig->getTargetServers(monitoringServers, option);
}

HatoholError UnifiedDataStore::addTargetServer(
  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege,
  const bool &autoRun)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->addTargetServer(&svInfo, privilege);
	if (err != HTERR_OK)
		return err;

	return m_ctx->startDataStore(svInfo, autoRun);
}

HatoholError UnifiedDataStore::updateTargetServer(
  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->updateTargetServer(&svInfo, privilege);
	if (err != HTERR_OK)
		return err;

	bool isRunning = false;
	err = m_ctx->stopDataStore(svInfo.id, &isRunning);
	if (err != HTERR_OK)
		return err;
	return m_ctx->startDataStore(svInfo, isRunning);
}

HatoholError UnifiedDataStore::deleteTargetServer(
  const ServerIdType &serverId, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->deleteTargetServer(serverId, privilege);
	if (err != HTERR_OK)
		return err;

	return m_ctx->stopDataStore(serverId);
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
		svConnStat.armInfo =
		   dataStorePtr->getArmBase().getArmStatus().getArmInfo();
		svConnStatVec.push_back(svConnStat);
	}
}

void UnifiedDataStore::virtualDataStoreForeach(
  VirtualDataStoreForeachProc *vdsProc)
{
	m_ctx->virtualDataStoreForeach(vdsProc);
}

DataStorePtr UnifiedDataStore::getDataStore(const ServerIdType &serverId)
{
	return m_ctx->getDataStore(serverId);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
