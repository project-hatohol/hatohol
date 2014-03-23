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
#include <gcutter.h>
#include "OptionTermGenerator.h"

using namespace std;

namespace testOptionTermGenerator {

void data_getInt(void)
{
	gcut_add_datum("Zero",
		       "val", G_TYPE_INT, 0,
		       "expect", G_TYPE_STRING, "0",
		       NULL);
	gcut_add_datum("Positive within 32bit",
		       "val", G_TYPE_INT, 3456,
		       "expect", G_TYPE_STRING, "3456",
		       NULL);
	gcut_add_datum("Positive 32bit Max",
		       "val", G_TYPE_INT, 2147483647,
		       "expect", G_TYPE_STRING, "2147483647",
		       NULL);
	gcut_add_datum("Negative within 32bit",
		       "val", G_TYPE_INT, -1389,
		       "expect", G_TYPE_STRING, "-1389",
		       NULL);
	gcut_add_datum("Positive 32bit Min",
		       "val", G_TYPE_INT, -2147483648,
		       "expect", G_TYPE_STRING, "-2147483648",
		       NULL);
}

void test_getInt(gconstpointer data)
{
	OptionTermGenerator termGen;
	string actual = termGen.get(gcut_data_get_int(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

void data_getUint64(void)
{
	gcut_add_datum("Zero",
		       "val", G_TYPE_UINT64, 0,
		       "expect", G_TYPE_STRING, "0",
		       NULL);
	gcut_add_datum("Positive within 32bit",
		       "val", G_TYPE_UINT64, 3456,
		       "expect", G_TYPE_STRING, "3456",
		       NULL);
	gcut_add_datum("Positive 32bit Max",
		       "val", G_TYPE_UINT64, 2147483647,
		       "expect", G_TYPE_STRING, "2147483647",
		       NULL);
	gcut_add_datum("Positive 32bit Max + 1",
		       "val", G_TYPE_UINT64, 2147483648,
		       "expect", G_TYPE_STRING, "2147483648",
		       NULL);
	gcut_add_datum("Positive 64bit Max",
		       "val", G_TYPE_UINT64, 18446744073709551615UL,
		       "expect", G_TYPE_STRING, "18446744073709551615",
		       NULL);
}

void test_getUint64(gconstpointer data)
{
	OptionTermGenerator termGen;
	string actual = termGen.get(gcut_data_get_uint64(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

} // testOptionTermGenerator
