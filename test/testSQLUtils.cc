#include <sstream>
#include <cppcutter.h>
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

} // namespace testSQLUtils
