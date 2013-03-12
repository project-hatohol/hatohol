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

#include <Logger.h>
using namespace mlpl;

#include <stdexcept>
#include "Utils.h"
#include "ItemTable.h"

struct ItemTable::CrossJoinArg
{
	ItemTable *newTable;
	const ItemTable *rightTable;
	const ItemGroup *itemGroupLTable;
};

struct ItemTable::InnerJoinArg
{
	ItemTable *newTable;
	const ItemTable *rightTable;
	const ItemGroup *itemGroupLTable;
	const size_t indexLeftColumn;
	const size_t indexRightColumn;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemTable::ItemTable(void)
{
}

ItemTable::ItemTable(const ItemTable &itemTable)
{
	itemTable.readLock();
	ItemGroupListConstIterator it = itemTable.m_groupList.begin();
	for (; it != itemTable.m_groupList.end(); ++it)
		m_groupList.push_back(*it);
	itemTable.readUnlock();

	for (size_t i = 0; i < m_indexVector.size(); i++) {
		ItemDataIndex *index = m_indexVector[i];
		if (index)
			delete index;
	}
}

ItemGroup *ItemTable::addNewGroup(void)
{
	ItemGroup *grp = new ItemGroup();
	add(grp, false);
	return grp;
}

void ItemTable::add(ItemGroup *group, bool doRef)
{
	writeLock();
	if (!m_groupList.empty()) {
		ItemGroup *tail = m_groupList.back();
		if (!freezeTailGroupIfFirstGroup(tail)) {
			writeUnlock();
			THROW_ASURA_EXCEPTION(
			  "Failed to freeze the tail group.");
		}
		const ItemGroupType *groupType0 = tail->getItemGroupType();
		const ItemGroupType *groupType1 = group->getItemGroupType();
		if (groupType1 == NULL) {
			if (!group->setItemGroupType(groupType0)) {
				writeUnlock();
				THROW_ASURA_EXCEPTION(
				  "Failed to call setItemGroupType.");
			}
		} else if (*groupType0 != *groupType1) {
			writeUnlock();
			THROW_ASURA_EXCEPTION("ItemGroupTypes unmatched");
		}
	} else if (!m_indexVector.empty()) {
		size_t sizeOfGroup = group->getNumberOfItems();
		if (m_indexVector.size() != sizeOfGroup) {
			THROW_ASURA_EXCEPTION(
			  "Index vector size (%zd) != group size (%zd)",
			  m_indexVector.size(), sizeOfGroup);
		}
	}

	if (!m_indexVector.empty())
		updateIndex(group);

	m_groupList.push_back(group);
	writeUnlock();
	if (doRef)
		group->ref();
}

size_t ItemTable::getNumberOfColumns(void) const
{
	readLock();
	if (m_groupList.empty()) {
		readUnlock();
		return 0;
	}
	size_t ret = (*m_groupList.begin())->getNumberOfItems();
	readUnlock();
	return ret;
}

size_t ItemTable::getNumberOfRows(void) const
{
	readLock();
	size_t ret = m_groupList.size();
	readUnlock();
	return ret;
}

ItemTable *ItemTable::innerJoin
  (const ItemTable *itemTable,
   size_t indexLeftColumn, size_t indexRightColumn) const
{
	readLock();
	itemTable->readLock();
	if (m_groupList.empty() || itemTable->m_groupList.empty()) {
		itemTable->readUnlock();
		readUnlock();
		return new ItemTable();
	}

	size_t numColumnLTable = getNumberOfColumns();
	size_t numColumnRTable = itemTable->getNumberOfColumns();
	if (indexLeftColumn >= numColumnLTable ||
	    indexRightColumn >= numColumnRTable) {
		itemTable->readUnlock();
		readUnlock();
		MLPL_BUG("Invalid parameter: numColumnL: %zd, indexL: %zd, "
		         "numColumnR: %zd, indexR: %zd\n",
		         numColumnLTable, indexLeftColumn,
		         numColumnRTable, indexRightColumn);
		return new ItemTable();
	}

	ItemTable *table = new ItemTable();
	InnerJoinArg arg = {
	  table, itemTable, NULL, indexLeftColumn, indexRightColumn};
	try {
		foreach<InnerJoinArg &>(innerJoinForeach, arg);
	} catch (...) {
		itemTable->readUnlock();
		readUnlock();
		throw;
	}
	itemTable->readUnlock();
	readUnlock();
	return table;
}

ItemTable *ItemTable::leftOuterJoin(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::rightOuterJoin(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::fullOuterJoin(const ItemTable *itemTable) const
{
	MLPL_BUG("Not implemneted: %s\n", __PRETTY_FUNCTION__);
	return NULL;
}

ItemTable *ItemTable::crossJoin(const ItemTable *itemTable) const
{
	readLock();
	itemTable->readLock();
	if (m_groupList.empty() || itemTable->m_groupList.empty()) {
		itemTable->readUnlock();
		readUnlock();
		return new ItemTable();
	}

	ItemTable *table = new ItemTable();
	CrossJoinArg arg = {table, itemTable};
	try {
		foreach<CrossJoinArg &>(crossJoinForeach, arg);
	} catch (...) {
		itemTable->readUnlock();
		readUnlock();
		throw;
	}
	itemTable->readUnlock();
	readUnlock();
	return table;
}

const ItemGroupList &ItemTable::getItemGroupList(void) const
{
	return m_groupList;
}

void ItemTable::defineIndex(vector<ItemDataIndexType> &indexTypeVector)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);

	// pre check
	if (!m_indexVector.empty())
		THROW_ASURA_EXCEPTION("m_indexVector is NOT empty.");

	if (!m_groupList.empty()) {
		if (m_groupList.size() != indexTypeVector.size()) {
			THROW_ASURA_EXCEPTION(
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

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemTable::~ItemTable()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemGroupListIterator it = m_groupList.begin();
	for (; it != m_groupList.end(); ++it) {
		ItemGroup *group = *it;
		group->unref();
	}
}

bool ItemTable::freezeTailGroupIfFirstGroup(ItemGroup *tail)
{
	if (tail->isFreezed())
		return true;

	size_t numList = m_groupList.size();
	if (numList == 1) {
		tail->freeze();
		return true;
	}
	return false;
}

void ItemTable::joinForeachCore(ItemTable *newTable,
                                const ItemGroup *itemGroupLTable,
                                const ItemGroup *itemGroupRTable)
{
	ItemGroup *newGroup = newTable->addNewGroup();
	const ItemGroup *itemGroupArray[] = {
	  itemGroupLTable, itemGroupRTable, NULL};
	for (size_t index = 0; itemGroupArray[index] != NULL; index++) {
		const ItemGroup *itemGroup = itemGroupArray[index];
		size_t numItems = itemGroup->getNumberOfItems();
		for (size_t i = 0; i < numItems; i++)
			newGroup->add(itemGroup->getItemAt(i));
	}
}

bool ItemTable::crossJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                       CrossJoinArg &arg)
{
	joinForeachCore(arg.newTable, arg.itemGroupLTable, itemGroupRTable);
	return true;
}

bool ItemTable::crossJoinForeach(const ItemGroup *itemGroup, CrossJoinArg &arg)
{
	arg.itemGroupLTable = itemGroup;
	arg.rightTable->foreach<CrossJoinArg &>(crossJoinForeachRTable, arg);
	return true;
}

bool ItemTable::innerJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                       InnerJoinArg &arg)
{
	ItemData *leftData =
	  arg.itemGroupLTable->getItemAt(arg.indexLeftColumn);
	ItemData *rightData = itemGroupRTable->getItemAt(arg.indexRightColumn);
	if (*leftData != *rightData)
		return true;

	joinForeachCore(arg.newTable, arg.itemGroupLTable, itemGroupRTable);
	return true;
}

bool ItemTable::innerJoinForeach(const ItemGroup *itemGroup, InnerJoinArg &arg)
{
	arg.itemGroupLTable = itemGroup;
	arg.rightTable->foreach<InnerJoinArg &>(innerJoinForeachRTable, arg);
	return true;
}

void ItemTable::updateIndex(ItemGroup *itemGroup)
{
	for (size_t i = 0; i < m_indexedColumnIndexes.size(); i++) {
		size_t columnIndex = m_indexedColumnIndexes[i];
		ItemDataIndex *index = m_indexVector[columnIndex];
		ItemData *itemData = itemGroup->getItemAt(columnIndex);
		if (!index->insert(itemData, itemGroup))
			THROW_ASURA_EXCEPTION("Failed to make index.");
	}
}
