/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#pragma once
#include <string>
#include <sstream>
#include <map>

#include <Logger.h>
#include <StringUtils.h>

#include <stdint.h>
#include <inttypes.h>

#include "UsedCountable.h"
#include "ReadWriteLock.h"
#include "HatoholException.h"

typedef uint64_t ItemId;
#define PRIx_ITEM PRIx64
#define PRIu_ITEM PRIu64

static const ItemId SYSTEM_ITEM_ID_ANONYMOUS = 0xffffffffffffffff;

enum ItemDataExceptionType {
	ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION,
	ITEM_DATA_EXCEPTION_INVALID_OPERATION,
	ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND,
	ITEM_DATA_EXCEPTION_UNKNOWN,
};

enum ItemDataNullFlagType {
	ITEM_DATA_NOT_NULL,
	ITEM_DATA_NULL,
};

class ItemData;
class ItemDataException : public HatoholException
{
public:
	ItemDataException(ItemDataExceptionType type,
	                  const std::string &sourceFileName,
	                  const int &lineNumber,
	                  const std::string &operatorName,
	                  const ItemData &lhs);

	ItemDataException(ItemDataExceptionType type,
	                  const std::string &sourceFileName,
	                  const int &lineNumber,
	                  const std::string &operatorName,
	                  const ItemData &lhs, const ItemData &rhs);

	ItemDataException(ItemDataExceptionType type,
	                  const std::string &sourceFileName,
	                  const int &lineNumber, const ItemId &itemId);

	ItemDataExceptionType getType(void) const;

protected:
	std::string getMessageHeader(const ItemDataExceptionType type);

private:
	ItemDataExceptionType m_type;
};

