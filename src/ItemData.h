#ifndef ItemData_h
#define ItemData_h

#include <string>
#include <map>
using namespace std;

#include <StringUtils.h>
using namespace mlpl;

#include <stdint.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "UsedCountable.h"
#include "ReadWriteLock.h"

typedef uint64_t ItemId;
#define PRIx_ITEM PRIx64
#define PRIu_ITEM PRIu64

typedef vector<ItemId>               ItemIdVector;
typedef ItemIdVector::iterator       ItemIdVectorIterator;
typedef ItemIdVector::const_iterator ItemIdVectorConstIterator;

class ItemData;
typedef map<ItemId, ItemData *>     ItemDataMap;
typedef ItemDataMap::iterator       ItemDataMapIterator;
typedef ItemDataMap::const_iterator ItemDataMapConstIterator;

enum ItemDataType {
	ITEM_TYPE_INT,
	ITEM_TYPE_UINT64,
	ITEM_TYPE_STRING,
};

class ItemData : public UsedCountable {
public:
	ItemId getId(void) const;
	virtual void set(void *src) = 0;
	virtual void get(void *dst) const = 0;
	virtual string getString(void) const = 0;

protected:
	ItemData(ItemId id, ItemDataType type);
	virtual ~ItemData();

private:
	ItemId       m_itemId;
	ItemDataType m_itemType;
};

template <typename T, ItemDataType ITEM_TYPE>
class ItemGeneric : public ItemData {
public:
	ItemGeneric(ItemId id, T data)
	: ItemData(id, ITEM_TYPE),
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

	virtual string getString(void) const {
		readLock();
		string str = m_data;
		readUnlock();
		return str;
	}

protected:
	virtual ~ItemGeneric() {
	}

private:
	T m_data;
};

template<>
string ItemGeneric<int, ITEM_TYPE_INT>::getString(void) const;
template<>
string ItemGeneric<uint64_t, ITEM_TYPE_UINT64>::getString(void) const;

typedef ItemGeneric<uint64_t, ITEM_TYPE_UINT64> ItemUint64;
typedef ItemGeneric<int,      ITEM_TYPE_INT>    ItemInt;
typedef ItemGeneric<string,   ITEM_TYPE_STRING> ItemString;

#endif // ItemData_h
