#include <stdexcept>

#include "Utils.h"
#include "VirtualDataStoreZabbix.h"
#include "ItemGroupEnum.h"
#include "ItemEnum.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
VirtualDataStoreZabbix::VirtualDataStoreZabbix(void)
{
	ItemGroup *grp;
	grp = createItemGroup(GROUP_ID_ZBX_CONFIG);
	grp->add(new ItemUint64(ITEM_ID_ZBX_CONFIGID, 1));
}

VirtualDataStoreZabbix::~VirtualDataStoreZabbix(void)
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup *VirtualDataStoreZabbix::createItemGroup(ItemGroupId groupId)
{
	ItemGroup *grp = new ItemGroup(groupId);
	pair<ItemGroupMapIterator, bool> result;
	result = m_itemGroupMap.insert
	         (pair<ItemGroupId, ItemGroup *>(groupId, grp));
	if (!result.second) {
		string msg =
		  AMSG("Failed: insert: groupId: %"PRIx_ITEM_GROUP"\n",
		       groupId);
		throw invalid_argument(msg);
	}
	return grp;
}
