#include "ItemTablePtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTablePtr::ItemTablePtr(void)
{
}

ItemTablePtr::ItemTablePtr(const ItemTablePtr &tablePtr)
: ItemPtr<ItemTable>(tablePtr)
{
}

ItemTablePtr::ItemTablePtr(const ItemTable *table, bool doRef)
: ItemPtr<ItemTable>(table, doRef)
{
}

ItemTablePtr::~ItemTablePtr()
{
}
