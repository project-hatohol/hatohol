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
};

static const int TEST_ITEM_ID = 1;

bool ItemIntTester::destructorCalled = false;

const static int NUM_ITEM_POOL = 10;
static ItemData *g_item[NUM_ITEM_POOL];
static ItemData *&x_item = g_item[0];
static ItemData *&y_item = g_item[1];
static ItemData *&z_item = g_item[2];
static ItemData *&w_item = g_item[3];

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
void test_getUint64(void)
{
	uint64_t val = 0xfedcba9876543210;
	ItemData *item = new ItemUint64(1, val);
	uint64_t readValue;
	item->get(&readValue);
	cut_assert_equal_int_least64(val, readValue);
}

void test_setUint64(void)
{
	uint64_t val = 0xfedcba9876543210;
	ItemData *item = new ItemUint64(1, val);
	val = 0x89abcdef01234567;
	uint64_t readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_int_least64(val, readValue);
}

void test_getInt(void)
{
	int val = 12345;
	ItemData *item = new ItemInt(1, val);
	int readValue;
	item->get(&readValue);
	cut_assert_equal_int(val, readValue);
}

void test_setInt(void)
{
	int val = 12345;
	ItemData *item = new ItemInt(1, val);
	val = 88;
	int readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_int(val, readValue);
}

void test_getString(void)
{
	string val = "test String";
	ItemData *item = new ItemString(1, val);
	string readValue;
	item->get(&readValue);
	cut_assert_equal_string(val.c_str(), readValue.c_str());
}

void test_setString(void)
{
	string val = "test String";
	ItemData *item = new ItemString(1, val);
	val = "FOO";
	string readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_string(val.c_str(), readValue.c_str());
}

void test_getValueInt(void)
{
	int val = 2345;
	ItemData *item = new ItemInt(TEST_ITEM_ID, val);
	ItemInt *itemInt = dynamic_cast<ItemInt *>(item);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(val, itemInt->get());
}

void test_getValueString(void)
{
	string val = "dog cat bird";
	ItemData *item = new ItemString(TEST_ITEM_ID, val);
	ItemString *itemString = dynamic_cast<ItemString *>(item);
	cppcut_assert_not_null(itemString);
	cppcut_assert_equal(val, itemString->get());
}

void test_getId(void)
{
	int id = 5;
	ItemData *item = new ItemInt(id, -5);
	cut_assert_equal_int(id, item->getId());
}

void test_ref_unref(void)
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

void test_operator_gt_int_positive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operator_gt_int_negative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operator_lt_int_positive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operator_lt_int_negative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operator_ge_int_positive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operator_ge_int_negative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operator_le_int_positive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operator_le_int_negative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operator_eq_int(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_equal(false, *x_item == *y_item);
	cppcut_assert_equal(true,  *x_item == *z_item);
	cppcut_assert_equal(false, *x_item == *w_item);
}

void test_operator_gt_uint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operator_lt_uint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operator_ge_uint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operator_le_uint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operator_eq_uint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 20);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 10);
	cppcut_assert_equal(false, *x_item == *y_item);
	cppcut_assert_equal(true,  *x_item == *z_item);
	cppcut_assert_equal(false, *x_item == *w_item);
}

void test_operator_gt_uint64_int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operator_lt_uint64_int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operator_ge_uint64_int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operator_le_uint64_int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operator_eq_uint64_int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_equal(false, *x_item == *y_item);
	cppcut_assert_equal(true,  *x_item == *z_item);
	cppcut_assert_equal(false, *x_item == *w_item);
}

} // namespace testItemData
