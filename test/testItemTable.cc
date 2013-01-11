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
	ITEM_ID_HEIGHT,
	ITEM_ID_NICKNAME,
};

struct TableStruct0 {
	int age;
	const char *name;
	const char *favoriteColor;
};

struct TableStruct1 {
	const char *name;
	int height;
	const char *nickname;
};

static tableContent0[] = {
  {10, "anri", "red"},
  {20, "mai", "blue"},
};
static const int NUM_TABLE0 = sizeof(tableContent0) / sizeof(TableStruct0);

static tableContent0[] = {
  {"anri", 150, "ann"},
  {"nobita",200, "snake"},
  {"maria", 170, "mai"},
};
static const int NUM_TABLE1 = sizeof(tableContent1) / sizeof(TableStruct1);

static void addItemTable0(ItemGroup *grp, TableStruct0 *table)
{
	grp->add(new ItemInt(ITEM_ID_AGE, table->age), false);
	grp->add(new ItemSring(ITEM_ID_NAME, table->.name), false);
	grp->add(new ItemSring(ITEM_ID_FAVORITE_COLOR, table->favoriteColor), false);
}

static void addItemTable1(ItemGroup *grp, TableStruct1 *table)
{
	grp->add(new ItemString(ITEM_ID_NAME, table->age), false);
	grp->add(new ItemInt(ITEM_ID_HEIGHT, table->height), false);
	grp->add(new ItemSring(ITEM_ID_NICKNAME, table->nickname), false);
}

template<typename T>
static ItemTable *
addItmes(ItemTable *table, T* srcTable, int numTable, int groupId,
         void (*addFunc)(ItemGroup *, T *))
{
	ItemTable *table = new ItemTable(groupId);
	for (int i = 0; i < numTable; i++) {
		ItemGroup *grp = new ItemGroup(table->getItemGroupId());
		srcTable->add(grp, false);
		addFunc(grp, srcTable);
	}
	return table;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_crossJoin(void)
{
	ItemTable *table0 =
	  addItems<TableStruct0>(table0, tableContent0,
	                         NUM_TABLE0, GROUP_ID_0, addItemTable0);

	ItemTable *table1 =
	  addItems<TableStruct1>(table1, tableContent1,
	                         NUM_TABLE1, GROUP_ID_1, addItemTable1);

	ItemTable *tableA = table0->crossJoin(table1);

	table0->unref();
	table1->unref();
	tableA->unref();
}

} // namespace testItemData
