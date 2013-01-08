#ifndef ItemDataPtr_h
#define ItemDataPtr_h

#include "ItemData.h"
#include "ItemGroup.h"
#include "ItemPtr.h"

class ItemDataPtr : public ItemPtr<ItemData> {
public:
	ItemDataPtr(ItemId itemId, const ItemGroup *itemGroup);
	virtual ~ItemDataPtr();
};

#endif // #define ItemDataPtr_h
