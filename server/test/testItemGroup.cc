/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemData.h"
#include "ItemGroup.h"

namespace testItemGroup {

enum {
	DEFAULT_ITEM_ID,
	ITEM_ID_0,
	ITEM_ID_1,
	ITEM_ID_2,
};

const static int NUM_GROUP_POOL = 10;
static ItemGroup *g_grp[NUM_GROUP_POOL];
static ItemGroup *&x_grp = g_grp[0];
static ItemGroup *&y_grp = g_grp[1];

template<typename NATIVE_TYPE>
void _assertAddNew(void)
{
	struct {
		size_t idx;
		operator int (void) const
		{
			return idx * 8;
		}

		operator uint64_t (void) const
		{
			return idx * 12;
		}

		operator double (void) const
		{
			return idx * 0.1;
		}

		operator string (void) const
		{
			return StringUtils::sprintf("idx:%zd", idx);
		}

		ItemDataNullFlagType getNullFlag(void)
		{
			return idx % 2 ? ITEM_DATA_NOT_NULL : ITEM_DATA_NULL;
		}
	} dataGen;

	x_grp = new ItemGroup();
	const size_t numData = 5;
	for (dataGen.idx = 0; dataGen.idx < numData; dataGen.idx++) {
		NATIVE_TYPE data = dataGen;
		x_grp->addNewItem(data, dataGen.getNullFlag());
	}

	// check
	cppcut_assert_equal(numData, x_grp->getNumberOfItems());
	for (dataGen.idx = 0; dataGen.idx < numData; dataGen.idx++) {
		const ItemData *itemData = x_grp->getItemAt(dataGen.idx);
		NATIVE_TYPE expect = dataGen;
		NATIVE_TYPE actual = *itemData;
		cppcut_assert_equal(expect, actual);
		bool expectNull = (dataGen.getNullFlag() == ITEM_DATA_NULL);
		cppcut_assert_equal(expectNull, itemData->isNull());
		cppcut_assert_equal(1, itemData->getUsedCount());
	}
}
#define assertAddNew(NATIVE_TYPE) cut_trace((_assertAddNew<NATIVE_TYPE>)())

void cut_teardown()
{
	for (int i = 0; i < NUM_GROUP_POOL; i++) {
		if (g_grp[i]) {
			g_grp[i]->unref();
			g_grp[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_addAndGet(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemString *item1 = new ItemString(ITEM_ID_1, "foo");

	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->add(item1, false);

	cppcut_assert_equal(static_cast<ItemData *>(item0),
	                    x_grp->getItem(ITEM_ID_0));
	cppcut_assert_equal(static_cast<ItemData *>(item1),
	                    x_grp->getItem(ITEM_ID_1));
}

void test_addDuplicativeItemId(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemInt *item1 = new ItemInt(ITEM_ID_0, -8500);
	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->add(item1, false);
	cppcut_assert_equal(static_cast<ItemData *>(item0),
	                    x_grp->getItem(ITEM_ID_0));
}

void test_addDuplicativeItemIdAndGetThem(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemInt *item1 = new ItemInt(ITEM_ID_0, -8500);
	ItemInt *item2 = new ItemInt(ITEM_ID_1, 100);
	ItemInt *item3 = new ItemInt(ITEM_ID_0, 200);
	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->add(item1, false);
	x_grp->add(item2, false);
	x_grp->add(item3, false);
	ItemDataVector vec = x_grp->getItems(ITEM_ID_0);
	cppcut_assert_equal((size_t)3, vec.size());
	cppcut_assert_equal(static_cast<ItemData *>(item0), vec[0]);
	cppcut_assert_equal(static_cast<ItemData *>(item1), vec[1]);
	cppcut_assert_equal(static_cast<ItemData *>(item3), vec[2]);
}

void test_addWhenFreezed(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->freeze();
	bool gotException = false;
	try {
		x_grp->add(new ItemInt(ITEM_ID_2, -3), false);
	} catch (const HatoholException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_addNewInt(void)
{
	assertAddNew(int);
}

void test_addNewUint64(void)
{
	assertAddNew(uint64_t);
}

void test_addNewDouble(void)
{
	assertAddNew(double);
}

void test_addNewString(void)
{
	assertAddNew(string);
}

void test_getNumberOfItems(void)
{
	x_grp = new ItemGroup();
	cppcut_assert_equal((size_t)0, x_grp->getNumberOfItems());

	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	cppcut_assert_equal((size_t)1, x_grp->getNumberOfItems());

	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal((size_t)2, x_grp->getNumberOfItems());
}

void test_freeze(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal(false, x_grp->isFreezed());
	cppcut_assert_equal((const ItemGroupType *) NULL,
	                    x_grp->getItemGroupType());
	x_grp->freeze();
	cppcut_assert_equal(true, x_grp->isFreezed());
	cut_assert_not_null(x_grp->getItemGroupType());
}

void test_getItemGroupType(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal((const ItemGroupType *) NULL,
	                    x_grp->getItemGroupType());
	x_grp->freeze();
	const ItemGroupType *groupType = x_grp->getItemGroupType();
	cppcut_assert_equal((size_t)2, groupType->getSize());
	cppcut_assert_equal(ITEM_TYPE_INT, groupType->getType(0));
	cppcut_assert_equal(ITEM_TYPE_STRING, groupType->getType(1));
}

void test_getItemPtrAt(void)
{
	const int expect = 5;
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(expect), false);

	const ItemDataPtr itemPtr = x_grp->getItemPtrAt(0);
	cppcut_assert_equal(true, itemPtr.hasData());
	const ItemData *itemData = static_cast<const ItemData *>(itemPtr); 
	const ItemInt *itemInt = dynamic_cast<const ItemInt*>(itemData);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(expect, itemInt->get());
}

} // testItemGroup


