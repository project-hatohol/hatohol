#ifndef ItemGroup_h
#define ItemGroup_h

#include <map>
#include <list>
using namespace std;

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "ItemData.h"
#include "ItemGroupType.h"

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
	ItemGroup(void);
	void add(ItemData *data, bool doRef = true);
	ItemData *getItem(ItemId itemId) const;
	ItemData *getItemAt(size_t index) const;
	size_t getNumberOfItems(void) const;
	void freeze();
	bool isFreezed(void) const;
	const ItemGroupType *getItemGroupType(void) const;
	void setItemGroupType(const ItemGroupType *itemGroupType);

protected:
	virtual ~ItemGroup();

private:
	bool            m_freeze;
	const ItemGroupType *m_groupType;
	ItemDataMap     m_itemMap;
	ItemDataVector  m_itemVector;
};

#endif  // ItemGroup_h
