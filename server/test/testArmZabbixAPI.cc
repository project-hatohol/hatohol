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
#include <gcutter.h>
#include <sys/time.h>
#include <errno.h>
#include <fstream>
#include "Hatohol.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "Helpers.h"
#include "Synchronizer.h"
#include "ConfigManager.h"
#include "ItemTablePtr.h"
#include "ItemTable.h"
#include "ItemGroup.h"
#include "ItemData.h"
#include "JSONParser.h"
#include "DBTablesAction.h"
#include "ThreadLocalDBCache.h"
#include "DBTablesTest.h"
#include "UnifiedDataStore.h"
using namespace std;

namespace testArmZabbixAPI {
static Synchronizer g_sync;

static const size_t NUM_TEST_READ_TIMES = 10;

static MonitoringServerInfo g_defaultServerInfo =
{
	0,                        // id
	MONITORING_SYSTEM_ZABBIX, // type
	"127.0.0.1",              // hostname
	"127.0.0.1",              // ip_address
	"No name",                // nickname
	0,                        // port
	10,                       // polling_interval_sec
	5,                        // retry_interval_sec
	"foofoo",                 // userName
	"barbar",                 // password
};


class ArmZabbixAPITestee :  public ArmZabbixAPI {

typedef bool (ArmZabbixAPITestee::*ThreadOneProc)(void);

public:
	enum GetTestType {
		GET_TEST_TYPE_TRIGGERS,
		GET_TEST_TYPE_TRIGGER_EXPANDED_DESCRIPTION,
		GET_TEST_TYPE_ITEMS,
		GET_TEST_TYPE_HOSTS,
		GET_TEST_TYPE_APPLICATIONS,
		GET_TEST_TYPE_EVENTS,
	};

	VariableItemTablePtr m_actualEventTablePtr;
	list<uint64_t>       m_numbersOfGotEvents;

	ArmZabbixAPITestee(const MonitoringServerInfo &serverInfo)
	: ArmZabbixAPI(serverInfo),
	  m_serverId(serverInfo.id),
	  m_result(false),
	  m_threadOneProc(&ArmZabbixAPITestee::defaultThreadOneProc),
	  m_countThreadOneProc(0),
	  m_repeatThreadOneProc(1)
	{
	}

	bool getResult(void) const
	{
		return m_result;
	}

	const string errorMessage(void) const
	{
		return m_errorMessage;
	}

	void testGetAPIVersion(ZabbixAPIEmulator::APIVersion expected =
			       ZabbixAPIEmulator::API_VERSION_2_0_4)
	{
		string version
		  = ZabbixAPIEmulator::getAPIVersionString(expected);
		cppcut_assert_equal(version, getAPIVersion());
	}

	bool testOpenSession(void)
	{
		g_sync.lock();
		setPollingInterval(0);
		m_threadOneProc = &ArmZabbixAPITestee::threadOneProcOpenSession;
		addExitCallback(exitCbDefault, this);
		start();
		g_sync.wait();
		return m_result;
	}

