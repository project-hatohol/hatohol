#ifndef ItemTablePtr_h
#define ItemTablePtr_h

#include "ItemPtr.h"
#include "ItemTable.h"

class ItemTablePtr : public ItemPtr<ItemTable> {
public:
	ItemTablePtr(const ItemTable *table);
	virtual ~ItemTablePtr();
};

#endif // #define ItemTablePtr_h

