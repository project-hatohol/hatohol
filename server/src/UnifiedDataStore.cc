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

#include <semaphore.h>
#include <errno.h>
#include <stdexcept>
#include <MutexLock.h>
#include "UnifiedDataStore.h"
#include "VirtualDataStoreZabbix.h"
#include "VirtualDataStoreNagios.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
#include "ActionManager.h"
#include "CacheServiceDBClient.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// UnifiedDataStoreEventProc
// ---------------------------------------------------------------------------
struct UnifiedDataStoreEventProc : public DataStoreEventProc
{
	bool enableCopyOnDemand;

	UnifiedDataStoreEventProc(bool copyOnDemand)
	: enableCopyOnDemand(copyOnDemand)
	{
	}

	virtual ~UnifiedDataStoreEventProc()
	{
	}

	virtual void onAdded(DataStore *dataStore)
	{
		dataStore->setCopyOnDemandEnable(enableCopyOnDemand);
	}
};


// ---------------------------------------------------------------------------
// UnifiedDataStore
// ---------------------------------------------------------------------------
struct UnifiedDataStore::PrivateContext
{
	const static size_t      maxRunningArms    = 8;
	const static time_t      minUpdateInterval = 10;
	static UnifiedDataStore *instance;
	static MutexLock         mutex;

	bool          isCopyOnDemandEnabled;
	sem_t         updatedSemaphore;
	ReadWriteLock rwlock;
	timespec      lastUpdateTime;
	size_t        remainingArmsCount;
	DataStoreVector updateArmsQueue;

	Signal        itemFetchedSignal;

	PrivateContext()
	: isCopyOnDemandEnabled(false),
	  remainingArmsCount(0)
	{
		sem_init(&updatedSemaphore, 0, 0);
		lastUpdateTime.tv_sec  = 0;
		lastUpdateTime.tv_nsec = 0;

		virtualDataStoreList.push_back(
		  VirtualDataStoreZabbix::getInstance());
		virtualDataStoreList.push_back(
		  VirtualDataStoreNagios::getInstance());
	};

	virtual ~PrivateContext()
	{
		sem_destroy(&updatedSemaphore);
	};

	void wakeArm(DataStore *dataStore)
	{
		struct ClosureWithDataStore : public Closure<PrivateContext>
		{
			DataStore *dataStore;

			ClosureWithDataStore(PrivateContext *ctx, DataStore *ds)
			: Closure<PrivateContext>(
			    ctx, &PrivateContext::updatedCallback),
			  dataStore(ds)
			{
			}

			~ClosureWithDataStore()
			{
				dataStore->unref();
			}
		};

		ArmBase &arm = dataStore->getArmBase();
		arm.fetchItems(new ClosureWithDataStore(this, dataStore));
	}

	bool updateIsNeeded(void)
	{
		bool shouldUpdate = true;

		rwlock.readLock();

		if (remainingArmsCount > 0) {
			shouldUpdate = false;
		} else {
			timespec ts;
			time_t banLiftTime
			  = lastUpdateTime.tv_sec + minUpdateInterval;
			if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
				MLPL_ERR("Failed to call clock_gettime: %d\n",
					 errno);
			} else if (ts.tv_sec < banLiftTime) {
				shouldUpdate = false;
			}
		}

		rwlock.unlock();

