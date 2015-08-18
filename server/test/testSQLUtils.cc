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

#include <sstream>
#include <cppcutter.h>
#include <time.h>
#include "DBAgentTest.h"
#include "SQLUtils.h"
using namespace std;
using namespace mlpl;

namespace testSQLUtils {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createFromStringInt(void)
{
	int val = 5;
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(StringUtils::sprintf("%d", val).c_str(),
	                             SQL_COLUMN_TYPE_INT);
	int actual = *dataPtr;
	cppcut_assert_equal(val, actual);
}

void test_createFromStringIntWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_INT);
	cppcut_assert_equal(true, dataPtr->isNull());
	cppcut_assert_equal(ITEM_TYPE_INT, dataPtr->getItemType());
}

void test_createFromStringBiguint(void)
{
	uint64_t val = 0x89abcdef01234567;
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(
	    StringUtils::sprintf("%" PRIu64, val).c_str(),
	                         SQL_COLUMN_TYPE_BIGUINT);
	uint64_t actual = *dataPtr;
	cppcut_assert_equal(val, actual);
}

void test_createFromStringBiguintWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_BIGUINT);
	cppcut_assert_equal(true, dataPtr->isNull());
	cppcut_assert_equal(ITEM_TYPE_UINT64, dataPtr->getItemType());
}

void test_createFromStringVarchar(void)
{
	string val = "I like a soft-serve ice cream.";
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(val.c_str(), SQL_COLUMN_TYPE_VARCHAR);
	string actual = *dataPtr;
	cppcut_assert_equal(val, actual);
}

void test_createFromStringVarcharWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_VARCHAR);
	cppcut_assert_equal(true, dataPtr->isNull());
	cppcut_assert_equal(ITEM_TYPE_STRING, dataPtr->getItemType());
}

void test_createFromStringDouble(void)
{
	double val = 0.123456789012345;
	string valStr = StringUtils::sprintf("%.15lf", val);
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(valStr.c_str(), SQL_COLUMN_TYPE_DOUBLE);
	double actual = *dataPtr;
	cppcut_assert_equal(val, actual);
}

void test_createFromStringDoubleWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_DOUBLE);
	cppcut_assert_equal(true, dataPtr->isNull());
	cppcut_assert_equal(ITEM_TYPE_DOUBLE, dataPtr->getItemType());
}

void test_createFromStringDatetime(void)
{
	tzset();
	time_t time_utc = 1369830365; // 2013-05-29 12:26:05 +000
	time_t time_local = time_utc + timezone;
	struct tm tm;
	tm.tm_year = 2013;
	tm.tm_mon  = 5;
	tm.tm_mday = 29;
	tm.tm_hour = 12;
	tm.tm_min  = 26;
	tm.tm_sec  = 5;
	string str = StringUtils::sprintf("%04d-%02d-%02d %02d:%02d:%02d",
	                                  tm.tm_year, tm.tm_mon, tm.tm_mday,
	                                  tm.tm_hour, tm.tm_min, tm.tm_sec);
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(str.c_str(), SQL_COLUMN_TYPE_DATETIME);
	int actual = *dataPtr;
	cppcut_assert_equal((int)time_local, actual);
}

void test_createFromStringDatetimeWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_DATETIME);
	cppcut_assert_equal(true, dataPtr->isNull());
	cppcut_assert_equal(ITEM_TYPE_INT, dataPtr->getItemType());
}
} // namespace testSQLUtils
