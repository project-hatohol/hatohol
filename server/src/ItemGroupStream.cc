/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ItemGroupStream.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroupStream::ItemGroupStream(const ItemGroup *itemGroup)
: m_itemGroup(itemGroup),
  m_index(0),
  m_reservedItem(NULL)
{
}

const ItemData *ItemGroupStream::getItem(void) const
{
	if (m_reservedItem)
		return m_reservedItem;
	HATOHOL_ASSERT(m_index < m_itemGroup->getNumberOfItems(),
	               "Invalid index: %zd, group: %zd",
	               m_index, m_itemGroup->getNumberOfItems());
	return m_itemGroup->getItemAt(m_index);
}
 
void ItemGroupStream::seek(const ItemId &itemId)
{
	const ItemData *itemData = m_itemGroup->getItem(itemId);
	if (!itemData)
		THROW_ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND(itemId);
	m_reservedItem = itemData;
}


// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
