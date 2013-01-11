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

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_compare(void)
{
	ItemGroup *grp0 = new ItemGroup(0);
	grp0->add(new ItemInt(1, 5), false);
	grp0->add(new ItemString(2, "foo"), false);

	ItemGroup *grp1 = new ItemGroup(1);
	grp1->add(new ItemInt(1, 10), false);
	grp1->add(new ItemString(2, "goo"), false);

	cppcut_assert_equal(true, grp0->compareType(grp0));
	cppcut_assert_equal(true, grp1->compareType(grp0));
	cppcut_assert_equal(true, grp0->compareType(grp1));
	cppcut_assert_equal(true, grp1->compareType(grp0));

	grp0->unref();
	grp1->unref();
}

void test_compareNotEq(void)
{
	ItemGroup *grp0 = new ItemGroup(0);
	grp0->add(new ItemInt(1, 5), false);
	grp0->add(new ItemString(2, "foo"), false);

	ItemGroup *grp1 = new ItemGroup(1);
	grp1->add(new ItemString(2, "goo"), false);
	grp1->add(new ItemInt(1, 10), false);

	cppcut_assert_equal(false, grp0->compareType(grp1));
	cppcut_assert_equal(false, grp1->compareType(grp0));

	grp0->unref();
	grp1->unref();
}

} // testItemGroup


