#ifndef ItemGroup_h
#define ItemGroup_h

#include <map>
using namespace std;

#include "ItemData."

typedef uint64_t ItemGroupId;

ItemGroup

class ItemGroup {
public:
	ItemGroup(ItemGroupId id);
	virtual ~ItemGroup();

private:
	ItemGroupId m_groupId;
	map<ItemId, ItemData *> m_itemMap;
};

#endif  // ItemGroup_h
