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
#include "ItemGroupPtr.h"
#include "ItemGroupStream.h"

namespace testItemGroupStream {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_operatorLeftShiftToInt(void)
{
	const int expects[] = {-3, 5, 8};
	const size_t num_expects = sizeof(expects) / sizeof(int);
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < num_expects; i++)
		itemGroup->add(new ItemInt(expects[i]), false);

	ItemGroupStream igStream(itemGroup);
	for (size_t i = 0; i < num_expects; i++) {
		int actual;
		actual << igStream;
		cppcut_assert_equal(actual, expects[i]);
	}
}

} // namespace testItemGroupStream
