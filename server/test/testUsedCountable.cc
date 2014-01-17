/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cppcutter.h>
#include "UsedCountable.h"

namespace testUsedCountable {

struct TestUsedCountable : public UsedCountable {
	TestUsedCountable(void)
	{
	}

	TestUsedCountable(int initialCount)
	: UsedCountable(initialCount)
	{
	}
};

static TestUsedCountable *g_countable = NULL;

void cut_teardown(void)
{
	if (g_countable) {
		while (g_countable->getUsedCount() >= 1)
			g_countable->unref();
		g_countable = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	g_countable = new TestUsedCountable();
	cppcut_assert_equal(1, g_countable->getUsedCount());
}

void test_constructorWithInitialCount(void)
{
	const int initialCount = 3;
	g_countable = new TestUsedCountable(initialCount);
	cppcut_assert_equal(initialCount, g_countable->getUsedCount());
}

} // namespace testUsedCountable
