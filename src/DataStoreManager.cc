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

	bool successed = result.second;
	if (successed) {
		m_dataStoreVector.push_back(dataStore);
	}
	return result.second;
}

const DataStoreVector &DataStoreManager::getDataStoreVector(void) const
{
	return m_dataStoreVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
