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
};

const static int NUM_GROUP_POOL = 10;
static ItemGroup *g_grp[NUM_GROUP_POOL];
static ItemGroup *&x_grp = g_grp[0];
static ItemGroup *&y_grp = g_grp[1];

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

void test_addBad(void)
{
	ItemInt *item0 = new ItemInt(ITEM_ID_0, 500);
	ItemInt *item1 = new ItemInt(ITEM_ID_0, -8500);

	x_grp = new ItemGroup();
	x_grp->add(item0, false);
	bool gotException = false;
	try {
		x_grp->add(item1, false);
	} catch (invalid_argument e) {
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}


void test_compareType(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	ItemGroup *y_grp = new ItemGroup();
	y_grp->add(new ItemInt(ITEM_ID_0, 10), false);
	y_grp->add(new ItemString(ITEM_ID_1, "goo"), false);

	cppcut_assert_equal(true, x_grp->compareType(x_grp));
	cppcut_assert_equal(true, y_grp->compareType(x_grp));
	cppcut_assert_equal(true, x_grp->compareType(y_grp));
	cppcut_assert_equal(true, y_grp->compareType(y_grp));
}

void test_compareNotEq(void)
{
	x_grp = new ItemGroup();
	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	y_grp = new ItemGroup();
	y_grp->add(new ItemString(ITEM_ID_1, "goo"), false);
	y_grp->add(new ItemInt(ITEM_ID_0, 10), false);

	cppcut_assert_equal(false, x_grp->compareType(y_grp));
	cppcut_assert_equal(false, y_grp->compareType(x_grp));
}

void test_getNumberOfItems(void)
{
	x_grp = new ItemGroup();
	cut_assert_equal_int(0, x_grp->getNumberOfItems());

	x_grp->add(new ItemInt(ITEM_ID_0, 5), false);
	cut_assert_equal_int(1, x_grp->getNumberOfItems());

	x_grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	cut_assert_equal_int(2, x_grp->getNumberOfItems());
}

} // testItemGroup


