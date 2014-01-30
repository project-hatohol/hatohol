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
using namespace std;

namespace testItemGroupStream {

template<typename T, typename ITEM_DATA>
void _assertOperatorLeftShift(const T *expects, const size_t num_expects)
{
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < num_expects; i++)
		itemGroup->add(new ITEM_DATA(expects[i]), false);

	ItemGroupStream igStream(itemGroup);
	for (size_t i = 0; i < num_expects; i++) {
		T actual;
		actual << igStream;
		cppcut_assert_equal(actual, expects[i]);
	}
}
#define assertOperatorLeftShift(T, ITEM_DATA, ARRAY, NUM) \
cut_trace((_assertOperatorLeftShift<T, ITEM_DATA>(ARRAY, NUM)));

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_operatorLeftShiftToInt(void)
{
	const int expects[] = {-3, 5, 8};
	const size_t num_expects = sizeof(expects) / sizeof(int);
	assertOperatorLeftShift(int, ItemInt, expects, num_expects);
}

void test_operatorLeftShiftToUint64(void)
{
	const uint64_t expects[] = {0xfedcba9876543210, 3, 0x7fffeeee5555};
	const size_t num_expects = sizeof(expects) / sizeof(uint64_t);
	assertOperatorLeftShift(uint64_t, ItemUint64, expects, num_expects);
}

void test_operatorLeftShiftToString(void)
{
	const string expects[] = {"FOO", "", "dog dog dog dog dog"};
	const size_t num_expects = sizeof(expects) / sizeof(string);
	assertOperatorLeftShift(string, ItemString, expects, num_expects);
}

} // namespace testItemGroupStream
