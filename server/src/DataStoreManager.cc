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

#include "DataStoreManager.h"
#include <MutexLock.h>
#include <Reaper.h>
using namespace std;
using namespace mlpl;

typedef map<uint32_t, DataStore*> DataStoreMap;
typedef DataStoreMap::iterator    DataStoreMapIterator;

// ---------------------------------------------------------------------------
// DataStoreEventProc
// ---------------------------------------------------------------------------
DataStoreEventProc::DataStoreEventProc()
{
}

DataStoreEventProc::~DataStoreEventProc()
{
}

void DataStoreEventProc::onAdded(DataStore *dataStore)
{
}

void DataStoreEventProc::onRemoved(DataStore *dataStore)
{
}

// ---------------------------------------------------------------------------
// DataStoreManager
// ---------------------------------------------------------------------------
struct DataStoreManager::PrivateContext {
	// Elements in dataStoreMap and dataStoreVector are the same.
	// So it's only necessary to free elements in one.
	DataStoreMap    dataStoreMap;
	MutexLock mutex;
	DataStoreEventProcList eventProcList;
	ReadWriteLock          eventProcListLock;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreManager::DataStoreManager(void)
{
	m_ctx = new PrivateContext;
}

DataStoreManager::~DataStoreManager()
{
	closeAllStores();
	if (m_ctx)
		delete m_ctx;
}

void DataStoreManager::passCommandLineArg(const CommandLineArg &cmdArg)
{
}

void DataStoreManager::registEventProc(DataStoreEventProc *eventProc)
{
	m_ctx->eventProcListLock.writeLock();
	m_ctx->eventProcList.push_back(eventProc);
	m_ctx->eventProcListLock.unlock();
}

bool DataStoreManager::hasDataStore(uint32_t storeId)
{
	m_ctx->mutex.lock();
	bool found =
	   m_ctx->dataStoreMap.find(storeId) != m_ctx->dataStoreMap.end();
	m_ctx->mutex.unlock();
	return found;
}

bool DataStoreManager::add(uint32_t storeId, DataStore *dataStore)
{
	m_ctx->mutex.lock();
	Reaper<MutexLock> lockReaper(&m_ctx->mutex, MutexLock::unlock);
	pair<DataStoreMapIterator, bool> result =
	  m_ctx->dataStoreMap.insert
	    (pair<uint32_t, DataStore *>(storeId, dataStore));

	const bool successed = result.second;
	if (!successed)
		return false;
	dataStore->ref();

	callAddedHandlers(dataStore);

	return true;
}

void DataStoreManager::remove(uint32_t storeId)
{
	DataStore *dataStore = NULL;

	m_ctx->mutex.lock();
	DataStoreMapIterator it = m_ctx->dataStoreMap.find(storeId);
	if (it != m_ctx->dataStoreMap.end()) {
		dataStore = it->second;
		m_ctx->dataStoreMap.erase(it);
	}
	m_ctx->mutex.unlock();

	if (!dataStore) {
		MLPL_WARN("Not found: storeId: %"PRIu32"\n", storeId);
		return;
	}

	callRemovedHandlers(dataStore);
	dataStore->unref();
}


DataStoreVector DataStoreManager::getDataStoreVector(void)
{
	m_ctx->mutex.lock();
	DataStoreVector dataStoreVector;
	DataStoreMapIterator it = m_ctx->dataStoreMap.begin();
	for (; it != m_ctx->dataStoreMap.end(); ++it) {
		DataStore *dataStore = it->second;
		dataStore->ref();
		dataStoreVector.push_back(dataStore);
	}
	m_ctx->mutex.unlock();

	return dataStoreVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DataStoreManager::closeAllStores(void)
{
	m_ctx->mutex.lock();
	DataStoreMapIterator it = m_ctx->dataStoreMap.begin();
	for (; it != m_ctx->dataStoreMap.end(); ++it) {
		DataStore *dataStore = it->second;
		callRemovedHandlers(dataStore);
		dataStore->unref();
	}
	m_ctx->dataStoreMap.clear();
	m_ctx->mutex.unlock();
}

void DataStoreManager::callAddedHandlers(DataStore *dataStore)
{
	m_ctx->eventProcListLock.readLock();
	Reaper<ReadWriteLock> unlocker(&m_ctx->eventProcListLock,
	                               ReadWriteLock::unlock);
	DataStoreEventProcListIterator evtProc = m_ctx->eventProcList.begin();
	for (; evtProc != m_ctx->eventProcList.end(); ++evtProc)
		(*evtProc)->onAdded(dataStore);
}

void DataStoreManager::callRemovedHandlers(DataStore *dataStore)
{
	m_ctx->eventProcListLock.readLock();
	Reaper<ReadWriteLock> unlocker(&m_ctx->eventProcListLock,
	                               ReadWriteLock::unlock);
	DataStoreEventProcListIterator evtProc = m_ctx->eventProcList.begin();
	for (; evtProc != m_ctx->eventProcList.end(); ++evtProc)
		(*evtProc)->onRemoved(dataStore);
}
