#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemId ItemData::getId(void)
{
	return m_itemId;
}

ItemData *ItemData::ref(void)
{
	writeLock();
	m_usedCount++;
	writeUnlock();
	return this;
}

void ItemData::unref(void)
{
	readLock();
	m_usedCount--;
	int usedCount = m_usedCount;
	readUnlock();
	if (usedCount == 0)
		delete this;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemData::ItemData(ItemId id, ItemDataType type)
: m_usedCount(1),
  m_itemId(id),
  m_itemType(type)
{
	g_rw_lock_init(&m_lock);
}

ItemData::~ItemData()
{
	g_rw_lock_clear(&m_lock);
}

void ItemData::readLock(void)
{
	g_rw_lock_reader_lock(&m_lock);
}

void ItemData::readUnlock(void)
{
	g_rw_lock_reader_unlock(&m_lock);
}

void ItemData::writeLock(void)
{
	g_rw_lock_writer_lock(&m_lock);
}

void ItemData::writeUnlock(void)
{
	g_rw_lock_writer_unlock(&m_lock);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Public methods (ItemUint64)
// ---------------------------------------------------------------------------
ItemUint64::ItemUint64(ItemId id, uint64_t data)
: ItemData(id, ITEM_TYPE_UINT64),
  m_data(data)
{
}

void ItemUint64::set(void *src)
{
	writeLock();
	m_data = *static_cast<uint64_t *>(src);
	writeUnlock();
}

void ItemUint64::get(void *dst)
{
	readLock();
	uint64_t data = m_data;
	readUnlock();
	*static_cast<uint64_t *>(dst) = data;
}
