/*
 * Copyright (C) 2013 Hatohol Project
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

#include "ItemDataUtils.h"

// ---------------------------------------------------------------------------
// ItemDataUtils
// ---------------------------------------------------------------------------
ItemDataPtr ItemDataUtils::createAsNumber(const string &word)
{
	bool isFloat;
	if (!StringUtils::isNumber(word, &isFloat))
		return ItemDataPtr();

	if (!isFloat) {
		int number = atoi(word.c_str());
		return ItemDataPtr(new ItemInt(ITEM_ID_NOBODY, number), false);
	}

	double number = atof(word.c_str());
	MLPL_BUG("Not implemented: %s (%f)\n", __PRETTY_FUNCTION__, number);
	return ItemDataPtr();
}

ItemDataPtr ItemDataUtils::createAsNumberOrString(const string &word)
{
	bool isFloat;
	if (!StringUtils::isNumber(word, &isFloat))
		return ItemDataPtr(new ItemString(word), false);

	// TODO: avoid call isNumber() twice (here and in createAsNumber()).
	return createAsNumber(word);
}

template<>
const bool &ItemDataUtils::get<bool> (const ItemData *itemData)
{
	return getBool(itemData);
}

template<>
const int &ItemDataUtils::get<int> (const ItemData *itemData)
{
	return getInt(itemData);
}

template<>
const uint64_t &ItemDataUtils::get<uint64_t> (const ItemData *itemData)
{
	return getUint64(itemData);
}

template<>
const double &ItemDataUtils::get<double>(const ItemData *itemData)
{
	return getDouble(itemData);
}

template<>
const string &ItemDataUtils::get<string>(const ItemData *itemData)
{
	return getString(itemData);
}

const bool &ItemDataUtils::getBool(const ItemData *itemData)
{
	return get<bool, ItemBool>(itemData);
}

const int &ItemDataUtils::getInt(const ItemData *itemData)
{
	return get<int, ItemInt>(itemData);
}

const uint64_t &ItemDataUtils::getUint64(const ItemData *itemData)
{
	return get<uint64_t, ItemUint64>(itemData);
}

const double &ItemDataUtils::getDouble(const ItemData *itemData)
{
	return get<double, ItemDouble>(itemData);
}

const string &ItemDataUtils::getString(const ItemData *itemData)
{
	return get<string, ItemString>(itemData);
}

// ---------------------------------------------------------------------------
// ItemDataPtrForIndex
// ---------------------------------------------------------------------------
ItemDataPtrForIndex::ItemDataPtrForIndex(const ItemData *itemData,
                                         const ItemGroup *itemGrp)
: ItemDataPtr(itemData),
  itemGroupPtr(itemGrp)
{
}

// ---------------------------------------------------------------------------
// ItemDataIndex
// ---------------------------------------------------------------------------
ItemDataIndex::ItemDataIndex(ItemDataIndexType type)
: m_type(type),
  m_index(NULL),
  m_multiIndex(NULL)
{
	if (m_type == ITEM_DATA_INDEX_TYPE_UNIQUE)
		m_index = new ItemDataForIndexSet();
	else if (m_type == ITEM_DATA_INDEX_TYPE_MULTI)
		m_multiIndex = new ItemDataForIndexMultiSet();
}

ItemDataIndex::~ItemDataIndex()
{
	if (m_index)
		delete m_index;
	if (m_multiIndex)
		delete m_multiIndex;
}

ItemDataIndexType ItemDataIndex::getIndexType(void) const
{
	return m_type;
}

bool ItemDataIndex::insert(const ItemData *itemData,
                           const ItemGroup* itemGroup)
{
	if (m_type == ITEM_DATA_INDEX_TYPE_UNIQUE) {
		ItemDataPtrForIndex itemPtrForIndex(itemData, itemGroup);
		pair<ItemDataForIndexSetIterator, bool> result = 
		  m_index->insert(itemPtrForIndex);
		if (!result.second) {
			MLPL_DBG("Failed to insert: %s\n",
			         itemData->getString().c_str());
		}
		return result.second;
	} else if (m_type == ITEM_DATA_INDEX_TYPE_MULTI) {
		ItemDataPtrForIndex itemPtrForIndex(itemData, itemGroup);
		m_multiIndex->insert(itemPtrForIndex);
		return true;
	}
	MLPL_WARN("Unexpectedly insert() is called: type: %d\n", m_type);
	return false;
}

void ItemDataIndex::find(const ItemData *itemData,
                         vector<ItemDataPtrForIndex> &foundItems) const
{
	if (m_type == ITEM_DATA_INDEX_TYPE_UNIQUE) {
		ItemDataPtrForIndex key(itemData, NULL);
		ItemDataForIndexSetIterator it = m_index->find(key);
		if (it == m_index->end())
			return;
		foundItems.push_back(*it);
	} else if (m_type == ITEM_DATA_INDEX_TYPE_MULTI) {
		ItemDataPtrForIndex key(itemData, NULL);
		pair<ItemDataForIndexMultiSetIterator,
		     ItemDataForIndexMultiSetIterator> itrSet =
		  m_multiIndex->equal_range(key);
		if (itrSet.first == itrSet.second)
			return;
		ItemDataForIndexMultiSetIterator it = itrSet.first;
		for (; it != itrSet.second; ++it)
			foundItems.push_back(*it);
	} else {
		MLPL_WARN("Unexpectedly find() is called: type: %d\n", m_type);
	}
}
