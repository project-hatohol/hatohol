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
#include "ItemTable.h"
#include "AssertJoin.h"

namespace testItemTable {

enum {
	DEFAULT_ITEM_ID,
	ITEM_ID_0,
	ITEM_ID_1,
	ITEM_ID_2,

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
  {30, "footaro", "blue"},
  {40, "rei", "blue"},
  {50, "love", "yellow"},
  {60, "while", "blue"},
};
static const size_t NUM_TABLE0 = sizeof(tableContent0) / sizeof(TableStruct0);

static TableStruct1 tableContent1[] = {
  {"anri",  150, "ann"},
  {"nobita",200, "snake"},
  {"maria", 170, "mai"},
  {"mai",   180, "maimai"},
  {"anri",  250, "tooower"},
};
static const size_t NUM_TABLE1 = sizeof(tableContent1) / sizeof(TableStruct1);

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

static void assertJoinRunner(const ItemGroup *itemGroup,
                             TableStruct0 *refData0, TableStruct1 *refData1,
                             size_t data0Index, size_t data1Index)
{
	int idx = 0;
	assertItemData(int,    itemGroup, refData0->age, idx);
	assertItemData(string, itemGroup, refData0->name, idx);
	assertItemData(string, itemGroup, refData0->favoriteColor, idx);
	assertItemData(string, itemGroup, refData1->name, idx);
	assertItemData(int,    itemGroup, refData1->height, idx);
	assertItemData(string, itemGroup, refData1->nickname, idx);
}

struct InnerJoinedRowsCheckerNameName {
	bool operator()(const TableStruct0 &row0,
	                const TableStruct1 &row1) const {
		return strcmp(row0.name, row1.name) == 0;
	}
};

static void assertEmptyTable(ItemTable *table)
{
	cut_trace(cut_assert_not_null(table));
	cut_trace(cppcut_assert_equal((size_t)0, table->getNumberOfRows()));
	cut_trace(cppcut_assert_equal((size_t)0, table->getNumberOfColumns()));
}

#define GET_ITEMS(TABLE, MEMBER, NUM_DATA, EXPECT, CONTAINER) \
{ \
  for (size_t i = 0; i < NUM_DATA; i++) { \
    if (TABLE[i].MEMBER == EXPECT) \
      CONTAINER.push_back(i); \
  } \
} while(0)

static const int NUM_TABLE_POOL = 10;
static ItemTable *g_table[NUM_TABLE_POOL];
static ItemTable *&x_table = g_table[0];
static ItemTable *&y_table = g_table[1];
static ItemTable *&z_table = g_table[2];

static void setIndexVector(ItemTable *table)
{
	vector<ItemDataIndexType> indexTypeVector;
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_UNIQUE);
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_NONE);
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_MULTI);
	table->defineIndex(indexTypeVector);
}

static void prepareFindIndex(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);
	setIndexVector(x_table);
}

static void _assertIndexUnique(void)
{
	const ItemDataIndexVector &actualIndexes = x_table->getIndexVector();
	const ItemDataIndex *itemDataIndex = actualIndexes[0];
	vector<ItemDataPtrForIndex> foundItems;
	const int findValue = tableContent0[0].age;
	ItemDataPtr itemData(new ItemInt(findValue), false);
	itemDataIndex->find((const ItemData *)itemData, foundItems);
	vector<size_t> expectedItemIndexes;
	GET_ITEMS(tableContent0, age, NUM_TABLE0,
	          findValue, expectedItemIndexes);
	cppcut_assert_equal(expectedItemIndexes.size(), foundItems.size());
	for (size_t i = 0; i < expectedItemIndexes.size(); i++) {
		size_t index = expectedItemIndexes[i];
		cppcut_assert_equal(findValue, tableContent0[index].age);
	}
}
#define assertIndexUnique() cut_trace(_assertIndexUnique())

static void _assertIndexMulti(void)
{
	const ItemDataIndexVector &actualIndexes = x_table->getIndexVector();
	const ItemDataIndex *itemDataIndex = actualIndexes[2];
	vector<ItemDataPtrForIndex> foundItems;
	const string findValue = "blue";
	ItemDataPtr itemData(new ItemString(findValue), false);
	itemDataIndex->find((const ItemData *)itemData, foundItems);

	vector<size_t> expectedItemIndexes;
	GET_ITEMS(tableContent0, favoriteColor, NUM_TABLE0,
	          findValue, expectedItemIndexes);
	cppcut_assert_equal(expectedItemIndexes.size(), foundItems.size());
	for (size_t i = 0; i < expectedItemIndexes.size(); i++) {
		size_t index = expectedItemIndexes[i];
		cppcut_assert_equal(findValue,
		                    string(tableContent0[index].favoriteColor));
	}
}
#define assertIndexMulti() cut_trace(_assertIndexMulti())

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
	cppcut_assert_equal(1, x_table->getUsedCount());
}

void test_addNoRef(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	x_table->add(grp, false);
	cppcut_assert_equal(1, grp->getUsedCount());
}

void test_addRef(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	x_table->add(grp, true);
	cppcut_assert_equal(2, grp->getUsedCount());
	grp->unref();
}

void test_getNumberOfRows(void)
{
	x_table = new ItemTable();
	cut_assert_equal_int(0, x_table->getNumberOfRows());

	x_table->add(new ItemGroup(), false);
	cut_assert_equal_int(1, x_table->getNumberOfRows());

	x_table->add(new ItemGroup(), false);
	cut_assert_equal_int(2, x_table->getNumberOfRows());
}

