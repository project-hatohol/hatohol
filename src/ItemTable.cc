#include <Logger.h>
using namespace mlpl;

#include <stdexcept>
#include "Utils.h"
#include "ItemTable.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTable::ItemTable(ItemGroupId id)
: m_groupId(id)
{
}

void ItemTable::add(ItemGroup *group, bool doRef)
{
	writeLock();
	m_groupList.push_back(group);
	writeUnlock();
	if (doRef)
		group->ref();
}

ItemGroupId ItemTable::getItemGroupId(void) const
{
	return m_groupId;
}

ItemTable *ItemTable::inner_join(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::left_outer_join(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::right_outer_join(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::cross_join(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}


// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTable::~ItemTable()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemGroupListIterator it = m_groupList.begin();
	for (; it != m_groupList.end(); ++it) {
		ItemGroup *group = *it;
		group->unref();
	}
}
