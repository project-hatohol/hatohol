/*
 * Copyright (C) 2013 Project Hatohol
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

#include <cstdio>
#include "Utils.h"
#include "ItemData.h"
using namespace std;
using namespace mlpl;

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

static __thread struct {
	uint64_t vUint64;
} refValsForCast;

// ---------------------------------------------------------------------------
// ItemDataException
// ---------------------------------------------------------------------------
ItemDataException::ItemDataException(
  ItemDataExceptionType type,
  const string &sourceFileName, const int &lineNumber,
  const string &operatorName, const ItemData &lhs)
: HatoholException("", sourceFileName, lineNumber),
  m_type(type)
{
	string header = getMessageHeader(type);
	string msg = StringUtils::sprintf(
	  "%s: '%s' (%s) (ItemID: %" PRIu_ITEM ")",
	  header.c_str(), operatorName.c_str(),
	  lhs.getNativeTypeName(), lhs.getId());
	setBrief(msg);
}

ItemDataException::ItemDataException(
  ItemDataExceptionType type,
  const string &sourceFileName, const int &lineNumber,
  const string &operatorName, const ItemData &lhs, const ItemData &rhs)
: HatoholException("", sourceFileName, lineNumber),
  m_type(type)
{
	string header = getMessageHeader(type);
	string msg = StringUtils::sprintf(
	  "%s: '%s' between %s and %s "
	  "(ItemID: %" PRIu_ITEM " and %" PRIu_ITEM ")",
	  header.c_str(), operatorName.c_str(),
	  lhs.getNativeTypeName(), rhs.getNativeTypeName(),
	  lhs.getId(), rhs.getId());

	setBrief(msg);
}

ItemDataException::ItemDataException(
  ItemDataExceptionType type,
   const string &sourceFileName, const int &lineNumber, const ItemId &itemId)
: HatoholException("", sourceFileName, lineNumber),
  m_type(type)
{
	string header = getMessageHeader(type);
	string msg =
	   StringUtils::sprintf("%s: %" PRIu_ITEM, header.c_str(), itemId);
	setBrief(msg);
}

ItemDataExceptionType ItemDataException::getType(void) const
{
	return m_type;
}

string ItemDataException::getMessageHeader(const ItemDataExceptionType type)
{
	string header;
	if (type == ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION)
		header = "Undefined operation";
	else if (type == ITEM_DATA_EXCEPTION_INVALID_OPERATION)
		header = "Invalid operation";
	else if (type == ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND)
		header = "Item not found";
	else
		header = StringUtils::sprintf("Unknown exception (%d)", type);
	return header;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ItemData::init(void)
{
	static_assert(ARRAY_SIZE(ItemData::m_nativeTypeNames) == NUM_ITEM_TYPE, "");
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
		THROW_HATOHOL_EXCEPTION("m_itemType (%d) >= NUM_ITEM_TYPE (%d)",
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
// ItemBool
//
template<> ItemBool::operator const bool &() const
{
	return get();
}

//
// ItemInt
//
template<> ItemInt::operator const int &() const
{
	return get();
}

template<> ItemInt::operator const uint64_t &() const
{
	const int &val = get();
	if (val < 0) {
		THROW_ITEM_DATA_EXCEPTION_INVALID_OPERATION(
		  "cast to const uint64_t &", *this);
	}
	refValsForCast.vUint64 = val;
	return refValsForCast.vUint64;
}


template<> bool ItemInt::operator >(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = cast(itemData)->get();
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
		const int &data = cast(itemData)->get();
		return (m_data < data);
	} else if (itemType == ITEM_TYPE_UINT64) {
		if (m_data < 0)
			return true;
		const uint64_t &data = ItemUint64::cast(itemData)->get();
		return (static_cast<uint64_t>(m_data) < data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<", itemData);
	}
	return false;
}

template<> bool ItemInt::operator >=(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = cast(itemData)->get();
		return (m_data >= data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">=", itemData);
	}
	return false;
}

template<> bool ItemInt::operator <=(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = cast(itemData)->get();
		return (m_data <= data);
	} else {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<=", itemData);
	}
	return false;
}

//
// ItemUint64
//
template<> ItemUint64::operator const uint64_t &() const
{
	return get();
}

template<> bool ItemUint64::operator >(const ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		const uint64_t &data = cast(itemData)->get();
		return (m_data > data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = ItemInt::cast(itemData)->get();
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
		const uint64_t &data = cast(itemData)->get();
		return (m_data < data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = ItemInt::cast(itemData)->get();
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
		const uint64_t &data = cast(itemData)->get();
		return (m_data >= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = ItemInt::cast(itemData)->get();
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
		const uint64_t &data = cast(itemData)->get();
		return (m_data <= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = ItemInt::cast(itemData)->get();
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
		const uint64_t &data = cast(itemData)->get();
		return (m_data == data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		const int &data = ItemInt::cast(itemData)->get();
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

//
// ItemDouble
//
template<> ItemDouble::operator const double &() const
{
	return get();
}

//
// ItemString
//
template<> ItemString::operator const std::string &() const
{
	return get();
}

template<> ItemData * ItemString::operator /(const ItemData &itemData) const
{
	THROW_ITEM_DATA_EXCEPTION_INVALID_OPERATION("/", itemData);
	return NULL;
}
