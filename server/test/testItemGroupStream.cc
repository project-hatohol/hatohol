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

void test_operatorRightShiftToUint64(void)
{
	const uint64_t expects[] = {0xfedcba9876543210, 3, 0x7fffeeee5555};
	const size_t numExepects = sizeof(expects) / sizeof(uint64_t);
	assertOperatorRightShift(uint64_t, ItemUint64, expects, numExepects);
}

void test_operatorRightShiftToDouble(void)
{
	const double expects[] = {0.5, -1.03e5, 3.141592};
	const size_t numExepects = sizeof(expects) / sizeof(double);
	assertOperatorRightShift(double, ItemDouble, expects, numExepects);
}

void test_operatorRightShiftToString(void)
{
	const string expects[] = {"FOO", "", "dog dog dog dog dog"};
	const size_t numExepects = sizeof(expects) / sizeof(string);
	assertOperatorRightShift(string, ItemString, expects, numExepects);
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

	ItemGroupStream itemGroupStream(itemGroup);
	for (size_t i = 0; i < num_test; i++) {
		const ItemData *actual = itemGroupStream.getItem();
		cppcut_assert_equal(itemDataVect[i], actual);
		itemGroupStream.read<int,int>(); // forward the stream position
	}
}

void test_set(void)
{
	struct {
		int calc(const ItemId id) {
			return id * 5;
		}
	} valueGenerator;

	// Make ItemGroup
	const size_t numItems = 10;
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < numItems; i++) {
		ItemId itemId = i;
		int data = valueGenerator.calc(itemId);
		itemGroup->add(new ItemInt(itemId, data), false);
	}
	ItemGroupStream itemGroupStream(itemGroup);

	// check: read()
	ItemId targetItemIds[] = {5, 1, 8, 4, 3};
	const size_t numTargetItemIds = sizeof(targetItemIds) / sizeof(ItemId);
	for (size_t i = 0; i < numTargetItemIds; i++) {
		const ItemId &targetItemId = targetItemIds[i];
		itemGroupStream.set(targetItemId);
		cppcut_assert_equal(valueGenerator.calc(targetItemId),
		                    itemGroupStream.read<int>());
	}

	// check: '>>'
	for (size_t i = 0; i < numTargetItemIds; i++) {
		int actual;
		const ItemId &targetItemId = targetItemIds[i];
		itemGroupStream.set(targetItemId);
		itemGroupStream >> actual;
		cppcut_assert_equal(valueGenerator.calc(targetItemId), actual);
	}
}

void test_setNotFound(void)
{
	// Make ItemGroup
	const size_t numItems = 3;
	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	for (size_t i = 0; i < numItems; i++)
		itemGroup->add(new ItemInt(i, 0), false);
	ItemGroupStream itemGroupStream(itemGroup);

	// check
	ItemDataExceptionType exceptionType = ITEM_DATA_EXCEPTION_UNKNOWN;
	int val = -1;
	try {
		itemGroupStream.set(numItems);
		val = itemGroupStream.read<int>();
	} catch (ItemDataException &e) {
		exceptionType = e.getType();
	}
	cppcut_assert_equal(ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND, exceptionType);
	cppcut_assert_equal(val, -1);
}

} // namespace testItemGroupStream