	bool testGet(GetTestType type,
		     ZabbixAPIEmulator::APIVersion expectedVersion =
		       ZabbixAPIEmulator::API_VERSION_2_0_4)
	{
		bool succeeded = false;
		if (type == GET_TEST_TYPE_TRIGGERS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcTriggers,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_TRIGGER_EXPANDED_DESCRIPTION) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcTriggerExpandedDescriptions,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_ITEMS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcItems,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_HOSTS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcHosts,
			         exitCbDefault, this, 1);
		} else if (type == GET_TEST_TYPE_APPLICATIONS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcApplications,
			         exitCbDefault, this, 1);
		} else if (type == GET_TEST_TYPE_EVENTS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcEvents,
			         exitCbDefault, this);
		} else {
			cut_fail("Unknown get test type: %d\n", type);
		}
		return succeeded;
	}

	bool testMainThreadOneProc(void)
	{
		// This test is typically executed without the start of
		// the thread of ArmZabbixAPI. The exit callback (that unlock
		// mutex in ArmZabbixAPI) is never called. So we explicitly
		// call exitCallbackFunc() here to unlock the mutex;
		const ArmPollingResult oneProcEndType = ArmZabbixAPI::mainThreadOneProc();
		return (oneProcEndType == COLLECT_OK);
	}

	bool testMainThreadOneProcFetchItems(void)
	{
		const ArmPollingResult oneProcEndType
		  = ArmZabbixAPI::mainThreadOneProcFetchItems();
		return (oneProcEndType == COLLECT_OK);
	}

	bool testMainThreadOneProcFetchHistory(
	  HistoryInfoVect &historyInfoVect)
	{
		ItemInfo itemInfo;
		itemInfo.serverId = 1;
		itemInfo.id = "25490";
		itemInfo.globalHostId = 0;
		itemInfo.valueType = ITEM_INFO_VALUE_TYPE_FLOAT;
		const ArmPollingResult oneProcEndType
		  = ArmZabbixAPI::mainThreadOneProcFetchHistory(
		      historyInfoVect, itemInfo, 1413265550, 1413268970);
		return (oneProcEndType == COLLECT_OK);
	}

	bool testGetCopyOnDemandEnabled(void)
	{
		return ArmZabbixAPI::getCopyOnDemandEnabled();
	}

	void testSetCopyOnDemandEnabled(bool enable)
	{
		ArmZabbixAPI::setCopyOnDemandEnabled(enable);
	}

	string testInitialJSONRequest(void)
	{
		return ArmZabbixAPI::getInitialJSONRequest();
	}

	string testAuthToken(void)
	{
		return ArmZabbixAPI::getAuthToken();
	}

	uint64_t testGetMaximumNumberGetEventPerOnce(void)
	{
		return ArmZabbixAPI::getMaximumNumberGetEventPerOnce();
	}

	void callUpdateEvents(void)
	{
		ArmZabbixAPI::updateEvents();
	}

	void callUpdateGroupInformation(void)
	{
		ArmZabbixAPI::updateHosts();
		ArmZabbixAPI::updateGroups();
	}

	void onGotNewEvents(const ItemTablePtr &itemPtr)
	{
		const ItemGroupList &itemList = itemPtr->getItemGroupList();
		ItemGroupListConstIterator itr = itemList.begin();
		m_numbersOfGotEvents.push_back(itemList.size());
		for (; itr != itemList.end(); ++itr) {
			const ItemGroup *itemGroup = *itr;
			m_actualEventTablePtr->add(itemGroup);
		}
	}

	ItemTablePtr makeExpectedEventsItemTable(const bool &lastOnly = false)
	{
		string fixturePath = getFixturesDir();
		fixturePath += "zabbix-api-res-events-002.json";
		ifstream ifs(fixturePath.c_str());
		cppcut_assert_equal(false, ifs.fail());

		string fixtureData;
		getline(ifs, fixtureData);
		JSONParser parser(fixtureData);
		cppcut_assert_equal(false, parser.hasError());
		startObject(parser, "result");

		VariableItemTablePtr tablePtr;
		int numData = parser.countElements();
		if (numData < 1)
			cut_fail("Value of the elements is empty.");
		int i = 0;
		if (lastOnly)
			i = numData - 1;
		for (; i < numData; i++)
			ArmZabbixAPI::parseAndPushEventsData(parser, tablePtr, i);
		return ItemTablePtr(tablePtr);
	}

	void loadHostInfoCacheForEmulator(void)
	{
		ZabbixAPIEmulator::loadHostInfoCache(getHostInfoCache(),
		                                     m_serverId);
	}