void test_getNumberOfColumns(void)
{
	x_table = new ItemTable();
	cut_assert_equal_int(0, x_table->getNumberOfColumns());

	x_table->add(new ItemGroup(), false);
	cut_assert_equal_int(0, x_table->getNumberOfColumns());
}

void test_addWhenHasMoreThanOneGroup(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	x_table->add(grp, false);

	// add second group
	grp = new ItemGroup();
	const ItemGroupType *itemGroupType = grp->getItemGroupType();
	cut_assert_null(itemGroupType);

	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	x_table->add(grp, false);

	itemGroupType = grp->getItemGroupType();
	cut_assert_not_null(itemGroupType);
	cppcut_assert_equal(ITEM_TYPE_INT, itemGroupType->getType(0));
	cppcut_assert_equal(ITEM_TYPE_STRING, itemGroupType->getType(1));

	// add thrid group
	grp = new ItemGroup();
	grp->add(new ItemInt(ITEM_ID_0, -20), false);
	grp->add(new ItemString(ITEM_ID_1, "dog"), false);
	x_table->add(grp, false);
	cppcut_assert_equal((size_t)3, x_table->getNumberOfRows());
}

void test_addInvalidItemsWhenHasMoreThanOneGroup(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	x_table->add(grp, false);

	// add second group
	grp = new ItemGroup();
	ItemData *item = new ItemString(ITEM_ID_1, "foo");
	bool gotException = false;
	try {
		grp->add(item, false);
		x_table->add(grp, false);
	} catch (const AsuraException &e) {
		item->unref();
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_addItemsWhenPreviousGroupIncompletion(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);
	x_table->add(grp, false);

	// add second group
	grp = new ItemGroup();
	grp->add(new ItemInt(ITEM_ID_0, 200), false);

	// add thrid group
	bool gotException = false;
	ItemData *item = new ItemInt(ITEM_ID_0, -5);
	try {
		grp->add(item, false);
		x_table->add(grp, false);
	} catch (const AsuraException &e) {
		item->unref();
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_crossJoin(void)
{
	cut_pend("Makes dead-lock in the revision: 45d784a2. "
	         "We'll remove locks from ItemTable as described in TODO. "
	         "Then this problem will naturally be resolved.");
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);

	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);

	z_table = x_table->crossJoin(y_table);
	cut_assert_not_null(z_table);

	// check the result
	size_t numColumns = x_table->getNumberOfColumns() +
	                    y_table->getNumberOfColumns();
	size_t numRows = NUM_TABLE0 * NUM_TABLE1;
	cppcut_assert_equal(numColumns, z_table->getNumberOfColumns());
	cppcut_assert_equal(numRows, z_table->getNumberOfRows());

	AssertCrossJoin<TableStruct0, TableStruct1>
	  assertJoin(z_table, tableContent0, tableContent1,
	             NUM_TABLE0, NUM_TABLE1);
	assertJoin.run(assertJoinRunner);
}

void test_crossJoinBothEmpty(void)
{
	x_table = new ItemTable();
	y_table = new ItemTable();
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_crossJoinRightEmpty(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);
	y_table = new ItemTable();
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_crossJoinLeftEmpty(void)
{
	x_table = new ItemTable();
	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_innerJoin(void)
{
	cut_pend("Makes dead-lock in the revision: 45d784a2. "
	         "We'll remove locks from ItemTable as described in TODO. "
	         "Then this problem will naturally be resolved.");
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);

	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);

	const size_t indexLeftColumn = 1; // name
	const size_t indexRightColumn = 0; // name
	z_table = x_table->innerJoin(y_table,
	                             indexLeftColumn, indexRightColumn);
	cut_assert_not_null(z_table);

	// check the result
	size_t numColumns = x_table->getNumberOfColumns() +
	                    y_table->getNumberOfColumns();
	cppcut_assert_equal(numColumns, z_table->getNumberOfColumns());

	AssertInnerJoin<TableStruct0, TableStruct1,
	                InnerJoinedRowsCheckerNameName>
	  assertJoin(z_table, tableContent0, tableContent1,
	             NUM_TABLE0, NUM_TABLE1);
	assertJoin.run(assertJoinRunner);
}

void test_defineIndex(void)
{
	vector<ItemDataIndexType> indexTypeVector;
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_UNIQUE);
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_NONE);
	indexTypeVector.push_back(ITEM_DATA_INDEX_TYPE_MULTI);
	x_table = new ItemTable();
	x_table->defineIndex(indexTypeVector);

	const ItemDataIndexVector &actualIndexes = x_table->getIndexVector();
	cppcut_assert_equal(indexTypeVector.size(), actualIndexes.size());
	for (size_t i = 0; i < actualIndexes.size(); i++) {
		const ItemDataIndex *itemDataIndex = actualIndexes[i];
		cppcut_assert_equal(indexTypeVector[i],
		                    itemDataIndex->getIndexType());
	}

	const vector<size_t> &indexedColumns = x_table->getIndexedColumns();
	const size_t expectedNumIndexedColumns = 2;
	cppcut_assert_equal(expectedNumIndexedColumns, indexedColumns.size());
	cppcut_assert_equal((size_t)0, indexedColumns[0]);
	cppcut_assert_equal((size_t)2, indexedColumns[1]);
}

void test_findIndexUnique(void)
{
	prepareFindIndex();
	assertIndexUnique();
}

void test_findIndexMulti(void)
{
	prepareFindIndex();
	assertIndexMulti();
}

void test_findIndexUniqueAddDataAfter(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0, setIndexVector);
	assertIndexUnique();
}


} // namespace testItemTable
