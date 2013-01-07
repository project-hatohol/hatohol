#ifndef ItemDataPtr_h
#define ItemDataPtr_h

#include "ItemData.h"
#include "ItemGroup.h"

class ItemDataPtr {
public:
	ItemDataPtr(ItemId itemId, const ItemGroup *itemGroup);
	virtual ~ItemDataPtr();
	ItemData *operator->();
	bool hasData(void) const;
private:
	ItemData *m_data;
};

#endif // #define ItemDataPtr_h
