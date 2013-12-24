/*
 * Copyright (C) 2013 Project Hatohol
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

	VirtualDataStoreList virtualDataStoreList;

	bool          isCopyOnDemandEnabled;
	sem_t         updatedSemaphore;
	ReadWriteLock rwlock;
	timespec      lastUpdateTime;
	size_t        remainingArmsCount;
	DataStoreVector updateArmsQueue;

	Signal        itemFetchedSignal;

	PrivateContext()
	: isCopyOnDemandEnabled(false), remainingArmsCount(0)
	{
		sem_init(&updatedSemaphore, 0, 0);
		lastUpdateTime.tv_sec  = 0;
		lastUpdateTime.tv_nsec = 0;
	};

	virtual ~PrivateContext()
	{
		sem_destroy(&updatedSemaphore);
	};

	static ArmBase *getArmBase(DataStore *dataStore)
	{
		// TODO: Make the design smart.
		// We assume each DataStore has one arm.
		ArmBaseVector arms;
		dataStore->collectArms(arms);
		HATOHOL_ASSERT(arms.size() == 1,
		               "arms.size(): %zd", arms.size());
		return arms.front();
	}

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

		ArmBase *arm = getArmBase(dataStore);
		arm->fetchItems(new ClosureWithDataStore(this, dataStore));
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

	bool startFetchingItems(uint32_t targetServerId = ALL_SERVERS,
				ClosureBase *closure = NULL)
	{
		// TODO: Make the design smart
		struct : public VirtualDataStoreForeachProc
		{
			ArmBaseVector arms;
			DataStoreVector allDataStores;
			virtual void operator()(VirtualDataStore *virtDataStore)
			{
				DataStoreVector stores =
				  virtDataStore->getDataStoreVector();
				for (size_t i = 0; i < stores.size(); i++) {
					DataStore *dataStore = stores[i];
					allDataStores.push_back(dataStore);
					arms.push_back(getArmBase(dataStore));
				}
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
			ArmBase *arm = getArmBase(dataStore);

			if (targetServerId != ALL_SERVERS) {
				const MonitoringServerInfo &info
					= arm->getServerInfo();
				if (static_cast<int>(targetServerId) != info.id) {
					remainingArmsCount--;
					dataStore->unref();
					continue;
				}
			}

			if (i < PrivateContext::maxRunningArms) {
				wakeArm(dataStore);
			} else {
				updateArmsQueue.push_back(dataStore);
			}
		}

		bool started = remainingArmsCount > 0;

		rwlock.unlock();

		return started;
	}

	struct VirtualDataStoreForeachProc
	{
		virtual void operator()(VirtualDataStore *virtDataStore) = 0;
	};

	void virtualDataStoreForeach(VirtualDataStoreForeachProc *vdsProc)
	{
		VirtualDataStoreListIterator it = virtualDataStoreList.begin();
		for (; it != virtualDataStoreList.end(); ++it)
			(*vdsProc)(*it);
	}
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
	m_ctx->virtualDataStoreList.push_back(
	  VirtualDataStoreZabbix::getInstance());
	m_ctx->virtualDataStoreList.push_back(
	  VirtualDataStoreNagios::getInstance());
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
	struct : public PrivateContext::VirtualDataStoreForeachProc
	{
		UnifiedDataStoreEventProc *evtProc;
		virtual void operator()(VirtualDataStore *virtDataStore)
		{
			virtDataStore->registEventProc(evtProc);
			virtDataStore->start();
		}
	} starter;

	starter.evtProc =
	   new UnifiedDataStoreEventProc(m_ctx->isCopyOnDemandEnabled);
	m_ctx->virtualDataStoreForeach(&starter);
}

void UnifiedDataStore::stop(void)
{
	struct : public PrivateContext::VirtualDataStoreForeachProc
	{
		virtual void operator()(VirtualDataStore *virtDataStore)
		{
			virtDataStore->stop();
		}
	} stopper;

	m_ctx->virtualDataStoreForeach(&stopper);
}

void UnifiedDataStore::fetchItems(uint32_t targetServerId)
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
                                      uint32_t targetServerId,
                                      uint64_t targetHostId,
                                      uint64_t targetTriggerId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList, targetServerId,
	                             targetHostId, targetTriggerId);
}

HatoholError UnifiedDataStore::getEventList(EventInfoList &eventList,
                                            EventQueryOption &option)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getEventInfoList(eventList, option);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList,
                                   uint32_t targetServerId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList, targetServerId);
}

bool UnifiedDataStore::getItemListAsync(ClosureBase *closure,
					uint32_t targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return false;
	if (!m_ctx->updateIsNeeded())
		return false;

	return m_ctx->startFetchingItems(targetServerId, closure);
}

void UnifiedDataStore::getHostList(
  HostInfoList &hostInfoList, uint32_t targetServerId, uint64_t targetHostId)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getHostInfoList(hostInfoList, targetServerId, targetHostId);
}

void UnifiedDataStore::getActionList(ActionDefList &actionList)
{
	DBClientAction dbAction;
	dbAction.getActionList(actionList);
}

void UnifiedDataStore::deleteActionList(const ActionIdList &actionIdList)
{
	DBClientAction dbAction;
	dbAction.deleteActions(actionIdList);
}

size_t UnifiedDataStore::getNumberOfTriggers(uint32_t serverId,
                                          uint64_t hostGroupId,
                                          TriggerSeverityType severity)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfTriggers(serverId, hostGroupId, severity);
}

size_t UnifiedDataStore::getNumberOfGoodHosts(uint32_t serverId,
                                              uint64_t hostGroupId)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfGoodHosts(serverId, hostGroupId);
}

size_t UnifiedDataStore::getNumberOfBadHosts(uint32_t serverId,
                                             uint64_t hostGroupId)
{
	DBClientHatohol dbHatohol;
	return dbHatohol.getNumberOfBadHosts(serverId, hostGroupId);
}

bool UnifiedDataStore::getCopyOnDemandEnabled(void) const
{
	return m_ctx->isCopyOnDemandEnabled;
}

void UnifiedDataStore::setCopyOnDemandEnabled(bool enable)
{
	m_ctx->isCopyOnDemandEnabled = enable;
}

void UnifiedDataStore::addAction(ActionDef &actionDef)
{
	DBClientAction dbAction;
	dbAction.addAction(actionDef);
}

void UnifiedDataStore::addEventList(const EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	ActionManager actionManager;
	actionManager.checkEvents(eventList);
	dbHatohol.addEventInfoList(eventList);
}

void UnifiedDataStore::getUserList(UserInfoList &userList,
                                   UserQueryOption &option)
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

HatoholError UnifiedDataStore::getAccessInfoMap(ServerAccessInfoMap &srvAccessInfoMap,
					AccessInfoQueryOption &option)
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

HatoholError UnifiedDataStore::addTargetServer(
  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();

	// FIXME: Add OperationPrivilege to signature of addTargetServer()
	HatoholError err = dbConfig->addTargetServer(&svInfo);
	if (err != HTERR_OK)
		return err;

	struct : public PrivateContext::VirtualDataStoreForeachProc {
		MonitoringServerInfo *svInfo;
		virtual void operator()(VirtualDataStore *virtDataStore) {
			virtDataStore->start(*svInfo);
		}
	} starter;
	starter.svInfo = &svInfo;
	m_ctx->virtualDataStoreForeach(&starter);
	return err;
}
// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
