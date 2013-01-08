#include "ReadWriteLock.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ReadWriteLock::ReadWriteLock(void)
{
	g_rw_lock_init(&m_lock);
}

ReadWriteLock::~ReadWriteLock()
{
	g_rw_lock_clear(&m_lock);
}

void ReadWriteLock::readLock(void) const
{
	g_rw_lock_reader_lock(&m_lock);
}

void ReadWriteLock::readUnlock(void) const
{
	g_rw_lock_reader_unlock(&m_lock);
}

void ReadWriteLock::writeLock(void) const
{
	g_rw_lock_writer_lock(&m_lock);
}

void ReadWriteLock::writeUnlock(void) const
{
	g_rw_lock_writer_unlock(&m_lock);
}

