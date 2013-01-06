#ifndef VirtualDataStoreZabbix_h
#define VirtualDataStoreZabbix_h

#include <map>
using namespace std;

#include "ItemGroup.h"
#include "VirtualDataStore.h"

class VirtualDataStoreZabbix : public VirtualDataStore
{
public:
	VirtualDataStoreZabbix(void);
	virtual ~VirtualDataStoreZabbix(void);

protected:
	ItemGroup *createItemGroup(ItemGroupId groupId);

private:
	ItemGroupMap m_itemGroupMap;
};

#endif // VirtualDataStoreZabbix_h
