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

#ifndef ItemTableUtils_h 
#define ItemTableUtils_h

#include <string>
using namespace std;

#include "ItemTable.h"
#include "AsuraException.h"
#include "Utils.h"

class ItemTableUtils {
public:
	template<typename NativeType, typename ItemDataType>
	static const NativeType &getFirstRowData(ItemTablePtr tablePtr,
	                                  size_t columnIndex = 0)
	{
		const ItemGroupList &grpList = tablePtr->getItemGroupList();
		ASURA_ASSERT(!grpList.empty(), "ItamTable: empty");

		const ItemGroup *itemGroup = *grpList.begin();
		ASURA_ASSERT(itemGroup->getNumberOfItems() > columnIndex,
		             "itemGroup:Items: %zd, columnIndex: %zd",
		             itemGroup->getNumberOfItems() > columnIndex);

		const ItemDataType *itemData =
		   dynamic_cast<const ItemDataType *>
		     (itemGroup->getItemAt(columnIndex));
		ASURA_ASSERT(itemData,
		   "Failed to dynamic_cast: required: %s, actual: %s",
		   DEMANGLED_TYPE_NAME(ItemDataType),
		   DEMANGLED_TYPE_NAME(itemData));
		return itemData->get();
	}
};

#endif // ItemTableUtils_h
