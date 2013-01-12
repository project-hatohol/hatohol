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
	DEFAULT_GROUP_ID,
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

static TableStruct0 tableContent0[] = {
  {10, "anri", "red"},
  {20, "mai", "blue"},
};
static const int NUM_TABLE0 = sizeof(tableContent0) / sizeof(TableStruct0);

static TableStruct1 tableContent1[] = {
  {"anri", 150, "ann"},
  {"nobita",200, "snake"},
  {"maria", 170, "mai"},
};
static const int NUM_TABLE1 = sizeof(tableContent1) / sizeof(TableStruct1);

static void addItemTable0(ItemGroup *grp, TableStruct0 *table)
{
	grp->add(new ItemInt(ITEM_ID_AGE, table->age), false);
	grp->add(new ItemString(ITEM_ID_NAME, table->name), false);
	grp->add(new ItemString(ITEM_ID_FAVORITE_COLOR, table->favoriteColor), false);
}

static void addItemTable1(ItemGroup *grp, TableStruct1 *table)
{
	grp->add(new ItemString(ITEM_ID_NAME, table->name), false);
	grp->add(new ItemInt(ITEM_ID_HEIGHT, table->height), false);
	grp->add(new ItemString(ITEM_ID_NICKNAME, table->nickname), false);
}

template<typename T>
static ItemTable * addItems(T* srcTable, int numTable,
                            void (*addFunc)(ItemGroup *, T *))
{
	ItemTable *table = new ItemTable();
	for (int i = 0; i < numTable; i++) {
		ItemGroup *grp = table->addNewGroup();
		(*addFunc)(grp, &srcTable[i]);
	}
	return table;
}

static const int NUM_TABLE_POOL = 10;
static ItemTable *g_table[NUM_TABLE_POOL];
static ItemTable *&x_table = g_table[0];
static ItemTable *&y_table = g_table[1];
static ItemTable *&z_table = g_table[2];

void teardown(void)
{
	for (int i = 0; i < NUM_TABLE_POOL; i++) {
		if (g_table[i]) {
			g_table[i]->unref();
			g_table[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	x_table = new ItemTable();
}

void test_crossJoin(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);

	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);

	z_table = x_table->crossJoin(y_table);

	// check the result

}

} // namespace testItemData
