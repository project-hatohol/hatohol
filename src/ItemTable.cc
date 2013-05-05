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
	ItemGroupListConstIterator it = itemTable.m_groupList.begin();
	for (; it != itemTable.m_groupList.end(); ++it)
		m_groupList.push_back(*it);

	for (size_t i = 0; i < m_indexVector.size(); i++) {
		ItemDataIndex *index = m_indexVector[i];
		if (index)
			delete index;
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
		if (groupType1 == NULL) {
			if (!group->setItemGroupType(groupType0)) {
				THROW_ASURA_EXCEPTION(
				  "Failed to call setItemGroupType.");
			}
		} else if (*groupType0 != *groupType1) {
			THROW_ASURA_EXCEPTION("ItemGroupTypes unmatched");
		}
	} else if (hasIndex()) {
		size_t sizeOfGroup = group->getNumberOfItems();
		if (m_indexVector.size() != sizeOfGroup) {
			THROW_ASURA_EXCEPTION(
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
	ASURA_ASSERT(group->isFreezed(), "Group not freezed.");

	const ItemGroupType *groupType1 = group->getItemGroupType();
	ASURA_ASSERT(groupType1, "ItemGroupType is NULL.");

	if (!m_groupList.empty()) {
		const ItemGroup *tail = m_groupList.back();
		const ItemGroupType *groupType0 = tail->getItemGroupType();
		ASURA_ASSERT(*groupType0 == *groupType1,
		             "ItemGroupTypes unmatched");
	} else if (hasIndex()) {
		size_t sizeOfGroup = group->getNumberOfItems();
		if (m_indexVector.size() != sizeOfGroup) {
			THROW_ASURA_EXCEPTION(
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

ItemTable *ItemTable::innerJoin
  (const ItemTable *itemTable,
   size_t indexLeftColumn, size_t indexRightColumn) const
{
	if (m_groupList.empty() || itemTable->m_groupList.empty())
		return new ItemTable();

	size_t numColumnLTable = getNumberOfColumns();
	size_t numColumnRTable = itemTable->getNumberOfColumns();
	if (indexLeftColumn >= numColumnLTable ||
	    indexRightColumn >= numColumnRTable) {
		MLPL_BUG("Invalid parameter: numColumnL: %zd, indexL: %zd, "
		         "numColumnR: %zd, indexR: %zd\n",
		         numColumnLTable, indexLeftColumn,
		         numColumnRTable, indexRightColumn);
		return new ItemTable();
	}

	ItemTable *table = new ItemTable();
	InnerJoinArg arg = {
	  table, itemTable, NULL, indexLeftColumn, indexRightColumn};
	foreach<InnerJoinArg &>(innerJoinForeach, arg);
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
	if (m_groupList.empty() || itemTable->m_groupList.empty())
		return new ItemTable();

	ItemTable *table = new ItemTable();
	CrossJoinArg arg = {table, itemTable};
	foreach<CrossJoinArg &>(crossJoinForeach, arg);
	return table;
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
		THROW_ASURA_EXCEPTION("m_indexVector is NOT empty.");

	if (!m_groupList.empty()) {
		const ItemGroup *firstGroup = *m_groupList.begin();
		if (firstGroup->getNumberOfItems() != indexTypeVector.size()) {
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
ItemTable::~ItemTable()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemGroupListIterator it = m_groupList.begin();
	for (; it != m_groupList.end(); ++it) {
		const ItemGroup *group = *it;
		group->unref();
	}
}

void ItemTable::joinForeachCore(ItemTable *newTable,
                                const ItemGroup *itemGroupLTable,
                                const ItemGroup *itemGroupRTable)
{
	VariableItemGroupPtr newGroup;
	const ItemGroup *itemGroupArray[] = {
	  itemGroupLTable, itemGroupRTable, NULL};
	for (size_t index = 0; itemGroupArray[index] != NULL; index++) {
		const ItemGroup *itemGroup = itemGroupArray[index];
		size_t numItems = itemGroup->getNumberOfItems();
		for (size_t i = 0; i < numItems; i++)
			newGroup->add(itemGroup->getItemAt(i));
	}
	newTable->add(newGroup);
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
	const ItemData *leftData =
	  arg.itemGroupLTable->getItemAt(arg.indexLeftColumn);
	const ItemData *rightData =
	   itemGroupRTable->getItemAt(arg.indexRightColumn);
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

void ItemTable::updateIndex(const ItemGroup *itemGroup)
{
	for (size_t i = 0; i < m_indexedColumnIndexes.size(); i++) {
		size_t columnIndex = m_indexedColumnIndexes[i];
		ItemDataIndex *index = m_indexVector[columnIndex];
		const ItemData *itemData = itemGroup->getItemAt(columnIndex);
		if (!index->insert(itemData, itemGroup))
			THROW_ASURA_EXCEPTION("Failed to make index.");
	}
}

