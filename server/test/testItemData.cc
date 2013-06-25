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
#include "ItemDataUtils.h"

namespace testItemData {

class ItemIntTester : public ItemInt {
public:
	static bool destructorCalled;

	ItemIntTester(ItemId id, int data)
	: ItemInt(id, data)
	{
	}

	~ItemIntTester() {
		destructorCalled = true;
	}

	void throwUndefinedOperator(void)
	{
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION("TestOperator");
	}

	void throwUndefinedOperatorTwoArg(void)
	{
		THROW_ITEM_DATA_EXCEPTION_UNDEFINED_OPERATION(
		  "TestOperator", *this);
	}
};

static const int TEST_ITEM_ID = 1;

bool ItemIntTester::destructorCalled = false;

const static int NUM_ITEM_POOL = 10;
static ItemData *g_item[NUM_ITEM_POOL];
static ItemData *&x_item = g_item[0];
static ItemData *&y_item = g_item[1];
static ItemData *&z_item = g_item[2];
static ItemData *&w_item = g_item[3];

// for assertion helpers
static ItemData *&A_item = g_item[9];
static ItemData *&B_item = g_item[8];
static ItemData *&C_item = g_item[7];

template<typename NativeType, class ItemDataType>
static void _assertGet(NativeType val)
{
	A_item = new ItemDataType(val);
	NativeType readValue = ItemDataUtils::get<NativeType>(A_item);
	cppcut_assert_equal(val, readValue);
}
#define assertGet(NT,IDT,V) cut_trace((_assertGet<NT,IDT>(V)))

template<typename NativeType, class ItemDataType>
static void _assertGetValue(NativeType val)
{
	A_item = new ItemDataType(val);
	ItemDataType *itemSub = dynamic_cast<ItemDataType *>(A_item);
	cppcut_assert_not_null(itemSub);
	cppcut_assert_equal(val, itemSub->get());
}
#define assertGetValue(NT,IDT,V) cut_trace((_assertGetValue<NT,IDT>(V)))

template<typename NativeType, class ItemDataType>
static void _assertGetString(NativeType val)
{
	A_item = new ItemDataType(val);
	stringstream ss;
	ss << val;
	cppcut_assert_equal(ss.str(), A_item->getString());
}
#define assertGetString(NT,IDT,V) cut_trace((_assertGetString<NT,IDT>(V)))

template <typename T, typename ItemDataType>
void assertOperatorPlus(T &v0, T &v1)
{
	A_item = new ItemDataType(v0);
	B_item = new ItemDataType(v1);
	C_item = *A_item + *B_item;
	cppcut_assert_not_null(C_item);
	ItemDataType *item = dynamic_cast<ItemDataType *>(C_item);
	cppcut_assert_not_null(item);
	cppcut_assert_equal(v0 + v1, item->get());
}

void teardown(void)
{
	for (int i = 0; i < NUM_ITEM_POOL; i++) {
		if (g_item[i]) {
			g_item[i]->unref();
			g_item[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructWithoutSpecificId(void)
{
	int val = 2080;
	x_item = new ItemUint64(val);
	cppcut_assert_equal(SYSTEM_ITEM_ID_ANONYMOUS, x_item->getId());
}

void test_getId(void)
{
	int id = 5;
	x_item = new ItemInt(id, -5);
	cut_assert_equal_int(id, x_item->getId());
}

void test_RefUnref(void)
{
	ItemIntTester *tester = new ItemIntTester(1, 10);
	tester->destructorCalled = false;
	cppcut_assert_equal(1, tester->getUsedCount());
	tester->ref();
	cppcut_assert_equal(2, tester->getUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(1, tester->getUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(true, tester->destructorCalled);
}

void test_Clone(void)
{
	const ItemId id = 5;
	const int val = -8;
	x_item = new ItemInt(id, val);
	cppcut_assert_not_null(x_item);
	y_item = x_item->clone();
	cppcut_assert_not_null(y_item);
	cppcut_assert_equal(id, y_item->getId());
	cppcut_assert_equal(x_item->getItemType(), y_item->getItemType());
	cppcut_assert_equal(val, ItemDataUtils::get<int>(y_item));
}

// -------------------------------------------------------------------------
// get
// -------------------------------------------------------------------------
void test_getBool(void)
{
	assertGet(bool, ItemBool, true);
}

void test_getInt(void)
{
	assertGet(int, ItemInt, 12345);
}

void test_getUint64(void)
{
	assertGet(uint64_t, ItemUint64, 0xfedcba9876543210);
}

void test_getDouble(void)
{
	assertGet(double, ItemDouble, 0.98);
}

void test_getString(void)
{
	assertGet(string, ItemString, "test String");
}

// -------------------------------------------------------------------------
// getValue
// -------------------------------------------------------------------------
void test_getValueBool(void)
{
	assertGetValue(bool, ItemBool, false);
}

void test_getValueInt(void)
{
	assertGetValue(int, ItemInt, 2345);
}

void test_getValueUint64(void)
{
	assertGetValue(uint64_t, ItemUint64, 0x123456789abcedef);
}

void test_getValueDouble(void)
{
	assertGetValue(double, ItemDouble, 5.8213e3);
}

void test_getValueString(void)
{
	assertGetValue(string, ItemString, "dog cat bird");
}

// -------------------------------------------------------------------------
// getString
// -------------------------------------------------------------------------
void test_getStringBool(void)
{
	assertGetString(bool, ItemBool, true);
	assertGetString(bool, ItemBool, false);
}

void test_getStringInt(void)
{
	assertGetString(int, ItemInt, -50);
}

void test_getStringUint64(void)
{
	assertGetString(uint64_t, ItemUint64, 0x1234567890abcdef);
}

void test_getStringDouble(void)
{
	assertGetString(double, ItemDouble, 2.34568e16);
}

void test_getStringString(void)
{
	assertGetString(string, ItemString, "music");
}

// -------------------------------------------------------------------------
// operator cast to bool
// -------------------------------------------------------------------------
void test_castBoolToBool(void)
{
	x_item = new ItemBool(true);
	cppcut_assert_equal(true, static_cast<bool>(*x_item));

	y_item = new ItemBool(false);
	cppcut_assert_equal(false, static_cast<bool>(*y_item));
}


// -------------------------------------------------------------------------
// operatorGt
// -------------------------------------------------------------------------
void test_operatorGtIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorGtIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorGtUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorGtUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorGtDouble(void)
{
	x_item = new ItemDouble(12.8);
	y_item = new ItemDouble(11);
	z_item = new ItemDouble(12.8);
	w_item = new ItemDouble(20.5);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

// -------------------------------------------------------------------------
// operatorGe
// -------------------------------------------------------------------------
void test_operatorGeIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorGeIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorGeUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorGeUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

// -------------------------------------------------------------------------
// operatorLt
// -------------------------------------------------------------------------
void test_operatorLtUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtIntUint64(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtIntUint64Negative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	cppcut_assert_equal(true, *x_item < *y_item);
}

// -------------------------------------------------------------------------
// operatorLe
// -------------------------------------------------------------------------
void test_operatorLeIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorLeIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorLeUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

// -------------------------------------------------------------------------
// operator Eq
// -------------------------------------------------------------------------
void test_operatorEqBool(void)
{
	x_item = new ItemBool(true);
	y_item = new ItemBool(true);
	cppcut_assert_equal(*x_item, *y_item);
}

void test_operatorEqInt(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}

void test_operatorEqUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 20);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}

void test_operatorLeUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorEqUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}

// -------------------------------------------------------------------------
// operator NotEq
// -------------------------------------------------------------------------
void test_operatorNotEqBool(void)
{
	x_item = new ItemBool(true);
	y_item = new ItemBool(false);
	cppcut_assert_not_equal(*x_item, *y_item);
}

// -------------------------------------------------------------------------
// operator Plus
// -------------------------------------------------------------------------
void test_operatorPlusInt(void)
{
	int v0 = 20;
	int v1 = -50;
	assertOperatorPlus<int, ItemInt>(v0, v1);
}

void test_operatorPlusUint64(void)
{
	uint64_t v0 = 58320;
	uint64_t v1 = 250;
	assertOperatorPlus<uint64_t, ItemUint64>(v0, v1);
}

// -------------------------------------------------------------------------
// operator Substitute
// -------------------------------------------------------------------------
void test_operatorSubstBoolString(void)
{
	bool gotException = false;
	x_item = new ItemBool(true);
	y_item = new ItemString("Foo");
	try {
		*x_item = *y_item;
	} catch (const ItemDataException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

// ---------------------------------------------------------------------------
// ItemDataException
// ---------------------------------------------------------------------------
void test_throwUndefinedOperator(void)
{
	ItemIntTester *tester = new ItemIntTester(SYSTEM_ITEM_ID_ANONYMOUS, 0);
	bool gotException = false;
	try {
		tester->throwUndefinedOperator();
	} catch (const ItemDataException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_throwUndefinedOperatorTwoArg(void)
{
	ItemIntTester *tester = new ItemIntTester(SYSTEM_ITEM_ID_ANONYMOUS, 0);
	bool gotException = false;
	try {
		tester->throwUndefinedOperatorTwoArg();
	} catch (const ItemDataException &e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

} // namespace testItemData
