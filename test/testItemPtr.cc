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
	item->unref();
}

void test_constructWithNull(void)
{
	ItemDataPtr dataPtr(NULL);
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
	item->unref();
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

void test_operatorSubst(void)
{
	ItemDataPtr dataPtr(new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE),
	                    false);
	cppcut_assert_equal(1, dataPtr->getUsedCount());
	{
		ItemDataPtr dataPtr2 = dataPtr;
		cppcut_assert_equal(2, dataPtr->getUsedCount());
		cppcut_assert_equal(2, dataPtr2->getUsedCount());
	}
	cppcut_assert_equal(1, dataPtr->getUsedCount());
}

void test_operatorSubtFromItemData(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, item->getUsedCount());
	ItemDataPtr dataPtr;
	dataPtr = item;
	cppcut_assert_equal(2, dataPtr->getUsedCount());
	cppcut_assert_equal(2, item->getUsedCount());
	item->unref();
}

void test_constructWithSubtFromItemData(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, item->getUsedCount());
	ItemDataPtr dataPtr = item;
	cppcut_assert_equal(2, dataPtr->getUsedCount());
	cppcut_assert_equal(2, item->getUsedCount());
	item->unref();
}

void test_operatorSubstChain(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr1, dataPtr2;
	dataPtr1 = dataPtr2 = item;
	cppcut_assert_equal(3, item->getUsedCount());
	cppcut_assert_equal(3, dataPtr1->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());
	item->unref();
}

void test_operatorSubstChangeRefCount(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(item);
	cppcut_assert_equal(2, dataPtr->getUsedCount());

	ItemData *item2 = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr2(item2);
	cppcut_assert_equal(2, item2->getUsedCount());
	cppcut_assert_equal(2, dataPtr2->getUsedCount());

	dataPtr2 = dataPtr;
	cppcut_assert_equal(1, item2->getUsedCount());
	cppcut_assert_equal(3, item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());
	item->unref();
	item2->unref();
}

void test_operatorSubstWithSameBody(void)
{
	ItemData *item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(item);
	cppcut_assert_equal(2, dataPtr->getUsedCount());

	ItemDataPtr dataPtr2(item);
	cppcut_assert_equal(3, item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());

	dataPtr2 = dataPtr;
	cppcut_assert_equal(3, item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());

	dataPtr = dataPtr2;
	cppcut_assert_equal(3, item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());

	item->unref();
}

} // namespace testItemPtr
