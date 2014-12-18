/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include "UsedCountable.h"

namespace testUsedCountable {

struct TestUsedCountable : public UsedCountable {
	TestUsedCountable(void)
	{
	}

	TestUsedCountable(const int &initialCount)
	: UsedCountable(initialCount)
	{
	}
};

static TestUsedCountable *g_countable = NULL;

void cut_teardown(void)
{
	if (g_countable) {
		while (true) {
			int count = g_countable->getUsedCount();
			g_countable->unref();
			if (count == 1)
				break;
		}
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

void test_refUnref(void)
{
	g_countable = new TestUsedCountable();
	cppcut_assert_equal(1, g_countable->getUsedCount());
	g_countable->ref();
	cppcut_assert_equal(2, g_countable->getUsedCount());
	g_countable->ref();
	cppcut_assert_equal(3, g_countable->getUsedCount());
	g_countable->unref();
	cppcut_assert_equal(2, g_countable->getUsedCount());
	g_countable->unref();
	cppcut_assert_equal(1, g_countable->getUsedCount());
}

void test_staticUnref(void)
{
	g_countable = new TestUsedCountable();
	g_countable->ref();
	cppcut_assert_equal(2, g_countable->getUsedCount());
	UsedCountable::unref(g_countable);
	cppcut_assert_equal(1, g_countable->getUsedCount());
}

} // namespace testUsedCountable
