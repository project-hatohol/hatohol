/*
 * Copyright (C) 2013-2014 Project Hatohol
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

static const int dupEventInfoType = 1;

static void addTriggerInfo(TriggerInfo *triggerInfo)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	dbMonitoring.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

static string makeTriggerOutput(const TriggerInfo &triggerInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%" FMT_SERVER_ID "|%" PRIu64 "|%d|%d|%ld|%lu|%" PRIu64 "|%s|%s\n",
	    triggerInfo.serverId,
	    triggerInfo.id,
	    triggerInfo.status, triggerInfo.severity,
	    triggerInfo.lastChangeTime.tv_sec,
	    triggerInfo.lastChangeTime.tv_nsec,
	    triggerInfo.hostId,
	    triggerInfo.hostName.c_str(),
	    triggerInfo.brief.c_str());
	return expectedOut;
}

struct AssertGetTriggersArg
  : public AssertGetHostResourceArg<TriggerInfo, TriggersQueryOption>
{
	AssertGetTriggersArg(gconstpointer ddtParam)
	{
		fixtures = testTriggerInfo;
		numberOfFixtures = NumTestTriggerInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual HostIdType getHostId(const TriggerInfo &info) const override
	{
		return info.hostId;
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
  gconstpointer ddtParam, uint32_t serverId, uint64_t hostId = ALL_HOSTS)
{
	loadTestDBTriggers();
	loadTestDBHosts();
	loadTestDBHostgroupElements();

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

static string makeItemOutput(const ItemInfo &itemInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%" PRIu32 "|%" PRIu64 "|%" PRIu64 "|%s|%ld|%lu|%s|%s|%s\n",
	    itemInfo.serverId, itemInfo.id,
	    itemInfo.hostId,
	    itemInfo.brief.c_str(),
	    itemInfo.lastValueTime.tv_sec,
	    itemInfo.lastValueTime.tv_nsec,
	    itemInfo.lastValue.c_str(),
	    itemInfo.prevValue.c_str(),
	    itemInfo.itemGroupName.c_str());
	return expectedOut;
}

struct AssertGetItemsArg
  : public AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
{
	string itemGroupName;

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
		option.setTargetItemGroupName(itemGroupName);
	}

	virtual bool filterOutExpectedRecord(ItemInfo *info) override
	{
		if (AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}

		if (!itemGroupName.empty() &&
		    info->itemGroupName != itemGroupName) {
			return true;
		}

		if (filterForDataOfDefunctSv) {
			if (!option.isValidServer(info->serverId))
				return true;
		}

		return false;
	}

	virtual HostIdType getHostId(const ItemInfo &info) const override
	{
		return info.hostId;
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

	HostgroupElementList hostgroupElementList;
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		hostgroupElementList.push_back(testHostgroupElement[i]);
	dbMonitoring.addHostgroupElementList(hostgroupElementList);

	HostgroupInfoList hostgroupInfoList;
	for (size_t i = 0; i < NumTestHostgroupInfo; i++)
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);

	AssertGetItemsArg arg(data);
	arg.targetServerId = serverId;
	assertGetItems(arg);
}
#define assertItemInfoList(DATA, SERVER_ID) \
cut_trace(_assertItemInfoList(DATA, SERVER_ID))

static string makeHostOutput(const HostInfo &hostInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%" PRIu32 "|%" PRIu64 "|%s\n",
	    hostInfo.serverId, hostInfo.id, hostInfo.hostName.c_str());
	return expectedOut;
}

struct AssertGetHostsArg
  : public AssertGetHostResourceArg<HostInfo, HostsQueryOption>
{
	HostInfoList expectedHostList;

	AssertGetHostsArg(gconstpointer ddtParam)
	{
		fixtures = testHostInfo;
		numberOfFixtures = NumTestHostInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual HostIdType getHostId(const HostInfo &info) const override
	{
		return info.id;
	}

	virtual string makeOutputText(const HostInfo &hostInfo)
	{
		return makeHostOutput(hostInfo);
	}
};

static void _assertGetHosts(AssertGetHostsArg &arg)
{
	loadTestDBHosts();
	loadTestDBHostgroupElements();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	dbMonitoring.getHostInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetHosts(A) cut_trace(_assertGetHosts(A))

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
	cppcut_assert_equal(expect.hostId, actual.hostId);
	cppcut_assert_equal(expect.hostName, actual.hostName);
	cppcut_assert_equal(expect.brief, actual.brief);
}
#define assertTriggerInfo(E,A) cut_trace(_assertTriggerInfo(E,A))

static string makeHostgroupsOutput(const HostgroupInfo &hostgroupInfo, size_t id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%" FMT_HOST_GROUP_ID "|%s\n",
	  id + 1, hostgroupInfo.serverId,
	  hostgroupInfo.groupId, hostgroupInfo.groupName.c_str());

	return expectedOut;
}

static string makeMapHostsHostgroupsOutput
  (const HostgroupElement &hostgroupElement, size_t id)
{
	HostgroupElementQueryOption option;
	const DBTermCodec *dbTermCodec = option.getDBTermCodec();
	string expectedOut = StringUtils::sprintf(
	  "%zd|%s|%s|%s\n",
	  id + 1,
	  dbTermCodec->enc(hostgroupElement.serverId).c_str(),
	  dbTermCodec->enc(hostgroupElement.hostId).c_str(),
	  dbTermCodec->enc(hostgroupElement.groupId).c_str());

	return expectedOut;
}

static string makeHostsOutput(const HostInfo &hostInfo, size_t id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%" FMT_SERVER_ID "|%" FMT_HOST_ID "|%s\n",
	  id + 1, hostInfo.serverId, hostInfo.id, hostInfo.hostName.c_str());

	return expectedOut;
}

static void prepareDataForAllHostgroupIds(void)
{
	set<HostgroupIdType> hostgroupIdSet = getTestHostgroupIdSet();
	hostgroupIdSet.insert(ALL_HOST_GROUPS);
	set<HostgroupIdType>::const_iterator hostgrpIdItr =
	  hostgroupIdSet.begin();
	for (; hostgrpIdItr != hostgroupIdSet.end(); ++hostgrpIdItr) {
		gcut_add_datum(
		  StringUtils::sprintf("Hostgroup ID: %" FMT_HOST_GROUP_ID,
		                       *hostgrpIdItr).c_str(),
		  "hostgroupId", G_TYPE_UINT64, *hostgrpIdItr, NULL);
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
	  DB_TABLES_ID_MONITORING, DBTablesMonitoring::MONITORING_DB_VERSION);
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
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string sql = StringUtils::sprintf("SELECT * from triggers");
	string expectedOut = makeTriggerOutput(*testInfo);
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
}

void test_getTriggerInfo(void)
{
	loadTestDBTriggers();
	loadTestDBHosts();
	loadTestDBHostgroupElements();

	int targetIdx = 2;
	TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
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
	const TriggerIdType invalidTrigId = -1;
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(invalidSvId);
	option.setTargetId(invalidTrigId);
	cppcut_assert_equal(false,
	                    dbMonitoring.getTriggerInfo(triggerInfo, option));
}

void data_getTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoList(gconstpointer data)
{
	assertGetTriggerInfoList(data, ALL_SERVERS);
}

void data_getTriggerInfoListForOneServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	assertGetTriggerInfoList(data, targetServerId);
}

void data_getTriggerInfoListForOneServerOneHost(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoListForOneServerOneHost(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[1].serverId;
	const HostIdType targetHostId = testTriggerInfo[1].hostId;
	assertGetTriggerInfoList(data, targetServerId, targetHostId);
}

void data_setTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_setTriggerInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	const ServerIdType serverId = testTriggerInfo[0].serverId;
	dbMonitoring.setTriggerInfoList(triggerInfoList, serverId);

	HostgroupElementList hostgroupElementList;
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		hostgroupElementList.push_back(testHostgroupElement[i]);
	dbMonitoring.addHostgroupElementList(hostgroupElementList);

	HostgroupInfoList hostgroupInfoList;
	for (size_t i = 0; i < NumTestHostgroupInfo; i++)
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);

	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_addTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
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

	// Add HostgroupElement
	HostgroupElementList hostgroupElementList;
	for (size_t j = 0; j < NumTestHostgroupElement; j++)
		hostgroupElementList.push_back(testHostgroupElement[j]);
	dbMonitoring.addHostgroupElementList(hostgroupElementList);

	// Add HostgroupInfo
	HostgroupInfoList hostgroupInfoList;
	for (size_t j = 0; j < NumTestHostgroupInfo; j++)
		hostgroupInfoList.push_back(testHostgroupInfo[j]);
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);

	// Check
	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_getTriggerWithOneAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 5;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 4;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithInvalidUserId(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetTriggersWithFilter(arg);
}

void data_itemInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemInfoList(gconstpointer data)
{
	assertItemInfoList(data, ALL_SERVERS);
}

void data_itemInfoListForOneServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testItemInfo[0].serverId;
	assertItemInfoList(data, targetServerId);
}

void data_addItemInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addItemInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbMonitoring.addItemInfoList(itemInfoList);

	HostgroupElementList hostgroupElementList;
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		hostgroupElementList.push_back(testHostgroupElement[i]);
	dbMonitoring.addHostgroupElementList(hostgroupElementList);

	HostgroupInfoList hostgroupInfoList;
	for (size_t i = 0; i < NumTestHostgroupInfo; i++)
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);

	AssertGetItemsArg arg(data);
	assertGetItems(arg);
}

void data_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithItemGroupName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemGroupName = "City";
	assertGetItemsWithFilter(arg);
}

void data_getNumberOfItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemsWithItemGroupName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemsWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemGroupName = "City";
	assertGetNumberOfItems(arg);
}

void data_addEventInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
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

void data_addDupEventInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getLastEventId(gconstpointer data)
{
	test_addEventInfoList(data);
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(findLastEventId(serverid),
	                    dbMonitoring.getLastEventId(serverid));
}

void test_getLastEventIdWithNoEvent(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(EVENT_NOT_FOUND,
	                    dbMonitoring.getLastEventId(serverid));
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
	const TriggerIdType triggerId = 3;
	cppcut_assert_equal(
	  findTimeOfLastEvent(serverId, triggerId),
	  dbMonitoring.getTimeOfLastEvent(serverId, triggerId));
}

void data_getHostInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoList(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	assertGetHosts(arg);
}

void data_getHostInfoListForOneServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListForOneServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.targetServerId = 1;
	assertGetHosts(arg);
}

void data_getHostInfoListWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 4;
	assertGetHosts(arg);
}

void data_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 5;
	assertGetHosts(arg);
}

void data_getHostInfoListWithUserWhoCanAccessSomeHostgroups(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getHostInfoListWithUserWhoCanAccessSomeHostgroups(gpointer data)
{
	AssertGetHostsArg arg(data);
	arg.userId = 3;
	assertGetHosts(arg);
}

void data_getNumberOfTriggers(void)
{
	prepareDataForAllHostgroupIds();
}

void test_getNumberOfTriggers(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupElements();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_int(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);
	cppcut_assert_equal(
	  dbMonitoring.getNumberOfTriggers(option),
	  getNumberOfTestTriggers(targetServerId, hostgroupId),
	  cut_message("sv: %" FMT_SERVER_ID ", hostgroup: %" FMT_HOST_GROUP_ID,
		      targetServerId, hostgroupId));
}

void test_getNumberOfTriggersForMultipleAuthorizedHostgroups(void)
{
	loadTestDBTriggers();
	loadTestDBHostgroupElements();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId = ALL_HOST_GROUPS;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(userIdWithMultipleAuthorizedHostgroups);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);

	cppcut_assert_equal(
	  dbMonitoring.getNumberOfTriggers(option),
	  getNumberOfTestTriggers(targetServerId, hostgroupId));
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
	cppcut_assert_equal(expect, actual,
			    cut_message(
			      "sv: %" FMT_SERVER_ID ", "
			      "hostgroup: %" FMT_HOST_GROUP_ID ", "
			      "severity: %d",
			      serverId, hostgroupId, severity));
}
#define assertGetNumberOfTriggers(D,S,H,V) \
cut_trace(_assertGetNumberOfTriggers(D,S,H,V))

void test_getNumberOfTriggersBySeverity(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupElements();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_int(data, "hostgroupId");

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
	loadTestDBHostgroupElements();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_int(data, "hostgroupId");

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
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventSortAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventSortDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventSortDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumber(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 4;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithInvalidUserId(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithInvalidUserId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMinSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMinSeverity(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.minSeverity = TRIGGER_SEVERITY_WARNING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTriggerStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithTriggerStatus(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerStatus = TRIGGER_STATUS_PROBLEM;
	assertGetEventsWithFilter(arg);
}

void data_getEventsWithIncidentInfo(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventsWithIncidentInfo(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.withIncidentInfo = true;
	assertGetEventsWithFilter(arg);
}

void data_getEventsWithIncidentInfoByAuthorizedUser(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
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
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithTriggerId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerId = 3;
	assertGetEventsWithFilter(arg);
}

void test_addHostgroupInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	HostgroupInfoList hostgroupInfoList;
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = "select * from hostgroups;";
	string expect;

	for (size_t i = 0; i < NumTestHostgroupInfo; i++) {
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
		expect += makeHostgroupsOutput(testHostgroupInfo[i], i);
	}
	dbMonitoring.addHostgroupInfoList(hostgroupInfoList);
	assertDBContent(&dbAgent, statement, expect);
}

void test_addHostgroupElement(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	HostgroupElementList hostgroupElementList;
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = "select * from map_hosts_hostgroups";
	string expect;

	for (size_t i = 0; i < NumTestHostgroupElement; i++) {
		hostgroupElementList.push_back(testHostgroupElement[i]);
		expect += makeMapHostsHostgroupsOutput(
		            testHostgroupElement[i], i);
	}
	dbMonitoring.addHostgroupElementList(hostgroupElementList);
	assertDBContent(&dbAgent, statement, expect);
}

void test_addHostInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	HostInfoList hostInfoList;
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = "select * from hosts;";
	string expect;

	for(size_t i = 0; i < NumTestHostInfo; i++) {
		hostInfoList.push_back(testHostInfo[i]);
		expect += makeHostsOutput(testHostInfo[i], i);
	}
	dbMonitoring.addHostInfoList(hostInfoList);
	assertDBContent(&dbAgent, statement, expect);
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
		dbMonitoring.addIncidentInfo(&testIncidentInfo[i]);
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
	dbMonitoring.updateIncidentInfo(incidentInfo);

	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
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

} // namespace testDBTablesMonitoring
