#ifndef VirtualDataStoreZabbix_h
#define VirtualDataStoreZabbix_h

#include <map>
using namespace std;

#include <glib.h>
#include "ItemTable.h"
#include "VirtualDataStore.h"
#include "ReadWriteLock.h"

class VirtualDataStoreZabbix : public VirtualDataStore
{
public:
	static VirtualDataStoreZabbix *getInstance(void);
	const ItemTable *getItemTable(ItemGroupId groupId) const;

protected:
	ItemTable *createStaticItemTable(ItemGroupId groupId);
	ItemGroup *createStaticItemGroup(ItemTable *itemTable);

private:
	static GMutex                  m_mutex;
	static VirtualDataStoreZabbix *m_instance;
	ItemGroupIdTableMap m_staticItemTableMap;
	ReadWriteLock       m_staticItemTableMapLock;

	VirtualDataStoreZabbix(void);
	virtual ~VirtualDataStoreZabbix();
};

#endif // VirtualDataStoreZabbix_h
