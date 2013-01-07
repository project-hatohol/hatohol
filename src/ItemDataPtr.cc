#include "ItemDataPtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemDataPtr::ItemDataPtr(ItemId itemId, const ItemGroup *itemGroup)
{
	m_data = itemGroup->getItem(itemId);
	m_data->ref();
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
