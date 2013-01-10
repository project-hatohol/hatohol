#include "ItemDataPtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemDataPtr::ItemDataPtr(ItemId itemId, ItemGroup *itemGroup)
: ItemPtr(itemGroup->getItem(itemId))
{
}

ItemDataPtr::~ItemDataPtr()
{
}
