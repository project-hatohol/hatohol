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

#include <cppcutter.h>
#include "HatoholException.h"
#include "ExceptionTestUtils.h"

namespace testHatoholException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(HatoholException, HatoholException);
}

void test_throwAsException(void)
{
	assertThrow(HatoholException, exception);
}

void test_hatoholAssertPass(void)
{
	bool gotException = false;
	int a = 1;
	try {
		HATOHOL_ASSERT(a == 1, "a == 1 @ %s", __func__);
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void test_hatoholAssertFail(void)
{
	bool gotException = false;
	int a = 2;
	try {
		HATOHOL_ASSERT(a == 1, "a = %d", a);
	} catch (const HatoholException &e) {
		cut_notify("<<MSG>>\n%s", e.getFancyMessage().c_str());
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

} // namespace testHatoholException