protected:
	static void exitCbDefault(void *)
	{
		g_sync.unlock();
	}

	bool launch(ThreadOneProc threadOneProc,
	            void (*exitCb)(void *), void *exitCbData,
	            size_t numRepeat = NUM_TEST_READ_TIMES)
	{
		m_countThreadOneProc = 0;
		m_repeatThreadOneProc = numRepeat;
		g_sync.lock();
		setPollingInterval(0);
		m_threadOneProc = threadOneProc;
		addExitCallback(exitCb, exitCbData);
		start();
		g_sync.wait();
		return m_result;
	}

	virtual int onCaughtException(const exception &e) override
	{
		m_result = false;
		m_errorMessage = e.what();
		g_sync.unlock();
		return EXIT_THREAD;
	}

	bool defaultThreadOneProc(void)
	{
		THROW_HATOHOL_EXCEPTION("m_threadOneProc is not set.");
		return false;
	}

	bool threadOneProcGetAPIVersion(void)
	{
		getAPIVersion();
		return true;
	}

	bool threadOneProcOpenSession(void)
	{
		bool succeeded = openSession();
		if (!succeeded)
			m_errorMessage = "Failed: mainThreadOneProc()";
		m_result = succeeded;
		requestExit();
		return succeeded;
	}

	bool threadOneProcTriggers(void)
	{
		updateTriggers();
		return true;
	}

	bool threadOneProcTriggerExpandedDescriptions(void)
	{
		updateTriggerExpandedDescriptions();
		return true;
	}

	bool threadOneProcItems(void)
	{
		updateItems();
		return true;
	}

	bool threadOneProcHosts(void)
	{
		updateHosts();
		return true;
	}

	bool threadOneProcApplications(void)
	{
		updateApplications();
		return true;
	}

	bool threadOneProcEvents(void)
	{
		updateEvents();
		return true;
	}

	// virtual function
	ArmPollingResult mainThreadOneProc(void)
	{
		if (!openSession()) {
			requestExit();
			return COLLECT_NG_DISCONNECT_ZABBIX;
		}
		if (!(this->*m_threadOneProc)()) {
			requestExit();
			return COLLECT_NG_INTERNAL_ERROR;
		}
		m_countThreadOneProc++;
		if (m_countThreadOneProc++ >= m_repeatThreadOneProc) {
			m_result = true;
			requestExit();
		}
		return COLLECT_OK;
	}

private:
	const ServerIdType m_serverId;
	bool m_result;
	string m_errorMessage;
	ThreadOneProc m_threadOneProc;
	size_t        m_countThreadOneProc;
	size_t        m_repeatThreadOneProc;
};

static const guint EMULATOR_PORT = 33333;

ZabbixAPIEmulator g_apiEmulator;

static guint getTestPort(void)
{
	static guint port = 0;
	if (port != 0)
		return port;

	char *env = getenv("TEST_ARM_ZABBIX_API_PORT");
	if (!env) {
		port = EMULATOR_PORT;
		return port;
	}

	guint _port = atoi(env);
	if (_port > 65535)
		cut_fail("Invalid string: %s", env);
	port = _port;
	return port;
}

static void _assertReceiveData(ArmZabbixAPITestee::GetTestType testType,
                               int svId,
			       ZabbixAPIEmulator::APIVersion expectedVersion =
			         ZabbixAPIEmulator::API_VERSION_2_0_4)
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();

	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	armZbxApiTestee.loadHostInfoCacheForEmulator();
	g_apiEmulator.setAPIVersion(expectedVersion);
	cppcut_assert_equal
	  (true, armZbxApiTestee.testGet(testType, expectedVersion),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}
#define assertReceiveData(TYPE, SERVER_ID, ...)	\
cut_trace(_assertReceiveData(TYPE, SERVER_ID, ##__VA_ARGS__))

static void _assertTestGet(ArmZabbixAPITestee::GetTestType testType,
			   ZabbixAPIEmulator::APIVersion expectedVersion =
			     ZabbixAPIEmulator::API_VERSION_2_0_4)
{
	int svId = 0;
	assertReceiveData(testType, svId, expectedVersion);

	// TODO: add the check of the database
}
#define assertTestGet(TYPE, ...) cut_trace(_assertTestGet(TYPE, ##__VA_ARGS__))

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);

	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
	loadTestDBAction();

}

void cut_teardown(void)
{
	g_sync.reset();
	g_apiEmulator.reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getTriggers(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS);
}

