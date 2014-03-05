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
#include "ItemFetchWorker.h"
#include "UnifiedDataStore.h"
#include "VirtualDataStore.h"

using namespace std;
using namespace mlpl;

struct ItemFetchWorker::PrivateContext
{
	const static size_t      maxRunningArms    = 8;
	const static time_t      minUpdateInterval = 10;

	ReadWriteLock   rwlock;
	DataStoreVector updateArmsQueue;
	size_t          remainingArmsCount;
	timespec        lastUpdateTime;
	sem_t           updatedSemaphore;
	Signal          itemFetchedSignal;

	PrivateContext(void)
	: remainingArmsCount(0)
	{
		lastUpdateTime.tv_sec  = 0;
		lastUpdateTime.tv_nsec = 0;
		sem_init(&updatedSemaphore, 0, 0);
	}

	virtual ~PrivateContext()
	{
		sem_destroy(&updatedSemaphore);
	};
};

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

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	m_ctx->rwlock.readLock();
	uds->virtualDataStoreForeach(&collector);
	m_ctx->rwlock.unlock();

	ArmBaseVector &arms = collector.arms;
	if (arms.empty())
		return false;

	m_ctx->rwlock.writeLock();
	if (closure)
	m_ctx->itemFetchedSignal.connect(closure);
	m_ctx->remainingArmsCount = arms.size();
	for (size_t i = 0; i < collector.allDataStores.size(); i++) {
		DataStore *dataStore = collector.allDataStores[i];

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
	bool shouldUpdate = true;

	m_ctx->rwlock.readLock();

	if (m_ctx->remainingArmsCount > 0) {
		shouldUpdate = false;
	} else {
		timespec ts;
		const time_t banLiftTime =
		   m_ctx->lastUpdateTime.tv_sec + m_ctx->minUpdateInterval;
		if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
			MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		} else if (ts.tv_sec < banLiftTime) {
			shouldUpdate = false;
		}
	}

	m_ctx->rwlock.unlock();

	return shouldUpdate;
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

	DataStoreVector &updateArmsQueue = m_ctx->updateArmsQueue;
	if (!updateArmsQueue.empty()) {
		wakeArm(updateArmsQueue.front());
		updateArmsQueue.erase(updateArmsQueue.begin());
	}

	m_ctx->remainingArmsCount--;
	if (m_ctx->remainingArmsCount == 0) {
		if (sem_post(&m_ctx->updatedSemaphore) == -1)
			MLPL_ERR("Failed to call sem_post: %d\n", errno);
		if (clock_gettime(CLOCK_REALTIME, &m_ctx->lastUpdateTime) == -1)
			MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		m_ctx->itemFetchedSignal();
		m_ctx->itemFetchedSignal.clear();
	}

	m_ctx->rwlock.unlock();
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

