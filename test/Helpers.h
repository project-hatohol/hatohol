#ifndef Helpers_h
#define Helpers_h

#include <cppcutter.h>
#include <StringUtils.h>
using namespace mlpl;

#include "ItemTable.h"
#include "DBAgent.h"

typedef pair<int,int>      IntIntPair;
typedef vector<IntIntPair> IntIntPairVector;

void _assertStringVector(StringVector &expected, StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

template<typename T>
static ItemTable * addItems(T* srcTable, int numTable,
                            void (*addFunc)(ItemGroup *, T *),
                            void (*tableCreatedCb)(ItemTable *) = NULL)
{
	ItemTable *table = new ItemTable();
	if (tableCreatedCb)
		(*tableCreatedCb)(table);
	for (int i = 0; i < numTable; i++) {
		ItemGroup* grp = new ItemGroup();
		(*addFunc)(grp, &srcTable[i]);
		table->add(grp, false);
	}
	return table;
}

template<typename T>
static void _assertItemData(const ItemGroup *itemGroup, const T &expected, int &idx)
{
	const ItemData *itemZ = itemGroup->getItemAt(idx);
	cut_assert_not_null(itemZ);
	idx++;
	const T &val = ItemDataUtils::get<T>(itemZ);
	cut_trace(cppcut_assert_equal(expected, val));
}
#define assertItemData(T, IGRP, E, IDX) \
cut_trace(_assertItemData<T>(IGRP, E, IDX))

extern void _assertExist(const string &target, const string &words);
#define assertExist(T,W) cut_trace(_assertExist(T,W))

string executeCommand(const string &commandLine);
string getFixturesDir(void);
bool isVerboseMode(void);
string deleteDBClientDB(DBDomainId domainId);
string deleteDBClientZabbixDB(int serverId);
string execSqlite3ForDBClient(DBDomainId domainId, const string &statement);
string execSqlite3ForDBClientZabbix(int serverId, const string &statement);

template<typename T> void _assertAddToDB(T *arg, void (*func)(T *))
{
	bool gotException = false;
	try {
		(*func)(arg);
	} catch (const AsuraException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

#endif // Helpers_h
