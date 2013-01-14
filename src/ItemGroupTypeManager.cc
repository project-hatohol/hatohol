#include "ItemGroupTypeManager.h"

ItemGroupTypeManager *ItemGroupTypeManager::m_instance = NULL;
ReadWriteLock ItemGroupTypeManager::m_lock;
ItemGroupTypeSet ItemGroupTypeManager::m_groupTypeSet;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroupTypeManager *ItemGroupTypeManager::getInstance(void)
{
	if (m_instance)
		return m_instance;

	m_lock.writeLock();
	if (!m_instance)
		m_instance = new ItemGroupTypeManager();
	m_lock.writeUnlock();
	return m_instance;
}

const ItemGroupType *
ItemGroupTypeManager::getItemGroupType(const ItemDataVector &itemDataVector)
{
	ItemGroupType *groupType = new ItemGroupType(itemDataVector);
	m_lock.writeLock();
	ItemGroupTypeSetIterator it = m_groupTypeSet.find(groupType);
	if (it == m_groupTypeSet.end())
		m_groupTypeSet.insert(groupType);
	else {
		delete groupType;
		groupType = *it;
	}
	m_lock.writeUnlock();

	return groupType;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
ItemGroupTypeManager::ItemGroupTypeManager(void)
{
}

ItemGroupTypeManager::~ItemGroupTypeManager()
{
}
