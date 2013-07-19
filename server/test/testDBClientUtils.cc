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

template <class NativeType, class ItemDataType>
static ItemGroupPtr makeItemGroup(const NativeType *DATA, const size_t NUM_DATA)
{
	VariableItemGroupPtr itemGrp;
	for (size_t i = 0; i < NUM_DATA; i++) {
		VariableItemDataPtr item = new ItemDataType(DATA[i]);
		itemGrp->add(item);
	}
	return (ItemGroup *)itemGrp;
}

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
	ItemGroupPtr itemGrp
	 = makeItemGroup<uint64_t, ItemUint64>(DATA, NUM_DATA);

	// check
	for (size_t i = 0; i < NUM_DATA; i++) {
		uint64_t data = GET_UINT64_FROM_GRP(itemGrp, i);
		cppcut_assert_equal(DATA[i], data);
	}
}

void test_getIntFromGrp(void)
{
	static const int DATA[] = {-2984322, 3285, 0};
	static const size_t NUM_DATA = sizeof(DATA) / sizeof(int);
	ItemGroupPtr itemGrp = makeItemGroup<int, ItemInt>(DATA, NUM_DATA);
	// check
	for (size_t i = 0; i < NUM_DATA; i++) {
		int data = GET_INT_FROM_GRP(itemGrp, i);
		cppcut_assert_equal(DATA[i], data);
	}
}

void test_getStringFromGrp(void)
{
	static const string DATA[] = {"ABCE", "", " - ! - #\"'\\"};
	static const size_t NUM_DATA = sizeof(DATA) / sizeof(string);
	ItemGroupPtr itemGrp =
	   makeItemGroup<string, ItemString>(DATA, NUM_DATA);

	// check
	for (size_t i = 0; i < NUM_DATA; i++) {
		string data = GET_STRING_FROM_GRP(itemGrp, i);
		cppcut_assert_equal(DATA[i], data);
	}
}

} // namespace testActionManager
