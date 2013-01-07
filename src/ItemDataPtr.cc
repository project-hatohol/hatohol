#include "ItemDataPtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemDataPtr::ItemDataPtr(ItemId itemId, const ItemGroup *itemGroup)
{
	ItemGroup *itemGroup2 = const_cast<ItemGroup *>(itemGroup);
	m_data = itemGroup2->getItem(itemId);
}

ItemDataPtr::~ItemDataPtr()
{
	if (m_data)
		m_data->unref();
}

bool ItemDataPtr::hasData(void) const
{
	return (m_data != NULL);
}

ItemData *ItemDataPtr::operator->()
{
	return m_data;
}
