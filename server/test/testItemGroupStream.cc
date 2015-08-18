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

#include <gcutter.h>
#include <cppcutter.h>
#include "Helpers.h"
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
	const size_t numExepects = ARRAY_SIZE(expects);
	assertOperatorRightShift(int, ItemInt, expects, numExepects);
}

void test_operatorRightShiftToUint64(void)
{
	const uint64_t expects[] = {0xfedcba9876543210, 3, 0x7fffeeee5555};
	const size_t numExepects = ARRAY_SIZE(expects);
	assertOperatorRightShift(uint64_t, ItemUint64, expects, numExepects);
}

void test_operatorRightShiftToDouble(void)
{
	const double expects[] = {0.5, -1.03e5, 3.141592};
	const size_t numExepects = ARRAY_SIZE(expects);
	assertOperatorRightShift(double, ItemDouble, expects, numExepects);
}

void test_operatorRightShiftToString(void)
{
	const string expects[] = {"FOO", "", "dog dog dog dog dog"};
	const size_t numExepects = ARRAY_SIZE(expects);
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
	const size_t numTargetItemIds = ARRAY_SIZE(targetItemIds);
	for (size_t i = 0; i < numTargetItemIds; i++) {
		const ItemId &targetItemId = targetItemIds[i];
		itemGroupStream.seek(targetItemId);
		cppcut_assert_equal(valueGenerator.calc(targetItemId),
		                    itemGroupStream.read<int>());
	}

	// check: '>>'
	for (size_t i = 0; i < numTargetItemIds; i++) {
		int actual;
		const ItemId &targetItemId = targetItemIds[i];
		itemGroupStream.seek(targetItemId);
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
		itemGroupStream.seek(numItems);
		val = itemGroupStream.read<int>();
	} catch (ItemDataException &e) {
		exceptionType = e.getType();
	}
	cppcut_assert_equal(ITEM_DATA_EXCEPTION_ITEM_NOT_FOUND, exceptionType);
	cppcut_assert_equal(val, -1);
}

void data_readUint64ViaString(void)
{
	gcut_add_datum("zero",
	               "string", G_TYPE_STRING, "0",
	               "number", G_TYPE_UINT64, 0,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("signed 32bit max",
	               "string", G_TYPE_STRING, "2147483647",
	               "number", G_TYPE_UINT64, 2147483647,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("signed 32bit max + 1",
	               "string", G_TYPE_STRING, "2147483648",
	               "number", G_TYPE_UINT64, 2147483648,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("signed 64bit",
	               "string", G_TYPE_STRING, "9223372036854775807",
	               "number", G_TYPE_UINT64, 9223372036854775807UL,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("signed 64bit + 1",
	               "string", G_TYPE_STRING, "9223372036854775808",
	               "number", G_TYPE_UINT64, 9223372036854775808UL,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("unsigned 64bit max",
	               "string", G_TYPE_STRING, "18446744073709551615",
	               "number", G_TYPE_UINT64, 18446744073709551615UL,
	               "valid",  G_TYPE_BOOLEAN, TRUE,  NULL);
	gcut_add_datum("Non-number",
	               "string", G_TYPE_STRING, "abc",
	               "number", G_TYPE_UINT64, 0,
	               "valid",  G_TYPE_BOOLEAN, FALSE,  NULL);
}

void test_readUint64ViaString(gconstpointer data)
{
	const char *srcString = gcut_data_get_string(data, "string");
	const uint64_t expect = gcut_data_get_uint64(data, "number");
	const bool valid = gcut_data_get_boolean(data, "valid");

	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	itemGroup->add(new ItemString(srcString), false);
	ItemGroupStream itemGroupStream(itemGroup);
	uint64_t actual = 0;
	HatoholError err(HTERR_OK);
	try {
		actual = itemGroupStream.read<string, uint64_t>();
	} catch (const HatoholException &e) {
		err = e.getErrCode();
	}

	if (valid) {
		assertHatoholError(HTERR_OK, err);
		cppcut_assert_equal(expect, actual);
	} else {
		assertHatoholError(HTERR_INTERNAL_ERROR, err);
	}
}

void data_readStringViaInt(void)
{
	gcut_add_datum("zero",
	               "native", G_TYPE_INT, 0,
	               "expect", G_TYPE_STRING, "0", NULL);
	gcut_add_datum("positive",
	               "native", G_TYPE_INT, 15,
	               "expect", G_TYPE_STRING, "15", NULL);
	gcut_add_datum("signed 32bit max",
	               "native", G_TYPE_INT, 2147483647,
	               "expect", G_TYPE_STRING, "2147483647", NULL);
	gcut_add_datum("nagative",
	               "native", G_TYPE_INT, -5,
	               "expect", G_TYPE_STRING, "-5", NULL);
	gcut_add_datum("signed 32bit min",
	               "native", G_TYPE_INT, -2147483648,
	               "expect", G_TYPE_STRING, "-2147483648", NULL);
}

void test_readStringViaInt(gconstpointer data)
{
	const int nativeValue = gcut_data_get_int(data, "native");
	const string expect = string(gcut_data_get_string(data, "expect"));

	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	itemGroup->add(new ItemInt(nativeValue), false);
	ItemGroupStream itemGroupStream(itemGroup);
	const string actual = itemGroupStream.read<int, string>();
	cppcut_assert_equal(expect, actual);
}

void data_readStringViaUint64(void)
{
	gcut_add_datum("zero",
	               "native", G_TYPE_UINT64, 0UL,
	               "expect", G_TYPE_STRING, "0", NULL);
	gcut_add_datum("under 32bit max",
	               "native", G_TYPE_UINT64, 15UL,
	               "expect", G_TYPE_STRING, "15", NULL);
	gcut_add_datum("unsinged 32bit max",
	               "native", G_TYPE_UINT64, 4294967295UL,
	               "expect", G_TYPE_STRING, "4294967295", NULL);
	gcut_add_datum("unsigned 32bit max + 1",
	               "native", G_TYPE_UINT64, 4294967296UL,
	               "expect", G_TYPE_STRING, "4294967296", NULL);
	gcut_add_datum("unsigned 64bit max",
	               "native", G_TYPE_UINT64, 18446744073709551615UL,
	               "expect", G_TYPE_STRING, "18446744073709551615", NULL);
}

void test_readStringViaUint64(gconstpointer data)
{
	const uint64_t nativeValue = gcut_data_get_uint64(data, "native");
	const string expect = string(gcut_data_get_string(data, "expect"));

	VariableItemGroupPtr itemGroup(new ItemGroup(), false);
	itemGroup->add(new ItemUint64(nativeValue), false);
	ItemGroupStream itemGroupStream(itemGroup);
	const string actual = itemGroupStream.read<uint64_t, string>();
	cppcut_assert_equal(expect, actual);
}

} // namespace testItemGroupStream
