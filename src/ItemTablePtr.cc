#include "ItemTablePtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTablePtr::ItemTablePtr(void)
{
}

ItemTablePtr::ItemTablePtr(const ItemTable *table)
: ItemPtr<ItemTable>(table)
{
}

ItemTablePtr::~ItemTablePtr()
{
}
