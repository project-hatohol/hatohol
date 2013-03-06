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

#ifndef VirtualDataStoreZabbix_h
#define VirtualDataStoreZabbix_h

#include <map>
using namespace std;

#include <glib.h>
#include "ItemTablePtr.h"
#include "VirtualDataStore.h"
#include "ReadWriteLock.h"
#include "DataStoreZabbix.h"
#include "DataStoreManager.h"

class VirtualDataStoreZabbix : public VirtualDataStore
{
public:
	static VirtualDataStoreZabbix *getInstance(void);
	const ItemTablePtr getItemTable(ItemGroupId groupId);
	virtual void passCommandLineArg(const CommandLineArg &cmdArg);

protected:
	ItemTable *createStaticItemTable(ItemGroupId groupId);
	ItemTablePtr getTriggers(void);
	ItemTablePtr getFunctions(void);
	ItemTablePtr getItems(void);
	ItemTablePtr getHosts(void);

private:
	typedef ItemTablePtr (VirtualDataStoreZabbix::*DataGenerator)(void);
	typedef map<ItemGroupId, DataGenerator>  DataGeneratorMap;
	typedef DataGeneratorMap::iterator       DataGeneratorMapIterator;

	static GMutex                  m_mutex;
	static VirtualDataStoreZabbix *m_instance;
	ItemGroupIdTableMap m_staticItemTableMap;
	ReadWriteLock       m_staticItemTableMapLock;

	DataGeneratorMap    m_dataGeneratorMap;

	VirtualDataStoreZabbix(void);
	virtual ~VirtualDataStoreZabbix();
	void registerProfiles(ItemTable *table);
};

#endif // VirtualDataStoreZabbix_h
