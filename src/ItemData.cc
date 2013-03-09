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

ostream &operator<<(ostream &os, const ItemData &itemData)
{
	os << itemData.getString();
	return os;
}

const char *ItemData::m_nativeTypeNames[] =
{
	"Boolean",
	"Integer",
	"Unsigned",
	"Double",
	"String",
};

// ---------------------------------------------------------------------------
// ItemDataException
// ---------------------------------------------------------------------------
ItemDataException::ItemDataException(ItemDataExceptionType type,
                                     const char *sourceFileName, int lineNumber,
                                     const char *operatorName,
                                     const ItemData &lhs)
: AsuraException("", sourceFileName, lineNumber)
{
	string header = getMessageHeader(type);
	string msg = StringUtils::sprintf(
	  "%s: '%s' (%s) (ItemID: %"PRIu_ITEM")",
	  header.c_str(), operatorName, lhs.getNativeTypeName(), lhs.getId());
	setBrief(msg);
}
ItemDataException::ItemDataException(ItemDataExceptionType type,
                                     const char *sourceFileName, int lineNumber,
                                     const char *operatorName,
                                     const ItemData &lhs, const ItemData &rhs)
: AsuraException("", sourceFileName, lineNumber)
{
	string header = getMessageHeader(type);
	string msg = StringUtils::sprintf(
	  "%s: '%s' between %s and %s (ItemID: %"PRIu_ITEM" and %"PRIu_ITEM")",
	  header.c_str(), operatorName,
	  lhs.getNativeTypeName(), rhs.getNativeTypeName(),
	  lhs.getId(), rhs.getId());

	setBrief(msg);
}

string ItemDataException::getMessageHeader(const ItemDataExceptionType type)
{
	string header;
	if (type == ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION)
		header = "Undefined operation";
	else if (type == ITEM_DATA_EXCEPTION_INVALID_OPERATION)
		header = "Invalid operation";
	else
		header = StringUtils::sprintf("Unknown exception (%d)", type);
	return header;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ItemData::init(void)
{
	size_t numTypeNames =
	  sizeof(ItemData::m_nativeTypeNames) / sizeof(const char *);
	if (numTypeNames != NUM_ITEM_TYPE) {
		THROW_ASURA_EXCEPTION(
		  "sizeof(m_nativeTypeNames) is invalid: "
		  "(expcect/actual: %d/%d).",
		  NUM_ITEM_TYPE, numTypeNames);
	}
}

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
  m_itemType(type),
  m_null(false)
{
}

ItemData::~ItemData()
{
}

const char *ItemData::getNativeTypeName(void) const
{
	if (m_itemType >= NUM_ITEM_TYPE) {
		THROW_ASURA_EXCEPTION("m_itemType (%d) >= NUM_ITEM_TYPE (%d)",
		                      m_itemType, NUM_ITEM_TYPE);
	}
	return m_nativeTypeNames[m_itemType];
}

bool ItemData::isNull(void) const
{
	return m_null;
}

void ItemData::setNull(void)
{
	m_null = true;
}

//
// ItemInt
//
template<> bool ItemInt::operator >(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data > data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">", itemData);
	}
	return false;
}

template<> bool ItemInt::operator <(const ItemData &itemData) const
{
	ItemDataType itemType = itemData.getItemType();
	if (itemType == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data < data);
	} else if (itemType == ITEM_TYPE_UINT64) {
		if (m_data < 0)
			return true;
		uint64_t data;
		itemData.get(&data);
		return (static_cast<uint64_t>(m_data) < data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<", itemData);
	}
	return false;
}

template<> bool ItemInt::operator >=(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data >= data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">=", itemData);
	}
	return false;
}

template<> bool ItemInt::operator <=(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data <= data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<=", itemData);
	}
	return false;
}

//
// ItemUint64
//
template<> bool ItemUint64::operator >(const ItemData &itemData) const
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
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">", itemData);
	}
	return false;
}

template<> bool ItemUint64::operator <(const ItemData &itemData) const
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
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<", itemData);
	}
	return false;
}

template<> bool ItemUint64::operator >=(const ItemData &itemData) const
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
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">=", itemData);
	}

	return false;
}

template<> bool ItemUint64::operator <=(const ItemData &itemData) const
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
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<=", itemData);
	}
	return false;
}

template<> bool ItemUint64::operator ==(const ItemData &itemData) const
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
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("==", itemData);
	}
	return false;
}

template<> ItemData * ItemString::operator /(const ItemData &itemData) const
{
	THROW_ITEM_DATA_EXCEPTION_INVALID_OPERATION("/", itemData);
	return NULL;
}
