#ifndef ItemTablePtr_h
#define ItemTablePtr_h

#include <list>
using namespace std;

#include "ItemPtr.h"
#include "ItemTable.h"

class ItemTablePtr : public ItemPtr<ItemTable> {
public:
	ItemTablePtr(void);
	ItemTablePtr(const ItemTablePtr &tablePtr);
	ItemTablePtr(const ItemTable *table);
	virtual ~ItemTablePtr();
};

typedef list<ItemTablePtr>                 ItemTablePtrList;
typedef list<ItemTablePtr>::iterator       ItemTablePtrListIterator;
typedef list<ItemTablePtr>::const_iterator ItemTablePtrListConstIterator;

#endif // #define ItemTablePtr_h

