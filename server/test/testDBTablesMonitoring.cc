/*
 * Copyright (C) 2013-2016 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <cutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "DBTablesMonitoring.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "Params.h"
#include "ThreadLocalDBCache.h"
#include "testDBTablesMonitoring.h"
#include <algorithm>
#include "TestHostResourceQueryOption.h"

using namespace std;
using namespace mlpl;

namespace testDBTablesMonitoring {

#define DECLARE_DBTABLES_MONITORING(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesMonitoring &VAR_NAME = _dbHatohol.getDBTablesMonitoring();

static const string serverIdColumnName = "server_id";
static const string hostgroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

static void addTriggerInfo(const TriggerInfo *triggerInfo)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	dbMonitoring.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<const TriggerInfo>(X, addTriggerInfo))

struct AssertGetTriggersArg
  : public AssertGetHostResourceArg<TriggerInfo, TriggersQueryOption>
{
	AssertGetTriggersArg(gconstpointer ddtParam)
	{
		fixtures = testTriggerInfo;
		numberOfFixtures = NumTestTriggerInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual LocalHostIdType getHostId(const TriggerInfo &info) const override
	{
		return info.hostIdInServer;
	}

	virtual string makeOutputText(const TriggerInfo &triggerInfo)
	{
		return makeTriggerOutput(triggerInfo);
	}
};

static void _assertGetTriggers(AssertGetTriggersArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	dbMonitoring.getTriggerInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetTriggers(A) cut_trace(_assertGetTriggers(A))

static void _assertGetTriggersWithFilter(AssertGetTriggersArg &arg)
{
	// setup trigger data
	void test_addTriggerInfoList(gconstpointer data);
	test_addTriggerInfoList(arg.ddtParam);
	assertGetTriggers(arg);
}
#define assertGetTriggersWithFilter(ARG) \
cut_trace(_assertGetTriggersWithFilter(ARG))

static void _assertGetTriggerInfoList(
  gconstpointer ddtParam,
  const ServerIdType &serverId,
  const HostIdType &hostId = ALL_HOSTS)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	AssertGetTriggersArg arg(ddtParam);
	arg.targetServerId = serverId;
	arg.targetHostId = hostId;
	assertGetTriggers(arg);
}
#define assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ...) \
cut_trace(_assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ##__VA_ARGS__))

static void _assertGetEvents(AssertGetEventsArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	IncidentInfoVect *incidentInfoVectPointer
	  = arg.withIncidentInfo ? &arg.actualIncidentInfoVect : NULL;
	assertHatoholError(
	  arg.expectedErrorCode,
	  dbMonitoring.getEventInfoList(arg.actualRecordList, arg.option,
				     incidentInfoVectPointer));
	if (arg.expectedErrorCode != HTERR_OK)
		return;
	arg.assert();
}
#define assertGetEvents(A) cut_trace(_assertGetEvents(A))

static void _assertGetEventsWithFilter(AssertGetEventsArg &arg)
{
	// setup event data
	void test_addEventInfoList(gconstpointer data);
	test_addEventInfoList(arg.ddtParam);
	if (arg.withIncidentInfo)
		loadTestDBIncidents();

	assertGetEvents(arg);
}
#define assertGetEventsWithFilter(ARG) \
cut_trace(_assertGetEventsWithFilter(ARG))

struct AssertGetItemsArg
  : public AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
{
	string itemCategoryName;

	AssertGetItemsArg(gconstpointer ddtParam)
	{
		fixtures = testItemInfo;
		numberOfFixtures = NumTestItemInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual void fixupOption(void) override
	{
		AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>::
			fixupOption();
		option.setTargetItemCategoryName(itemCategoryName);
	}

	virtual bool filterOutExpectedRecord(const ItemInfo *info) override
	{
		if (AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}

		if (!itemCategoryName.empty()) {
			if (info->categoryNames.size() != 1)
				return true;
			if (info->categoryNames[0] != itemCategoryName)
				return true;
		}

		if (excludeDefunctServers) {
			if (!option.isValidServer(info->serverId))
				return true;
		}

		return false;
	}

	virtual LocalHostIdType getHostId(const ItemInfo &info) const override
	{
		return info.hostIdInServer;
	}

	virtual string makeOutputText(const ItemInfo &itemInfo) override
	{
		return makeItemOutput(itemInfo);
	}
};

static void _assertGetItems(AssertGetItemsArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	dbMonitoring.getItemInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetItems(A) cut_trace(_assertGetItems(A))

static void _assertGetItemsWithFilter(AssertGetItemsArg &arg)
{
	// setup item data
	void test_addItemInfoList(gconstpointer data);
	test_addItemInfoList(arg.ddtParam);
	assertGetItems(arg);
}
#define assertGetItemsWithFilter(ARG) \
cut_trace(_assertGetItemsWithFilter(ARG))

static void _assertGetNumberOfItems(AssertGetItemsArg &arg)
{
	void test_addItemInfoList(gconstpointer data);
	test_addItemInfoList(arg.ddtParam);
	arg.fixup();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	cppcut_assert_equal(arg.expectedRecords.size(),
			    dbMonitoring.getNumberOfItems(arg.option));
}
#define assertGetNumberOfItems(A) cut_trace(_assertGetNumberOfItems(A))

void _assertItemInfoList(gconstpointer data, uint32_t serverId)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbMonitoring.addItemInfoList(itemInfoList);

	AssertGetItemsArg arg(data);
	arg.targetServerId = serverId;
	assertGetItems(arg);
}
#define assertItemInfoList(DATA, SERVER_ID) \
cut_trace(_assertItemInfoList(DATA, SERVER_ID))

static void _assertGetNumberOfHostsWithUserAndStatus(UserIdType userId, bool status)
{
	loadTestDBTriggers();

	const ServerIdType serverId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	const HostgroupIdType hostgroupId = ALL_HOST_GROUPS;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	int expected = getNumberOfTestHostsWithStatus(serverId, hostgroupId,
	                                              status, userId);
	int actual;
	TriggersQueryOption option(userId);
	option.setTargetServerId(serverId);
	option.setTargetHostgroupId(hostgroupId);

	if (status)
		actual  = dbMonitoring.getNumberOfGoodHosts(option);
	else
		actual  = dbMonitoring.getNumberOfBadHosts(option);
	cppcut_assert_equal(expected, actual);
}
#define assertGetNumberOfHostsWithUserAndStatus(U, ST) \
cut_trace(_assertGetNumberOfHostsWithUserAndStatus(U, ST))

static
void _assertTriggerInfo(const TriggerInfo &expect, const TriggerInfo &actual)
{
	cppcut_assert_equal(expect.serverId, actual.serverId);
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.status, actual.status);
	cppcut_assert_equal(expect.severity, actual.severity);
	cppcut_assert_equal(expect.lastChangeTime.tv_sec,
	                    actual.lastChangeTime.tv_sec);
	cppcut_assert_equal(expect.lastChangeTime.tv_nsec,
	                    actual.lastChangeTime.tv_nsec);
	cppcut_assert_equal(expect.globalHostId, actual.globalHostId);
	cppcut_assert_equal(expect.hostIdInServer, actual.hostIdInServer);
	cppcut_assert_equal(expect.hostName, actual.hostName);
	cppcut_assert_equal(expect.brief, actual.brief);
}
#define assertTriggerInfo(E,A) cut_trace(_assertTriggerInfo(E,A))

static
void _assertEventInfo(const EventInfo &expect, const EventInfo &actual)
{
	cppcut_assert_equal(expect.unifiedId, actual.unifiedId);
	cppcut_assert_equal(expect.serverId, actual.serverId);
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.time.tv_sec,
	                    actual.time.tv_sec);
	cppcut_assert_equal(expect.time.tv_nsec,
	                    actual.time.tv_nsec);
	cppcut_assert_equal(expect.type, actual.type);
	cppcut_assert_equal(expect.triggerId, actual.triggerId);
	cppcut_assert_equal(expect.status, actual.status);
	cppcut_assert_equal(expect.severity, actual.severity);
	cppcut_assert_equal(expect.globalHostId, actual.globalHostId);
	cppcut_assert_equal(expect.hostIdInServer, actual.hostIdInServer);
	cppcut_assert_equal(expect.hostName, actual.hostName);
	cppcut_assert_equal(expect.brief, actual.brief);
	cppcut_assert_equal(expect.extendedInfo, actual.extendedInfo);
}
#define assertEventInfo(E,A) cut_trace(_assertEventInfo(E,A))

static void prepareDataForAllHostgroupIds(void)
{
	set<HostgroupIdType> hostgroupIdSet = getTestHostgroupIdSet();
	hostgroupIdSet.insert(ALL_HOST_GROUPS);
	set<HostgroupIdType>::const_iterator hostgrpIdItr =
	  hostgroupIdSet.begin();
	for (; hostgrpIdItr != hostgroupIdSet.end(); ++hostgrpIdItr) {
		string label = "Hostgroup ID: ";
		label += *hostgrpIdItr;
		gcut_add_datum(label.c_str(), "hostgroupId",
		               G_TYPE_STRING, hostgrpIdItr->c_str(), NULL);
	}
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	assertDBTablesVersion(
	  dbMonitoring.getDBAgent(),
	  DBTablesId::MONITORING, DBTablesMonitoring::MONITORING_DB_VERSION);
}


void test_createTableTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// check content
	string statement = "SELECT * FROM triggers";
	string expectedOut = ""; // currently no data
	assertDBContent(&dbMonitoring.getDBAgent(), statement, expectedOut);
}

void test_addTriggerInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// added a record
	const TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string sql = StringUtils::sprintf("SELECT * from triggers");
	string expectedOut = makeTriggerOutput(*testInfo);
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
}

void test_deleteTriggerInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();

	TriggerIdList triggerIdList = { "2", "3" };
	constexpr ServerIdType targetServerId = 1;

	for (auto triggerId : triggerIdList) {
		// check triggerInfo existence
		string sql = StringUtils::sprintf("SELECT * FROM triggers");
		sql += StringUtils::sprintf(
		  " WHERE id = '%" FMT_TRIGGER_ID "'"
		  " AND server_id = %" FMT_SERVER_ID,
		  triggerId.c_str(), targetServerId);
		uint64_t targetTestDataId = StringUtils::toUint64(triggerId) - 1;
		string expectedOut;
		expectedOut +=
			makeTriggerOutput(testTriggerInfo[targetTestDataId]);
		assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
	}

	HatoholError err =
		dbMonitoring.deleteTriggerInfo(triggerIdList, targetServerId);
	assertHatoholError(HTERR_OK, err);
	for (auto triggerId : triggerIdList) {
		string statement = StringUtils::sprintf("SELECT * FROM triggers");
		statement += StringUtils::sprintf(
		  " WHERE id = %" FMT_TRIGGER_ID " AND server_id = %" FMT_SERVER_ID,
		  triggerId.c_str(), targetServerId);
		string expected = "";
		assertDBContent(&dbMonitoring.getDBAgent(), statement, expected);
	}
}

void test_syncTriggers(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;
	const TriggerIdType targetTriggerId = "2";
	constexpr const int testTriggerDataId = 2;
	const TriggerIdList targetServerTriggerIds = {"1", "2", "3", "4", "5"};

	string sql = StringUtils::sprintf("SELECT * FROM triggers");
	sql += StringUtils::sprintf(
	  " WHERE server_id = %" FMT_SERVER_ID " ORDER BY server_id ASC",
	  targetServerId);

	string expectedOut;
	// check triggerInfo existence
	for (auto triggerId : targetServerTriggerIds) {
		int64_t targetId = StringUtils::toUint64(triggerId) - 1;
		expectedOut += makeTriggerOutput(testTriggerInfo[targetId]);
	}
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);

	map<TriggerIdType, const TriggerInfo *> triggerMap;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.id != targetTriggerId)
			continue;
		triggerMap[svTriggerInfo.id] = &svTriggerInfo;
	}

	TriggerInfoList svTriggers =
	{
		{
			testTriggerInfo[testTriggerDataId - 1]
		}
	};

	// sanity check if we use the proper data
	cppcut_assert_equal(false, svTriggers.empty());
	// Prepare for the expected result.
	string expect;
	for (auto triggerPair : triggerMap) {
		const TriggerInfo svTrigger = *triggerPair.second;
		expect += StringUtils::sprintf(
		  "%" FMT_LOCAL_HOST_ID "|%s\n",
		  svTrigger.hostIdInServer.c_str(), svTrigger.hostName.c_str());
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select host_id_in_server,hostname from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncTriggersAddNewTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;

	TriggerInfo newTriggerInfo = {
		1,                        // serverId
		"7",                      // id
		TRIGGER_STATUS_OK,        // status
		TRIGGER_SEVERITY_INFO,    // severity
		{1362958197,0},           // lastChangeTime
		10,                       // globalHostId,
		"235013",                 // hostIdInServer,
		"hostX2",                 // hostName,
		"TEST New Trigger 1",     // brief,
		"",                       // extendedInfo
		TRIGGER_VALID,            // validity
	};

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.id == newTriggerInfo.id)
			cut_fail("We use the wrong test data");
	}

	string expect;
	TriggerInfoList svTriggers;
	{
		size_t i = 0;
		for (; i < NumTestTriggerInfo; i++) {
			const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
			if (svTriggerInfo.serverId != targetServerId)
				continue;
			svTriggers.push_back(svTriggerInfo);
			expect += makeTriggerOutput(svTriggerInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svTriggers.empty());

		// Add newTriggerInfo to the expected result
		svTriggers.push_back(newTriggerInfo);
		expect += makeTriggerOutput(newTriggerInfo);
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select * from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncTriggersModifiedTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;

	TriggerInfo modifiedTriggerInfo = {
		1,                        // serverId
		"1",                      // id
		TRIGGER_STATUS_OK,        // status
		TRIGGER_SEVERITY_INFO,    // severity
		{1362957197,0},           // lastChangeTime
		10,                       // globalHostId,
		"235012",                 // hostIdInServer,
		"hostX1 revised",         // hostName,
		"TEST Trigger 1 Revised", // brief,
		"{\"expandedDescription\":\"Test Trigger on hostX1 revised\"}", // extendedInfo
		TRIGGER_VALID,            // validity
	};

	// sanity check for test data
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.brief == modifiedTriggerInfo.brief)
			cut_fail("We use the wrong test data");
	}

	string expect;
	TriggerInfoList svTriggers;
	{
		size_t i = 0;
		// Add modifiedTriggerInfo to the expected result
		svTriggers.push_back(modifiedTriggerInfo);
		expect += makeTriggerOutput(modifiedTriggerInfo);

		for (i = 1; i < NumTestTriggerInfo; i++) {
			const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
			if (svTriggerInfo.serverId != targetServerId)
				continue;
			svTriggers.push_back(svTriggerInfo);
			expect += makeTriggerOutput(svTriggerInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svTriggers.empty());
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select * from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_deleteItemInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBItems();

	ItemIdList itemIdList = { "1", "2" };
	constexpr ServerIdType targetServerId = 3;

	for (auto itemId : itemIdList) {
		// check itemInfo existence
		string sql = StringUtils::sprintf("SELECT * FROM items");
		sql += StringUtils::sprintf(
		  " WHERE id = '%" FMT_ITEM_ID "'"
		  " AND server_id = %" FMT_SERVER_ID,
		  itemId.c_str(), targetServerId);
		uint64_t targetTestDataId = StringUtils::toUint64(itemId);
		string expectedOut;
		expectedOut +=
			makeItemOutput(testItemInfo[targetTestDataId]);
		assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
	}

	HatoholError err =
		dbMonitoring.deleteItemInfo(itemIdList, targetServerId);
	assertHatoholError(HTERR_OK, err);
	for (auto itemId : itemIdList) {
		string statement = StringUtils::sprintf("SELECT * FROM items");
		statement += StringUtils::sprintf(
		  " WHERE id = %" FMT_ITEM_ID " AND server_id = %" FMT_SERVER_ID,
		  itemId.c_str(), targetServerId);
		string expected = "";
		assertDBContent(&dbMonitoring.getDBAgent(), statement, expected);
	}
}

void test_syncItems(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBItems();
	constexpr const ServerIdType targetServerId = 3;
	const ItemIdType targetItemId = "2";
	constexpr const int testItemDataId = 3;
	const ItemIdList targetServerItemIds = {"2", "3"};

	string sql = StringUtils::sprintf("SELECT * FROM items");
	sql += StringUtils::sprintf(
	  " WHERE server_id = %" FMT_SERVER_ID " ORDER BY server_id ASC",
	  targetServerId);

	string expectedOut;
	// check itemInfo existence
	for (auto itemId : targetServerItemIds) {
		int64_t targetId = StringUtils::toUint64(itemId) - 1;
		expectedOut += makeItemOutput(testItemInfo[targetId]);
	}
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);

	map<ItemIdType, const ItemInfo *> itemMap;
	for (size_t i = 0; i < NumTestItemInfo; i++) {
	const ItemInfo &svItemInfo = testItemInfo[i];
		if (svItemInfo.serverId != targetServerId)
			continue;
		itemMap[svItemInfo.id] = &svItemInfo;
	}

	ItemInfoList svItems =
	{
		{
			testItemInfo[testItemDataId - 1]
		}
	};

	// sanity check if we use the proper data
	cppcut_assert_equal(false, svItems.empty());
	// Prepare for the expected result.
	string expect;
	for (auto itemPair : itemMap) {
		const ItemInfo svItem = *itemPair.second;
		expect += StringUtils::sprintf(
		  "%" FMT_LOCAL_HOST_ID "\n",
		  svItem.hostIdInServer.c_str());
	}
	HatoholError err = dbMonitoring.syncItems(svItems, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "SELECT host_id_in_server FROM items"
	  " WHERE server_id=%" FMT_SERVER_ID " ORDER BY id ASC;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncItemsAddNewItem(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBItems();
	constexpr const ServerIdType targetServerId = 1;

	ItemInfo newItemInfo = {
		7,                  // globalId
		1,                  // serverId
		"3",                // id
		31,                 // globalHostId
		"1130",             // hostIdInServer
		"I like coffee.",   // brief
		{1362951129,0},     // lastValueTime
		"Coffee",           // lastValue
		"MilkTea",          // prevValue
		{"Drink"},          // categoryNames;
		0,                  // delay
		ITEM_INFO_VALUE_TYPE_STRING, // valueType
		"",                 // unit
	 };

	for (size_t i = 0; i < NumTestItemInfo; i++) {
		const ItemInfo &svItemInfo = testItemInfo[i];
		if (svItemInfo.serverId != targetServerId)
			continue;
		if (svItemInfo.id == newItemInfo.id)
			cut_fail("We use the wrong test data");
	}

	string expect;
	ItemInfoList svItems;
	{
		size_t i = 0;
		for (; i < NumTestItemInfo; i++) {
			const ItemInfo &svItemInfo = testItemInfo[i];
			if (svItemInfo.serverId != targetServerId)
				continue;
			svItems.push_back(svItemInfo);
			expect += makeItemOutput(svItemInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svItems.empty());

		// Add newItemInfo to the expected result
		svItems.push_back(newItemInfo);
		expect += makeItemOutput(newItemInfo);
	}
	HatoholError err = dbMonitoring.syncItems(svItems, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select * from items"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncItemsModifiedItem(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBItems();
	constexpr const ServerIdType targetServerId = 1;

	ItemInfo modifiedItemInfo = {
		7,                 // globalId
		1,                 // serverId
		"2",               // id
		30,                // globalHostId
		"1129",            // hostIdInServer
		"Roma is Roma.",   // brief
		{1362951129,0},    // lastValueTime
		"Sapporo",         // lastValue
		"Tokushima",       // prevValue
		{"City"},          // categoryNames;
		0,                 // delay
		ITEM_INFO_VALUE_TYPE_STRING, // valueType
		"",                // unit
	};

	// sanity check for test data
	for (size_t i = 0; i < NumTestItemInfo; i++) {
		const ItemInfo &svItemInfo = testItemInfo[i];
		if (svItemInfo.serverId != targetServerId)
			continue;
		if (svItemInfo.brief == modifiedItemInfo.brief)
			cut_fail("We use the wrong test data");
	}

	string expect;
	ItemInfoList svItems;
	{
		size_t i = 0;
		// Add modifiedItemInfo to the expected result
		svItems.push_back(modifiedItemInfo);
		expect += makeItemOutput(modifiedItemInfo);

		for (i = 1; i < NumTestItemInfo; i++) {
			const ItemInfo &svItemInfo = testItemInfo[i];
			if (svItemInfo.serverId != targetServerId)
				continue;
			svItems.push_back(svItemInfo);
			expect += makeItemOutput(svItemInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svItems.empty());
	}
	HatoholError err = dbMonitoring.syncItems(svItems, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "SELECT * FROM items"
	  " WHERE server_id=%" FMT_SERVER_ID " ORDER BY id ASC;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_getTriggerInfo(void)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	int targetIdx = 2;
	const TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetTriggerInfo.serverId);
	option.setTargetId(targetTriggerInfo.id);
	cppcut_assert_equal(true,
	   dbMonitoring.getTriggerInfo(triggerInfo, option));
	assertTriggerInfo(targetTriggerInfo, triggerInfo);
}

void test_getTriggerInfoNotFound(void)
{
	loadTestDBTriggers();

	const UserIdType invalidSvId = -1;
	const TriggerIdType invalidTrigId;
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(invalidSvId);
	option.setTargetId(invalidTrigId);
	cppcut_assert_equal(false,
	                    dbMonitoring.getTriggerInfo(triggerInfo, option));
}

void data_getTriggerInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerInfoList(gconstpointer data)
{
	assertGetTriggerInfoList(data, ALL_SERVERS);
}

void test_getTriggerInfoListWithHostnameList(void){
	loadTestDBTriggers();

	TriggerInfoList triggerInfoList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setHostnameList({"hostX1"});
	option.setSortType(TriggersQueryOption::SORT_ID,
	                   DataQueryOption::SORT_ASCENDING);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	TriggerInfo expectedTriggerInfo[] = {
		testTriggerInfo[0],
		testTriggerInfo[1],
		testTriggerInfo[3],
	};

	{
		size_t i = 0;
		for (auto triggerInfo : triggerInfoList) {
			assertTriggerInfo(expectedTriggerInfo[i], triggerInfo);
			++i;
		}
	}
}

void test_getTriggerInfoListWithTimeRange(void)
{
	loadTestDBTriggers();

	TriggerInfoList triggerInfoList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setBeginTime({1362957197, 0});
	option.setEndTime({1362957200, 0});
	option.setSortType(TriggersQueryOption::SORT_TIME,
	                   DataQueryOption::SORT_ASCENDING);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	cppcut_assert_equal(triggerInfoList.size(), static_cast<size_t>(5));
	TriggerInfo expectedTriggerInfo[] = {
		testTriggerInfo[0],
		testTriggerInfo[3],
		testTriggerInfo[1],
		testTriggerInfo[4],
		testTriggerInfo[5],
	};

	{
		size_t i = 0;
		for (auto triggerInfo : triggerInfoList) {
			assertTriggerInfo(expectedTriggerInfo[i], triggerInfo);
			++i;
		}
	}
}

void test_getTriggerInfoListWithTimeRangeNotFound(void)
{
	loadTestDBTriggers();

	TriggerInfoList triggerInfoList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setBeginTime({1363000000, 0});
	option.setEndTime({1389123457, 0});
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	cppcut_assert_equal(triggerInfoList.size(), static_cast<size_t>(0));
}

void test_getTriggerInfoWithBrief(void)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	int targetIdx = 2;
	const TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	string brief = "TEST Trigger 1b";
	option.setTriggerBrief(brief);
	cppcut_assert_equal(true,
	   dbMonitoring.getTriggerInfo(triggerInfo, option));
	assertTriggerInfo(targetTriggerInfo, triggerInfo);
}

void test_getTriggerInfoWithBriefNotFound(void)
{
	loadTestDBTriggers();

	TriggerInfoList triggerInfoList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	string brief = "TEST Trigger noexistence";
	option.setTriggerBrief(brief);
	dbMonitoring.getTriggerInfoList(triggerInfoList, option);
	cppcut_assert_equal(triggerInfoList.size(), static_cast<size_t>(0));
}

void data_getTriggerInfoListForOneServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	assertGetTriggerInfoList(data, targetServerId);
}

void data_getTriggerInfoListForOneServerOneHost(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerInfoListForOneServerOneHost(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[1].serverId;
	const HostIdType targetHostId = testTriggerInfo[1].globalHostId;
	assertGetTriggerInfoList(data, targetServerId, targetHostId);
}

void data_setTriggerInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_setTriggerInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	const ServerIdType serverId = testTriggerInfo[0].serverId;
	dbMonitoring.setTriggerInfoList(triggerInfoList, serverId);

	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_addTriggerInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_addTriggerInfoList(gconstpointer data)
{
	size_t i;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// First call
	size_t numFirstAdd = NumTestTriggerInfo / 2;
	TriggerInfoList triggerInfoList0;
	for (i = 0; i < numFirstAdd; i++)
		triggerInfoList0.push_back(testTriggerInfo[i]);
	dbMonitoring.addTriggerInfoList(triggerInfoList0);

	// Second call
	TriggerInfoList triggerInfoList1;
	for (; i < NumTestTriggerInfo; i++)
		triggerInfoList1.push_back(testTriggerInfo[i]);
	dbMonitoring.addTriggerInfoList(triggerInfoList1);

	// Check
	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_getTriggerWithOneAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 5;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithNoAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 4;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithInvalidUserId(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getTriggerWithInvalidUserId(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetTriggersWithFilter(arg);
}

void test_getTriggerBriefListOnlyWithOrderOption(void)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	list<string> triggerBriefList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setSortType(TriggersQueryOption::SORT_TIME,
	                   DataQueryOption::SORT_ASCENDING);
	dbMonitoring.getTriggerBriefList(triggerBriefList, option);
	cppcut_assert_equal((size_t)10, triggerBriefList.size());
	vector<string> expectedBriefs = {
		"TEST Trigger Action",
		"TEST Trigger 3",
		"Status:Unknown, Severity:Critical",
		"TEST Trigger Action 2",
		"TEST Trigger 1b",
		"TEST Trigger 1",
		"TEST Trigger 1c",
		"TEST Trigger 1a",
		"TEST Trigger 1d",
		"TEST Trigger 2",
	};
	{
		size_t i = 0;
		for (auto triggerBrief : triggerBriefList) {
			cppcut_assert_equal(expectedBriefs.at(i), triggerBrief);
			i++;
		}
	}
}

void test_getTriggerBriefListWithOption(void)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	int targetIdx = 2;
	const TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
	list<string> triggerBriefList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetTriggerInfo.serverId);
	option.setTargetId(targetTriggerInfo.id);
	option.setExcludeFlags(EXCLUDE_SELF_MONITORING);
	dbMonitoring.getTriggerBriefList(triggerBriefList, option);
	cppcut_assert_equal((size_t)1, triggerBriefList.size());
	cppcut_assert_equal(targetTriggerInfo.brief, *triggerBriefList.begin());
}

void data_itemInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_itemInfoList(gconstpointer data)
{
	assertItemInfoList(data, ALL_SERVERS);
}

void data_itemInfoListForOneServer(gconstpointer data)
{
	prepareTestDataExcludeDefunctServers();
}

void test_itemInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testItemInfo[0].serverId;
	assertItemInfoList(data, targetServerId);
}

void data_addItemInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_addItemInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbMonitoring.addItemInfoList(itemInfoList);

	AssertGetItemsArg arg(data);
	assertGetItems(arg);
}

void data_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithNoAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithInvalidUserId(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithItemGroupName(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getItemWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemCategoryName = "City";
	assertGetItemsWithFilter(arg);
}

void data_getNumberOfItems(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getNumberOfItems(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemsWithOneAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getNumberOfItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithNoAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getNumberOfItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithInvalidUserId(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getNumberOfItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemsWithItemGroupName(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getNumberOfItemsWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemCategoryName = "City";
	assertGetNumberOfItems(arg);
}

void test_updateItemCategories()
{
	const vector<string> categoriesArray[] = {
		{"DOG", "CAT", "I am a pefect human."},
		{"DOG"},              // removed 2 categoires
		{"DOG", "Room!"},     // add 1 category
		{"Room!", "(^_^)"},   // remove 1 and add 1 category
		{},                   // remove all
		{"Bread", "CPU"},
	};

	auto assertCategories = [] (const vector<string> &expect,
	                            const ItemInfo &itemInfo) {
	};

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemsQueryOption option(USER_ID_SYSTEM);
	ItemInfo itemInfo = testItemInfo[0];
	for (const auto &categories: categoriesArray) {
		itemInfo.categoryNames = categories;
		dbMonitoring.addItemInfo(&itemInfo);
		ItemInfoList itemInfoList;
		dbMonitoring.getItemInfoList(itemInfoList, option);
		cppcut_assert_equal((size_t)1, itemInfoList.size());
		assertCategories(categories, *itemInfoList.begin());
	}
}

void data_addEventInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_addEventInfoList(gconstpointer data)
{
	// DBTablesMonitoring internally joins the trigger table and the event table.
	// So we also have to add trigger data.
	// When the internal join is removed, the following line will not be
	// needed.
	test_setTriggerInfoList(data);

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbMonitoring.addEventInfoList(eventInfoList);

	AssertGetEventsArg arg(data);
	assertGetEvents(arg);
}

void test_addEventInfoUnifiedId(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbMonitoring.addEventInfoList(eventInfoList);

	EventInfoListIterator it = eventInfoList.begin();
	string expected;
	string actual;
	for (int i = 1; it != eventInfoList.end(); it++, i++) {
		if (!expected.empty())
			expected += "\n";
		if (!actual.empty())
			actual += "\n";
		expected += to_string(i);
		expected += string("|") + makeEventOutput(*it);
		actual += to_string(it->unifiedId);
		actual += string("|") + makeEventOutput(*it);
	}
	cppcut_assert_equal(expected, actual);
	assertDBContent(&dbMonitoring.getDBAgent(),
			"select * from events", expected);
}

void data_addDupEventInfoList(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_addDupEventInfoList(gconstpointer data)
{
	test_setTriggerInfoList(data);

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestDupEventInfo; i++){
		eventInfoList.push_back(testDupEventInfo[i]);
	}
	dbMonitoring.addEventInfoList(eventInfoList);

	AssertGetEventsArg arg(data, testDupEventInfo, NumTestDupEventInfo);
	assertGetEvents(arg);
}

void data_getLastEventId(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getMaxEventId(gconstpointer data)
{
	test_addEventInfoList(data);
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(findLastEventId(serverid),
	                    dbMonitoring.getMaxEventId(serverid));
}

void test_getMaxEventIdWithNoEvent(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(EVENT_NOT_FOUND,
	                    dbMonitoring.getMaxEventId(serverid));
}

void test_getTimeOfLastEvent(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverId = 1;
	cppcut_assert_equal(findTimeOfLastEvent(serverId),
	                    dbMonitoring.getTimeOfLastEvent(serverId));
}

void test_getTimeOfLastEventWithTriggerId(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverId = 3;
	const TriggerIdType triggerId = "3";
	cppcut_assert_equal(
	  findTimeOfLastEvent(serverId, triggerId),
	  dbMonitoring.getTimeOfLastEvent(serverId, triggerId));
}

void data_getNumberOfTriggers(void)
{
	prepareDataForAllHostgroupIds();
}

void test_getNumberOfTriggers(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);
	cppcut_assert_equal(
	  getNumberOfTestTriggers(targetServerId, hostgroupId),
	  dbMonitoring.getNumberOfTriggers(option),
	  cut_message("sv: %" FMT_SERVER_ID ", hostgroup: %" FMT_HOST_GROUP_ID,
	              targetServerId, hostgroupId.c_str()));
}

void test_getNumberOfTriggersForMultipleAuthorizedHostgroups(void)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId = ALL_HOST_GROUPS;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(userIdWithMultipleAuthorizedHostgroups);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);

	cppcut_assert_equal(
	  getNumberOfTestTriggers(targetServerId, hostgroupId),
	  dbMonitoring.getNumberOfTriggers(option));
}

void data_getNumberOfTriggersBySeverity(void)
{
	prepareDataForAllHostgroupIds();
}

void _assertGetNumberOfTriggers(DBTablesMonitoring &dbMonitoring,
				const ServerIdType &serverId,
				const HostgroupIdType &hostgroupId,
				const TriggerSeverityType &severity)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setTargetHostgroupId(hostgroupId);
	const size_t actual =
	  dbMonitoring.getNumberOfBadTriggers(option, severity);
	const size_t expect = getNumberOfTestTriggers(
	  serverId, hostgroupId, severity);
	if (isVerboseMode())
		cut_notify("The expected number of triggesr: %zd\n", expect);
	cppcut_assert_equal(expect, actual,
			    cut_message(
			      "sv: %" FMT_SERVER_ID ", "
			      "hostgroup: %" FMT_HOST_GROUP_ID ", "
			      "severity: %d",
			      serverId, hostgroupId.c_str(), severity));
}
#define assertGetNumberOfTriggers(D,S,H,V) \
cut_trace(_assertGetNumberOfTriggers(D,S,H,V))

void test_getNumberOfTriggersBySeverity(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		const TriggerSeverityType severity
		  = static_cast<TriggerSeverityType>(i);
		assertGetNumberOfTriggers(
		  dbMonitoring, targetServerId, hostgroupId, severity);
	}
}

void data_getNumberOfAllBadTriggers(void)
{
	prepareDataForAllHostgroupIds();
}

void test_getNumberOfAllBadTriggers(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	assertGetNumberOfTriggers(dbMonitoring, targetServerId, hostgroupId,
				  TRIGGER_SEVERITY_ALL);
}

void test_getNumberOfTriggersBySeverityWithoutPriviledge(void)
{
	loadTestDBTriggers();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	//uint64_t hostgroupId = 0;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		TriggersQueryOption option;
		option.setTargetServerId(targetServerId);
		//TODO: uncomment it after Hatohol supports host group
		//option.setTargetHostgroupId(hostgroupId);
		TriggerSeverityType severity = (TriggerSeverityType)i;
		size_t actual
		  = dbMonitoring.getNumberOfBadTriggers(option, severity);
		size_t expected = 0;
		cppcut_assert_equal(expected, actual,
		                    cut_message("severity: %d", i));
	}
}

void test_getLastChangeTimeOfTriggerWithNoTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(
	  0, dbMonitoring.getLastChangeTimeOfTrigger(serverid));
}

void test_getNumberOfGoodHosts(void)
{
	assertGetNumberOfHostsWithUserAndStatus(USER_ID_SYSTEM, true);
}

void test_getNumberOfBadHosts(void)
{
	assertGetNumberOfHostsWithUserAndStatus(USER_ID_SYSTEM, false);
}

void test_getNumberOfGoodHostsWithNoAuthorizedServer(void)
{
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithNoAuthorizedServer(void)
{
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_getNumberOfGoodHostsWithOneAuthorizedServer(void)
{
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithOneAuthorizedServer(void)
{
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_getEventSortAscending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventSortAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventSortDescending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventSortDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumber(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMaximumNumber(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	// In this test, we only specify the maximum number. So the output
	// rows cannot be expected.
	arg.assertContent = false;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberDescending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMaximumNumberDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberAndOffsetAscending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMaximumNumberAndOffsetAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberAndOffsetDescending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMaximumNumberAndOffsetDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithLimitOfUnifiedIdAscending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithLimitOfUnifiedIdAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.limitOfUnifiedId = 2;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithSortTimeAscending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithSortTimeAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortType = EventsQueryOption::SORT_TIME;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithSortTimeDescending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithSortTimeDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortType = EventsQueryOption::SORT_TIME;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithOffsetWithoutLimit(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithOffsetWithoutLimit(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.offset = 2;
	arg.expectedErrorCode = HTERR_OFFSET_WITHOUT_LIMIT;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaxNumAndOffsetAndLimitOfUnifiedIdDescending(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMaxNumAndOffsetAndLimitOfUnifiedIdDescending(
  gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.limitOfUnifiedId = 2;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithOneAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithNoAuthorizedServer(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 4;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithInvalidUserId(gconstpointer data)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithInvalidUserId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithEventType(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithEventType(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.type = EVENT_TYPE_BAD;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMinSeverity(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithMinSeverity(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.minSeverity = TRIGGER_SEVERITY_WARNING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTriggerStatus(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithTriggerStatus(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerStatus = TRIGGER_STATUS_PROBLEM;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTimeRange(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithTimeRange(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.beginTime = {1363000000, 0};
	arg.endTime = {1389123457, 0};
	assertGetEventsWithFilter(arg);
}

void test_getEventWithHostnameList(void) {
	loadTestDBEvents();

	EventInfoList eventInfoList;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setHostnameList({"hostX1"});
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
	                   DataQueryOption::SORT_ASCENDING);
	dbMonitoring.getEventInfoList(eventInfoList, option);
	EventInfo expectedEventInfo[] = {
		testEventInfo[2],
		testEventInfo[3],
	};
	// unifiedId is auto increment.
	expectedEventInfo[0].unifiedId = 3;
	expectedEventInfo[1].unifiedId = 4;

	{
		size_t i = 0;
		for (auto eventInfo : eventInfoList) {
			assertEventInfo(expectedEventInfo[i], eventInfo);
			++i;
		}
	}
}

void data_getEventsWithIncidentInfo(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventsWithIncidentInfo(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.withIncidentInfo = true;
	assertGetEventsWithFilter(arg);
}

void data_getEventsWithIncidentInfoByAuthorizedUser(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventsWithIncidentInfoByAuthorizedUser(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	arg.withIncidentInfo = true;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTriggerId(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_getEventWithTriggerId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerId = 3;
	assertGetEventsWithFilter(arg);
}

void test_addIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = "select * from incidents;";
	string expect;

	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		IncidentInfo expectedIncidentInfo = testIncidentInfo[i];
		expect += makeIncidentOutput(expectedIncidentInfo);
		IncidentInfo incidentInfo = testIncidentInfo[i];
		dbMonitoring.addIncidentInfo(&incidentInfo);
	}
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.status = "Assigned";
	incidentInfo.assignee = "hikeshi";
	incidentInfo.updatedAt.tv_sec = time(NULL);
	incidentInfo.updatedAt.tv_nsec = 0;
	dbMonitoring.addIncidentInfo(&incidentInfo);

	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateIncidentInfoByDedicatedFuction(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.status = "Assigned";
	incidentInfo.assignee = "hikeshi";
	incidentInfo.updatedAt.tv_sec = time(NULL);
	incidentInfo.updatedAt.tv_nsec = 0;
	HatoholError err = dbMonitoring.updateIncidentInfo(incidentInfo);

	cppcut_assert_equal(HTERR_OK, err.getCode());
	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateIncidentInfoWithSpace(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.status = "Status With Space";
	HatoholError err = dbMonitoring.updateIncidentInfo(incidentInfo);

	cppcut_assert_equal(HTERR_OK, err.getCode());
	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateNonExistentIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.identifier = "8888"; // non existent identifier
	incidentInfo.status = "Assigned";
	incidentInfo.assignee = "hikeshi";
	incidentInfo.updatedAt.tv_sec = time(NULL);
	incidentInfo.updatedAt.tv_nsec = 0;

	// Should return an error
	HatoholError err = dbMonitoring.updateIncidentInfo(incidentInfo);

	cppcut_assert_equal(HTERR_NOT_FOUND_TARGET_RECORD, err.getCode());
	string statement("select * from incidents;");
	string expect(makeIncidentOutput(testIncidentInfo[0]));
	assertDBContent(&dbAgent, statement, expect);
}

void test_getIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	IncidentInfoVect incidents;
	IncidentsQueryOption option(USER_ID_SYSTEM);
	string expected, actual;

	loadTestDBIncidents();
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		IncidentInfo expectedIncidentInfo = testIncidentInfo[i];
		expected += makeIncidentOutput(expectedIncidentInfo);
	}

	dbMonitoring.getIncidentInfoVect(incidents, option);
	IncidentInfoVectIterator it = incidents.begin();
	for (; it != incidents.end(); ++it) {
		IncidentInfo &actualIncidentInfo = *it;
		actual += makeIncidentOutput(actualIncidentInfo);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentInfoWithUnifiedEventId(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	size_t index = 2;
	IncidentInfo expectedIncidentInfo = testIncidentInfo[index];
	IncidentInfoVect incidents;
	IncidentsQueryOption option(USER_ID_SYSTEM);
	option.setTargetUnifiedEventId(expectedIncidentInfo.unifiedEventId);
	string expected, actual;

	loadTestDBIncidents();
	expected = makeIncidentOutput(expectedIncidentInfo);

	dbMonitoring.getIncidentInfoVect(incidents, option);
	IncidentInfoVectIterator it = incidents.begin();
	for (; it != incidents.end(); ++it) {
		IncidentInfo &actualIncidentInfo = *it;
		actual += makeIncidentOutput(actualIncidentInfo);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getLastUpdateTimeOfIncidents(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBIncidents();
	const IncidentTrackerIdType trackerId = 3;

	uint64_t expected = 0;
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		if (testIncidentInfo[i].trackerId == trackerId &&
		    testIncidentInfo[i].updatedAt.tv_sec > expected) {
			expected = testIncidentInfo[i].updatedAt.tv_sec;
		}
	}

	cppcut_assert_equal(
	  expected, dbMonitoring.getLastUpdateTimeOfIncidents(trackerId));
}

void test_getLastUpdateTimeOfIncidentsWithNoRecord(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const IncidentTrackerIdType trackerId = 3;
	uint64_t expected = 0;
	cppcut_assert_equal(
	  expected, dbMonitoring.getLastUpdateTimeOfIncidents(trackerId));
}

void test_getEventsSelectByHosts(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1, hostIdSet3;
	hostIdSet1.insert("235013");
	hostIdSet3.insert("10002");
	ServerHostSetMap map;
	map[1] = hostIdSet1;
	map[3] = hostIdSet3;
	option.setSelectedHostIds(map);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n"
	  "3|2|1362958000|0|0|3|1|1|41|10002|hostZ2|TEST Trigger 3|\n"
	  "3|4|1390000100|123456789|2|4|2|4|41|10002|hostZ2|"
	    "Status:Unknown, Severity:Critical|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsExcludeByHosts(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1, hostIdSet3;
	hostIdSet1.insert("235012");
	hostIdSet3.insert("10001");
	ServerHostSetMap map;
	map[1] = hostIdSet1;
	map[3] = hostIdSet3;
	option.setExcludedHostIds(map);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n"
	  "3|2|1362958000|0|0|3|1|1|41|10002|hostZ2|TEST Trigger 3|\n"
	  "3|4|1390000100|123456789|2|4|2|4|41|10002|hostZ2|"
	    "Status:Unknown, Severity:Critical|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByServersAndHosts(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1;

	ServerIdSet serverIdSet;
	serverIdSet.insert(3);

	hostIdSet1.insert("235013");
	ServerHostSetMap hostsMap;
	hostsMap[1] = hostIdSet1;
	option.setSelectedServerIds(serverIdSet);
	option.setSelectedHostIds(hostsMap);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n"
	  "3|1|1362957200|0|0|2|1|2|35|10001|hostZ1|TEST Trigger 2|"
	    "{\"expandedDescription\":\"Test Trigger on hostZ1\"}\n"
	  "3|2|1362958000|0|0|3|1|1|41|10002|hostZ2|TEST Trigger 3|\n"
	  "3|3|1390000000|123456789|1|2|1|2|35|10001|hostZ1|TEST Trigger 2|"
	    "{\"expandedDescription\":\"Test Trigger on hostZ1\"}\n"
	  "3|4|1390000100|123456789|2|4|2|4|41|10002|hostZ2|"
	    "Status:Unknown, Severity:Critical|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsExcludeByServersAndSelectByHosts(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1;

	ServerIdSet serverIdSet;
	serverIdSet.insert(1);
	serverIdSet.insert(3);

	hostIdSet1.insert("235013");
	ServerHostSetMap hostsMap;
	hostsMap[1] = hostIdSet1;
	option.setExcludedServerIds(serverIdSet);
	option.setSelectedHostIds(hostsMap);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByServersAndExcludeByHosts(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1;

	ServerIdSet serverIdSet;
	serverIdSet.insert(1);

	hostIdSet1.insert("235012");
	ServerHostSetMap hostsMap;
	hostsMap[1] = hostIdSet1;
	option.setSelectedServerIds(serverIdSet);
	option.setExcludedHostIds(hostsMap);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByHostgroup(void)
{
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	HostgroupIdSet hostgroupIdSet;
	hostgroupIdSet.insert("1");
	ServerHostSetMap map;
	map[1] = hostgroupIdSet;
	option.setSelectedHostgroupIds(map);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|1|1363123456|0|0|2|1|1|10|235012|hostX1|TEST Trigger 1a|"
	    "{\"expandedDescription\":\"Test Trigger on hostX1\"}\n"
	  "1|2|1378900022|0|0|1|0|1|10|235012|hostX1|TEST Trigger 1|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsExcludeByHostgroup(void)
{
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	HostgroupIdSet hostgroupIdSet1, hostgroupIdSet3;
	hostgroupIdSet1.insert("2");
	hostgroupIdSet3.insert("1");
	hostgroupIdSet3.insert("2");
	ServerHostSetMap map;
	map[1] = hostgroupIdSet1;
	map[3] = hostgroupIdSet3;
	option.setExcludedHostgroupIds(map);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|1|1363123456|0|0|2|1|1|10|235012|hostX1|TEST Trigger 1a|"
	    "{\"expandedDescription\":\"Test Trigger on hostX1\"}\n"
	  "1|2|1378900022|0|0|1|0|1|10|235012|hostX1|TEST Trigger 1|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByTypes(void)
{
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();
	loadTestDBEvents();
	loadTestDBIncidents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	set<EventType> types;
	types.insert(EVENT_TYPE_BAD);
	types.insert(EVENT_TYPE_UNKNOWN);
	option.setEventTypes(types);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	   "3|3|1390000000|123456789|1|2|1|2|35|10001|hostZ1|TEST Trigger 2|"
	     "{\"expandedDescription\":\"Test Trigger on hostZ1\"}\n"
	   "3|4|1390000100|123456789|2|4|2|4|41|10002|hostZ2|"
	     "Status:Unknown, Severity:Critical|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByIncidentStatuses(void)
{
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();
	loadTestDBEvents();
	loadTestDBIncidents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	set<string> statuses;
	statuses.insert("New");
	statuses.insert("NONE");
	option.setIncidentStatuses(statuses);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|1|1363123456|0|0|2|1|1|10|235012|hostX1|TEST Trigger 1a|"
	    "{\"expandedDescription\":\"Test Trigger on hostX1\"}\n"
	  "1|2|1378900022|0|0|1|0|1|10|235012|hostX1|TEST Trigger 1|\n");
	cppcut_assert_equal(expected, actual);
}

void test_getEventsSelectByEmptyIncidentStatus(void)
{
	loadTestDBServerHostDef();
	loadTestDBHostgroup();
	loadTestDBHostgroupMember();
	loadTestDBEvents();
	loadTestDBIncidents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	EventsQueryOption option(USER_ID_SYSTEM);
	set<string> statuses;
	statuses.insert("");
	option.setIncidentStatuses(statuses);

	EventInfoList events;
	dbMonitoring.getEventInfoList(events, option, NULL);
	string actual = sortedJoin(events);
	string expected(
	  "1|3|1389123457|0|0|3|1|1|11|235013|hostX2|TEST Trigger 1b|\n"
	  "2147418113|1|1362957117|0|1|3|1|1|4293844428|10002|defunctSv1Host1|"
	    "defunctSv1Host1 material|\n"
	  "3|1|1362957200|0|0|2|1|2|35|10001|hostZ1|TEST Trigger 2|"
	    "{\"expandedDescription\":\"Test Trigger on hostZ1\"}\n"
	  "3|2|1362958000|0|0|3|1|1|41|10002|hostZ2|TEST Trigger 3|\n"
	  "3|3|1390000000|123456789|1|2|1|2|35|10001|hostZ1|TEST Trigger 2|"
	    "{\"expandedDescription\":\"Test Trigger on hostZ1\"}\n"
	  "3|4|1390000100|123456789|2|4|2|4|41|10002|hostZ2|"
	    "Status:Unknown, Severity:Critical|\n"
);
	cppcut_assert_equal(expected, actual);
}

void test_getSystemInfo(void)
{
	DataQueryOption option(findUserWith(OPPRVLG_GET_SYSTEM_INFO));
	// get the current number of the events
	DBTablesMonitoring::SystemInfo systemInfo0;
	assertHatoholError(
	  HTERR_OK, DBTablesMonitoring::getSystemInfo(systemInfo0, option));

	// write events
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbMonitoring.addEventInfoList(eventInfoList);

	// get the latest one
	DBTablesMonitoring::SystemInfo systemInfo1;
	assertHatoholError(
	  HTERR_OK, DBTablesMonitoring::getSystemInfo(systemInfo1, option));

	for (size_t i = 0; i < DBTablesMonitoring::NUM_EVENTS_COUNTERS; i++) {
		uint64_t diffPrev =
		  systemInfo1.eventsCounterPrevSlots[i].number
		  - systemInfo0.eventsCounterPrevSlots[i].number;
		cppcut_assert_equal(static_cast<uint64_t>(0), diffPrev);

		uint64_t diffCurr =
		   systemInfo1.eventsCounterCurrSlots[i].number
		   - systemInfo0.eventsCounterCurrSlots[i].number;
		cppcut_assert_equal(static_cast<uint64_t>(NumTestEventInfo),
				    diffCurr);
	}
}

void test_getSystemInfoByInvalidUser(void)
{
	DataQueryOption option(findUserWithout(OPPRVLG_GET_SYSTEM_INFO));
	DBTablesMonitoring::SystemInfo systemInfo;
	assertHatoholError(
	  HTERR_INVALID_USER,
	  DBTablesMonitoring::getSystemInfo(systemInfo, option));
}

void test_addIncidentHistory(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	string statement = "select * from incident_histories;";
	for (size_t i = 0; i < NumTestIncidentHistory; i++) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i];
		expectedIncidentHistory.id = i + 1;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
		IncidentHistory incidentHistory =
		  testIncidentHistory[i];
		dbMonitoring.addIncidentHistory(incidentHistory);
	}
	assertDBContent(&dbAgent, statement, expected);
}

static const IncidentInfo *findIncidentInfoWithUnifiedEventId(
  const UnifiedEventIdType unifiedEventId)
{
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		if (testIncidentInfo[i].unifiedEventId == unifiedEventId)
			return &testIncidentInfo[i];
	}
	return nullptr;
}

void test_incrementIncidentCommentCount(void)
{
	loadTestDBIncidents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	IncidentHistory history = testIncidentHistory[1];
	dbMonitoring.addIncidentHistory(history);
	history.status = "DONE";
	history.comment = "Done.";
	history.createdAt.tv_sec += 60;
	dbMonitoring.addIncidentHistory(history);

	IncidentInfo incident =
	  *findIncidentInfoWithUnifiedEventId(history.unifiedEventId);
	incident.commentCount = 2;
	string statement =
	  StringUtils::sprintf(
	    "select * from incidents where unified_event_id=%"
	    FMT_UNIFIED_EVENT_ID, history.unifiedEventId);
	string expected = makeIncidentOutput(incident);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	assertDBContent(&dbAgent, statement, expected);
}

void test_notIncrementIncidentCommentCount(void)
{
	loadTestDBIncidents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	IncidentHistory history = testIncidentHistory[1];
	history.comment = "";
	dbMonitoring.addIncidentHistory(history);

	IncidentInfo incident =
	  *findIncidentInfoWithUnifiedEventId(history.unifiedEventId);
	string statement =
	  StringUtils::sprintf(
	    "select * from incidents where unified_event_id=%"
	    FMT_UNIFIED_EVENT_ID, history.unifiedEventId);
	string expected = makeIncidentOutput(incident);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	assertDBContent(&dbAgent, statement, expected);
}

void test_updateIncidentHistory(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentHistory history = testIncidentHistory[0];
	history.id = 1;
	history.comment = "Oops!";

	string expected;
	for (size_t i = 0; i < NumTestIncidentHistory; i++) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i];
		expectedIncidentHistory.id = i + 1;
		if (expectedIncidentHistory.id == history.id)
			expectedIncidentHistory.comment = history.comment;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
	}

	dbMonitoring.updateIncidentHistory(history);

	string statement("select * from incident_histories;");
	assertDBContent(&dbAgent, statement, expected);
}

void test_getIncidentHistory(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setTargetUnifiedEventId(3);
	option.setTargetUserId(1);

	IncidentHistory expectedIncidentHistory =
	  testIncidentHistory[0];
	expectedIncidentHistory.id = 1;
	expected = makeIncidentHistoryOutput(expectedIncidentHistory);

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentHistoryById(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setTargetId(2);

	IncidentHistory expectedIncidentHistory =
	  testIncidentHistory[1];
	expectedIncidentHistory.id = 2;
	expected = makeIncidentHistoryOutput(expectedIncidentHistory);

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentHistoryWithSortTimeAscending(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setSortType(IncidentHistoriesQueryOption::SORT_TIME,
			   DataQueryOption::SORT_ASCENDING);

	for (size_t i = 0; i < NumTestIncidentHistory; i++) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i];
		expectedIncidentHistory.id = i + 1;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
	}

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentHistoryWithSortTimeDescending(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setSortType(IncidentHistoriesQueryOption::SORT_TIME,
			   DataQueryOption::SORT_DESCENDING);

	// Reverse access
	for (size_t i = NumTestIncidentHistory; i > 0; i--) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i - 1];
		expectedIncidentHistory.id = i;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
	}

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentHistoryWithSortUnifiedEventIdAscending(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setSortType(IncidentHistoriesQueryOption::SORT_UNIFIED_EVENT_ID,
			   DataQueryOption::SORT_ASCENDING);

	for (size_t i = 0; i < NumTestIncidentHistory; i++) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i];
		expectedIncidentHistory.id = i + 1;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
	}

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getIncidentHistoryWithSortUnifiedEventIdDescending(void)
{
	loadTestDBIncidentHistory();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	string expected, actual;

	IncidentHistoriesQueryOption option(USER_ID_SYSTEM);
	option.setSortType(IncidentHistoriesQueryOption::SORT_UNIFIED_EVENT_ID,
			   DataQueryOption::SORT_DESCENDING);

	// Reverse access
	for (size_t i = NumTestIncidentHistory; i > 0; i--) {
		IncidentHistory expectedIncidentHistory =
		  testIncidentHistory[i - 1];
		expectedIncidentHistory.id = i;
		expected +=
		  makeIncidentHistoryOutput(expectedIncidentHistory);
	}

	list<IncidentHistory> incidentHistoryList;
	dbMonitoring.getIncidentHistory(incidentHistoryList, option);
	for (auto incidentHistory : incidentHistoryList) {
		actual += makeIncidentHistoryOutput(incidentHistory);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getNumberOfEvents(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(1);
	cppcut_assert_equal(static_cast<size_t>(3),
			    dbMonitoring.getNumberOfEvents(option));
}

void test_getNumberOfHostsWithSpecifiedEvents(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	std::set<TriggerSeverityType> severitiesSet;
	severitiesSet.insert(TRIGGER_SEVERITY_WARNING);
	severitiesSet.insert(TRIGGER_SEVERITY_CRITICAL);
	option.setTriggerSeverities(severitiesSet);
	cppcut_assert_equal(static_cast<size_t>(2),
			    dbMonitoring.getNumberOfHostsWithSpecifiedEvents(option));
}

void test_getEventSeverityStatisticsWithImportantSeverityRankOption(void)
{
	loadTestDBEvents();
	loadTestDBSeverityRankInfo();

	DBTablesMonitoring::EventSeverityStatistics
	expectedSeverityStatistics[] = {
		{TRIGGER_SEVERITY_CRITICAL, 1},
	};

	DBHatohol dbHatohol;
	DBTablesConfig &dbConfig = dbHatohol.getDBTablesConfig();

	SeverityRankQueryOption severityRankOption(USER_ID_SYSTEM);
	severityRankOption.setTargetAsImportant(static_cast<int>(true));
	SeverityRankInfoVect importantSeverityRanks;
	dbConfig.getSeverityRanks(importantSeverityRanks, severityRankOption);
	std::set<TriggerSeverityType> importantStatusSet;
	for (auto &severityRank : importantSeverityRanks) {
		importantStatusSet.insert(static_cast<TriggerSeverityType>(severityRank.status));
	}

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTriggerSeverities(importantStatusSet);
	std::vector<DBTablesMonitoring::EventSeverityStatistics> severityStatisticsVect;
	dbMonitoring.getEventSeverityStatistics(severityStatisticsVect, option);
	{
		size_t i = 0;
		for (auto statistics : severityStatisticsVect) {
			cppcut_assert_equal(expectedSeverityStatistics[i].severity,
					    statistics.severity);
			cppcut_assert_equal(expectedSeverityStatistics[i].num,
					    statistics.num);
			++i;
		}
	}
}

void test_getEventSeverityStatisticsWithOutImportantSeverityRankOption(void)
{
	loadTestDBEvents();

	DBTablesMonitoring::EventSeverityStatistics
	expectedSeverityStatistics[] = {
		{TRIGGER_SEVERITY_INFO, 4},
		{TRIGGER_SEVERITY_WARNING, 2},
		{TRIGGER_SEVERITY_CRITICAL, 1},
	};

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	std::vector<DBTablesMonitoring::EventSeverityStatistics> severityStatisticsVect;
	dbMonitoring.getEventSeverityStatistics(severityStatisticsVect, option);
	{
		size_t i = 0;
		for (auto statistics : severityStatisticsVect) {
			cppcut_assert_equal(expectedSeverityStatistics[i].severity,
					    statistics.severity);
			cppcut_assert_equal(expectedSeverityStatistics[i].num,
					    statistics.num);
			++i;
		}
	}
}

void test_getNumberOfEventsWithHostgroup(void)
{
	loadTestDBEvents();
	loadTestDBHostgroupMember();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(3);
	option.setTargetHostgroupId("1");
	cppcut_assert_equal(static_cast<size_t>(2),
			    dbMonitoring.getNumberOfEvents(option));
}

} // namespace testDBTablesMonitoring
