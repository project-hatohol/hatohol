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

#include <cstdio>
#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemId ItemData::getId(void) const
{
	return m_itemId;
}

const ItemDataType &ItemData::getItemType(void) const
{
	return m_itemType;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemData::ItemData(ItemId id, ItemDataType type)
: m_itemId(id),
  m_itemType(type)
{
}

ItemData::~ItemData()
{
}

//
// ItemInt
//
template<> bool ItemInt::operator >(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data > data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator <(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data < data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator >=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data >= data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator <=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data <= data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

//
// ItemUint64
//
template<> bool ItemUint64::operator >(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data > data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data > (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator <(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data < data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data < (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator >=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data >= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data >= (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}

	return false;
}

template<> bool ItemUint64::operator <=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data <= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return false;
		}
		return (m_data <= (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator ==(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data == data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return false;
		}
		return (m_data == (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

