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
