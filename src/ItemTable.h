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

#ifndef ItemTable_h
#define ItemTable_h

#include <map>
using namespace std;

#include "UsedCountable.h"
#include "ItemGroup.h"
#include "ItemDataUtils.h"

class ItemTable;
typedef map<ItemGroupId, ItemTable *>       ItemGroupIdTableMap;
typedef ItemGroupIdTableMap::iterator       ItemGroupIdTableMapIterator;
typedef ItemGroupIdTableMap::const_iterator ItemGroupIdTableMapConstIterator;

class ItemTable : public UsedCountable {
public:
	ItemTable(void);
	ItemTable(const ItemTable &itemTable);
	ItemGroup *addNewGroup(void);
	void add(ItemGroup *group, bool doRef = true);
	size_t getNumberOfColumns(void) const;
	size_t getNumberOfRows(void) const;
	ItemTable *innerJoin(const ItemTable *itemTable,
	                     size_t indexLeftJoinColumn,
	                     size_t indexRightJoinColumn) const;
	ItemTable *leftOuterJoin(const ItemTable *itemTable) const;
	ItemTable *rightOuterJoin(const ItemTable *itemTable) const;
	ItemTable *fullOuterJoin(const ItemTable *itemTable) const;
	ItemTable *crossJoin(const ItemTable *itemTable) const;
	const ItemGroupList &getItemGroupList(void) const;

	template <typename T>
	bool foreach(bool (*func)(const ItemGroup *, T arg), T arg) const
	{
		bool ret = true;
		readLock();
		try {
			ItemGroupListConstIterator it = m_groupList.begin();
			for (; it != m_groupList.end(); ++it) {
				if (!(*func)(*it, arg)) {
					ret = false;
					break;
				}
			}
		} catch (...) {
			readUnlock();
			throw;
		}
		readUnlock();
		return ret;
	}

protected:
	virtual ~ItemTable();
	bool freezeTailGroupIfFirstGroup(ItemGroup *tail);
	static void joinForeachCore(ItemTable *newTable,
	                            const ItemGroup *itemGroupLTable,
	                            const ItemGroup *itemGroupRTable);

	struct CrossJoinArg;
	static bool crossJoinForeach(const ItemGroup *itemGroup,
	                             CrossJoinArg &arg);
	static bool crossJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                           CrossJoinArg &arg);
	struct InnerJoinArg;
	static bool innerJoinForeach(const ItemGroup *itemGroup,
	                             InnerJoinArg &arg);
	static bool innerJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                           InnerJoinArg &arg);

private:
	ItemGroupList m_groupList;
	ItemDataSet       *m_indexVector;
	ItemDataMultiSet  *m_multiIndexVector;
};

#endif  // ItemTable_h
