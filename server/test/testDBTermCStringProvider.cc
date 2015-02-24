/*
 * Copyright (C) 2015 Project Hatohol
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
#include <gcutter.h>
#include "DBTermCStringProvider.h"

using namespace std;

namespace testDBTermCStringProvider {

void test_int(void)
{
	DBTermCodec codec;
	DBTermCStringProvider csProvider(codec);
	const int val = 1234567;
	cppcut_assert_equal("1234567", csProvider(val));
	cppcut_assert_equal((size_t)1, csProvider.getStoredStringList().size());

	const int val2 = -10;
	cppcut_assert_equal("-10", csProvider(val2));
	cppcut_assert_equal((size_t)2, csProvider.getStoredStringList().size());
}

void test_uint64(void)
{
	DBTermCodec codec;
	DBTermCStringProvider csProvider(codec);
	const uint64_t val = 0x123456789abcdef0;
	cppcut_assert_equal("1311768467463790320", csProvider(val));
	cppcut_assert_equal((size_t)1, csProvider.getStoredStringList().size());

	const uint64_t val2 = 0xfedcba9876543210;
	cppcut_assert_equal("18364758544493064720", csProvider(val2));
	cppcut_assert_equal((size_t)2, csProvider.getStoredStringList().size());
}

void test_string(void)
{
	DBTermCodec codec;
	DBTermCStringProvider csProvider(codec);
	const string val = "Lovin' You";
	cppcut_assert_equal("'Lovin'' You'", csProvider(val));
	cppcut_assert_equal((size_t)1, csProvider.getStoredStringList().size());

	const string val2 = "I'm a cat.";
	cppcut_assert_equal("'I''m a cat.'", csProvider(val2));
	cppcut_assert_equal((size_t)2, csProvider.getStoredStringList().size());
}

} // testDBTermCStringProvider