void test_getTriggerWithExpandedDescriptions(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGER_EXPANDED_DESCRIPTION);
}

void test_getTriggers_2_2_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getTriggers_2_2_2(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS,
		      ZabbixAPIEmulator::API_VERSION_2_2_2);
}

void test_getTriggers_2_3_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS,
		      ZabbixAPIEmulator::API_VERSION_2_3_0);
}

void test_getItems(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_ITEMS);
}

void test_getItems_2_2_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_ITEMS,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getItems_2_3_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_ITEMS,
		      ZabbixAPIEmulator::API_VERSION_2_3_0);
}

void test_getHosts(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_HOSTS);
}

void test_getEvents(void)
{
	assertReceiveData(ArmZabbixAPITestee::GET_TEST_TYPE_EVENTS, 0);
}

void test_getEvents_2_2_0(void)
{
	assertReceiveData(ArmZabbixAPITestee::GET_TEST_TYPE_EVENTS, 0,
			  ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getApplications(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_APPLICATIONS);
}

void test_getApplications_2_2_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_APPLICATIONS,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_httpNotFound(void)
{
	if (getTestPort() != EMULATOR_PORT)
		cut_pend("Test port is not emulator port.");

	g_apiEmulator.setOperationMode(OPE_MODE_HTTP_NOT_FOUND);
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal
	  (false, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

void test_mainThreadOneProc()
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	armZbxApiTestee.loadHostInfoCacheForEmulator();
	cppcut_assert_equal(true, armZbxApiTestee.testMainThreadOneProc());
}

void test_setCopyOnDemandEnabled()
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal(false,
			    armZbxApiTestee.testGetCopyOnDemandEnabled());
	armZbxApiTestee.testSetCopyOnDemandEnabled(true);
	cppcut_assert_equal(true,
			    armZbxApiTestee.testGetCopyOnDemandEnabled());
}

MonitoringServerInfo setupServer(void)
{
	int svId = 1; // We should use one of testServerInfo
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	return serverInfo;
}

void test_oneProcWithoutFetchItems()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.loadHostInfoCacheForEmulator();
	armZbxApiTestee.testMainThreadOneProc();

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupVect hostgrps;
	HostgroupMemberVect hostgrpMembers;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupMembersQueryOption hostgroupMembersQueryOption(USER_ID_SYSTEM);

	dbMonitoring.getEventInfoList(eventInfoList, eventsQueryOption);
	dbMonitoring.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	dbMonitoring.getItemInfoList(itemInfoList, itemsQueryOption);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK, uds->getHostgroups(hostgrps, hostgroupsQueryOption));
	assertHatoholError(
	  HTERR_OK,
	  uds->getHostgroupMembers(hostgrpMembers, hostgroupMembersQueryOption));

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgrps.empty());
	cppcut_assert_equal(false, hostgrpMembers.empty());

	cppcut_assert_equal(false, eventInfoList.empty());
	cppcut_assert_equal(false, triggerInfoList.empty());
	cppcut_assert_equal(false, itemInfoList.empty());
}

void test_oneProcWithCopyOnDemandEnabled()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.loadHostInfoCacheForEmulator();
	armZbxApiTestee.testSetCopyOnDemandEnabled(true);
	armZbxApiTestee.testMainThreadOneProc();

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupVect hostgrps;
	HostgroupMemberVect hostgrpMembers;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupMembersQueryOption hostgroupMembersQueryOption(USER_ID_SYSTEM);

	dbMonitoring.getEventInfoList(eventInfoList, eventsQueryOption);
	dbMonitoring.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	dbMonitoring.getItemInfoList(itemInfoList, itemsQueryOption);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK, uds->getHostgroups(hostgrps, hostgroupsQueryOption));
	assertHatoholError(
	  HTERR_OK,
	  uds->getHostgroupMembers(hostgrpMembers, hostgroupMembersQueryOption));

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgrps.empty());
	cppcut_assert_equal(false, hostgrpMembers.empty());

	cppcut_assert_equal(false, eventInfoList.empty());
	cppcut_assert_equal(false, triggerInfoList.empty());
	cppcut_assert_equal(true, itemInfoList.empty());
}

