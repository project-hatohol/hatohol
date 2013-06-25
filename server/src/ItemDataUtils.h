/*
 * Copyright (C) 2013 Hatohol Project
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

#ifndef ItemDataUtils_h 
#define ItemDataUtils_h

#include <string>
using namespace std;

#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemEnum.h"
#include "HatoholException.h"

class ItemDataUtils {
public:
	static ItemDataPtr createAsNumber(const string &word);
	static ItemDataPtr createAsNumberOrString(const string &word);
	template<typename NativeType, typename ItemDataType>
	static const NativeType &get(const ItemData *itemData) {
		HATOHOL_ASSERT(itemData, "itemData: NULL");
		const ItemDataType *casted = ItemDataType::cast(*itemData);
		HATOHOL_ASSERT(casted, "Invalid cast: %s -> %s",
		             DEMANGLED_TYPE_NAME(itemData),
		             DEMANGLED_TYPE_NAME(ItemDataType));
		return casted->get();
	}

	template<typename NativeType>
	static const NativeType &get(const ItemData *itemData) {
		// Without bug, this is never called, because
		//  the specilizations are is defined below.
		THROW_HATOHOL_EXCEPTION("Unknown type: %d: %s",
		                      itemData->getItemType(),
		                      DEMANGLED_TYPE_NAME(itemData));
		return *(new NativeType()); // never executed, just to build
	}

	static const bool     &getBool  (const ItemData *itemData);
	static const int      &getInt   (const ItemData *itemData);
	static const uint64_t &getUint64(const ItemData *itemData);
	static const double   &getDouble(const ItemData *itemData);
	static const string   &getString(const ItemData *itemData);
};

// The reason why this function is specialized:
// Ex.) If the above general template function ItemData::get() is
//      called with [NativeType = int], the build fails because
//      'string' cannot be converted to 'int'.
template<>
const bool &ItemDataUtils::get<bool> (const ItemData *itemData);
template<>
const int &ItemDataUtils::get<int> (const ItemData *itemData);
template<>
const uint64_t &ItemDataUtils::get<uint64_t> (const ItemData *itemData);
template<>
const double &ItemDataUtils::get<double>(const ItemData *itemData);
template<>
const string &ItemDataUtils::get<string>(const ItemData *itemData);

struct ItemDataPtrComparator {
	bool operator()(const ItemDataPtr &dataPtr0,
	                const ItemDataPtr &dataPtr1) const {
		return *dataPtr0 < *dataPtr1;
	}
};

typedef set<ItemDataPtr, ItemDataPtrComparator> ItemDataSet;
typedef multiset<ItemDataPtr, ItemDataPtrComparator> ItemDataMultiSet;

struct ItemGroupPtrComparator {
	bool operator()(const ItemGroupPtr &grpPtr0,
	                const ItemGroupPtr &grpPtr1) const {
		size_t size0 = grpPtr0->getNumberOfItems();
		size_t size1 = grpPtr1->getNumberOfItems();
		if (size0 != size1)
			return size0 < size1;

		for (size_t i = 0; i < size0; i++) {
			const ItemData *data0 = grpPtr0->getItemAt(i);
			const ItemData *data1 = grpPtr1->getItemAt(i);
			if (*data0 < *data1)
				return true;
			if (*data1 < *data0)
				return false;
		}
		return false;
	}
};

enum ItemDataIndexType {
	ITEM_DATA_INDEX_TYPE_NONE,
	ITEM_DATA_INDEX_TYPE_UNIQUE,
	ITEM_DATA_INDEX_TYPE_MULTI,
};

struct ItemDataPtrForIndex : public ItemDataPtr {
	ItemGroupPtr itemGroupPtr;

	// construct
	ItemDataPtrForIndex(const ItemData *itemData, const ItemGroup *itemGrp);
};

typedef set<ItemDataPtrForIndex, ItemDataPtrComparator> ItemDataForIndexSet;
typedef ItemDataForIndexSet::iterator ItemDataForIndexSetIterator;

typedef multiset<ItemDataPtrForIndex, ItemDataPtrComparator> ItemDataForIndexMultiSet;
typedef ItemDataForIndexMultiSet::iterator ItemDataForIndexMultiSetIterator;

class ItemDataIndex {
public:
	ItemDataIndex(ItemDataIndexType type);
	virtual ~ItemDataIndex();
	ItemDataIndexType getIndexType(void) const;
	bool insert(const ItemData *itemData, const ItemGroup* itemGroup);
	void find(const ItemData *itemData,
	          vector<ItemDataPtrForIndex> &foundItems) const;
private:
	ItemDataIndexType m_type;
	ItemDataForIndexSet      *m_index;
	ItemDataForIndexMultiSet *m_multiIndex;
};

typedef vector<ItemDataIndex *>             ItemDataIndexVector;
typedef ItemDataIndexVector::iterator       ItemDataIndexVectorIterator;
typedef ItemDataIndexVector::const_iterator ItemDataIndexVectorConstIterator;

#endif // ItemDataUtils_h


