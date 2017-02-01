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
#include <map>
#include "UsedCountable.h"
#include "ItemGroup.h"
#include "ItemDataUtils.h"

class ItemTable;
typedef std::map<ItemGroupId, ItemTable *>  ItemGroupIdTableMap;
typedef ItemGroupIdTableMap::iterator       ItemGroupIdTableMapIterator;
typedef ItemGroupIdTableMap::const_iterator ItemGroupIdTableMapConstIterator;

class ItemTable : public UsedCountable {
public:
	ItemTable(void);
	ItemTable(const ItemTable &itemTable);
	void add(ItemGroup *group, bool doRef = true);
	void add(const ItemGroup *group);
	size_t getNumberOfColumns(void) const;
	size_t getNumberOfRows(void) const;
	ItemTable *innerJoin(const ItemTable *itemTable,
	                     size_t indexLeftJoinColumn,
	                     size_t indexRightJoinColumn) const;
	ItemTable *leftOuterJoin(const ItemTable *itemTable) const;
	ItemTable *rightOuterJoin(const ItemTable *itemTable) const;
	ItemTable *fullOuterJoin(const ItemTable *itemTable) const;
	ItemTable *crossJoin(const ItemTable *itemTable) const;
	const ItemGroupList &getItemGroupList(void) const;
	bool hasIndex(void) const;
	void defineIndex(const std::vector<ItemDataIndexType> &indexTypeVector);
	const ItemDataIndexVector &getIndexVector(void) const;
	const std::vector<size_t> &getIndexedColumns(void) const;

	template <typename T>
	bool foreach(bool (*func)(const ItemGroup *, T arg), T arg) const
	{
		bool ret = true;
		ItemGroupListConstIterator it = m_groupList.begin();
		for (; it != m_groupList.end(); ++it) {
			if (!(*func)(*it, arg)) {
				ret = false;
				break;
			}
		}
		return ret;
	}

protected:
	virtual ~ItemTable();
	static void joinForeachCore(ItemTable *newTable,
	                            const ItemGroup *itemGroupLTable,
	                            const ItemGroup *itemGroupRTable);

	struct CrossJoinArg;
	static bool crossJoinForeach(const ItemGroup *itemGroup,
	                             CrossJoinArg &arg);
	static bool crossJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                           CrossJoinArg &arg);
	struct InnerJoinArg;
	static bool innerJoinForeach(const ItemGroup *itemGroup,
	                             InnerJoinArg &arg);
	static bool innerJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                           InnerJoinArg &arg);
	void updateIndex(const ItemGroup *itemGroup);

private:
	ItemGroupList m_groupList;
	ItemDataIndexVector m_indexVector;
	std::vector<size_t> m_indexedColumnIndexes;
};

