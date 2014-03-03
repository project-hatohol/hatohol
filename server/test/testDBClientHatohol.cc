/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <cutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "DBClientHatohol.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "Params.h"
#include "CacheServiceDBClient.h"
#include "testDBClientHatohol.h"
#include <algorithm>
using namespace std;
using namespace mlpl;

namespace testDBClientHatohol {

class TestHostResourceQueryOption : public HostResourceQueryOption {
public:
	string callGetServerIdColumnName(const string &tableAlias = "") const
	{
		return getServerIdColumnName(tableAlias);
	}

	static string callMakeConditionServer(const ServerIdSet &serverIdSet,
	                                const string &serverIdColumnName)
	{
		return makeConditionServer(serverIdSet, serverIdColumnName);
	}

	static
	string callMakeCondition(const ServerHostGrpSetMap &srvHostGrpSetMap,
				 const string &serverIdColumnName,
				 const string &hostGroupIdColumnName,
				 const string &hostIdColumnName,
				 uint32_t targetServerId = ALL_SERVERS,
				 uint64_t targetHostgroupId = ALL_HOST_GROUPS,
				 uint64_t targetHostId = ALL_HOSTS)
	{
		return makeCondition(srvHostGrpSetMap,
				     serverIdColumnName,
				     hostGroupIdColumnName,
				     hostIdColumnName,
				     targetServerId,
				     targetHostgroupId,
				     targetHostId);
	}
};

static const string serverIdColumnName = "server_id";
static const string hostGroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

static void addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBClientHatohol dbHatohol;
	dbHatohol.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

static string makeTriggerOutput(const TriggerInfo &triggerInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%"FMT_SERVER_ID"|%"PRIu64"|%d|%d|%ld|%lu|%"PRIu64"|%s|%s\n",
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

static void addHostgroupInfo(HostgroupInfo *hostgroupInfo)
{
	DBClientHatohol dbHatohol;
	dbHatohol.addHostgroupInfo(hostgroupInfo);
}
#define assertAddHostgroupInfoToDB(X) \
cut_trace(_assertAddToDB<HostgroupInfo>(X, addHostgroupInfo))

static void addHostgroupElement(HostgroupElement *hostgroupElement)
{
	DBClientHatohol dbHatohol;
	dbHatohol.addHostgroupElement(hostgroupElement);
}
#define assertAddHostgroupElementToDB(X) \
cut_trace(_assertAddToDB<HostgroupElement>(X, addHostgroupElement))

struct AssertGetTriggersArg
  : public AssertGetHostResourceArg<TriggerInfo, TriggersQueryOption>
{
	AssertGetTriggersArg(gconstpointer ddtParam)
	{
		fixtures = testTriggerInfo;
		numberOfFixtures = NumTestTriggerInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual uint64_t getHostId(TriggerInfo &info)
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
	DBClientHatohol dbHatohol;
	arg.fixup();
	dbHatohol.getTriggerInfoList(arg.actualRecordList, arg.option);
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

static void _setupTestTriggerDB(void)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		assertAddTriggerToDB(&testTriggerInfo[i]);
}
#define setupTestTriggerDB() cut_trace(_setupTestTriggerDB())

static void _setupTestHostgroupInfoDB(void)
{
	for (size_t i = 0; i < NumTestHostgroupInfo; i++)
		assertAddHostgroupInfoToDB(&testHostgroupInfo[i]);
}
#define setupTestHostgroupInfoDB() cut_trace(_setupTestHostgroupInfoDB())

static void _setupTestHostgroupElementDB(void)
{
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		assertAddHostgroupElementToDB(&testHostgroupElement[i]);
}
#define setupTestHostgroupElementDB() cut_trace(_setupTestHostgroupElementDB())

static void _assertGetTriggerInfoList(
  gconstpointer ddtParam, uint32_t serverId, uint64_t hostId = ALL_HOSTS)
{
	setupTestTriggerDB();
	setupTestHostgroupElementDB();
	setupTestHostgroupInfoDB();
	AssertGetTriggersArg arg(ddtParam);
	arg.targetServerId = serverId;
	arg.targetHostId = hostId;
	assertGetTriggers(arg);
}
#define assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ...) \
cut_trace(_assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ##__VA_ARGS__))

static void _assertGetEvents(AssertGetEventsArg &arg)
{
	DBClientHatohol dbHatohol;
	arg.fixup();
	assertHatoholError(
	  arg.expectedErrorCode,
	  dbHatohol.getEventInfoList(arg.actualRecordList, arg.option));
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

	if (arg.maxNumber)
		arg.option.setMaximumNumber(arg.maxNumber);
	arg.option.setSortType(arg.sortType, arg.sortDirection);
	if (arg.offset)
		arg.option.setOffset(arg.offset);
	if (arg.limitOfUnifiedId)
		arg.option.setLimitOfUnifiedId(arg.limitOfUnifiedId);
	assertGetEvents(arg);
}
#define assertGetEventsWithFilter(ARG) \
cut_trace(_assertGetEventsWithFilter(ARG))

static string makeItemOutput(const ItemInfo &itemInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%"PRIu32"|%"PRIu64"|%"PRIu64"|%s|%ld|%lu|%s|%s|%s\n",
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
	AssertGetItemsArg(gconstpointer ddtParam)
	{
		fixtures = testItemInfo;
		numberOfFixtures = NumTestItemInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual uint64_t getHostId(ItemInfo &info)
	{
		return info.hostId;
	}

	virtual string makeOutputText(const ItemInfo &itemInfo)
	{
		return makeItemOutput(itemInfo);
	}

	virtual bool filterOutExpectedRecord(ItemInfo *info) // override
	{
		if (AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}

		if (!filterForDataOfDefunctSv)
			return false;
		return !option.isValidServer(info->serverId);
	}
};

static void _assertGetItems(AssertGetItemsArg &arg)
{
	DBClientHatohol dbHatohol;
	arg.fixup();
	dbHatohol.getItemInfoList(arg.actualRecordList, arg.option);
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

void _assertItemInfoList(gconstpointer data, uint32_t serverId)
{
	DBClientHatohol dbHatohol;
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbHatohol.addItemInfoList(itemInfoList);

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
	    "%"PRIu32"|%"PRIu64"|%s\n",
	    hostInfo.serverId, hostInfo.id, hostInfo.hostName.c_str());
	return expectedOut;
}

struct AssertGetHostsArg
  : public AssertGetHostResourceArg<HostInfo, HostsQueryOption>
{
	HostInfoList expectedHostList;

	AssertGetHostsArg(gconstpointer ddtParam)
	{
		setDataDrivenTestParam(ddtParam);
	}

	virtual void fixupExpectedRecords(void) // override
	{
		getTestHostInfoList(expectedHostList, targetServerId, NULL);
		HostInfoListIterator it = expectedHostList.begin();
		for (; it != expectedHostList.end(); ++it) {	
			HostInfo &record = *it;
			if (filterOutExpectedRecord(&record))
				continue;
			if (isAuthorized(record))
				expectedRecords.push_back(&record);
		}
	}

	virtual uint64_t getHostId(HostInfo &info)
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
	// We have to insert test trigger data in DB first. Because current
	// implementation of DBClientHatohol creates hostInfoList from trigger
	// table.
	// TODO: The implementation will be fixed in the future. The DB table
	//       for host will be added. After that, we will fix this setup.
	setupTestTriggerDB();

	DBClientHatohol dbHatohol;
	arg.fixup();
	dbHatohol.getHostInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetHosts(A) cut_trace(_assertGetHosts(A))

static void _assertGetNumberOfHostsWithUserAndStatus(UserIdType userId, bool status)
{
	setupTestTriggerDB();

	uint32_t serverId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	uint64_t hostGroupId = 0;

	DBClientHatohol dbHatohol;
	int expected = getNumberOfTestHostsWithStatus(serverId, hostGroupId,
	                                              status, userId);
	int actual;
	HostsQueryOption option(userId);
	option.setTargetServerId(serverId);
	//TODO: hostGroupId isn't supported yet
	//option.setTargetHostGroupId(hostGroupId);

	if (status)
		actual  = dbHatohol.getNumberOfGoodHosts(option);
	else
		actual  = dbHatohol.getNumberOfBadHosts(option);
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

// FIXME: Change order of parameter.
static void _assertMakeCondition(const ServerHostGrpSetMap &srvHostGrpSetMap,
				 const string &expect,
				 uint32_t targetServerId = ALL_SERVERS,
				 uint64_t targetHostId = ALL_HOSTS,
				 uint64_t targetHostgroupId = ALL_HOST_GROUPS)
{
	string cond = TestHostResourceQueryOption::callMakeCondition(
			srvHostGrpSetMap,
			serverIdColumnName,
			hostGroupIdColumnName,
			hostIdColumnName,
			targetServerId,
			targetHostgroupId,
			targetHostId);
	cppcut_assert_equal(expect, cond);
}
#define assertMakeCondition(M, ...) \
  cut_trace(_assertMakeCondition(M, ##__VA_ARGS__))

static string makeExpectedConditionForUser(
  UserIdType userId, OperationPrivilegeFlag flags)
{
	string exp;
	UserIdIndexMap userIdIndexMap;
	makeTestUserIdIndexMap(userIdIndexMap);
	UserIdIndexMapIterator it = userIdIndexMap.find(userId);
	if (flags & (1 << OPPRVLG_GET_ALL_SERVER)) {
		// Although it's not correct when a target*Id is specified,
		// currently no target is specified in this function.
		return "";
	}
	if (it == userIdIndexMap.end())
		return DBClientHatohol::getAlwaysFalseCondition();

	ServerHostGrpSetMap srvHostGrpSetMap;
	const set<int> &indexes = it->second;
	set<int>::const_iterator jt = indexes.begin();
	for (; jt != indexes.end(); ++jt) {
		const AccessInfo &accInfo = testAccessInfo[*jt];
		srvHostGrpSetMap[accInfo.serverId].insert(accInfo.hostGroupId);
	}
	exp = TestHostResourceQueryOption::callMakeCondition(
		srvHostGrpSetMap,
		serverIdColumnName,
		hostGroupIdColumnName,
		hostIdColumnName);
	return exp;
}

static string makeHostgroupsOutput(const HostgroupInfo &hostgroupInfo, size_t id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%"FMT_SERVER_ID"|%"FMT_HOST_GROUP_ID"|%s\n",
	  id + 1, hostgroupInfo.serverId,
	  hostgroupInfo.groupId, hostgroupInfo.groupName.c_str());

	return expectedOut;
}

static string makeMapHostsHostgroupsOutput
  (const HostgroupElement &hostgroupElement, size_t id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%"FMT_SERVER_ID"|%"FMT_HOST_ID"|%"FMT_HOST_GROUP_ID"\n",
	  id + 1, hostgroupElement.serverId,
	  hostgroupElement.hostId, hostgroupElement.groupId);

	return expectedOut;
}

static string makeHostsOutput(const HostInfo &hostInfo, size_t id)
{
	string expectedOut = StringUtils::sprintf(
	  "%zd|%"FMT_SERVER_ID"|%"FMT_HOST_ID"|%s\n",
	  id + 1, hostInfo.serverId, hostInfo.id, hostInfo.hostName.c_str());

	return expectedOut;
}

static void prepareTestDataForFilterForDataOfDefunctServersFalseOnly(void)
{
	// This is temporary method to avoid the failure of tests.
	// This function should be replaced with
	// prepareTestDataForFilterForDataOfDefunctServers()
	gcut_add_datum("Not filter data of defunct servers",
		       "filterDataOfDefunctServers", G_TYPE_BOOLEAN, FALSE,
		       NULL);
	MLPL_BUG("This function is a temporary workaround and should be replaced.\n");
}

void cut_setup(void)
{
	hatoholInit();
	deleteDBClientHatoholDB();
	setupTestDBConfig(true, true);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createDB(void)
{
	// remove the DB that already exists
	string dbPath = getDBPathForDBClientHatohol();

	// create an instance (the database will be automatically created)
	DBClientHatohol dbHatohol;
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient_version";
	string output = execSqlite3ForDBClientHatohol(statement);
	string expectedOut = StringUtils::sprintf
	                       ("%d|%d\n", DB_DOMAIN_ID_HATOHOL,
	                                   DBClientHatohol::HATOHOL_DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableTrigger(void)
{
	const string tableName = "triggers";
	string dbPath = getDBPathForDBClientHatohol();
	DBClientHatohol dbHatohol;
	string command = "sqlite3 " + dbPath + " \".table\"";
	assertExist(tableName, executeCommand(command));

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClientHatohol(statement);
	string expectedOut = ""; // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_addTriggerInfo(void)
{
	string dbPath = getDBPathForDBClientHatohol();

	// added a record
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from triggers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeTriggerOutput(*testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_getTriggerInfo(void)
{
	setupTestTriggerDB();
	setupTestHostgroupElementDB();
	setupTestHostgroupInfoDB();
	int targetIdx = 2;
	TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
	TriggerInfo triggerInfo;
	DBClientHatohol dbHatohol;
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetTriggerInfo.serverId);
	cppcut_assert_equal(true,
	   dbHatohol.getTriggerInfo(
	      triggerInfo, option, targetTriggerInfo.id));
	assertTriggerInfo(targetTriggerInfo, triggerInfo);
}

void test_getTriggerInfoNotFound(void)
{
	setupTestTriggerDB();
	const UserIdType invalidSvId = -1;
	const uint64_t   invalidTrigId = -1;
	TriggerInfo triggerInfo;
	DBClientHatohol dbHatohol;
	TriggersQueryOption option(invalidSvId);
	cppcut_assert_equal(false,
	   dbHatohol.getTriggerInfo(triggerInfo, option, invalidTrigId));
}

void data_getTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoList(gconstpointer data)
{
	assertGetTriggerInfoList(data, ALL_SERVERS);
}

void test_getTriggerInfoListForOneServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getTriggerInfoListForOneServer(gconstpointer data)
{
	uint32_t targetServerId = testTriggerInfo[0].serverId;
	assertGetTriggerInfoList(data, targetServerId);
}

void data_getTriggerInfoListForOneServerOneHost(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoListForOneServerOneHost(gconstpointer data)
{
	uint32_t targetServerId = testTriggerInfo[1].serverId;
	uint64_t targetHostId = testTriggerInfo[1].hostId;
	assertGetTriggerInfoList(data, targetServerId, targetHostId);
}

void test_setTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_setTriggerInfoList(gconstpointer data)
{
	DBClientHatohol dbHatohol;
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	uint32_t serverId = testTriggerInfo[0].serverId;
	dbHatohol.setTriggerInfoList(triggerInfoList, serverId);

	HostgroupElementList hostgroupElementList;
	for (size_t i = 0; i < NumTestHostgroupElement; i++)
		hostgroupElementList.push_back(testHostgroupElement[i]);
	dbHatohol.addHostgroupElementList(hostgroupElementList);

	HostgroupInfoList hostgroupInfoList;
	for (size_t i = 0; i < NumTestHostgroupInfo; i++)
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
	dbHatohol.addHostgroupInfoList(hostgroupInfoList);

	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_addTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_addTriggerInfoList(gconstpointer data)
{
	size_t i;
	DBClientHatohol dbHatohol;

	// First call
	size_t numFirstAdd = NumTestTriggerInfo / 2;
	TriggerInfoList triggerInfoList0;
	for (i = 0; i < numFirstAdd; i++)
		triggerInfoList0.push_back(testTriggerInfo[i]);
	dbHatohol.addTriggerInfoList(triggerInfoList0);

	// Second call
	TriggerInfoList triggerInfoList1;
	for (; i < NumTestTriggerInfo; i++)
		triggerInfoList1.push_back(testTriggerInfo[i]);
	dbHatohol.addTriggerInfoList(triggerInfoList1);

	// Add HostgroupElement
	HostgroupElementList hostgroupElementList;
	for (size_t j = 0; j < NumTestHostgroupElement; j++)
		hostgroupElementList.push_back(testHostgroupElement[j]);
	dbHatohol.addHostgroupElementList(hostgroupElementList);

	// Add HostgroupInfo
	HostgroupInfoList hostgroupInfoList;
	for (size_t j = 0; j < NumTestHostgroupInfo; j++)
		hostgroupInfoList.push_back(testHostgroupInfo[j]);
	dbHatohol.addHostgroupInfoList(hostgroupInfoList);

	// Check
	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_getTriggerWithOneAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getTriggerWithOneAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
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
	setupTestDBUser(true, true);
	AssertGetTriggersArg arg(data);
	arg.userId = 4;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getTriggerWithInvalidUserId(gconstpointer data)
{
	setupTestDBUser(true, true);
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
	uint32_t targetServerId = testItemInfo[0].serverId;
	assertItemInfoList(data, targetServerId);
}

void data_addItemInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addItemInfoList(gconstpointer data)
{
	DBClientHatohol dbHatohol;
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbHatohol.addItemInfoList(itemInfoList);

	AssertGetItemsArg arg(data);
	assertGetItems(arg);
}

void data_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
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
	setupTestDBUser(true, true);
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
	setupTestDBUser(true, true);
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetItemsWithFilter(arg);
}

void data_addEventInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addEventInfoList(gconstpointer data)
{
	// DBClientHatohol internally joins the trigger table and the event table.
	// So we also have to add trigger data.
	// When the internal join is removed, the following line will not be
	// needed.
	test_setTriggerInfoList(data);

	DBClientHatohol dbHatohol;
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbHatohol.addEventInfoList(eventInfoList);

	AssertGetEventsArg arg(data);
	assertGetEvents(arg);
}

void data_getLastEventId(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getLastEventId(gconstpointer data)
{
	test_addEventInfoList(data);
	DBClientHatohol dbHatohol;
	const int serverid = 3;
	cppcut_assert_equal(findLastEventId(serverid),
	                    dbHatohol.getLastEventId(serverid));
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
	arg.targetServerId = testTriggerInfo[0].serverId;
	assertGetHosts(arg);
}

void data_getHostInfoListWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getHostInfoListWithNoAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
	AssertGetHostsArg arg(data);
	arg.userId = 4;
	assertGetHosts(arg);
}

void data_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getHostInfoListWithOneAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
	AssertGetHostsArg arg(data);
	arg.userId = 5;
	assertGetHosts(arg);
}

void test_getNumberOfTriggersBySeverity(void)
{
	setupTestTriggerDB();

	uint32_t targetServerId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	uint64_t hostGroupId = 0;

	DBClientHatohol dbHatohol;
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		TriggersQueryOption option(USER_ID_SYSTEM);
		option.setTargetServerId(targetServerId);
		//TODO: uncomment it after Hatohol supports host group
		//option.setTargetHostGroupId(hostGroupId);
		TriggerSeverityType severity = (TriggerSeverityType)i;
		size_t actual = dbHatohol.getNumberOfTriggers(option, severity);
		size_t expected = getNumberOfTestTriggers
		                    (targetServerId, hostGroupId, severity);
		cppcut_assert_equal(expected, actual,
		                    cut_message("severity: %d", i));
	}
}

void test_getNumberOfTriggersBySeverityWithoutPriviledge(void)
{
	setupTestTriggerDB();

	uint32_t targetServerId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	//uint64_t hostGroupId = 0;

	DBClientHatohol dbHatohol;
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		TriggersQueryOption option;
		option.setTargetServerId(targetServerId);
		//TODO: uncomment it after Hatohol supports host group
		//option.setTargetHostGroupId(hostGroupId);
		TriggerSeverityType severity = (TriggerSeverityType)i;
		size_t actual = dbHatohol.getNumberOfTriggers(option, severity);
		size_t expected = 0;
		cppcut_assert_equal(expected, actual,
		                    cut_message("severity: %d", i));
	}
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
	setupTestDBUser(true, true);
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithNoAuthorizedServer(void)
{
	setupTestDBUser(true, true);
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_getNumberOfGoodHostsWithOneAuthorizedServer(void)
{
	setupTestDBUser(true, true);
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithOneAuthorizedServer(void)
{
	setupTestDBUser(true, true);
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_makeConditionEmpty(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	string expect = DBClientHatohol::getAlwaysFalseCondition();
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionAllServers(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[ALL_SERVERS].insert(ALL_HOST_GROUPS);
	string expect = "";
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionAllServersWithOthers(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(1);
	srvHostGrpSetMap[1].insert(2);
	srvHostGrpSetMap[3].insert(1);
	srvHostGrpSetMap[ALL_SERVERS].insert(ALL_HOST_GROUPS);
	string expect = "";
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionAllServersWithSpecifiedHostGroup(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[ALL_SERVERS].insert(1);
	string expect = "";
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionOneServerAllHostGrp(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(ALL_HOST_GROUPS);
	string expect =
	  StringUtils::sprintf("%s=1", serverIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionOneServerAndOneHostGroup(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(3);
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN (3))",
	  serverIdColumnName.c_str(),
	  hostGroupIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionOneServerAndHostGroups(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(3);
	srvHostGrpSetMap[1].insert(1003);
	srvHostGrpSetMap[1].insert(2048);
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN (3,1003,2048))",
	  serverIdColumnName.c_str(),
	  hostGroupIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionMultipleServers(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	string expect = StringUtils::sprintf("(%s=5 OR %s=14 OR %s=768)",
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionWithTargetServer(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	string expect = StringUtils::sprintf("%s=14",
	  serverIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect, 14);
}

void test_makeConditionWithUnauthorizedTargetServer(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	assertMakeCondition(srvHostGrpSetMap, "0", 7);
}

void test_makeConditionWithTargetServerAndHost(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	string expect = StringUtils::sprintf("((%s=14) AND %s=21)",
					     serverIdColumnName.c_str(),
					     hostIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect, 14, 21);
}

void data_conditionForAdminWithTargetServerAndHost(void)
{
}

void test_conditionForAdminWithTargetServerAndHost(gconstpointer data)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	if (filterForDataOfDefunctSv)
		cut_pend("To be implemented");
	HostResourceQueryOption option(USER_ID_SYSTEM);
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	option.setTargetServerId(26);
	option.setTargetHostId(32);
	string expect = StringUtils::sprintf("%s=26 AND %s=32",
					     serverIdColumnName.c_str(),
					     hostIdColumnName.c_str());
	cppcut_assert_equal(expect, option.getCondition());
}

void data_eventQueryOptionGetServerIdColumnName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionGetServerIdColumnName(gconstpointer data)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	HostResourceQueryOption option(USER_ID_SYSTEM);
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	const string tableAlias = "test_event_table_alias";
	option.setTargetServerId(26);
	option.setTargetHostgroupId(48);
	option.setTargetHostId(32);
	string expect = StringUtils::sprintf(
	                  "%s.%s=26 AND %s.%s=32 AND %s.%s=48",
			  tableAlias.c_str(),
			  serverIdColumnName.c_str(),
			  tableAlias.c_str(),
			  hostIdColumnName.c_str(),
			  tableAlias.c_str(),
			  hostGroupIdColumnName.c_str());
	if (filterForDataOfDefunctSv)
		insertValidServerCond(expect, option, tableAlias);
	cppcut_assert_equal(expect, option.getCondition(tableAlias));

}

void test_makeConditionComplicated(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(205);
	srvHostGrpSetMap[5].insert(800);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(817);
	srvHostGrpSetMap[768].insert(818);
	srvHostGrpSetMap[768].insert(12817);
	srvHostGrpSetMap[2000].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[2001].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[8192].insert(4096);
	string expect = StringUtils::sprintf(
	  "((%s=5 AND %s IN (205,800)) OR "
	  "%s=14 OR "
	  "(%s=768 AND %s IN (817,818,12817)) OR "
	  "%s=2000 OR "
	  "%s=2001 OR "
	  "(%s=8192 AND %s IN (4096)))",
	  serverIdColumnName.c_str(), hostGroupIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostGroupIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostGroupIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void data_makeSelectConditionUserAdmin(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_makeSelectConditionUserAdmin(gconstpointer data)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	HostResourceQueryOption option(USER_ID_SYSTEM);
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	string actual = option.getCondition();
	string expect = "";
	if (filterForDataOfDefunctSv)
		insertValidServerCond(expect, option);
	cppcut_assert_equal(actual, expect);
}

void data_makeSelectConditionAllEvents(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_makeSelectConditionAllEvents(gconstpointer data)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	HostResourceQueryOption option;
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	option.setFlags(OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER));
	string actual = option.getCondition();
	string expect = "";
	if (filterForDataOfDefunctSv)
		insertValidServerCond(expect, option);
	cppcut_assert_equal(actual, expect);
}

void test_makeSelectConditionNoneUser(void)
{
	setupTestDBUser(true, true);
	HostResourceQueryOption option;
	string actual = option.getCondition();
	string expect = DBClientHatohol::getAlwaysFalseCondition();
	cppcut_assert_equal(actual, expect);
}

void data_makeSelectCondition(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_makeSelectCondition(gconstpointer data)
{
	const bool filterForDataOfDefunctSv =
	  gcut_data_get_boolean(data, "filterDataOfDefunctServers");
	setupTestDBUser(true, true);
	HostResourceQueryOption option;
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		UserIdType userId = i + 1;
		option.setUserId(userId);
		string actual = option.getCondition();
		string expect = makeExpectedConditionForUser(
		                  userId, testUserInfo[i].flags);
		if (filterForDataOfDefunctSv)
			insertValidServerCond(expect, option);
		cppcut_assert_equal(expect, actual);
	}
}

void test_eventQueryOptionWithNoSortType(void)
{
	EventsQueryOption option;
	const string expected = "";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortTypeId(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
			   DataQueryOption::SORT_DESCENDING);
	const string expected = "unified_id DESC";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortDontCare(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
			   DataQueryOption::SORT_DONT_CARE);
	const string expected = "";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortTypeTime(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_TIME,
			   DataQueryOption::SORT_ASCENDING);
	const string expected =  "time_sec ASC, time_ns ASC, unified_id ASC";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void data_getEventSortAscending(void)
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
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
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
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
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
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
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
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getEventWithOneAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getEventWithNoAuthorizedServer(gconstpointer data)
{
	setupTestDBUser(true, true);
	AssertGetEventsArg arg(data);
	arg.userId = 4;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithInvalidUserId(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServersFalseOnly();
}

void test_getEventWithInvalidUserId(gconstpointer data)
{
	setupTestDBUser(true, true);
	AssertGetEventsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetEventsWithFilter(arg);
}

void test_addHostgroupInfo(void)
{
	DBClientHatohol dbClientHatohol;
	HostgroupInfoList hostgroupInfoList;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from hostgroups;";
	string expect;

	for (size_t i = 0; i < NumTestHostgroupInfo; i++) {
		hostgroupInfoList.push_back(testHostgroupInfo[i]);
		expect += makeHostgroupsOutput(testHostgroupInfo[i], i);
	}
	dbClientHatohol.addHostgroupInfoList(hostgroupInfoList);
	assertDBContent(dbAgent, statement, expect);
}

void test_addHostgroupElement(void)
{
	DBClientHatohol dbClientHatohol;
	HostgroupElementList hostgroupElementList;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from map_hosts_hostgroups";
	string expect;

	for (size_t i = 0; i < NumTestHostgroupElement; i++) {
		hostgroupElementList.push_back(testHostgroupElement[i]);
		expect += makeMapHostsHostgroupsOutput(
		            testHostgroupElement[i], i);
	}
	dbClientHatohol.addHostgroupElementList(hostgroupElementList);
	assertDBContent(dbAgent, statement, expect);
}

void test_addHostInfo(void)
{
	DBClientHatohol dbClientHatohol;
	HostInfoList hostInfoList;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from hosts;";
	string expect;

	for(size_t i = 0; i < NumTestHostInfo; i++) {
		hostInfoList.push_back(testHostInfo[i]);
		expect += makeHostsOutput(testHostInfo[i], i);
	}
	dbClientHatohol.addHostInfoList(hostInfoList);
	assertDBContent(dbAgent, statement, expect);
}

//
// Tests for HostResourceQueryOption
//
void test_copyConstructor(void)
{
	HostResourceQueryOption opt0;
	HostResourceQueryOption opt1(opt0);
	cppcut_assert_equal(&opt0.getDataQueryContext(),
	                    &opt1.getDataQueryContext());
}

void test_makeConditionServer(void)
{
	const string serverIdColumnName = "cat";
	const ServerIdType serverIds[] = {5, 15, 105, 1080};
	const size_t numServerIds = sizeof(serverIds) / sizeof(ServerIdType);

	ServerIdSet svIdSet;
	for (size_t i = 0; i < numServerIds; i++)
		svIdSet.insert(serverIds[i]);

	string actual = TestHostResourceQueryOption::callMakeConditionServer(
	                  svIdSet, serverIdColumnName);

	// check
	string expectHead = serverIdColumnName;
	expectHead += " IN (";
	string actualHead(actual, 0, expectHead.size());
	cppcut_assert_equal(expectHead, actualHead);

	string expectTail = ")";
	string actualTail(actual, actual.size()-1, expectTail.size());
	cppcut_assert_equal(expectTail, actualTail);

	StringVector actualIds;
	size_t len = actual.size() -  expectHead.size() - 1;
	string actualBody(actual, expectHead.size(), len);
	StringUtils::split(actualIds, actualBody, ',');
	cppcut_assert_equal(numServerIds, actualIds.size());
	ServerIdSetIterator expectId = svIdSet.begin(); 
	for (int i = 0; expectId != svIdSet.end(); ++expectId, i++) {
		string expect =
		  StringUtils::sprintf("%"FMT_SERVER_ID, *expectId);
		cppcut_assert_equal(expect, actualIds[i]);
	}
}

void test_makeConditionServerWithEmptyIdSet(void)
{
	ServerIdSet svIdSet;
	string actual = TestHostResourceQueryOption::callMakeConditionServer(
	                  svIdSet, "meet");
	cppcut_assert_equal(DBClientHatohol::getAlwaysFalseCondition(), actual);
}

void test_defaultValueOfFilterForDataOfDefunctServers(void)
{
	HostResourceQueryOption opt;
	cppcut_assert_equal(true, opt.getFilterForDataOfDefunctServers());
}

void data_setGetOfFilterForDataOfDefunctServers(void)
{
	gcut_add_datum("Disable filtering",
	               "enable", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Eanble filtering",
	               "enable", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_setGetOfFilterForDataOfDefunctServers(gconstpointer data)
{
	HostResourceQueryOption opt;
	bool enable = gcut_data_get_boolean(data, "enable");
	opt.setFilterForDataOfDefunctServers(enable);
	cppcut_assert_equal(enable, opt.getFilterForDataOfDefunctServers());
}

} // namespace testDBClientHatohol
