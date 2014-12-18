/*
 * Copyright (C) 2014 Project Hatohol
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

#include <errno.h>
#include <semaphore.h>
#include <ReadWriteLock.h>
#include <Reaper.h>
#include <SmartTime.h>
#include "ItemFetchWorker.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

struct ItemFetchWorker::Impl
{
	const static size_t      maxRunningArms    = 8;
	const static timespec    minUpdateInterval;

	ReadWriteLock   rwlock;
	DataStoreVector updateArmsQueue;
	size_t          remainingArmsCount;
	SmartTime       nextAllowedUpdateTime;
	sem_t           updatedSemaphore;
	Signal          itemFetchedSignal;

	Impl(void)
	: remainingArmsCount(0)
	{
		sem_init(&updatedSemaphore, 0, 0);
	}

	virtual ~Impl()
	{
		sem_destroy(&updatedSemaphore);
	};
};

const timespec ItemFetchWorker::Impl::minUpdateInterval = {10, 0};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemFetchWorker::ItemFetchWorker(void)
: m_impl(new Impl())
{
}

ItemFetchWorker::~ItemFetchWorker()
{
}

bool ItemFetchWorker::start(
  const ServerIdType &targetServerId, ClosureBase *closure)
{
	DataStoreVector allDataStores =
	  UnifiedDataStore::getInstance()->getDataStoreVector();

	if (allDataStores.empty())
		return false;

	m_impl->rwlock.writeLock();
	if (closure)
		m_impl->itemFetchedSignal.connect(closure);
	m_impl->remainingArmsCount = allDataStores.size();
	for (size_t i = 0; i < allDataStores.size(); i++) {
		DataStore *dataStore = allDataStores[i];

		bool shouldWake = true;
		ArmBase &arm = dataStore->getArmBase();
		if (targetServerId != ALL_SERVERS &&
		    targetServerId != arm.getServerInfo().id)
			shouldWake = false;
		else if (!arm.isFetchItemsSupported())
			shouldWake = false;

		if (!shouldWake) {
			m_impl->remainingArmsCount--;
			dataStore->unref();
			continue;
		}

		if (i < Impl::maxRunningArms)
			wakeArm(dataStore);
		else
			m_impl->updateArmsQueue.push_back(dataStore);
	}

	bool started = m_impl->remainingArmsCount > 0;
	m_impl->rwlock.unlock();

	return started;
}

bool ItemFetchWorker::updateIsNeeded(void)
{
	m_impl->rwlock.readLock();
	Reaper<ReadWriteLock> lockReaper(&m_impl->rwlock, ReadWriteLock::unlock);

	if (m_impl->remainingArmsCount > 0)
		return false;

	SmartTime currTime(SmartTime::INIT_CURR_TIME);
	return currTime >= m_impl->nextAllowedUpdateTime;
}

void ItemFetchWorker::waitCompletion(void)
{
	if (sem_wait(&m_impl->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_wait: %d\n", errno);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ItemFetchWorker::updatedCallback(ClosureBase *closure)
{
	m_impl->rwlock.writeLock();
	Reaper<ReadWriteLock> lockReaper(&m_impl->rwlock, ReadWriteLock::unlock);

	DataStoreVector &updateArmsQueue = m_impl->updateArmsQueue;
	if (!updateArmsQueue.empty()) {
		wakeArm(updateArmsQueue.front());
		updateArmsQueue.erase(updateArmsQueue.begin());
	}

	m_impl->remainingArmsCount--;
	if (m_impl->remainingArmsCount > 0)
		return;

	if (sem_post(&m_impl->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
	m_impl->nextAllowedUpdateTime.setCurrTime();
	m_impl->nextAllowedUpdateTime += m_impl->minUpdateInterval;
	m_impl->itemFetchedSignal();
	m_impl->itemFetchedSignal.clear();
}

void ItemFetchWorker::wakeArm(DataStore *dataStore)
{
	struct ClosureWithDataStore : public Closure<ItemFetchWorker>
	{
		DataStore *dataStore;

		ClosureWithDataStore(ItemFetchWorker *impl, DataStore *ds)
		: Closure<ItemFetchWorker>(impl,
		                           &ItemFetchWorker::updatedCallback),
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

