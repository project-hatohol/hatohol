#include <stdexcept>
#include "Utils.h"
#include "ItemGroup.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroup::ItemGroup(ItemGroupId id)
: m_groupId(id)
{
}

void ItemGroup::add(ItemData *data)
{
	pair<ItemDataMapIterator, bool> result;
	ItemId itemId = data->getId();
	result = m_itemMap.insert(pair<ItemId, ItemData *>(itemId, data));
	if (!result.second) {
		string msg =
		  AMSG("Failed: insert: groupId: %"PRIx_ITEM_GROUP
		       ", itemId: %"PRIx_ITEM"\n", m_groupId, itemId);
		throw invalid_argument(msg);
	}
	data->ref();
}

ItemData *ItemGroup::getItem(ItemId itemId) const
{
	ItemDataMapConstIterator it = m_itemMap.find(itemId);
	if (it == m_itemMap.end())
		return NULL;
	return it->second;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup::~ItemGroup()
{
	ItemDataMapIterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data = it->second;
		data->unref();
	}
}

