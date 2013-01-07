#ifndef VirtualDataStoreZabbix_h
#define VirtualDataStoreZabbix_h

#include <map>
using namespace std;

#include <glib.h>
#include "ItemGroup.h"
#include "VirtualDataStore.h"

class VirtualDataStoreZabbix : public VirtualDataStore
{
public:
	static VirtualDataStoreZabbix *getInstance(void);

protected:
	ItemGroup *createItemGroup(ItemGroupId groupId);

private:
	static GMutex                  m_mutex;
	static VirtualDataStoreZabbix *m_instance;
	ItemGroupMap m_itemGroupMap;

	VirtualDataStoreZabbix(void);
	virtual ~VirtualDataStoreZabbix();

};

#endif // VirtualDataStoreZabbix_h
