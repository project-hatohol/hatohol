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

void ItemGroup::add(ItemData *data, bool doRef)
{
	pair<ItemDataMapIterator, bool> result;
	ItemId itemId = data->getId();
	writeLock();
	result = m_itemMap.insert(pair<ItemId, ItemData *>(itemId, data));
	writeUnlock();
	if (!result.second) {
		string msg =
		  AMSG("Failed: insert: groupId: %"PRIx_ITEM_GROUP
		       ", itemId: %"PRIx_ITEM"\n", m_groupId, itemId);
		throw invalid_argument(msg);
	}
	if (doRef)
		data->ref();
}

const ItemData *ItemGroup::getItem(ItemId itemId) const
{
	ItemData *data = NULL;
	readLock();
	ItemDataMapConstIterator it = m_itemMap.find(itemId);
	if (it != m_itemMap.end())
		return it->second;
	readUnlock();
	return data;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup::~ItemGroup()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemDataMapIterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data = it->second;
		data->unref();
	}
}

