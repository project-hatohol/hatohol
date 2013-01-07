#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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

int ItemData::getUsedCount(void)
{
	readLock();
	int usedCount = m_usedCount;
	readUnlock();
	return usedCount;
}

// ---------------------------------------------------------------------------
// Public methods (ItemGeneric)
// ---------------------------------------------------------------------------
template<>
string ItemGeneric<int, ITEM_TYPE_INT>::getString(void)
{
	return StringUtils::sprintf("%d", m_data);
};

template<>
string ItemGeneric<uint64_t, ITEM_TYPE_UINT64>::getString(void)
{
	return StringUtils::sprintf("%"PRIu64, m_data);
};

