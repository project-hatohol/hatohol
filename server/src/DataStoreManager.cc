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
#include "DataStoreEventProc.h"
#include <MutexLock.h>
typedef map<uint32_t, DataStore*> DataStoreMap;
typedef DataStoreMap::iterator    DataStoreMapIterator;

struct DataStoreManager::PrivateContext {
	// Elements in dataStoreMap and dataStoreVector are the same.
	// So it's only necessary to free elements in one.
	DataStoreMap    dataStoreMap;
	DataStoreVector dataStoreVector;
	MutexLock mutex;
	list<DataStoreEventProc *> eventProcList;
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
	if (m_ctx)
		delete m_ctx;

	closeAllStores();
}

void DataStoreManager::passCommandLineArg(const CommandLineArg &cmdArg)
{
}

void DataStoreManager::registEventProc(DataStoreEventProc *proc)
{
	m_ctx->eventProcList.push_back(proc);
}

bool DataStoreManager::add(uint32_t storeId, DataStore *dataStore)
{
	m_ctx->mutex.lock();
	pair<DataStoreMapIterator, bool> result =
	  m_ctx->dataStoreMap.insert
	    (pair<uint32_t, DataStore *>(storeId, dataStore));

	bool successed = result.second;
	if (successed) {
		m_ctx->dataStoreVector.push_back(dataStore);
	}
	m_ctx->mutex.unlock();
	return result.second;
}

DataStoreVector &DataStoreManager::getDataStoreVector(void)
{
	return m_ctx->dataStoreVector;
}

DataStoreVector DataStoreManager::getSnapShotDataStoreVector(void)
{
	m_ctx->mutex.lock();
	DataStoreVector returnVector = m_ctx->dataStoreVector;
	m_ctx->mutex.unlock();

	return returnVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DataStoreManager::closeAllStores(void)
{
	for (size_t i = 0; i < m_ctx->dataStoreVector.size(); i++)
		m_ctx->dataStoreVector[i]->unref();
	m_ctx->dataStoreVector.clear();
}
