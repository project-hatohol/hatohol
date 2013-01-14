#ifndef ItemGroupTypeManager_h
#define ItemGroupTypeManager_h

#include <glib.h>
#include "ReadWriteLock.h"
#include "ItemGroupType.h"

class ItemGroupTypeManager
{
public:
	static ItemGroupTypeManager *getInstance(void);
	const ItemGroupType *
	getItemGroupType(const ItemDataVector &itemDataVector);

private:
	static ItemGroupTypeManager *m_instance;
	static ReadWriteLock         m_lock;
	static ItemGroupTypeSet      m_groupTypeSet;

	ItemGroupTypeManager(void);
	virtual ~ItemGroupTypeManager();
};

#endif // ItemGroupTypeManager_h


