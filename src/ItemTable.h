#ifndef ItemTable_h
#define ItemTable_h

#include <map>
using namespace std;

#include "ItemGroup.h"

class ItemTable : public UsedCountable {
public:
	ItemTable(ItemGroupId id);
	void add(ItemGroup *group, bool doRef = true);

protected:
	virtual ~ItemTable();

private:
	ItemGroupId   m_groupId;
	ItemGroupList m_groupList;
};

#endif  // ItemTable_h
