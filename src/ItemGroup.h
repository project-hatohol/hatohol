#ifndef ItemGroup_h
#define ItemGroup_h

#include <map>
#include <list>
using namespace std;

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "ItemData.h"

typedef uint64_t ItemGroupId;
#define PRIx_ITEM_GROUP PRIx64
#define PRIu_ITEM_GROUP PRIu64

class ItemGroup;
typedef map<ItemGroupId, ItemGroup *> ItemGroupMap;
typedef ItemGroupMap::iterator        ItemGroupMapIterator;
typedef ItemGroupMap::const_iterator  ItemGroupMapConstIterator;

typedef list<ItemGroup *>             ItemGroupList;
typedef ItemGroupList::iterator       ItemGroupListIterator;
typedef ItemGroupList::const_iterator ItemGroupListConstIterator;

class ItemGroup : public UsedCountable {
public:
	ItemGroup(ItemGroupId id);
	void add(ItemData *data, bool doRef = true);
	ItemData *getItem(ItemId itemId) const;

protected:
	virtual ~ItemGroup();

private:
	ItemGroupId m_groupId;
	ItemDataMap m_itemMap;
};

#endif  // ItemGroup_h
