/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UsedCountable.h"
#include "HatoholException.h"
using namespace hfl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void UsedCountable::ref(void) const
{
	m_usedCount.add(1);
}

void UsedCountable::unref(void) const
{
	if (m_usedCount.sub(1) == 0)
		delete this;
}

int UsedCountable::getUsedCount(void) const
{
	return m_usedCount.get();
}

void UsedCountable::unref(UsedCountable *countable)
{
	countable->unref();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
UsedCountable::UsedCountable(const int &initialUsedCount)
: m_usedCount(initialUsedCount)
{
}

UsedCountable::~UsedCountable()
{
	int count = m_usedCount.get();
	HATOHOL_ASSERT(count == 0, "used count: %d.", count);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
