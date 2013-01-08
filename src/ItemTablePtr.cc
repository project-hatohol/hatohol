#include "ItemTablePtr.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTablePtr::ItemTablePtr(const ItemTable *table)
: ItemPtr<ItemTable>(table)
{
}

ItemTablePtr::~ItemTablePtr()
{
}
