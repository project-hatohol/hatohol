#ifndef Helpers_h
#define Helpers_h

#include <StringUtils.h>
using namespace mlpl;

#include "ItemTable.h"

typedef pair<int,int>      IntIntPair;
typedef vector<IntIntPair> IntIntPairVector;

void _assertStringVector(StringVector &expected, StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

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

template<typename T>
static void _assertItemData(const ItemGroup *itemGroup, T expected, int &idx)
{
	T val;
	ItemData *itemZ = itemGroup->getItemAt(idx);
	cut_assert_not_null(itemZ);
	idx++;
	itemZ->get(&val);
	cut_trace(cppcut_assert_equal(expected, val));
}
#define assertItemData(T, IGRP, E, IDX) \
cut_trace(_assertItemData<T>(IGRP, E, IDX))


#endif // Helpers_h
