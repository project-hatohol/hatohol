#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemDataPtr.h"

namespace testItemPtr {
static const int DEFAULT_ITEM_ID = 100;
static const int DEFAULT_INT_VALUE = 3;

static ItemDataPtr makeItemData(void)
{
	ItemDataPtr dataPtr(new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE),
	                    false);
	return dataPtr;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_numUsed(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID,
	                             DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, item->getUsedCount());
	ItemDataPtr dataPtr(item);
	cppcut_assert_equal(2, item->getUsedCount());
}

void test_numUsedWithoutRef(void)
{
	int val = 20;
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID,
	                             DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(item, false);
	cppcut_assert_equal(1, item->getUsedCount());
}

void test_numUsedWithBlock(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID,
	                             DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, item->getUsedCount());
	{
		ItemDataPtr dataPtr(item);
		cppcut_assert_equal(2, item->getUsedCount());
	}
	cppcut_assert_equal(1, item->getUsedCount());
}

void test_numUsedOneLine(void)
{
	ItemDataPtr dataPtr(new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE),
	                    false);
	cppcut_assert_equal(1, dataPtr->getUsedCount());
}

void test_getFromFunc(void)
{
	ItemDataPtr dataPtr = makeItemData();
	cppcut_assert_equal(1, dataPtr->getUsedCount());
}

void test_operateEq(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr;
	dataPtr = item;
	cppcut_assert_equal(2, dataPtr->getUsedCount());
}

} // namespace testItemPtr
