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
#include "ActionManager.h"
#include "CacheServiceDBClient.h"

using namespace mlpl;

struct UnifiedDataStore::PrivateContext
{
	const static size_t      maxRunningArms    = 8;
	const static time_t      minUpdateInterval = 10;
	static UnifiedDataStore *instance;
	static MutexLock         mutex;

	VirtualDataStoreZabbix *vdsZabbix;
	VirtualDataStoreNagios *vdsNagios;

	bool          isCopyOnDemandEnabled;
	sem_t         updatedSemaphore;
	ReadWriteLock rwlock;
	timespec      lastUpdateTime;
	size_t        remainingArmsCount;
	ArmBaseVector updateArmsQueue;

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

	void wakeArm(ArmBase *arm)
	{
		Closure<PrivateContext> *closure =
		  new Closure<PrivateContext>(
		    this, &PrivateContext::updatedCallback);
		arm->fetchItems(closure);
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

	void updatedCallback(void)
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

	void startFetchingItems(uint32_t targetServerId = ALL_SERVERS,
				ClosureBase *closure = NULL)
	{
		ArmBaseVector arms;
		rwlock.readLock();
		vdsZabbix->collectArms(arms);
		vdsNagios->collectArms(arms);
		rwlock.unlock();
		if (arms.empty())
			return;

		rwlock.writeLock();
		remainingArmsCount = arms.size();
		ArmBaseVectorIterator arms_it = arms.begin();
		for (size_t i = 0; arms_it != arms.end(); i++, arms_it++) {
			ArmBase *arm = *arms_it;

			if (targetServerId != ALL_SERVERS) {
				const MonitoringServerInfo &info
					= arm->getServerInfo();
				if (static_cast<int>(targetServerId) != info.id) {
					remainingArmsCount--;
					continue;
				}
			}

			if (i < PrivateContext::maxRunningArms) {
				wakeArm(arm);
			} else {
				updateArmsQueue.push_back(arm);
			}
		}
		rwlock.unlock();
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
	m_ctx->vdsZabbix = VirtualDataStoreZabbix::getInstance();
	m_ctx->vdsNagios = VirtualDataStoreNagios::getInstance();
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
	m_ctx->vdsZabbix->start();
	m_ctx->vdsNagios->start();
	setCopyOnDemandEnabled(m_ctx->isCopyOnDemandEnabled);
}

void UnifiedDataStore::stop(void)
{
	m_ctx->vdsZabbix->stop();
	m_ctx->vdsNagios->stop();
}

void UnifiedDataStore::fetchItems(uint32_t targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return;
	if (!m_ctx->updateIsNeeded())
		return;

	m_ctx->startFetchingItems(targetServerId, NULL);

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
	fetchItems(targetServerId);
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList, targetServerId);
}

void UnifiedDataStore::getItemListAsync(ClosureBase *closure,
					uint32_t targetServerId)
{
	if (!getCopyOnDemandEnabled())
		return;
	if (!m_ctx->updateIsNeeded())
		return;

	m_ctx->startFetchingItems(targetServerId, closure);
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

	ArmBaseVector arms;
	m_ctx->vdsZabbix->collectArms(arms);
	m_ctx->vdsNagios->collectArms(arms);

	ArmBaseVectorIterator it = arms.begin();
	for (; it != arms.end(); it++) {
		ArmBase *arm = *it;
		arm->setCopyOnDemandEnabled(enable);
	}
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

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
