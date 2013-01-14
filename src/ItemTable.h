#ifndef ItemTable_h
#define ItemTable_h

#include <map>
using namespace std;

#include "UsedCountable.h"
#include "ItemGroup.h"

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
	ItemTable *innerJoin(const ItemTable *itemTable) const;
	ItemTable *leftOuterJoin(const ItemTable *itemTable) const;
	ItemTable *rightOuterJoin(const ItemTable *itemTable) const;
	ItemTable *fullOuterJoin(const ItemTable *itemTable) const;
	ItemTable *crossJoin(const ItemTable *itemTable) const;

	template <typename T>
	bool foreach(bool (*func)(const ItemGroup *, T arg), T arg) const
	{
		bool ret = true;
		readLock();
		ItemGroupListConstIterator it = m_groupList.begin();
		for (; it != m_groupList.end(); ++it) {
			if (!(*func)(*it, arg)) {
				ret = false;
				break;
			}
		}
		readUnlock();
		return ret;
	}

protected:
	virtual ~ItemTable();
	bool freezeTailGroupIfFirstGroup(ItemGroup *tail);

	struct CrossJoinArg;
	static bool crossJoinForeach(const ItemGroup *itemGroup,
	                             CrossJoinArg &arg);
	static bool crossJoinForeachRTable(const ItemGroup *itemGroupRTable,
                                           CrossJoinArg &arg);

private:
	ItemGroupList m_groupList;
};

#endif  // ItemTable_h
