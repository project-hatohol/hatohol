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

#ifndef ItemTableUtils_h 
#define ItemTableUtils_h

#include <string>
using namespace std;

#include "ItemTable.h"
#include "HatoholException.h"
#include "Utils.h"

class ItemTableUtils {
public:
	template<typename NativeType, typename ItemDataType>
	static const NativeType &getFirstRowData(ItemTablePtr tablePtr,
	                                  size_t columnIndex = 0)
	{
		const ItemGroupList &grpList = tablePtr->getItemGroupList();
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

#endif // ItemTableUtils_h
