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
};

struct ItemDataPtrComparator {
	bool operator()(const ItemDataPtr &dataPtr0,
	                const ItemDataPtr &dataPtr1) const {
		return *dataPtr0 < *dataPtr1;
	}
};

typedef set<ItemDataPtr, ItemDataPtrComparator> ItemDataSet;
typedef multiset<ItemDataPtr, ItemDataPtrComparator> ItemDataMultiSet;

enum ItemDataIndexType {
	ITEM_DATA_INDEX_TYPE_NONE,
	ITEM_DATA_INDEX_TYPE_UNIQUE,
	ITEM_DATA_INDEX_TYPE_MULTI,
};

struct ItemDataPtrForIndex : public ItemDataPtr {
	ItemGroupPtr itemGroup;
};

typedef set<ItemDataPtrForIndex, ItemDataPtrComparator> ItemDataForIndexSet;
typedef multiset<ItemDataPtrForIndex, ItemDataPtrComparator> ItemDataForIndexMultiSet;

class ItemDataIndex {
public:
	ItemDataIndex(ItemDataIndexType type);
	virtual ~ItemDataIndex();
	bool insert(const ItemData *itemData, ItemGroup* itemGroup);
	bool find(const ItemData *itemData, vector<ItemDataPtrForIndex> &);
private:
	ItemDataIndexType m_type;
	ItemDataForIndexSet      *m_index;
	ItemDataForIndexMultiSet *m_multiIndex;
};

typedef vector<ItemDataIndex *>             ItemDataIndexVector;
typedef ItemDataIndexVector::iterator       ItemDataIndexVectorIterator;
typedef ItemDataIndexVector::const_iterator ItemDataIndexVectorConstIterator;

#endif // ItemDataUtils_h


