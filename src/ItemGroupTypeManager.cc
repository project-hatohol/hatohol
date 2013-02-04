/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
