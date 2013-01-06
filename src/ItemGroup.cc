#include "ItemGroup.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroup::ItemGroup(ItemGroupId id)
: m_groupId(id)
{
}

ItemGroup::~ItemGroup()
{
	map<ItemId, ItemData *>::iterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data;
		data->unref();
	}
}
