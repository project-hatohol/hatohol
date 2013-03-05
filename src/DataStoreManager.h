/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DataStoreManager_h
#define DataStoreManager_h

#include <map>
#include <string>
using namespace std;

#include "DataStore.h"
#include "Utils.h"

class DataStoreManager {
	// Currently multi-thread unsafe.
public:
	DataStoreManager(void);
	virtual ~DataStoreManager();
	virtual void passCommandLineArg(const CommandLineArg &cmdArg);

	// Elements regisgtered in this function should be freed by
	// this class (i.e. owner is changed).
	bool add(const string &storeName, DataStore *dataStore);

	DataStoreVector &getDataStoreVector(void);

private:
	typedef map<const string, DataStore*> DataStoreMap;
	typedef DataStoreMap::iterator        DataStoreMapIterator;

	// Elements in m_dataStoreMap and m_dataStoreVector are the same.
	// So it's only necessary to free elements in one.
	DataStoreMap    m_dataStoreMap;
	DataStoreVector m_dataStoreVector;
};

#endif // DataStoreManager_h