#define THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(OP, ...) \
throw ItemDataException(ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION, __FILE__, __LINE__, OP, *this, ##__VA_ARGS__)

#define THROW_ITEM_DATA_EXCEPTION_INVALID_OPERATION(OP, ...) \
throw ItemDataException(ITEM_DATA_EXCEPTION_INVALID_OPERATION, __FILE__, __LINE__, OP, *this, ##__VA_ARGS__)

#define THROW_ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND(ITEM_ID) \
throw ItemDataException(ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND, __FILE__, __LINE__, ITEM_ID);

typedef std::vector<ItemId>          ItemIdVector;
typedef ItemIdVector::iterator       ItemIdVectorIterator;
typedef ItemIdVector::const_iterator ItemIdVectorConstIterator;

class ItemData;
typedef std::map<ItemId, const ItemData *> ItemDataMap;
typedef ItemDataMap::iterator              ItemDataMapIterator;
typedef ItemDataMap::const_iterator        ItemDataMapConstIterator;

typedef std::multimap<ItemId, const ItemData *> ItemDataMultimap;
typedef ItemDataMultimap::iterator              ItemDataMultimapIterator;
typedef ItemDataMultimap::const_iterator        ItemDataMultimapConstIterator;

typedef std::vector<const ItemData *>  ItemDataVector;
typedef ItemDataVector::iterator       ItemDataVectorIterator;
typedef ItemDataVector::const_iterator ItemDataVectorConstIterator;

enum ItemDataType {
	ITEM_TYPE_BOOL,
	ITEM_TYPE_INT,
	ITEM_TYPE_UINT64,
	ITEM_TYPE_DOUBLE,
	ITEM_TYPE_STRING,
	NUM_ITEM_TYPE,
};

class ItemData : public UsedCountable {
public:
	static void init(void);
	ItemId getId(void) const;
	const ItemDataType &getItemType(void) const;
	const char *getNativeTypeName(void) const;
	virtual std::string getString(void) const = 0;
	virtual bool isNull(void) const;
	virtual void setNull(void);
	virtual ItemData *clone(void) const = 0;

	/**
	 * Cast an ItemData instance to a reference of the native type.
	 *
	 * Note that the returned reference value
	 * shall not be passed to other threads and
	 * shall not be used after casting to the same type again is
	 * performed in the same thread.
	 * If you want to do the above things, you just make a copy of it.
	 */
	virtual operator const bool &() const = 0;
	virtual operator const int &() const = 0;
	virtual operator const uint64_t &() const = 0;
	virtual operator const double &() const = 0;
	virtual operator const std::string &() const = 0;

	virtual ItemData & operator =(const ItemData &itemData) = 0;
	virtual ItemData * operator +(const ItemData &itemData) const = 0;
	virtual ItemData * operator /(const ItemData &itemData) const = 0;
	virtual bool operator >(const ItemData &itemData) const = 0;
	virtual bool operator <(const ItemData &itemData) const = 0;
	virtual bool operator >=(const ItemData &itemData) const = 0;
	virtual bool operator <=(const ItemData &itemData) const = 0;
	virtual bool operator ==(const ItemData &itemData) const = 0;
	virtual bool operator !=(const ItemData &itemData) const = 0;
	virtual void operator +=(const ItemData &itemData) = 0;

protected:
	ItemData(ItemId id, ItemDataType type);
	virtual ~ItemData();

private:
	static const char *m_nativeTypeNames[];
	ItemId       m_itemId;
	ItemDataType m_itemType;
	bool         m_null;
};

std::ostream &operator<<(std::ostream &os, const ItemData &itemData);

template <typename T, ItemDataType ITEM_TYPE>
class ItemGeneric : public ItemData {
public:
	ItemGeneric(const ItemId &id, const T &data,
	            const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
	: ItemData(id, ITEM_TYPE),
	  m_data(data)
	{
		if (nullFlag == ITEM_DATA_NULL)
			setNull();
	}

	ItemGeneric(const T &data,
	            const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
	: ItemData(SYSTEM_ITEM_ID_ANONYMOUS, ITEM_TYPE),
	  m_data(data)
	{
		if (nullFlag == ITEM_DATA_NULL)
			setNull();
	}

	static const ItemGeneric<T, ITEM_TYPE> *cast(const ItemData &itemData)
	{
		const ItemGeneric<T, ITEM_TYPE> *castedItem =
		  dynamic_cast<const ItemGeneric<T, ITEM_TYPE> *>(&itemData);
		return castedItem;
	}

	bool isSameType(const ItemData &itemData) const
	{
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		return type0 == type1;
	}

	// virtual methods defined in this class
	virtual const T &get(void) const
	{
		return m_data;
	}

	// virtual methods (override)
	virtual ItemData *clone(void) const
	{
		return new ItemGeneric<T, ITEM_TYPE>(getId(), m_data);
	}

	virtual std::string getString(void) const
	{
		std::stringstream ss;
		ss << m_data;
		return ss.str();
	}

	// The following cast operators are assumed to be specialized
	virtual operator const bool &() const
	{
		static const bool ret = false;
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "cast to const bool &");
		return ret;
	}

	virtual operator const int &() const
	{
		static const int ret = 0;
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "cast to const int &");
		return ret;
	}

	virtual operator const uint64_t &() const
	{
		static const uint64_t ret = 0;
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "cast to const uint64_t &");
		return ret;
	}

	virtual operator const double &() const
	{
		static const double ret = 0;
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "cast to const double &");
		return ret;
	}

	virtual operator const std::string &() const
	{
		static const std::string ret;
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "cast to const std::string &");
		return ret;
	}

	virtual ItemData & operator =(const ItemData &itemData)
	{
		if (isSameType(itemData)) {
			m_data = cast(itemData)->get();
			return *this;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("=", itemData);
		return *this;
	}

	virtual ItemData * operator +(const ItemData &itemData) const
	{
		if (isSameType(itemData)) {
			const T &v0 = m_data;
			const T &v1 = cast(itemData)->get();
			return new ItemGeneric(v0 + v1);
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("+", itemData);
		return NULL;
	}

	virtual ItemData * operator /(const ItemData &itemData) const
	{
		if (isSameType(itemData)) {
			const T &v0 = m_data;
			const T &v1 = cast(itemData)->get();
			return new ItemGeneric(v0 / v1);
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("+", itemData);
		return NULL;
	}

	virtual bool operator >(const ItemData &itemData) const
	{
		if (isSameType(itemData)) {
			const T &v0 = m_data;
			const T &v1 = cast(itemData)->get();
			return v0 > v1;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">", itemData);
		return false;
	}

	virtual bool operator <(const ItemData &itemData) const
	{
		if (isSameType(itemData)) {
			const T &v0 = m_data;
			const T &v1 = cast(itemData)->get();
			return v0 < v1;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<", itemData);
		return false;
	}

	virtual bool operator >=(const ItemData &itemData) const
	{
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">=", itemData);
		return false;
	}

	virtual bool operator <=(const ItemData &itemData) const
	{
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<=", itemData);
		return false;
	}

	virtual bool operator ==(const ItemData &itemData) const
	{
		if (isSameType(itemData)) {
			const T &v0 = m_data;
			const T &v1 = cast(itemData)->get();
			return (v0 == v1);
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("==", itemData);
		return false;
	}

	virtual bool operator !=(const ItemData &itemData) const
	{
		return !(*this == itemData);
	}

	virtual void operator +=(const ItemData &itemData)
	{
		if (isSameType(itemData)) {
			m_data += cast(itemData)->get();
			return;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("+=", itemData);
	}

protected:
	virtual ~ItemGeneric() {
	}

private:
	T m_data;
};

typedef ItemGeneric<bool,        ITEM_TYPE_BOOL>   ItemBool;
typedef ItemGeneric<uint64_t,    ITEM_TYPE_UINT64> ItemUint64;
typedef ItemGeneric<int,         ITEM_TYPE_INT>    ItemInt;
typedef ItemGeneric<double,      ITEM_TYPE_DOUBLE> ItemDouble;
typedef ItemGeneric<std::string, ITEM_TYPE_STRING> ItemString;

template<> ItemBool::operator   const bool        &() const;
template<> ItemInt::operator    const int         &() const;
template<> ItemInt::operator    const uint64_t    &() const;
template<> ItemUint64::operator const uint64_t    &() const;
template<> ItemDouble::operator const double      &() const;
template<> ItemString::operator const std::string &() const;

template<> bool ItemInt::operator >(const ItemData &itemData) const;
template<> bool ItemInt::operator <(const ItemData &itemData) const;
template<> bool ItemInt::operator >=(const ItemData &itemData) const;
template<> bool ItemInt::operator <=(const ItemData &itemData) const;

template<> bool ItemUint64::operator >(const ItemData &itemData) const;
template<> bool ItemUint64::operator <(const ItemData &itemData) const;
template<> bool ItemUint64::operator >=(const ItemData &itemData) const;
template<> bool ItemUint64::operator <=(const ItemData &itemData) const;
template<> bool ItemUint64::operator ==(const ItemData &itemData) const;

template<> ItemData * ItemString::operator /(const ItemData &itemData) const;
template<> ItemData * ItemString::operator /(const ItemData &itemData) const;

