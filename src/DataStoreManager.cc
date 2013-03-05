#include "DataStoreManager.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreManager::DataStoreManager(void)
{
}

DataStoreManager::~DataStoreManager()
{
	for (size_t i = 0; i < m_dataStoreVector.size(); i++)
		delete m_dataStoreVector[i];
	// No need to free elements in m_dataStoreMap.
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
