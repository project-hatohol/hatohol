#include <stdexcept>
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
#include "ItemGroup.h"

namespace testItemGroup {

enum {
	DEFAULT_ITEM_ID,
	ITEM_ID_0,
	ITEM_ID_1,
	ITEM_ID_2,
};

const static int NUM_GROUP_POOL = 10;
static ItemGroup *g_grp[NUM_GROUP_POOL];
static ItemGroup *&x_grp = g_grp[0];
static ItemGroup *&y_grp = g_grp[1];

static void assertSetItemGroupType(void)
{
	// make ItemDataVector
	y_grp = new ItemGroup();
	y_grp->add(new ItemInt(ITEM_ID_0, 500), false);
	y_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	y_grp->freeze();
	const ItemGroupType *itemGroupType = y_grp->getItemGroupType();
	cut_trace(cut_assert_not_null(itemGroupType));

	// set ItemGroupTable
	x_grp = new ItemGroup();
	x_grp->setItemGroupType(itemGroupType);
	cut_trace(
	  cppcut_assert_equal(itemGroupType, x_grp->getItemGroupType()));
}

void teardown()
{
	for (int i = 0; i < NUM_GROUP_POOL; i++) {
		if (g_grp[i]) {
			g_grp[i]->unref();
			g_grp[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_addAndGet(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemString *item1 = new ItemString(ITEM_ID_1, "foo");

	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->add(item1, false);

	cppcut_assert_equal(static_cast<ItemData *>(item0),
	                    x_grp->getItem(ITEM_ID_0));
	cppcut_assert_equal(static_cast<ItemData *>(item1),
	                    x_grp->getItem(ITEM_ID_1));
}

void test_addDuplicativeItemId(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemInt *item1 = new ItemInt(ITEM_ID_0, -8500);
	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->add(item1, false);
	cppcut_assert_equal(static_cast<ItemData *>(item0),
	                    x_grp->getItem(ITEM_ID_0));
}

void test_addDuplicativeItemIdAndGetThem(void)
{
	cut_fail("Not implemented yet.");
}

void test_addWhenFreezed(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemInt *item1 = new ItemInt(ITEM_ID_0, -8500);
	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	x_grp->freeze();
	bool gotException = false;
	try {
		x_grp->add(new ItemInt(ITEM_ID_2, -3), false);
	} catch (invalid_argument e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_getNumberOfItems(void)
{
	x_grp = new ItemGroup();
	cppcut_assert_equal((size_t)0, x_grp->getNumberOfItems());

	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	cppcut_assert_equal((size_t)1, x_grp->getNumberOfItems());

	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal((size_t)2, x_grp->getNumberOfItems());
}

void test_freeze(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal(false, x_grp->isFreezed());
	cppcut_assert_equal((const ItemGroupType *) NULL,
	                    x_grp->getItemGroupType());
	x_grp->freeze();
	cppcut_assert_equal(true, x_grp->isFreezed());
	cut_assert_not_null(x_grp->getItemGroupType());
}

void test_getItemGroupType(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cppcut_assert_equal((const ItemGroupType *) NULL,
	                    x_grp->getItemGroupType());
	x_grp->freeze();
	const ItemGroupType *groupType = x_grp->getItemGroupType();
	cppcut_assert_equal((size_t)2, groupType->getSize());
	cppcut_assert_equal(ITEM_TYPE_INT, groupType->getType(0));
	cppcut_assert_equal(ITEM_TYPE_STRING, groupType->getType(1));
}

void test_setItemGroupType(void)
{
	assertSetItemGroupType();
}

void test_setItemGroupTypeWhenAlreadySet(void)
{
	// make ItemDataVector
	y_grp = new ItemGroup();
	y_grp->add(new ItemInt(ITEM_ID_0, 500), false);
	y_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	const ItemGroupType *itemGroupType = y_grp->getItemGroupType();

	// set ItemGroupTable
	x_grp = new ItemGroup();
	x_grp->setItemGroupType(itemGroupType);
	x_grp->add(new ItemInt(ITEM_ID_0, 500), false);
	x_grp->freeze();
	cut_assert_not_null(x_grp->getItemGroupType());

	bool gotException = false;
	try {
		x_grp->setItemGroupType(itemGroupType);
	} catch (logic_error e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_setItemGroupTypeWhenHasData(void)
{
	// make ItemDataVector
	y_grp = new ItemGroup();
	y_grp->add(new ItemInt(ITEM_ID_0, 500), false);
	y_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	const ItemGroupType *itemGroupType = y_grp->getItemGroupType();

	// set ItemGroupTable
	x_grp = new ItemGroup();
	x_grp->setItemGroupType(itemGroupType);
	x_grp->add(new ItemInt(ITEM_ID_0, 500), false);
	bool gotException = false;
	try {
		x_grp->setItemGroupType(itemGroupType);
	} catch (logic_error e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_addWhenHasItemGroupType(void)
{
	assertSetItemGroupType();
	x_grp->add(new ItemInt(ITEM_ID_0, 400), false);
	x_grp->add(new ItemString(ITEM_ID_1, "goo"), false);
	cppcut_assert_equal((size_t)2, x_grp->getNumberOfItems());
}

void test_addInvalidItemTypeWhenHasItemGroupType(void)
{
	assertSetItemGroupType();
	x_grp->add(new ItemInt(ITEM_ID_0, 400), false);
	bool gotException = false;
	ItemData *item = NULL;
	try {
		item = new ItemInt(ITEM_ID_1, 10);
		x_grp->add(item, false);
	} catch (invalid_argument e) {
		gotException = true;
		item->unref();
	}
	cppcut_assert_equal(true, gotException);
}

void test_autoFreeze(void)
{
	assertSetItemGroupType();
	x_grp->add(new ItemInt(ITEM_ID_0, 400), false);
	cppcut_assert_equal(false, x_grp->isFreezed());
	x_grp->add(new ItemString(ITEM_ID_1, "goo"), false);
	cppcut_assert_equal(true, x_grp->isFreezed());
}

} // testItemGroup


