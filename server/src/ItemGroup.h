/*
 * Copyright (C) 2013 Project Hatohol
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

#ifndef ItemGroup_h
#define ItemGroup_h

#include <map>
#include <list>
#include <inttypes.h>
#include "ItemData.h"
#include "ItemDataPtr.h"
#include "ItemGroupType.h"

typedef uint64_t ItemGroupId;
#define PRIx_ITEM_GROUP PRIx64
#define PRIu_ITEM_GROUP PRIu64

class ItemGroup;
typedef std::map<ItemGroupId, ItemGroup *> ItemGroupMap;
typedef ItemGroupMap::iterator             ItemGroupMapIterator;
typedef ItemGroupMap::const_iterator       ItemGroupMapConstIterator;

typedef std::list<const ItemGroup *>  ItemGroupList;
typedef ItemGroupList::iterator       ItemGroupListIterator;
typedef ItemGroupList::const_iterator ItemGroupListConstIterator;

class ItemGroup : public UsedCountable {
public:
	ItemGroup(void);
	void add(const ItemData *data, bool doRef = true);

	/**
	 * Create an ItemData family instance and append it to this group.
	 *
	 * @param T        A type to be created.
	 * @param data     An initial data.
	 * @param nullFlag A null flag of the created item.
	 */
	template <typename T>
	void add_new(const int &data,
	             const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
	{
		add(new T(data, nullFlag), false);
	}

	template <typename T>
	void add_new(const uint64_t &data,
	             const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
	{
		add(new T(data, nullFlag), false);
	}

	template <typename T>
	void add_new(const double &data,
	             const ItemDataNullFlagType &nullFlag = ITEM_DATA_NOT_NULL)
	{
		add(new T(data, nullFlag), false);
	}

	const ItemData *getItem(ItemId itemId) const;
	ItemDataVector getItems(ItemId itemId) const;
	const ItemData *getItemAt(size_t index) const;
	const ItemDataPtr getItemPtrAt(size_t index) const;
	size_t getNumberOfItems(void) const;
	void freeze();
	bool isFreezed(void) const;
	const ItemGroupType *getItemGroupType(void) const;

protected:
	virtual ~ItemGroup();

private:
	bool                 m_freeze;
	const ItemGroupType *m_groupType;
	ItemDataMultimap     m_itemMap;
	ItemDataVector       m_itemVector;
};

#endif  // ItemGroup_h
