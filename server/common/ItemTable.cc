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

#include <Logger.h>
#include <stdexcept>
#include "Utils.h"
#include "ItemTable.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTable::ItemTable(void)
{
}

ItemTable::ItemTable(const ItemTable &itemTable)
{
	ItemGroupListConstIterator it = itemTable.m_groupList.begin();
	for (; it != itemTable.m_groupList.end(); ++it)
		m_groupList.push_back(*it);

	for (size_t i = 0; i < m_indexVector.size(); i++)
		delete m_indexVector[i];
}
ItemTable::~ItemTable()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemGroupListIterator it = m_groupList.begin();
	for (; it != m_groupList.end(); ++it) {
		const ItemGroup *group = *it;
		group->unref();
	}
}

void ItemTable::add(ItemGroup *group, bool doRef)
{
	if (!group->isFreezed())
		group->freeze();

	if (!m_groupList.empty()) {
		const ItemGroup *tail = m_groupList.back();
		const ItemGroupType *groupType0 = tail->getItemGroupType();
		const ItemGroupType *groupType1 = group->getItemGroupType();
		HATOHOL_ASSERT(groupType0, "groupType0: NULL");
		HATOHOL_ASSERT(groupType1, "groupType1: NULL");
		HATOHOL_ASSERT(*groupType0 == *groupType1,
		             "ItemGroupTypes unmatched");
	} else if (hasIndex()) {
		size_t sizeOfGroup = group->getNumberOfItems();
		if (m_indexVector.size() != sizeOfGroup) {
			THROW_HATOHOL_EXCEPTION(
			  "Index vector size (%zd) != group size (%zd)",
			  m_indexVector.size(), sizeOfGroup);
		}
	}

	if (hasIndex())
		updateIndex(group);

	m_groupList.push_back(group);
	if (doRef)
		group->ref();
}

void ItemTable::add(const ItemGroup *group)
{
	// TODO: extract common part from this function and add(ItemGroup*)
	HATOHOL_ASSERT(group->isFreezed(), "Group not freezed.");

	const ItemGroupType *groupType1 = group->getItemGroupType();
	HATOHOL_ASSERT(groupType1, "ItemGroupType is NULL.");

	if (!m_groupList.empty()) {
		const ItemGroup *tail = m_groupList.back();
		const ItemGroupType *groupType0 = tail->getItemGroupType();
		HATOHOL_ASSERT(*groupType0 == *groupType1,
		             "ItemGroupTypes unmatched");
	} else if (hasIndex()) {
		size_t sizeOfGroup = group->getNumberOfItems();
		if (m_indexVector.size() != sizeOfGroup) {
			THROW_HATOHOL_EXCEPTION(
			  "Index vector size (%zd) != group size (%zd)",
			  m_indexVector.size(), sizeOfGroup);
		}
	}

	if (hasIndex())
		updateIndex(group);

	m_groupList.push_back(group);
	group->ref();
}

size_t ItemTable::getNumberOfColumns(void) const
{
	if (m_groupList.empty())
		return 0;
	return (*m_groupList.begin())->getNumberOfItems();
}

size_t ItemTable::getNumberOfRows(void) const
{
	return m_groupList.size();
}

const ItemGroupList &ItemTable::getItemGroupList(void) const
{
	return m_groupList;
}

bool ItemTable::hasIndex(void) const
{
	return !m_indexVector.empty();
}

void ItemTable::defineIndex(const vector<ItemDataIndexType> &indexTypeVector)
{
	// pre check
	if (hasIndex())
		THROW_HATOHOL_EXCEPTION("m_indexVector is NOT empty.");

	if (!m_groupList.empty()) {
		const ItemGroup *firstGroup = *m_groupList.begin();
		if (firstGroup->getNumberOfItems() != indexTypeVector.size()) {
			THROW_HATOHOL_EXCEPTION(
			  "m_groupList.size() [%zd] != "
			  "indexTypeVector.size() [%zd]",
			  m_groupList.size(), indexTypeVector.size());
		}
	}

	// make ItemDataIndex instances
	for (size_t i = 0; i < indexTypeVector.size(); i++) {
		ItemDataIndexType type = indexTypeVector[i];
		m_indexVector.push_back(new ItemDataIndex(type));
		if (type != ITEM_DATA_INDEX_TYPE_NONE)
			m_indexedColumnIndexes.push_back(i);
	}

	// make indexes if this table has data
	ItemGroupListIterator it = m_groupList.begin();
	for (; it != m_groupList.end(); ++it)
		updateIndex(*it);
}

const ItemDataIndexVector &ItemTable::getIndexVector(void) const
{
	return m_indexVector;
}

const vector<size_t> &ItemTable::getIndexedColumns(void) const
{
	return m_indexedColumnIndexes;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ItemTable::updateIndex(const ItemGroup *itemGroup)
{
	for (size_t i = 0; i < m_indexedColumnIndexes.size(); i++) {
		size_t columnIndex = m_indexedColumnIndexes[i];
		ItemDataIndex *index = m_indexVector[columnIndex];
		const ItemData *itemData = itemGroup->getItemAt(columnIndex);
		if (!index->insert(itemData, itemGroup))
			THROW_HATOHOL_EXCEPTION("Failed to make index.");
	}
}

