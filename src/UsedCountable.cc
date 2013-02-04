/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
