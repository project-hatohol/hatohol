#ifndef ItemDataPtr_h
#define ItemDataPtr_h

#include <Logger.h>
using namespace mlpl;

#include "ItemData.h"
#include "ItemGroup.h"
#include "ItemPtr.h"

/*
class ItemDataPtr : public ItemPtr<ItemData> {
public:
	ItemDataPtr(void);
	ItemDataPtr(ItemData *item, bool doRef = true);
	ItemDataPtr(ItemId itemId, ItemGroup *itemGroup);
	virtual ~ItemDataPtr();
	ItemDataPtr &operator=(ItemData *itemData);
};
*/

typedef ItemPtr<ItemData> ItemDataPtr;

#endif // #define ItemDataPtr_h
