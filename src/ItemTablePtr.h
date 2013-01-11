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

#endif // #define ItemTablePtr_h

