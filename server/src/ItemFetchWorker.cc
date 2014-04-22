/*
 * Copyright (C) 2014 Project Hatohol
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

#include <errno.h>
#include <semaphore.h>
#include <ReadWriteLock.h>
#include <Reaper.h>
#include <SmartTime.h>
#include "ItemFetchWorker.h"
#include "UnifiedDataStore.h"
#include "VirtualDataStore.h"

using namespace std;
using namespace mlpl;

struct ItemFetchWorker::PrivateContext
{
	const static size_t      maxRunningArms    = 8;
	const static timespec    minUpdateInterval;

	ReadWriteLock   rwlock;
	DataStoreVector updateArmsQueue;
	size_t          remainingArmsCount;
	SmartTime       nextAllowedUpdateTime;
	sem_t           updatedSemaphore;
	Signal          itemFetchedSignal;

	PrivateContext(void)
	: remainingArmsCount(0)
	{
		sem_init(&updatedSemaphore, 0, 0);
	}

	virtual ~PrivateContext()
	{
		sem_destroy(&updatedSemaphore);
	};
};

const timespec ItemFetchWorker::PrivateContext::minUpdateInterval = {10, 0};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemFetchWorker::ItemFetchWorker(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ItemFetchWorker::~ItemFetchWorker()
{
	if (m_ctx)
		delete m_ctx;
}

bool ItemFetchWorker::start(
  const ServerIdType &targetServerId, ClosureBase *closure)
{
	DataStoreVector allDataStores =
	  UnifiedDataStore::getInstance()->getDataStoreVector();

	if (allDataStores.empty())
		return false;

	m_ctx->rwlock.writeLock();
	if (closure)
		m_ctx->itemFetchedSignal.connect(closure);
	m_ctx->remainingArmsCount = allDataStores.size();
	for (size_t i = 0; i < allDataStores.size(); i++) {
		DataStore *dataStore = allDataStores[i];

		if (targetServerId != ALL_SERVERS) {
			ArmBase &arm = dataStore->getArmBase();
			if (targetServerId != arm.getServerInfo().id) {
				m_ctx->remainingArmsCount--;
				dataStore->unref();
				continue;
			}
		}

		if (i < PrivateContext::maxRunningArms)
			wakeArm(dataStore);
		else
			m_ctx->updateArmsQueue.push_back(dataStore);
	}

	bool started = m_ctx->remainingArmsCount > 0;
	m_ctx->rwlock.unlock();

	return started;
}

bool ItemFetchWorker::updateIsNeeded(void)
{
	m_ctx->rwlock.readLock();
	Reaper<ReadWriteLock> lockReaper(&m_ctx->rwlock, ReadWriteLock::unlock);

	if (m_ctx->remainingArmsCount > 0)
		return false;

	SmartTime currTime(SmartTime::INIT_CURR_TIME);
	return currTime >= m_ctx->nextAllowedUpdateTime;
}

void ItemFetchWorker::waitCompletion(void)
{
	if (sem_wait(&m_ctx->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_wait: %d\n", errno);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ItemFetchWorker::updatedCallback(ClosureBase *closure)
{
	m_ctx->rwlock.writeLock();
	Reaper<ReadWriteLock> lockReaper(&m_ctx->rwlock, ReadWriteLock::unlock);

	DataStoreVector &updateArmsQueue = m_ctx->updateArmsQueue;
	if (!updateArmsQueue.empty()) {
		wakeArm(updateArmsQueue.front());
		updateArmsQueue.erase(updateArmsQueue.begin());
	}

	m_ctx->remainingArmsCount--;
	if (m_ctx->remainingArmsCount > 0)
		return;

	if (sem_post(&m_ctx->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
	m_ctx->nextAllowedUpdateTime.setCurrTime();
	m_ctx->nextAllowedUpdateTime += m_ctx->minUpdateInterval;
	m_ctx->itemFetchedSignal();
	m_ctx->itemFetchedSignal.clear();
}

void ItemFetchWorker::wakeArm(DataStore *dataStore)
{
	struct ClosureWithDataStore : public Closure<ItemFetchWorker>
	{
		DataStore *dataStore;

		ClosureWithDataStore(ItemFetchWorker *ctx, DataStore *ds)
		: Closure<ItemFetchWorker>(ctx,
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

