#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemId ItemData::getId(void)
{
	return m_itemId;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemData::ItemData(ItemId id, ItemDataType type)
: m_itemId(id),
  m_itemType(type)
{
}

ItemData::~ItemData()
{
}

// ---------------------------------------------------------------------------
// Public methods (ItemGeneric)
// ---------------------------------------------------------------------------
template<>
string ItemGeneric<int, ITEM_TYPE_INT>::getString(void)
{
	return StringUtils::sprintf("%d", m_data);
};

template<>
string ItemGeneric<uint64_t, ITEM_TYPE_UINT64>::getString(void)
{
	return StringUtils::sprintf("%"PRIu64, m_data);
};