void test_oneProcWithFetchItems()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.loadHostInfoCacheForEmulator();
	armZbxApiTestee.testMainThreadOneProcFetchItems();

	// DBClientHatoholl::getItemInfoList() function
	// needs information about hostgroup.
	armZbxApiTestee.callUpdateGroupInformation();

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupVect hostgrps;
	HostgroupMemberVect hostgrpMembers;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupMembersQueryOption hostgroupMembersQueryOption(USER_ID_SYSTEM);

	dbMonitoring.getEventInfoList(eventInfoList, eventsQueryOption);
	dbMonitoring.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	dbMonitoring.getItemInfoList(itemInfoList, itemsQueryOption);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK, uds->getHostgroups(hostgrps, hostgroupsQueryOption));
	assertHatoholError(
	  HTERR_OK,
	  uds->getHostgroupMembers(hostgrpMembers, hostgroupMembersQueryOption));

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgrps.empty());
	cppcut_assert_equal(false, hostgrpMembers.empty());

	cppcut_assert_equal(true, eventInfoList.empty());
	cppcut_assert_equal(true, triggerInfoList.empty());
	cppcut_assert_equal(false, itemInfoList.empty());
}

void test_oneProcFetchHistory()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	HistoryInfoVect historyInfoVect;
	armZbxApiTestee.testMainThreadOneProcFetchHistory(historyInfoVect);

	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupVect hostgrps;
	HostgroupMemberVect hostgrpMembers;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupMembersQueryOption hostgroupMembersQueryOption(USER_ID_SYSTEM);

	dbMonitoring.getEventInfoList(eventInfoList, eventsQueryOption);
	dbMonitoring.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	dbMonitoring.getItemInfoList(itemInfoList, itemsQueryOption);

	UnifiedDataStore *uds = UnifiedDataStore::getInstance();
	assertHatoholError(
	  HTERR_OK, uds->getHostgroups(hostgrps, hostgroupsQueryOption));
	assertHatoholError(
	  HTERR_OK,
	  uds->getHostgroupMembers(hostgrpMembers, hostgroupMembersQueryOption));

	cppcut_assert_equal(true, hostgrps.empty());
	cppcut_assert_equal(true, hostgrpMembers.empty());
	cppcut_assert_equal(true, eventInfoList.empty());
	cppcut_assert_equal(true, triggerInfoList.empty());
	cppcut_assert_equal(true, itemInfoList.empty());

	// TODO: Should check the contents
	cppcut_assert_equal(false, historyInfoVect.empty());
}

void test_checkUsernamePassword(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	JSONParser parser(armZbxApiTestee.testInitialJSONRequest());
	string jsonUserName;
	string jsonPassword;

	cppcut_assert_equal(false, parser.hasError());
	cppcut_assert_equal(true, parser.startObject("params"));
	cppcut_assert_equal(true, parser.read("user", jsonUserName));
	cppcut_assert_equal(true, parser.read("password", jsonPassword));
	cppcut_assert_equal(g_defaultServerInfo.userName, jsonUserName);
	cppcut_assert_equal(g_defaultServerInfo.password, jsonPassword);
}

void test_httpErrorAuthToken(void)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);

	string token;
	token = armZbxApiTestee.testAuthToken();
	cppcut_assert_equal(false, token.empty());
	g_apiEmulator.setOperationMode(OPE_MODE_HTTP_NOT_FOUND);
	armZbxApiTestee.testMainThreadOneProc();
	token = armZbxApiTestee.testAuthToken();
	cppcut_assert_equal(true, token.empty());
}

void test_sessionErrorAuthToken(void)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);

	string token;
	token = armZbxApiTestee.testAuthToken();
	cppcut_assert_equal(false, token.empty());
	g_apiEmulator.stop();
	cppcut_assert_equal(false, g_apiEmulator.isRunning());

	armZbxApiTestee.testMainThreadOneProc();
	token = armZbxApiTestee.testAuthToken();
	cppcut_assert_equal(true, token.empty());

}

