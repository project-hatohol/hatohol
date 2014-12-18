/*
 * Copyright (C) 2013 Project Hatohol
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

#include <cppcutter.h>
#include "ItemDataUtils.h"
#include "StringUtils.h"
using namespace std;
using namespace mlpl;

namespace testItemDataUtils {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createAsNumberInt(void)
{
	int number = 50;
	string numberStr = StringUtils::sprintf("%d", number);
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(numberStr);
	cppcut_assert_equal(true, dataPtr.hasData());
	const ItemInt *itemInt = dynamic_cast<const ItemInt *>(&*dataPtr);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(number, itemInt->get());
}

void test_createAsNumberInvalid(void)
{
	string word = "ABC";
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(word);
	cppcut_assert_equal(false, dataPtr.hasData());
}

} // namespace testItemDataUtils


