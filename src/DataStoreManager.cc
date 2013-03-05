#include "DataStoreManager.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreManager::DataStoreManager(void)
{
}

DataStoreManager::~DataStoreManager()
{
}

bool DataStoreManager::add(const string &storeName, DataStore *dataStore)
{
	pair<DataStoreMapIterator, bool> result = 
	  m_dataStoreMap.insert
	    (pair<const string, DataStore *>(storeName, dataStore));
	return result.second;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
