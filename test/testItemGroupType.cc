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
#include "ItemGroupType.h"

namespace testItemGroupType {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_add(void)
{
	ItemInt *itemInt = new ItemInt(1, 5);
	ItemString *itemString = new ItemString(2, "foo");

	ItemGroupType groupType;
	const vector<ItemDataType> &dataType = groupType.getTypeVector();
	cut_assert_equal_int(0, dataType.size());

	groupType.addType(itemInt);
	cut_assert_equal_int(1, dataType.size());
	cppcut_assert_equal(ITEM_TYPE_INT, dataType[0]);

	groupType.addType(itemString);
	cut_assert_equal_int(2, dataType.size());
	cppcut_assert_equal(ITEM_TYPE_INT, dataType[0]);
	cppcut_assert_equal(ITEM_TYPE_STRING, dataType[1]);

	itemInt->unref();
	itemString->unref();
}

void test_equal(void)
{
	ItemInt *itemInt = new ItemInt(1, 5);
	ItemString *itemString = new ItemString(2, "foo");

	ItemGroupType groupType0;
	groupType0.addType(itemInt);
	groupType0.addType(itemString);

	ItemGroupType groupType1;
	groupType1.addType(itemInt);
	groupType1.addType(itemString);

	ItemGroupType groupType2;
	groupType2.addType(itemString);
	groupType2.addType(itemInt);

	cppcut_assert_equal(true,  groupType0 == groupType0);
	cppcut_assert_equal(true,  groupType0 == groupType1);
	cppcut_assert_equal(false, groupType0 == groupType2);

	cppcut_assert_equal(true,  groupType1 == groupType0);
	cppcut_assert_equal(true,  groupType1 == groupType1);
	cppcut_assert_equal(false, groupType1 == groupType2);

	cppcut_assert_equal(false,  groupType2 == groupType0);
	cppcut_assert_equal(false,  groupType2 == groupType1);
	cppcut_assert_equal(true,   groupType2 == groupType2);

	itemInt->unref();
	itemString->unref();
}



} // testItemGroupType


