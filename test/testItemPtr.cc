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

const static int NUM_ITEM_POOL = 10;
static ItemData *g_item[NUM_ITEM_POOL];
static ItemData *&x_item = g_item[0];
static ItemData *&y_item = g_item[1];

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
void test_numUsed(void)
{
	x_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, x_item->getUsedCount());
	ItemDataPtr dataPtr(x_item);
	cppcut_assert_equal(2, x_item->getUsedCount());
}

void test_constructWithNull(void)
{
	ItemInt *item = NULL;
	ItemDataPtr dataPtr(item);
}

void test_numUsedWithoutRef(void)
{
	x_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(x_item, false);
	cppcut_assert_equal(1, x_item->getUsedCount());
	x_item = NULL;
}

void test_numUsedWithBlock(void)
{
	x_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	cppcut_assert_equal(1, x_item->getUsedCount());
	{
		ItemDataPtr dataPtr(x_item);
		cppcut_assert_equal(2, x_item->getUsedCount());
	}
	cppcut_assert_equal(1, x_item->getUsedCount());
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
	x_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(x_item);
	cppcut_assert_equal(2, dataPtr->getUsedCount());

	y_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr2(y_item);
	cppcut_assert_equal(2, y_item->getUsedCount());
	cppcut_assert_equal(2, dataPtr2->getUsedCount());

	dataPtr2 = dataPtr;
	cppcut_assert_equal(1, y_item->getUsedCount());
	cppcut_assert_equal(3, x_item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());
}

void test_operatorSubstWithSameBody(void)
{
	x_item = new ItemInt(DEFAULT_ITEM_ID, DEFAULT_INT_VALUE);
	ItemDataPtr dataPtr(x_item);
	cppcut_assert_equal(2, dataPtr->getUsedCount());

	ItemDataPtr dataPtr2(x_item);
	cppcut_assert_equal(3, x_item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());

	dataPtr2 = dataPtr;
	cppcut_assert_equal(3, x_item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());

	dataPtr = dataPtr2;
	cppcut_assert_equal(3, x_item->getUsedCount());
	cppcut_assert_equal(3, dataPtr->getUsedCount());
	cppcut_assert_equal(3, dataPtr2->getUsedCount());
}

} // namespace testItemPtr
