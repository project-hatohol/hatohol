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
#include <memory>
#include "ItemGroup.h"
#include "ItemDataUtils.h"

class ItemTable
{
public:
	ItemTable(void);
	ItemTable(const ItemTable &itemTable);
	virtual ~ItemTable();
	void add(ItemGroup *group, bool doRef = true);
	void add(const ItemGroup *group);
	size_t getNumberOfColumns(void) const;
	size_t getNumberOfRows(void) const;
	const ItemGroupList &getItemGroupList(void) const;
	bool hasIndex(void) const;
	void defineIndex(const std::vector<ItemDataIndexType> &indexTypeVector);
	const ItemDataIndexVector &getIndexVector(void) const;
	const std::vector<size_t> &getIndexedColumns(void) const;

protected:
	void updateIndex(const ItemGroup *itemGroup);

private:
	ItemGroupList m_groupList;
	ItemDataIndexVector m_indexVector;
	std::vector<size_t> m_indexedColumnIndexes;
};

