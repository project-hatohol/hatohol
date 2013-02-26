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

#ifndef ItemDataUtils_h 
#define ItemDataUtils_h

#include <string>
using namespace std;

#include "ItemDataPtr.h"
#include "ItemEnum.h"
#include "ItemTablePtr.h"

class ItemDataUtils {
public:
	static ItemDataPtr createAsNumber(const string &word);
	static ItemDataPtr createAsNumberOrString(const string &word);
};

struct ItemDataPtrComparator {
	bool operator()(const ItemDataPtr &dataPtr0,
	                const ItemDataPtr &dataPtr1) const {
		return *dataPtr0 < *dataPtr1;
	}
};

typedef set<ItemDataPtr, ItemDataPtrComparator> ItemDataSet;
typedef map<ItemDataPtr, ItemTablePtr, ItemDataPtrComparator> ItemDataTableMap;
typedef ItemDataTableMap::iterator ItemDataTableMapIterator;

#endif // ItemDataUtils_h


