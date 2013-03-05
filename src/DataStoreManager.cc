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

void DataStoreManager::passCommandLineArg(const CommandLineArg &cmdArg)
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

DataStoreVector &DataStoreManager::getDataStoreVector(void)
{
	return m_dataStoreVector;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
