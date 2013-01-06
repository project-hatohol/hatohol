#ifndef ItemData_h
#define ItemData_h

#include <stdint.h>
#include <glib.h>

typedef uint64_t ItemId;

enum ItemDataType {
	ITEM_TYPE_UINT64,
	ITEM_TYPE_STRING,
};

class ItemData {
public:
	ItemData *ref(void);
	void unref(void);
	virtual void set(void *src) = 0;
	virtual void get(void *dst) = 0;

protected:
	// Constructor and destructor. This class cannot be created directly.
	ItemData(ItemId id, ItemDataType type);
	virtual ~ItemData();

private:
	int          m_usedCount;
	ItemId       m_itemId;
	ItemDataType m_itemType;
	GRWLock      m_lock;
};

#endif // ItemData_h
