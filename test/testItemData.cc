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

	int callGetUsedCount(void) {
		return getUsedCount();
	}
};

bool ItemIntTester::destructorCalled = false;

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
	cppcut_assert_equal(1, tester->callGetUsedCount());
	tester->ref();
	cppcut_assert_equal(2, tester->callGetUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(1, tester->callGetUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(true, tester->destructorCalled);
}

} // namespace testItemData
