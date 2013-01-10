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
	ItemTable(ItemGroupId id);
	void add(ItemGroup *group, bool doRef = true);
	ItemGroupId getItemGroupId(void) const;
	ItemTable *inner_join(const ItemTable *itemTable) const;
	ItemTable *left_outer_join(const ItemTable *itemTable) const;
	ItemTable *right_outer_join(const ItemTable *itemTable) const;
	ItemTable *full_outer_join(const ItemTable *itemTable) const;
	ItemTable *cross_join(const ItemTable *itemTable) const;

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

private:
	ItemGroupId   m_groupId;
	ItemGroupList m_groupList;
};

#endif  // ItemTable_h
