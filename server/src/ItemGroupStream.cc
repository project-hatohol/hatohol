/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <StringUtils.h>
#include "ItemGroupStream.h"

using namespace std;
using namespace mlpl;

template<> uint64_t ItemGroupStream::read<string, uint64_t>(void)
{
	string str;
	uint64_t dest;
	*this >> str;
	Utils::conv(dest, str);
	return dest;
}

template<typename T>
static string readTempl(ItemGroupStream &itemGrpStream, const char *fmt)
{
	T val;
	itemGrpStream >> val;
	return StringUtils::sprintf(fmt, val);
}

template<> string ItemGroupStream::read<int, string>(void)
{
	return readTempl<int>(*this, "%d");
}

template<> string ItemGroupStream::read<uint64_t, string>(void)
{
	return readTempl<uint64_t>(*this, "%" PRIu64);
}

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
