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

#ifndef ItemData_h
#define ItemData_h

#include <string>
#include <sstream>
#include <map>
using namespace std;

#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <stdint.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "UsedCountable.h"
#include "ReadWriteLock.h"
#include "AsuraException.h"

typedef uint64_t ItemId;
#define PRIx_ITEM PRIx64
#define PRIu_ITEM PRIu64

static const ItemId SYSTEM_ITEM_ID_ANONYMOUS = 0xffffffffffffffff;

enum ItemDataExceptionType {
	ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION,
	ITEM_DATA_EXCEPTION_INVALID_OPERATION,
};

class ItemData;
class ItemDataException : public AsuraException
{
public:
	ItemDataException(ItemDataExceptionType type,
	                  const char *sourceFileName, int lineNumber,
	                  const char *operatorName,
	                  const ItemData &lhs, const ItemData &rhs);
};
#define THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(OP, RHS) \
throw ItemDataException(ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION, __FILE__, __LINE__, OP, *this, RHS)

#define THROW_ITEM_DATA_EXCEPTION_INVALID_OPERATION(OP, RHS) \
throw ItemDataException(ITEM_DATA_EXCEPTION_INVALID_OPERATION, __FILE__, __LINE__, OP, *this, RHS)

typedef vector<ItemId>               ItemIdVector;
typedef ItemIdVector::iterator       ItemIdVectorIterator;
typedef ItemIdVector::const_iterator ItemIdVectorConstIterator;

class ItemData;
typedef map<ItemId, ItemData *>     ItemDataMap;
typedef ItemDataMap::iterator       ItemDataMapIterator;
typedef ItemDataMap::const_iterator ItemDataMapConstIterator;

typedef multimap<ItemId, ItemData *>     ItemDataMultimap;
typedef ItemDataMultimap::iterator       ItemDataMultimapIterator;
typedef ItemDataMultimap::const_iterator ItemDataMultimapConstIterator;

typedef vector<ItemData *>             ItemDataVector;
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
	virtual void set(void *src) = 0;
	virtual void get(void *dst) const = 0;
	virtual string getString(void) const = 0;
	virtual bool isNull(void) const;
	virtual void setNull(void);

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

ostream &operator<<(ostream &os, const ItemData &itemData);

template <typename T, ItemDataType ITEM_TYPE>
class ItemGeneric : public ItemData {
public:
	ItemGeneric(ItemId id, T data)
	: ItemData(id, ITEM_TYPE),
	  m_data(data) {
	}

	ItemGeneric(T data)
	: ItemData(SYSTEM_ITEM_ID_ANONYMOUS, ITEM_TYPE),
	  m_data(data) {
	}

	// virtual methods
	virtual void set(void *src) {
		writeLock();
		m_data = *static_cast<T *>(src);
		writeUnlock();
	}

	virtual void get(void *dst) const {
		readLock();
		*static_cast<T *>(dst) = m_data;
		readUnlock();
	}

	virtual T get(void) const {
		T val;
		get(&val);
		return val;
	}

	virtual string getString(void) const {
		stringstream ss;
		readLock();
		ss << m_data;
		readUnlock();
		return ss.str();
	}

	virtual ItemData & operator =(const ItemData &itemData) {
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T val;
			itemData.get(&val);
			set(&val);
			return *this;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("=", itemData);
		return *this;
	}

	virtual ItemData * operator +(const ItemData &itemData) const {
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T v0, v1;
			get(&v0);
			itemData.get(&v1);
			return new ItemGeneric(v0 + v1);
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("+", itemData);
		return NULL;
	}

	virtual ItemData * operator /(const ItemData &itemData) const {
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T v0, v1;
			get(&v0);
			itemData.get(&v1);
			return new ItemGeneric(v0 / v1);
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("+", itemData);
		return NULL;
	}

	virtual bool operator >(const ItemData &itemData) const {
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T v0, v1;
			get(&v0);
			itemData.get(&v1);
			return v0 > v1;
		}
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">", itemData);
		return false;
	}

	virtual bool operator <(const ItemData &itemData) const {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<", itemData);
		return false;
	}

	virtual bool operator >=(const ItemData &itemData) const {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(">=", itemData);
		return false;
	}
	virtual bool operator <=(const ItemData &itemData) const {
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("<=", itemData);
		return false;
	}

	virtual bool operator ==(const ItemData &itemData) const {
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T v0, v1;
			get(&v0);
			itemData.get(&v1);
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
		const ItemDataType &type0 = getItemType();
		const ItemDataType &type1 = itemData.getItemType();
		if (type0 == type1) {
			T v0, v1;
			get(&v0);
			itemData.get(&v1);
			v0 += v1;
			set(&v0);
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

typedef ItemGeneric<bool,     ITEM_TYPE_BOOL>   ItemBool;
typedef ItemGeneric<uint64_t, ITEM_TYPE_UINT64> ItemUint64;
typedef ItemGeneric<int,      ITEM_TYPE_INT>    ItemInt;
typedef ItemGeneric<double,   ITEM_TYPE_DOUBLE> ItemDouble;
typedef ItemGeneric<string,   ITEM_TYPE_STRING> ItemString;

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

#endif // ItemData_h
