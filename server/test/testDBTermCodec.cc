/*
 * Copyright (C) 2014-2015 Project Hatohol
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
#include "DBTermCodec.h"
#include "DBAgentSQLite3.h"
#include "DataSamples.h"

using namespace std;

namespace testDBTermCodec {

void data_getInt(void)
{
	addDataSamplesForGCutInt();
}

void test_getInt(gconstpointer data)
{
	DBTermCodec dbTermCodec;
	string actual = dbTermCodec.enc(gcut_data_get_int(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

void data_getUint64(void)
{
	gcut_add_datum("Zero",
		       "val", G_TYPE_UINT64, (guint64)0,
		       "expect", G_TYPE_STRING, "0",
		       NULL);
	gcut_add_datum("Positive within 32bit",
		       "val", G_TYPE_UINT64, (guint64)3456,
		       "expect", G_TYPE_STRING, "3456",
		       NULL);
	gcut_add_datum("Positive 32bit Max",
		       "val", G_TYPE_UINT64, (guint64)2147483647,
		       "expect", G_TYPE_STRING, "2147483647",
		       NULL);
	gcut_add_datum("Positive 32bit Max + 1",
		       "val", G_TYPE_UINT64, (guint64)G_MAXINT32 + 1,
		       "expect", G_TYPE_STRING, "2147483648",
		       NULL);
	gcut_add_datum("Positive 64bit Poistive Max",
		       "val", G_TYPE_UINT64, 9223372036854775807UL,
		       "expect", G_TYPE_STRING, "9223372036854775807",
		       NULL);
	gcut_add_datum("Positive 64bit Poistive Max+1",
		       "val", G_TYPE_UINT64, 9223372036854775808UL,
		       "expect", G_TYPE_STRING, "9223372036854775808",
		       NULL);
	gcut_add_datum("Positive 64bit Max",
		       "val", G_TYPE_UINT64, 18446744073709551615UL,
		       "expect", G_TYPE_STRING, "18446744073709551615",
		       NULL);
}

void test_getUint64(gconstpointer data)
{
	DBTermCodec dbTermCodec;
	string actual = dbTermCodec.enc(gcut_data_get_uint64(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

void data_getString(void)
{
	gcut_add_datum("Empty",
	               "val", G_TYPE_STRING, "",
	               "expect", G_TYPE_STRING, "''",
	               NULL);
	gcut_add_datum("No quotations",
	               "val", G_TYPE_STRING,    "ABC foo",
	               "expect", G_TYPE_STRING, "'ABC foo'",
	               NULL);
	gcut_add_datum("At the first",
	               "val", G_TYPE_STRING,    "'ABC foo",
	               "expect", G_TYPE_STRING, "'''ABC foo'",
	               NULL);
	gcut_add_datum("At the last",
	               "val", G_TYPE_STRING,    "ABC foo'",
	               "expect", G_TYPE_STRING, "'ABC foo'''",
	               NULL);
	gcut_add_datum("At the middle",
	               "val", G_TYPE_STRING,    "AB'C fo'o",
	               "expect", G_TYPE_STRING, "'AB''C fo''o'",
	               NULL);
	gcut_add_datum("Consecutive",
	               "val", G_TYPE_STRING,    "AB'''C fo''o",
	               "expect", G_TYPE_STRING, "'AB''''''C fo''''o'",
	               NULL);
}

void test_getString(gconstpointer data)
{
	DBTermCodec dbTermCodec;
	string actual = dbTermCodec.enc(gcut_data_get_string(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

} // testDBTermCodec

namespace testDBTermCodecSQLite3 {

void data_getUint64(void)
{
	addDataSamplesForGCutUint64();
}

void test_getUint64(gconstpointer data)
{
	DBAgentSQLite3 dbAgent;
	const DBTermCodec *dbTermCodec = dbAgent.getDBTermCodec();
	string actual = dbTermCodec->enc(gcut_data_get_uint64(data, "val"));
	string expect = gcut_data_get_string(data, "expect");
	cppcut_assert_equal(expect, actual);
}

} // testDBTermCodecSQLite3
