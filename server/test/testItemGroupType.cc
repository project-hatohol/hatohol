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

#include <string>
#include <vector>
#include <stdexcept>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemData.h"
#include "ItemGroupType.h"

namespace testItemGroupType {

enum {
	DEFAULT_ITEM_ID,
	ITEM_ID_0,
	ITEM_ID_1,
	ITEM_ID_2,
	ITEM_ID_3,
};


const static int NUM_ITEM_VEC_POOL = 10;
static ItemDataVector g_itemVec[NUM_ITEM_VEC_POOL];
static ItemDataVector &x_itemVec = g_itemVec[0];
static ItemDataVector &y_itemVec = g_itemVec[1];

void cut_teardown()
{
	for (int i = 0; i < NUM_ITEM_VEC_POOL; i++) {
		if (!g_itemVec[i].empty()) {
			for (size_t j = 0; j < g_itemVec[i].size(); j++)
				g_itemVec[i][j]->unref();
			g_itemVec[i].clear();
		}
	}
}

static void assertGotOutOfRangeException(ItemGroupType &grpType, size_t index)
{
	bool gotException = false;
	try {
		grpType.getType(index);
	} catch (out_of_range e) {
		gotException = true;
	}
	cut_trace(cppcut_assert_equal(true, gotException));
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType(x_itemVec);
}

void test_getSize(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType(x_itemVec);
	cut_assert_equal_int(2, grpType.getSize());
}

void test_getType(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType(x_itemVec);
	cut_assert_equal_int(ITEM_TYPE_INT, grpType.getType(0));
	cut_assert_equal_int(ITEM_TYPE_STRING, grpType.getType(1));
}

void test_getTypeInvalid(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType(x_itemVec);
	assertGotOutOfRangeException(grpType, 2);
}

void test_operatorLessSizeDiffLess(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(true, grpType0 < grpType1);
}

void test_operatorLessSizeEqLess(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemString(ITEM_ID_2, "foo"));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(ITEM_TYPE_INT < ITEM_TYPE_STRING,
	                    grpType0 < grpType1);
}

void test_operatorLessSizeEqGreate(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_2, "foo"));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(ITEM_TYPE_STRING < ITEM_TYPE_INT,
	                    grpType0 < grpType1);
}

void test_operatorLessEq(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 < grpType1);
}


void test_operatorLessSizeDiffGreat(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 < grpType1);
}

void test_operatorEqSizeDiff(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 == grpType1);
}

void test_operatorEqSizeEqTypeDiff(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemString(ITEM_ID_2, "foo"));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 == grpType1);
}

void test_operatorEqTypeDiff2(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_2, "foo"));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 == grpType1);
}

void test_operatorEqEq(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	y_itemVec.push_back(new ItemInt(ITEM_ID_1, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(true, grpType0 == grpType1);
}


void test_operatorEqSizeDiffGreat(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(false, grpType0 == grpType1);
}

void test_operatorNotEqSizeDiffGreat(void)
{
	x_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	x_itemVec.push_back(new ItemString(ITEM_ID_1, "foo"));
	ItemGroupType grpType0(x_itemVec);

	y_itemVec.push_back(new ItemInt(ITEM_ID_0, 5));
	ItemGroupType grpType1(y_itemVec);

	cppcut_assert_equal(true, grpType0 != grpType1);
}


} // testItemGroupType


