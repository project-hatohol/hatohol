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
#include "DBClientUtils.h"
#include "ItemGroupPtr.h"
#include "ItemDataPtr.h"

namespace testDBClientUtils {

void setup(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getUint64FromGrp(void)
{
	static const uint64_t DATA[] = {
	  0xfedcba9876543210, 0x8000000000000000, 0x7fffffffffffffff, 0x5};
	static const size_t NUM_DATA = sizeof(DATA) / sizeof(const uint64_t);
	VariableItemGroupPtr itemGrp;
	for (size_t i = 0; i < NUM_DATA; i++) {
		VariableItemDataPtr item = new ItemUint64(DATA[i]);
		itemGrp->add(item);
	}

	// check
	for (size_t i = 0; i < NUM_DATA; i++) {
		uint64_t data = GET_UINT64_FROM_GRP(itemGrp, i);
		cppcut_assert_equal(DATA[i], data);
	}
}

} // namespace testActionManager
