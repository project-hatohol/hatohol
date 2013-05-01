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

#include "Logger.h"
using namespace mlpl;

#include <stdexcept>
#include "Utils.h"
#include "ItemGroup.h"
#include "ItemGroupTypeManager.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroup::ItemGroup(void)
: m_freeze(false),
  m_groupType(NULL)
{
}

void ItemGroup::add(ItemData *data, bool doRef)
{
	if (m_freeze) {
		THROW_ASURA_EXCEPTION("Object: freezed");
	}

	ItemDataType itemDataType = data->getItemType();
	if (m_groupType) {
		size_t index = m_itemVector.size();
		ItemDataType expectedType = m_groupType->getType(index);
		if (expectedType != itemDataType) {
			THROW_ASURA_EXCEPTION(
			  "ItemDataType (%d) is not the expected (%d)",
			  itemDataType, expectedType);
		}
		if ((index + 1) == m_groupType->getSize())
			m_freeze = true;
	}

	ItemId itemId = data->getId();
	m_itemMap.insert(pair<ItemId, ItemData *>(itemId, data));
	m_itemVector.push_back(data);
	if (doRef)
		data->ref();
}

ItemData *ItemGroup::getItem(ItemId itemId) const
{
	ItemData *data = NULL;
	ItemDataMapConstIterator it = m_itemMap.find(itemId);
	if (it != m_itemMap.end())
		data = it->second;
	return data;
}

ItemDataVector ItemGroup::getItems(ItemId itemId) const
{
	ItemDataVector v;
	pair<ItemDataMultimapConstIterator, ItemDataMultimapConstIterator>
	  itPair = m_itemMap.equal_range(itemId);
	for (; itPair.first != itPair.second; ++itPair.first) {
		ItemData *item = (itPair.first)->second;
		v.push_back(item);
	}
	return v;
}

ItemData *ItemGroup::getItemAt(size_t index) const
{
	ItemData *data = m_itemVector[index];
	return data;
}

size_t ItemGroup::getNumberOfItems(void) const
{
	size_t ret = m_itemVector.size();
	return ret;
}

void ItemGroup::freeze(void)
{
	if (m_freeze) {
		MLPL_WARN("m_freeze: already set.\n");
		return;
	}
	if (m_groupType) {
		THROW_ASURA_EXCEPTION("m_groupType: NULL");
	}

	m_freeze = true;

	ItemGroupTypeManager *typeManager = ItemGroupTypeManager::getInstance();
	m_groupType = typeManager->getItemGroupType(m_itemVector);
}

bool ItemGroup::isFreezed(void) const
{
	return m_freeze;
}

const ItemGroupType *ItemGroup::getItemGroupType(void) const
{
	return m_groupType;
}

bool ItemGroup::setItemGroupType(const ItemGroupType *itemGroupType)
{
	string msg;
	if (m_groupType) {
		if (m_groupType == itemGroupType) {
			MLPL_WARN("The indentical ItemGroupType is set.\n");
			return true;
		}
		TRMSG(msg, "m_groupType: alread set.");
		MLPL_BUG(msg.c_str());
		return false;
	}

	if (!m_itemVector.empty()) {
		TRMSG(msg, "m_itemVector is not empty.");
		MLPL_BUG(msg.c_str());
		return false;
	}

	m_groupType = itemGroupType;
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup::~ItemGroup()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemDataMapIterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data = it->second;
		data->unref();
	}
}

