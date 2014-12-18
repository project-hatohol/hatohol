/*
 * Copyright (C) 2013 Project Hatohol
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

#include "DataStoreManager.h"
#include <Mutex.h>
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
struct DataStoreManager::Impl {
	// Elements in dataStoreMap and dataStoreVector are the same.
	// So it's only necessary to free elements in one.
	DataStoreMap    dataStoreMap;
	Mutex                  mutex;
	DataStoreEventProcList eventProcList;
	ReadWriteLock          eventProcListLock;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreManager::DataStoreManager(void)
: m_impl(new Impl)
{
}

DataStoreManager::~DataStoreManager()
{
	closeAllStores();
}

void DataStoreManager::registEventProc(DataStoreEventProc *eventProc)
{
	m_impl->eventProcListLock.writeLock();
	m_impl->eventProcList.push_back(eventProc);
	m_impl->eventProcListLock.unlock();
}

bool DataStoreManager::hasDataStore(uint32_t storeId)
{
	m_impl->mutex.lock();
	bool found =
	   m_impl->dataStoreMap.find(storeId) != m_impl->dataStoreMap.end();
	m_impl->mutex.unlock();
	return found;
}

bool DataStoreManager::add(uint32_t storeId, DataStore *dataStore)
{
	AutoMutex autoMutex(&m_impl->mutex);
	pair<DataStoreMapIterator, bool> result =
	  m_impl->dataStoreMap.insert
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

	m_impl->mutex.lock();
	DataStoreMapIterator it = m_impl->dataStoreMap.find(storeId);
	if (it != m_impl->dataStoreMap.end()) {
		dataStore = it->second;
		m_impl->dataStoreMap.erase(it);
	}
	m_impl->mutex.unlock();

	if (!dataStore) {
		MLPL_WARN("Not found: storeId: %" PRIu32 "\n", storeId);
		return;
	}

	callRemovedHandlers(dataStore);
	dataStore->unref();
}


DataStoreVector DataStoreManager::getDataStoreVector(void)
{
	m_impl->mutex.lock();
	DataStoreVector dataStoreVector;
	DataStoreMapIterator it = m_impl->dataStoreMap.begin();
	for (; it != m_impl->dataStoreMap.end(); ++it) {
		DataStore *dataStore = it->second;
		dataStore->ref();
		dataStoreVector.push_back(dataStore);
	}
	m_impl->mutex.unlock();

	return dataStoreVector;
}

size_t DataStoreManager::getNumberOfDataStores(void) const
{
	m_impl->mutex.lock();
	size_t num = m_impl->dataStoreMap.size();
	m_impl->mutex.unlock();
	return num;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DataStoreManager::closeAllStores(void)
{
	m_impl->mutex.lock();
	DataStoreMapIterator it = m_impl->dataStoreMap.begin();
	for (; it != m_impl->dataStoreMap.end(); ++it) {
		DataStore *dataStore = it->second;
		callRemovedHandlers(dataStore);
		dataStore->unref();
	}
	m_impl->dataStoreMap.clear();
	m_impl->mutex.unlock();
}

void DataStoreManager::callAddedHandlers(DataStore *dataStore)
{
	m_impl->eventProcListLock.readLock();
	Reaper<ReadWriteLock> unlocker(&m_impl->eventProcListLock,
	                               ReadWriteLock::unlock);
	DataStoreEventProcListIterator evtProc = m_impl->eventProcList.begin();
	for (; evtProc != m_impl->eventProcList.end(); ++evtProc)
		(*evtProc)->onAdded(dataStore);
}

void DataStoreManager::callRemovedHandlers(DataStore *dataStore)
{
	m_impl->eventProcListLock.readLock();
	Reaper<ReadWriteLock> unlocker(&m_impl->eventProcListLock,
	                               ReadWriteLock::unlock);
	DataStoreEventProcListIterator evtProc = m_impl->eventProcList.begin();
	for (; evtProc != m_impl->eventProcList.end(); ++evtProc)
		(*evtProc)->onRemoved(dataStore);
}
