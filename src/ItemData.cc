#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemData *ItemData::ref(void)
{
	g_rw_lock_writer_lock(&m_lock);
	m_usedCount++;
	g_rw_lock_writer_unlock(&m_lock);
	return this;
}

void ItemData::unref(void)
{
	g_rw_lock_writer_lock(&m_lock);
	m_usedCount--;
	g_rw_lock_writer_unlock(&m_lock);
	if (m_usedCount == 0)
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

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
