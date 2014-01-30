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
static ItemGroupPtr makeTestData(const T *expects, const size_t numExpects)
{
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < numExpects; i++)
		itemGroup->add(new ITEM_DATA(expects[i]), false);
	return ItemGroupPtr(itemGroup);
}

template<typename T, typename ITEM_DATA>
void _assertOperatorLeftShift(const T *expects, const size_t numExepects)
{
	ItemGroupPtr itemGroup
	  = makeTestData<T, ITEM_DATA>(expects, numExepects);
	ItemGroupStream igStream(itemGroup);
	for (size_t i = 0; i < numExepects; i++) {
		T actual;
		actual << igStream;
		cppcut_assert_equal(actual, expects[i]);
	}
}
#define assertOperatorLeftShift(T, ITEM_DATA, ARRAY, NUM) \
cut_trace((_assertOperatorLeftShift<T, ITEM_DATA>(ARRAY, NUM)));

template<typename T, typename ITEM_DATA>
void _assertOperatorRightShift(const T *expects, const size_t numExpects)
{
	ItemGroupPtr itemGroup
	  = makeTestData<T, ITEM_DATA>(expects, numExpects);
	ItemGroupStream igStream(itemGroup);
	for (size_t i = 0; i < numExpects; i++) {
		T actual;
		igStream >> actual;
		cppcut_assert_equal(actual, expects[i]);
	}
}
#define assertOperatorRightShift(T, ITEM_DATA, ARRAY, NUM) \
cut_trace((_assertOperatorRightShift<T, ITEM_DATA>(ARRAY, NUM)));

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_operatorRightShiftToInt(void)
{
	const int expects[] = {-3, 5, 8};
	const size_t numExepects = sizeof(expects) / sizeof(int);
	assertOperatorRightShift(int, ItemInt, expects, numExepects);
}

void test_operatorLeftShiftToInt(void)
{
	const int expects[] = {-3, 5, 8};
	const size_t numExepects = sizeof(expects) / sizeof(int);
	assertOperatorLeftShift(int, ItemInt, expects, numExepects);
}

void test_operatorLeftShiftToUint64(void)
{
	const uint64_t expects[] = {0xfedcba9876543210, 3, 0x7fffeeee5555};
	const size_t numExepects = sizeof(expects) / sizeof(uint64_t);
	assertOperatorLeftShift(uint64_t, ItemUint64, expects, numExepects);
}

void test_operatorLeftShiftToString(void)
{
	const string expects[] = {"FOO", "", "dog dog dog dog dog"};
	const size_t numExepects = sizeof(expects) / sizeof(string);
	assertOperatorLeftShift(string, ItemString, expects, numExepects);
}

void test_getItem(void)
{
	const size_t num_test = 3;
	vector<const ItemData *> itemDataVect;
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < num_test; i++) {
		const ItemData *itemData = new ItemInt(i);
		itemGroup->add(itemData, false);
		itemDataVect.push_back(itemData);
	}

	int dummy;
	ItemGroupStream itemGroupStream(itemGroup);
	for (size_t i = 0; i < num_test; i++) {
		const ItemData *actual = itemGroupStream.getItem();
		cppcut_assert_equal(itemDataVect[i], actual);
		dummy << itemGroupStream;
	}
}

} // namespace testItemGroupStream
