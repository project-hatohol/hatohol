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
#include "ItemTable.h"
#include "HatoholException.h"
#include "Utils.h"

class ItemTableUtils {
public:
	template<typename NativeType, typename ItemDataType>
	static const NativeType &getFirstRowData(std::shared_ptr<const ItemTable> itemTable,
	                                  size_t columnIndex = 0)
	{
		const ItemGroupList &grpList = itemTable->getItemGroupList();
		HATOHOL_ASSERT(!grpList.empty(), "ItamTable: empty");

		const ItemGroup *itemGroup = *grpList.begin();
		HATOHOL_ASSERT(itemGroup->getNumberOfItems() > columnIndex,
		             "itemGroup:Items: %zd, columnIndex: %zd",
		             itemGroup->getNumberOfItems() > columnIndex);

		const ItemDataType *itemData =
		   dynamic_cast<const ItemDataType *>
		     (itemGroup->getItemAt(columnIndex));
		HATOHOL_ASSERT(itemData,
		   "Failed to dynamic_cast: required: %s, actual: %s",
		   DEMANGLED_TYPE_NAME(ItemDataType),
		   DEMANGLED_TYPE_NAME(itemData));
		return itemData->get();
	}
};

