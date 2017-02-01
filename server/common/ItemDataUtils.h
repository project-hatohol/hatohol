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

#pragma once
#include <string>
#include "ItemDataPtr.h"
#include "ItemGroupPtr.h"
#include "ItemEnum.h"
#include "HatoholException.h"

class ItemDataUtils {
public:
	static ItemDataPtr createAsNumber(const std::string &word);
	static ItemDataPtr createAsNumberOrString(const std::string &word);
};

struct ItemDataPtrComparator {
	bool operator()(const ItemDataPtr &dataPtr0,
	                const ItemDataPtr &dataPtr1) const {
		return *dataPtr0 < *dataPtr1;
	}
};

typedef std::set<ItemDataPtr, ItemDataPtrComparator> ItemDataSet;
typedef std::multiset<ItemDataPtr, ItemDataPtrComparator> ItemDataMultiSet;

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

typedef std::set<ItemDataPtrForIndex, ItemDataPtrComparator>
  ItemDataForIndexSet;
typedef ItemDataForIndexSet::iterator ItemDataForIndexSetIterator;

typedef std::multiset<ItemDataPtrForIndex, ItemDataPtrComparator>
  ItemDataForIndexMultiSet;
typedef ItemDataForIndexMultiSet::iterator ItemDataForIndexMultiSetIterator;

class ItemDataIndex {
public:
	ItemDataIndex(ItemDataIndexType type);
	virtual ~ItemDataIndex();
	ItemDataIndexType getIndexType(void) const;
	bool insert(const ItemData *itemData, const ItemGroup* itemGroup);
	void find(const ItemData *itemData,
	          std::vector<ItemDataPtrForIndex> &foundItems) const;
private:
	ItemDataIndexType m_type;
	ItemDataForIndexSet      *m_index;
	ItemDataForIndexMultiSet *m_multiIndex;
};

typedef std::vector<ItemDataIndex *>        ItemDataIndexVector;
typedef ItemDataIndexVector::iterator       ItemDataIndexVectorIterator;
typedef ItemDataIndexVector::const_iterator ItemDataIndexVectorConstIterator;

