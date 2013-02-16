#include <sstream>
#include <cppcutter.h>
#include "SQLUtils.h"

namespace testSQLUtils {

static ItemId testItemId = 1;
static const char *testTableName = "TestTable";
static const char *testColumnName = "TestTable";

static ColumnBaseDefinition testDefInt = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_INT, 11, 0
};

static ColumnBaseDefinition testDefBiguint = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_BIGUINT, 20, 0
};

static ColumnBaseDefinition testDefVarchar = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_VARCHAR, 30, 0
};

static ColumnBaseDefinition testDefChar = {
	testItemId, testTableName, testColumnName,
	SQL_COLUMN_TYPE_BIGUINT, 8, 0
};

template<class ValueType, class ItemDataType>
void _assertCreateItemData(const ColumnBaseDefinition *columnDefinition,
                           ValueType data)
{
	stringstream ss;
	ss << data;
	string value = ss.str();
	ItemDataPtr dataPtr = SQLUtils::createItemData(columnDefinition, value);
	ItemDataType *createdItemData =
	  dynamic_cast<ItemDataType *>((ItemData *)dataPtr);
	cppcut_assert_not_null(createdItemData);
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

void test_itemBiguint(void)
{
	uint64_t data = 0xfedcba9876543210;
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

} // namespace testSQLUtils
