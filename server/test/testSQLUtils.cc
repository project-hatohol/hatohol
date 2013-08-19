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

#include <sstream>
#include <cppcutter.h>
#include <time.h>
#include "SQLUtils.h"

namespace testSQLUtils {

static ItemId testItemId = 1;
static const char *testTableName = "TestTable";
static const char *testColumnName = "TestTable";

static ColumnDef testDefInt = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_INT, 11, 0
};

static ColumnDef testDefBiguint = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_BIGUINT, 20, 0
};

static ColumnDef testDefVarchar = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_VARCHAR, 30, 0
};

static ColumnDef testDefChar = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_CHAR, 8, 0
};

template<class ValueType, class ItemDataType>
void _assertCreateItemData(const ColumnDef *columnDefinition,
                           ValueType data)
{
	stringstream ss;
	ss << data;
	string value = ss.str();
	ItemDataPtr dataPtr = SQLUtils::createItemData(columnDefinition, value);
	const ItemDataType *createdItemData =
	  dynamic_cast<const ItemDataType *>(&*dataPtr);
	cppcut_assert_not_null(createdItemData);
	cppcut_assert_equal(testItemId, createdItemData->getId());
	ValueType actual = createdItemData->get();
	cppcut_assert_equal(data, actual);
}
#define assertCreateItemData(VT, IDT, DEF, V) \
cut_trace((_assertCreateItemData<VT,IDT>(DEF, V)))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_itemInt(void)
{
	int data = 10;
	assertCreateItemData(int, ItemInt, &testDefInt, data);
}

void test_itemIntNegative(void)
{
	int data = -510;
	assertCreateItemData(int, ItemInt, &testDefInt, data);
}

void test_itemBiguint32bitMax(void)
{
	uint64_t data = 0x00000000ffffffff;
	assertCreateItemData(uint64_t, ItemUint64, &testDefBiguint, data);
}

void test_itemBiguint64bitMin(void)
{
	uint64_t data = 0x0000000100000000;
	assertCreateItemData(uint64_t, ItemUint64, &testDefBiguint, data);
}

void test_itemBiguint64bitHalf7f(void)
{
	uint64_t data = 0x7fffffffffffffff;
	assertCreateItemData(uint64_t, ItemUint64, &testDefBiguint, data);
}

void test_itemBiguint64bitHalf80(void)
{
	uint64_t data = 0x8000000000000000;
	assertCreateItemData(uint64_t, ItemUint64, &testDefBiguint, data);
}

void test_itemBiguint64bitMax(void)
{
	uint64_t data = 0xffffffffffffffff;
	assertCreateItemData(uint64_t, ItemUint64, &testDefBiguint, data);
}

void test_itemVarchar(void)
{
	string data = "foo";
	assertCreateItemData(string, ItemString, &testDefVarchar, data);
}

void test_itemChar(void)
{
	string data = "foo";
	assertCreateItemData(string, ItemString, &testDefChar, data);
}

void test_createFromStringInt(void)
{
	int val = 5;
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(StringUtils::sprintf("%d", val).c_str(),
	                             SQL_COLUMN_TYPE_INT);
	cppcut_assert_equal(val, ItemDataUtils::getInt(dataPtr));
}

void test_createFromStringIntWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_INT);
	cppcut_assert_equal(true, dataPtr->isNull());
}

void test_createFromStringBiguint(void)
{
	uint64_t val = 0x89abcdef01234567;
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(
	    StringUtils::sprintf("%"PRIu64, val).c_str(),
	                         SQL_COLUMN_TYPE_BIGUINT);
	cppcut_assert_equal(val, ItemDataUtils::getUint64(dataPtr));
}

void test_createFromStringBiguintWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_BIGUINT);
	cppcut_assert_equal(true, dataPtr->isNull());
}

void test_createFromStringVarchar(void)
{
	string val = "I like a soft-serve ice cream.";
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(val.c_str(), SQL_COLUMN_TYPE_VARCHAR);
	cppcut_assert_equal(val, ItemDataUtils::getString(dataPtr));
}

void test_createFromStringVarcharWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_VARCHAR);
	cppcut_assert_equal(true, dataPtr->isNull());
}

void test_createFromStringDouble(void)
{
	double val = 0.123456789012345;
	string val_str = StringUtils::sprintf("%.15lf", val);
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(val_str.c_str(), SQL_COLUMN_TYPE_DOUBLE);
	cppcut_assert_equal(val, ItemDataUtils::getDouble(dataPtr));
}

void test_createFromStringDoubleWithNull(void)
{
	ItemDataPtr dataPtr =
	  SQLUtils::createFromString(NULL, SQL_COLUMN_TYPE_DOUBLE);
	cppcut_assert_equal(true, dataPtr->isNull());
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
	cppcut_assert_equal((int)time_local, ItemDataUtils::getInt(dataPtr));
}

} // namespace testSQLUtils
