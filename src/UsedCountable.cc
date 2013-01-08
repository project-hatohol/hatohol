#include "UsedCountable.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void UsedCountable::ref(void)
{
	writeLock();
	m_usedCount++;
	writeUnlock();
}

void UsedCountable::unref(void)
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
UsedCountable::UsedCountable(int initialUsedCount)
: m_usedCount(initialUsedCount)
{
	g_rw_lock_init(&m_lock);
}

UsedCountable::~UsedCountable()
{
	g_rw_lock_clear(&m_lock);
}

void UsedCountable::readLock(void)
{
	g_rw_lock_reader_lock(&m_lock);
}

void UsedCountable::readUnlock(void)
{
	g_rw_lock_reader_unlock(&m_lock);
}

void UsedCountable::writeLock(void)
{
	g_rw_lock_writer_lock(&m_lock);
}

void UsedCountable::writeUnlock(void)
{
	g_rw_lock_writer_unlock(&m_lock);
}

int UsedCountable::getUsedCount(void)
{
	readLock();
	int usedCount = m_usedCount;
	readUnlock();
	return usedCount;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
