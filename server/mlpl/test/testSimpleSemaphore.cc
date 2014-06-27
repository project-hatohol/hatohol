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
#include <errno.h>

#include "SimpleSemaphore.h"

using namespace mlpl;

namespace testSimpleSemaphore {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	SimpleSemaphore sem(0);
}

void test_postAndWait(void)
{
	SimpleSemaphore sem(0);
	cppcut_assert_equal(0, sem.post());
	cppcut_assert_equal(0, sem.post());
	cppcut_assert_equal(0, sem.wait());
	cppcut_assert_equal(0, sem.wait());
}

void test_timedWait(void)
{
	SimpleSemaphore sem(1);
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, sem.timedWait(1));
	cppcut_assert_equal(SimpleSemaphore::STAT_TIMEDOUT, sem.timedWait(1));
}

void test_initr(void)
{
	SimpleSemaphore sem(0);
	sem.init(1);
	cppcut_assert_equal(SimpleSemaphore::STAT_OK, sem.timedWait(1));
	cppcut_assert_equal(SimpleSemaphore::STAT_TIMEDOUT, sem.timedWait(1));
}


} // namespace testSimpleSemaphore
