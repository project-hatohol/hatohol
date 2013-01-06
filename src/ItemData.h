#ifndef ItemData_h
#define ItemData_h

#include <stdint.h>
#include <glib.h>

typedef uint64_t ItemId;
#define PRIx_ITEM PRIx64
#define PRIu_ITEM PRIu64

enum ItemDataType {
	ITEM_TYPE_UINT64,
	ITEM_TYPE_STRING,
};

class ItemData {
public:
	ItemId getId(void);
	ItemData *ref(void);
	void unref(void);
	virtual void set(void *src) = 0;
	virtual void get(void *dst) = 0;

protected:
	// Constructor and destructor. This class cannot be created directly.
	ItemData(ItemId id, ItemDataType type);
	virtual ~ItemData();

	void readLock(void);
	void readUnlock(void);
	void writeLock(void);
	void writeUnlock(void);

private:
	int          m_usedCount;
	ItemId       m_itemId;
	ItemDataType m_itemType;
	GRWLock      m_lock;
};

class ItemUint64 : public ItemData {
public:
	ItemUint64(ItemId id, uint64_t data);

	// virtual method
	void set(void *src);
	void get(void *dst);

private:
	uint64_t m_data;
};

#endif // ItemData_h