static void addDummyFirstEvent(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfo eventInfo = testEventInfo[0];
	eventInfo.serverId = 1;
	eventInfo.id = "0";
	eventInfo.time.tv_sec = 0;
	eventInfo.time.tv_nsec = 0;
	dbMonitoring.addEventInfo(&eventInfo);
}

void test_getEventsBySplitWay(void)
{
	addDummyFirstEvent();

	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();

	ItemTablePtr expectTable
	  = armZbxApiTestee.makeExpectedEventsItemTable();
	armZbxApiTestee.callUpdateEvents();
	assertItemTable(expectTable,
	                ItemTablePtr(armZbxApiTestee.m_actualEventTablePtr));
	const uint64_t upperLimitOfEventsAtOneTime =
	  armZbxApiTestee.testGetMaximumNumberGetEventPerOnce();
	list<uint64_t>::iterator itr =
	   armZbxApiTestee.m_numbersOfGotEvents.begin();
	for (; itr != armZbxApiTestee.m_numbersOfGotEvents.end(); ++itr)
		cppcut_assert_equal(true, *itr <= upperLimitOfEventsAtOneTime);
}

void test_getInitialEvent(void)
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();

	const bool lastOnly = true;
	ItemTablePtr expectTable
	  = armZbxApiTestee.makeExpectedEventsItemTable(lastOnly);
	armZbxApiTestee.callUpdateEvents();
	assertItemTable(expectTable,
	                ItemTablePtr(armZbxApiTestee.m_actualEventTablePtr));
}

void test_getOldEvents(void)
{
	CommandArgHelper commandArgs;
	commandArgs << "--test-mode";
	commandArgs << "--load-old-events";
	commandArgs.activate();

	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();

	ItemTablePtr expectTable
	  = armZbxApiTestee.makeExpectedEventsItemTable();
	armZbxApiTestee.callUpdateEvents();
	assertItemTable(expectTable,
	                ItemTablePtr(armZbxApiTestee.m_actualEventTablePtr));
}

// for issue #447 (can't fetch one new event)
void test_oneNewEvent(void)
{
	MonitoringServerInfo serverInfo = setupServer();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	armZbxApiTestee.testOpenSession();

	uint64_t expectedEventId = 8000;
	g_apiEmulator.setExpectedLastEventId(expectedEventId);
	armZbxApiTestee.callUpdateEvents();
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	uint64_t actualEventId;
	Utils::conv(actualEventId, dbMonitoring.getMaxEventId(serverInfo.id));
	cppcut_assert_equal(expectedEventId, actualEventId);

	++expectedEventId;
	g_apiEmulator.setExpectedLastEventId(expectedEventId);
	armZbxApiTestee.callUpdateEvents();
	Utils::conv(actualEventId, dbMonitoring.getMaxEventId(serverInfo.id));
	cppcut_assert_equal(expectedEventId, actualEventId);
}

// for issue #252
// (can't get events when the first event ID is bigger than 1000)
void test_getStubbedEventList(void)
{
	addDummyFirstEvent();

	MonitoringServerInfo serverInfo = setupServer();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	armZbxApiTestee.testOpenSession();

	uint64_t expectedFirstEventId = 2014;
	uint64_t expectedLastEventId = 5000;
	g_apiEmulator.setExpectedFirstEventId(expectedFirstEventId);
	g_apiEmulator.setExpectedLastEventId(expectedLastEventId);
	armZbxApiTestee.callUpdateEvents();
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	EventInfoList eventList;
	EventsQueryOption option(USER_ID_SYSTEM);
	dbMonitoring.getEventInfoList(eventList, option);
	size_t actualSize = eventList.size() - 1; // subtract a dummy event
	cppcut_assert_equal(expectedLastEventId - expectedFirstEventId + 1,
			    static_cast<uint64_t>(actualSize));
}

} // namespace testArmZabbixAPI
