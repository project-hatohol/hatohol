#ifndef ItemData_h
#define ItemData_h

#include <string>
using namespace std;

#include <stdint.h>
#include <glib.h>

typedef uint64_t ItemId;
#define PRIx_ITEM PRIx64
#define PRIu_ITEM PRIu64

enum ItemDataType {
	ITEM_TYPE_INT,
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

	int getUsedCount(void);

private:
	int          m_usedCount;
	ItemId       m_itemId;
	ItemDataType m_itemType;
	GRWLock      m_lock;
};

template <typename T, ItemDataType ITEM_TYPE>
class ItemGeneric : public ItemData {
public:
	ItemGeneric(ItemId id, T data)
	: ItemData(id, ITEM_TYPE),
	  m_data(data) {
	}

	// virtual methods
	void set(void *src) {
		writeLock();
		m_data = *static_cast<T *>(src);
		writeUnlock();
	}

	void get(void *dst) {
		readLock();
		T data = m_data;
		readUnlock();
		*static_cast<T *>(dst) = data;
	}

private:
	T m_data;
};

typedef ItemGeneric<uint64_t, ITEM_TYPE_UINT64> ItemUint64;
typedef ItemGeneric<int,      ITEM_TYPE_INT>    ItemInt;
typedef ItemGeneric<string,   ITEM_TYPE_STRING> ItemString;

#endif // ItemData_h
