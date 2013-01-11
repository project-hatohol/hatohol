#ifndef ItemTablePtr_h
#define ItemTablePtr_h

#include <list>
using namespace std;

#include "ItemPtr.h"
#include "ItemTable.h"

typedef ItemPtr<ItemTable> ItemTablePtr;

typedef list<ItemTablePtr>                 ItemTablePtrList;
typedef list<ItemTablePtr>::iterator       ItemTablePtrListIterator;
typedef list<ItemTablePtr>::const_iterator ItemTablePtrListConstIterator;

ItemTablePtr
innerJoin     (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
leftOuterJoin (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
rightOuterJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
fullOuterJoin (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
crossJoin     (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

#endif // #define ItemTablePtr_h

