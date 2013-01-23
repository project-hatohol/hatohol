#include <cstdio>
#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemId ItemData::getId(void) const
{
	return m_itemId;
}

const ItemDataType &ItemData::getItemType(void) const
{
	return m_itemType;
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
template<> string ItemInt::getString(void) const
{
	return StringUtils::sprintf("%d", m_data);
};

template<> string ItemUint64::getString(void) const
{
	return StringUtils::sprintf("%"PRIu64, m_data);
};


template<> bool ItemInt::operator >=(ItemData &itemData) const
{
	printf("*** %s\n", __PRETTY_FUNCTION__);
	return false;
}

template<> bool ItemInt::operator <=(ItemData &itemData) const
{
	printf("*** %s\n", __PRETTY_FUNCTION__);
	return false;
}