		return shouldUpdate;
	}

	void updatedCallback(ClosureBase *closure)
	{
		rwlock.writeLock();

		if (!updateArmsQueue.empty()) {
			wakeArm(updateArmsQueue.front());
			updateArmsQueue.erase(updateArmsQueue.begin());
		}

		remainingArmsCount--;
		if (remainingArmsCount == 0) {
			if (sem_post(&updatedSemaphore) == -1)
				MLPL_ERR("Failed to call sem_post: %d\n",
					 errno);
			if (clock_gettime(CLOCK_REALTIME, &lastUpdateTime) == -1)
				MLPL_ERR("Failed to call clock_gettime: %d\n",
					 errno);
			itemFetchedSignal();
			itemFetchedSignal.clear();
		}

		rwlock.unlock();
	}

	bool startFetchingItems(
	  const ServerIdType &targetServerId = ALL_SERVERS,
	  ClosureBase *closure = NULL)
	{
		// TODO: Make the design smart
		struct : public VirtualDataStoreForeachProc
		{
			ArmBaseVector arms;
			DataStoreVector allDataStores;
			virtual bool operator()(VirtualDataStore *virtDataStore)
			{
				DataStoreVector stores =
				  virtDataStore->getDataStoreVector();
				for (size_t i = 0; i < stores.size(); i++) {
					DataStore *dataStore = stores[i];
					allDataStores.push_back(dataStore);
					arms.push_back(&dataStore->getArmBase());
				}
				return false;
			}
		} collector;

		rwlock.readLock();
		virtualDataStoreForeach(&collector);
		rwlock.unlock();

		ArmBaseVector &arms = collector.arms;
		if (arms.empty())
			return false;

		rwlock.writeLock();
		if (closure)
			itemFetchedSignal.connect(closure);
		remainingArmsCount = arms.size();
		for (size_t i = 0; i < collector.allDataStores.size(); i++) {
			DataStore *dataStore = collector.allDataStores[i];

			if (targetServerId != ALL_SERVERS) {
				ArmBase &arm = dataStore->getArmBase();
				if (targetServerId != arm.getServerInfo().id) {
					remainingArmsCount--;
					dataStore->unref();
					continue;
				}
			}

			if (i < PrivateContext::maxRunningArms)
				wakeArm(dataStore);
			else
				updateArmsQueue.push_back(dataStore);
		}

		bool started = remainingArmsCount > 0;

		rwlock.unlock();

		return started;
	}

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

private:
	// We assume that the following list is initialized once at
	// the constructor. This is a limitation to realize a lock-free
	// structure.
	VirtualDataStoreList virtualDataStoreList;
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

void UnifiedDataStore::start(void)
{
	struct : public VirtualDataStoreForeachProc
	{
		UnifiedDataStoreEventProc *evtProc;
		virtual bool operator()(VirtualDataStore *virtDataStore)
		{
			virtDataStore->registEventProc(evtProc);
			virtDataStore->start();
			return false;
		}
	} starter;

	starter.evtProc =
	   new UnifiedDataStoreEventProc(m_ctx->isCopyOnDemandEnabled);
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
	if (!m_ctx->updateIsNeeded())
		return;

	bool started = m_ctx->startFetchingItems(targetServerId, NULL);
	if (!started)
		return;

	if (sem_wait(&m_ctx->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_wait: %d\n", errno);
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
	if (!m_ctx->updateIsNeeded())
		return false;

	return m_ctx->startFetchingItems(targetServerId, closure);
}

void UnifiedDataStore::getHostgroupList(HostgroupInfoList &hostgroupInfoList,
                                        const HostgroupsQueryOption &option)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getHostgroupInfoList(hostgroupInfoList, option);
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
  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->addTargetServer(&svInfo, privilege);
	if (err != HTERR_OK)
		return err;

	struct : public VirtualDataStoreForeachProc {
		MonitoringServerInfo *svInfo;
		virtual bool operator()(VirtualDataStore *virtDataStore) {
			bool started = virtDataStore->start(*svInfo);
			bool breakFlag = started;
			return breakFlag;
		}
	} starter;
	starter.svInfo = &svInfo;
	m_ctx->virtualDataStoreForeach(&starter);
	return err;
}

HatoholError UnifiedDataStore::updateTargetServer(
  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->updateTargetServer(&svInfo, privilege);
	if (err != HTERR_OK)
		return err;

	struct : public VirtualDataStoreForeachProc {
		MonitoringServerInfo *svInfo;
		virtual bool operator()(VirtualDataStore *virtDataStore) {
			bool stopped = virtDataStore->stop(svInfo->id);
			if (!stopped)
				return false;
			bool started = virtDataStore->start(*svInfo);
			bool breakFlag = started;
			return breakFlag;
		}
	} restarter;
	restarter.svInfo = &svInfo;
	m_ctx->virtualDataStoreForeach(&restarter);
	return err;
}

HatoholError UnifiedDataStore::deleteTargetServer(
  const ServerIdType &serverId, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	HatoholError err = dbConfig->deleteTargetServer(serverId, privilege);
	if (err != HTERR_OK)
		return err;

	struct : public VirtualDataStoreForeachProc {
		ServerIdType serverId;
		virtual bool operator()(VirtualDataStore *virtDataStore) {
			bool stopped = virtDataStore->stop(serverId);
			bool breakFlag = stopped;
			return breakFlag;
		}
	} stopper;
	stopper.serverId = serverId;
	m_ctx->virtualDataStoreForeach(&stopper);
	return err;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
