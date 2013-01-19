#include <stdexcept>
#include "UsedCountable.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void UsedCountable::ref(void) const
{
	writeLock();
	m_usedCount++;
	writeUnlock();
}

void UsedCountable::unref(void)
{
	writeLock();
	m_usedCount--;
	int usedCount = m_usedCount;
	writeUnlock();
	if (usedCount == 0)
		delete this;
}

int UsedCountable::getUsedCount(void) const
{
	readLock();
	int usedCount = m_usedCount;
	readUnlock();
	return usedCount;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
UsedCountable::UsedCountable(int initialUsedCount)
: m_usedCount(1)
{
}

UsedCountable::~UsedCountable()
{
	if (getUsedCount() > 0) {
		string msg;
		TRMSG(msg, "used count: %d.", m_usedCount);
		throw logic_error(msg);
	}
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
