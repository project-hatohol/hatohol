/*
 * Copyright (C) 2013 Hatohol Project
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

#include <glib.h>
#include "UsedCountable.h"
#include "HatoholException.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void UsedCountable::ref(void) const
{
	g_atomic_int_inc(&m_usedCount);
}

void UsedCountable::unref(void) const
{
	if (g_atomic_int_dec_and_test(&m_usedCount))
		delete this;
}

int UsedCountable::getUsedCount(void) const
{
	return g_atomic_int_get(&m_usedCount);
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
	HATOHOL_ASSERT(m_usedCount == 0,
	             "used count: %d.", m_usedCount);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
