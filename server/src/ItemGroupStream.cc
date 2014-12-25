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

static void conv(uint64_t &dest, const string &src)
{
	int numConv = sscanf(src.c_str(), "%" PRIu64, &dest);
	if (numConv != 1) {
		THROW_HATOHOL_EXCEPTION_WITH_ERROR_CODE(
		  HTERR_INTERNAL_ERROR, "Failed to convert %s.\n", src.c_str());
	}
}

template<> uint64_t ItemGroupStream::read<string, uint64_t>(void)
{
	string str;
	uint64_t dest;
	*this >> str;
	conv(dest, str);
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
	int val;
	*this >> val;
	return StringUtils::sprintf("%d", val);
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
