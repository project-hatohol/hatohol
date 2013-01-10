#include "ItemDataPtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemDataPtr::ItemDataPtr(void)
: ItemPtr()
{
}

ItemDataPtr::ItemDataPtr(ItemData *item, bool doRef)
: ItemPtr(item, doRef)
{
	MLPL_INFO("%s\n", __PRETTY_FUNCTION__);
}

ItemDataPtr::ItemDataPtr(ItemId itemId, ItemGroup *itemGroup)
: ItemPtr(itemGroup->getItem(itemId))
{
}

ItemDataPtr::~ItemDataPtr()
{
	MLPL_INFO("%s\n", __PRETTY_FUNCTION__);
}

ItemDataPtr &ItemDataPtr::operator=(ItemData *itemData)
{
	MLPL_INFO("%s\n", __PRETTY_FUNCTION__);
	return *this;
}
