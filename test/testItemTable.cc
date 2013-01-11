#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemTable.h"

namespace testItemData {

enum {
	GROUP_ID_0,
	GROUP_ID_1,
};

enum {
	ITEM_ID_AGE,
	ITEM_ID_NAME,
	ITEM_ID_FAVORITE_COLOR,
	ITEM_ID_MONEY,
	ITEM_ID_NICKNAME,
};

struct TableStruct0 {
	int age;
	const char *name;
	const char *favoriteColor;
};

struct TableStruct1 {
	const char *name;
	int haveMoney;
	const char *nickname;
};

static const int NUM_TABLE0 = 2;
static const int NUM_TABLE1 = 3;

void test_crossJoin(void)
{
	ItemTable *table0 = new ItemTable(GROUP_ID_0);
	ItemGroup *grp = new ItemGroup(GROUP_ID_0);
	table0->add(grp, false);
	grp->add(new ItemInt(ITEM_ID_AGE, 2), false);

	ItemTable *table1 = new ItemTable(GROUP_ID_1);
	ItemGroup *grp = new ItemGroup(GROUP_ID_1);
	table1->add(grp, false);
	grp->add(new ItemInt(ITEM_ID_NAME, ""), false);

	table0->unref();
	table1->unref();
}

} // namespace testItemData
