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

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreManager::DataStoreManager(void)
{
}

DataStoreManager::~DataStoreManager()
{
	closeAllStores();
}

void DataStoreManager::passCommandLineArg(const CommandLineArg &cmdArg)
{
}

bool DataStoreManager::add(uint32_t storeId, DataStore *dataStore)
{
	pair<DataStoreMapIterator, bool> result = 
	  m_dataStoreMap.insert
	    (pair<uint32_t, DataStore *>(storeId, dataStore));

	bool successed = result.second;
	if (successed) {
		m_dataStoreVector.push_back(dataStore);
	}
	return result.second;
}

DataStoreVector &DataStoreManager::getDataStoreVector(void)
{
	return m_dataStoreVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DataStoreManager::closeAllStores(void)
{
	for (size_t i = 0; i < m_dataStoreVector.size(); i++)
		delete m_dataStoreVector[i];
	m_dataStoreVector.clear();
}
