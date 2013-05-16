#include <cppcutter.h>
#include <cutter.h>

#include "Asura.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"

namespace testDBClientAsura {

static void addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBClientAsura dbAsura;
	dbAsura.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

static string makeExpectedOutput(TriggerInfo *triggerInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%d|%d|%d|%u|%u|%s|%s\n",
	                        triggerInfo->serverId,
	                        triggerInfo->id,
	                        triggerInfo->status, triggerInfo->severity,
	                        triggerInfo->lastChangeTime.tv_sec,
	                        triggerInfo->lastChangeTime.tv_nsec,
	                        triggerInfo->hostId,
	                        triggerInfo->hostName.c_str(),
	                        triggerInfo->brief.c_str());
	return expectedOut;
}

static void _assertGetTriggers(void)
{
	TriggerInfoList triggerInfoList;
	DBClientAsura dbAsura;
	dbAsura.getTriggerInfoList(triggerInfoList);
	cppcut_assert_equal(NumTestTriggerInfo, triggerInfoList.size());

	string expectedText;
	string actualText;
	TriggerInfoListIterator it = triggerInfoList.begin();
	for (size_t i = 0; i < NumTestTriggerInfo; i++, ++it) {
		expectedText += makeExpectedOutput(&testTriggerInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetTriggers() cut_trace(_assertGetTriggers())

static string makeExpectedItemOutput(ItemInfo *itemInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%u|%s|%d|%d|%s|%s|%s\n",
	                        itemInfo->serverId,
	                        itemInfo->id,
	                        itemInfo->hostId,
	                        itemInfo->brief.c_str(),
	                        itemInfo->lastValueTime.tv_sec,
	                        itemInfo->lastValueTime.tv_nsec,
	                        itemInfo->lastValue.c_str(),
	                        itemInfo->prevValue.c_str(),
	                        itemInfo->itemGroupName.c_str());
	return expectedOut;
}

static void _assertGetItems(void)
{
	ItemInfoList itemInfoList;
	DBClientAsura dbAsura;
	dbAsura.getItemInfoList(itemInfoList);
	cppcut_assert_equal(NumTestItemInfo, itemInfoList.size());

	string expectedText;
	string actualText;
	ItemInfoListIterator it = itemInfoList.begin();
	for (size_t i = 0; i < NumTestItemInfo; i++, ++it) {
		expectedText += makeExpectedItemOutput(&testItemInfo[i]);
		actualText += makeExpectedItemOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetItems() cut_trace(_assertGetItems())

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createDB(void)
{
	// remove the DB that already exists
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_ASURA);

	// create an instance (the database will be automatically created)
	DBClientAsura dbAsura;
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient";
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_ASURA, statement);
	string expectedOut = StringUtils::sprintf
	                       ("%d|%d\n", DBClient::DBCLIENT_DB_VERSION,
	                                   DBClientAsura::ASURA_DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableTrigger(void)
{
	const string tableName = "triggers";
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_ASURA);
	DBClientAsura dbAsura;
	string command = "sqlite3 " + dbPath + " \".table\"";
	assertExist(tableName, executeCommand(command));

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_ASURA, statement);
	string expectedOut = StringUtils::sprintf(""); // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_testAddTriggerInfo(void)
{
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_ASURA);

	// added a record
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from triggers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_testGetTriggerInfoList(void)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		assertAddTriggerToDB(&testTriggerInfo[i]);
	assertGetTriggers();
}

void test_setTriggerInfoList(void)
{
	deleteDBClientDB(DB_DOMAIN_ID_ASURA);

	DBClientAsura dbAsura;
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	uint32_t serverId = testTriggerInfo[0].serverId;
	dbAsura.setTriggerInfoList(triggerInfoList, serverId);

	assertGetTriggers();
}

void test_addItemInfoList(void)
{
	deleteDBClientDB(DB_DOMAIN_ID_ASURA);

	DBClientAsura dbAsura;
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbAsura.addItemInfoList(itemInfoList);

	assertGetItems();
}

} // namespace testDBClientAsura
