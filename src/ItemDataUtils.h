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

#ifndef ItemDataUtils_h 
#define ItemDataUtils_h

#include <string>
using namespace std;

#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemEnum.h"

class ItemDataUtils {
public:
	static ItemDataPtr createAsNumber(const string &word);
	static ItemDataPtr createAsNumberOrString(const string &word);
	template<typename NativeType, typename ItemDataType>
	static NativeType get(ItemData *itemData) {
		ASURA_ASSERT(itemData, "itemData: NULL");
		ItemDataType *casted = dynamic_cast<ItemDataType *>(itemData);
		ASURA_ASSERT(casted, "Invalid cast: %s -> %s",
		             DEMANGLED_TYPE_NAME(itemData),
		             DEMANGLED_TYPE_NAME(ItemDataType));
		return casted->get();
	}
};

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
	bool insert(const ItemData *itemData, ItemGroup* itemGroup);
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


