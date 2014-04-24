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
#include <gcutter.h>
#include <sys/time.h>
#include <errno.h>
#include <fstream>
#include "Hatohol.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "ArmZabbixAPI-template.h"
#include "Helpers.h"
#include "Synchronizer.h"
#include "DBClientZabbix.h"
#include "ConfigManager.h"
#include "ItemTablePtr.h"
#include "ItemTable.h"
#include "ItemGroup.h"
#include "ItemData.h"
#include "JsonParserAgent.h"
#include "DBClientAction.h"
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
		GET_TEST_TYPE_API_VERSION,
		GET_TEST_TYPE_TRIGGERS,
		GET_TEST_TYPE_FUNCTIONS,
		GET_TEST_TYPE_ITEMS,
		GET_TEST_TYPE_HOSTS,
		GET_TEST_TYPE_APPLICATIONS,
		GET_TEST_TYPE_EVENTS,
	};

	VariableItemTablePtr m_actualEventTablePtr;
	list<uint64_t>       m_numbersOfGotEvents;

	ArmZabbixAPITestee(const MonitoringServerInfo &serverInfo)
	: ArmZabbixAPI(serverInfo),
	  m_result(false),
	  m_threadOneProc(&ArmZabbixAPITestee::defaultThreadOneProc),
	  m_countThreadOneProc(0),
	  m_repeatThreadOneProc(1)
	{
		addExceptionCallback(_exceptionCb, this);
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

	void testCheckAPIVersion(bool expected,
				 int major, int minor, int micro)
	{
		const string &version = getAPIVersion();
		cppcut_assert_equal(string("2.0.4"), version);
		cppcut_assert_equal(expected,
				    checkAPIVersion(major, minor, micro));
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
		if (type == GET_TEST_TYPE_API_VERSION) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcTriggers,
			         exitCbDefault, this);
			testGetAPIVersion(expectedVersion);
		} else if (type == GET_TEST_TYPE_TRIGGERS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcTriggers,
			         exitCbDefault, this);
		} else if (type == GET_TEST_TYPE_FUNCTIONS) {
			succeeded =
			  launch(&ArmZabbixAPITestee::threadOneProcFunctions,
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
		return ArmZabbixAPI::mainThreadOneProc();
	}

	UpdateType testGetUpdateType(void)
	{
		return ArmZabbixAPI::getUpdateType();
	}

	void testSetUpdateType(UpdateType type)
	{
		ArmZabbixAPI::setUpdateType(type);
	}

	bool testGetCopyOnDemandEnabled(void)
	{
		return ArmZabbixAPI::getCopyOnDemandEnabled();
	}

	void testSetCopyOnDemandEnabled(bool enable)
	{
		ArmZabbixAPI::setCopyOnDemandEnabled(enable);
	}

	string testInitialJsonRequest(void)
	{
		return ArmZabbixAPI::getInitialJsonRequest();
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

	ItemTablePtr testMakeEventsItemTable(void)
	{
		ifstream ifs("fixtures/zabbix-api-res-events-002.json");
		cppcut_assert_equal(false, ifs.fail());

		string fixtureData;
		getline(ifs, fixtureData);
		JsonParserAgent parser(fixtureData);
		cppcut_assert_equal(false, parser.hasError());
		startObject(parser, "result");

		VariableItemTablePtr tablePtr;
		int numData = parser.countElements();
		if (numData < 1)
			cut_fail("Value of the elements is empty.");
		for (int i = 0; i < numData; i++)
			ArmZabbixAPI::parseAndPushEventsData(parser, tablePtr, i);
		return ItemTablePtr(tablePtr);
	}

	void testMakeGroupsItemTable(ItemTablePtr &groupsTablePtr)
	{
		ifstream ifs("fixtures/zabbix-api-res-hostgroup-002-refer.json");
		cppcut_assert_equal(false, ifs.fail());

		string fixtureData;
		getline(ifs, fixtureData);
		JsonParserAgent parser(fixtureData);
		cppcut_assert_equal(false, parser.hasError());
		startObject(parser, "result");

		VariableItemTablePtr variableGroupsTablePtr;
		int numData = parser.countElements();
		if (numData < 1)
			cut_fail("Value of the elements is empty.");
		for (int i = 0; i < numData; i++) {
			ArmZabbixAPI::parseAndPushGroupsData(parser,
			                                     variableGroupsTablePtr,
			                                     i);
		}

		groupsTablePtr = ItemTablePtr(variableGroupsTablePtr);
	}

	void testMakeMapHostsHostgroupsItemTable(ItemTablePtr &hostsGroupsTablePtr)
	{
		ifstream ifs("fixtures/zabbix-api-res-hosts-002.json");
		cppcut_assert_equal(false, ifs.fail());

		string fixtureData;
		getline(ifs, fixtureData);
		JsonParserAgent parser(fixtureData);
		cppcut_assert_equal(false, parser.hasError());
		startObject(parser, "result");

		VariableItemTablePtr variableHostsGroupsTablePtr;
		int numData = parser.countElements();
		if (numData < 1)
			cut_fail("Value of the elements is empty.");
		for (int i = 0; i < numData; i++) {
			ArmZabbixAPI::parseAndPushHostsGroupsData(parser,
			                                          variableHostsGroupsTablePtr,
			                                          i);
		}

		hostsGroupsTablePtr = ItemTablePtr(variableHostsGroupsTablePtr);
	}

	bool assertItemTable(const ItemTablePtr &expect, const ItemTablePtr &actual)
	{
		const ItemGroupList &expectList = expect->getItemGroupList();
		const ItemGroupList &actualList = actual->getItemGroupList();
		cppcut_assert_equal(expectList.size(), actualList.size());
		ItemGroupListConstIterator exItr = expectList.begin();
		ItemGroupListConstIterator acItr = actualList.begin();

		size_t grpCnt = 0;
		for (; exItr != expectList.end(); ++exItr, ++acItr, grpCnt++) {
			const ItemGroup *expectGroup = *exItr;
			const ItemGroup *actualGroup = *acItr;
			size_t numberOfItems = expectGroup->getNumberOfItems();
			cppcut_assert_equal(numberOfItems, actualGroup->getNumberOfItems());

			for (size_t index = 0; index < numberOfItems; index++){
				const ItemData *expectData = expectGroup->getItemAt(index);
				const ItemData *actualData = actualGroup->getItemAt(index);
				cppcut_assert_equal(*expectData, *actualData,
				  cut_message("index: %zd, group count: %zd",
				              index, grpCnt));
			}
		}
		return true;
	}

	void assertMakeItemVector(bool testNull = false)
	{
		// make test data and call the target method.
		const ItemId pickupItemId = 1;
		const size_t numTestData = 10;
		vector<int> itemVector;
		vector<int> expectedItemVector;
		VariableItemTablePtr table;
		for (size_t i = 0; i < numTestData; i++) {
			VariableItemGroupPtr grp;
			ItemInt *item = new ItemInt(pickupItemId, i);
			if (testNull && i % 2 == 0)
				item->setNull();
			else
				expectedItemVector.push_back(i);
			grp->add(item, false);
			table->add(grp);
		}
		makeItemVector<int>(itemVector, table, pickupItemId);
		
		// check
		cppcut_assert_equal(expectedItemVector.size(),
		                    itemVector.size());
		for (size_t i = 0; i < expectedItemVector.size(); i++) {
			int expected = expectedItemVector[i];
			int actual = itemVector[i];
			cppcut_assert_equal(expected, actual);
		}
	}

protected:
	static void _exceptionCb(const exception &e, void *data)
	{
		ArmZabbixAPITestee *obj =
		   static_cast<ArmZabbixAPITestee *>(data);
		obj->exceptionCb(e);
	}

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

	void exceptionCb(const exception &e)
	{
		m_result = false;
		m_errorMessage = e.what();
		g_sync.unlock();
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

	bool threadOneProcFunctions(void)
	{
		// updateTriggers() is neccessary before updateFunctions(),
		// because 'function' information is obtained at the same time
		// as a response of 'triggers.get' request.
		updateTriggers();
		updateFunctions();
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
	bool mainThreadOneProc(void)
	{
		if (!openSession()) {
			requestExit();
			return false;
		}
		if (!(this->*m_threadOneProc)()) {
			requestExit();
			return false;
		}
		m_countThreadOneProc++;
		if (m_countThreadOneProc++ >= m_repeatThreadOneProc) {
			m_result = true;
			requestExit();
		}
		return true;
	}

private:
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

	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
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

static void _assertCheckAPIVersion(
  bool expected, int major, int minor , int micro)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	armZbxApiTestee.testCheckAPIVersion(expected, major, minor, micro);
}
#define assertCheckAPIVersion(EXPECTED,MAJOR,MINOR,MICRO) \
  cut_trace(_assertCheckAPIVersion(EXPECTED,MAJOR,MINOR,MICRO))

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);

	deleteDBClientHatoholDB();
	setupTestDBConfig(true, true);
	setupTestDBAction();
}

void cut_teardown(void)
{
	g_sync.reset();
	g_apiEmulator.reset();
	deleteDBClientHatoholDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getAPIVersion(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_API_VERSION);
}

void test_getAPIVersion_2_2_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_API_VERSION,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void data_getAPIVersion(void)
{
	gcut_add_datum("Equal",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, "2",
		       "minor", G_TYPE_INT, "0",
		       "micro", G_TYPE_INT, "4",
		       NULL);
	gcut_add_datum("Lower micro version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, "2",
		       "minor", G_TYPE_INT, "0",
		       "micro", G_TYPE_INT, "3",
		       NULL);
	gcut_add_datum("Higher micro version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, "2",
		       "minor", G_TYPE_INT, "0",
		       "micro", G_TYPE_INT, "5",
		       NULL);
	gcut_add_datum("Higher minor version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, "2",
		       "minor", G_TYPE_INT, "1",
		       "micro", G_TYPE_INT, "4",
		       NULL);
	gcut_add_datum("Lower major version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, "1",
		       "minor", G_TYPE_INT, "0",
		       "micro", G_TYPE_INT, "4",
		       NULL);
	gcut_add_datum("Higher major version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, "3",
		       "minor", G_TYPE_INT, "0",
		       "micro", G_TYPE_INT, "4",
		       NULL);
}

void test_checkAPIVersion(gconstpointer data)
{
	assertCheckAPIVersion(gcut_data_get_boolean(data, "expected"),
			      gcut_data_get_int(data, "major"),
			      gcut_data_get_int(data, "minor"),
			      gcut_data_get_int(data, "micro"));
}

void test_openSession(void)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();

	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal
	  (true, armZbxApiTestee.testOpenSession(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

void test_getTriggers(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS);
}

void test_getTriggers_2_3_0(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_TRIGGERS,
		      ZabbixAPIEmulator::API_VERSION_2_3_0);
}

void test_getFunctions(void)
{
	assertTestGet(ArmZabbixAPITestee::GET_TEST_TYPE_FUNCTIONS);
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
	// We expect empty data for the last two times.
	g_apiEmulator.setNumberOfEventSlices(NUM_TEST_READ_TIMES-2);
	assertReceiveData(ArmZabbixAPITestee::GET_TEST_TYPE_EVENTS, 0);
}

void test_getEvents_2_2_0(void)
{
	// We expect empty data for the last two times.
	g_apiEmulator.setNumberOfEventSlices(NUM_TEST_READ_TIMES-2);
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
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal(true, armZbxApiTestee.testMainThreadOneProc());
}

void test_updateTypeShouldBeChangedOnFetchItems()
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal(ArmBase::UPDATE_POLLING,
			    armZbxApiTestee.testGetUpdateType());
	armZbxApiTestee.fetchItems();
	cppcut_assert_equal(ArmBase::UPDATE_ITEM_REQUEST,
			    armZbxApiTestee.testGetUpdateType());
}

void test_setUpdateType()
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);
	cppcut_assert_equal(ArmBase::UPDATE_POLLING,
			    armZbxApiTestee.testGetUpdateType());
	armZbxApiTestee.testSetUpdateType(ArmBase::UPDATE_ITEM_REQUEST);
	cppcut_assert_equal(ArmBase::UPDATE_ITEM_REQUEST,
			    armZbxApiTestee.testGetUpdateType());
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
	deleteDBClientZabbixDB(svId);
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	return serverInfo;
}

void test_oneProcWithoutFetchItems()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testMainThreadOneProc();

	DBClientHatohol db;
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupInfoList hostgroupInfoList;
	HostgroupElementList hostgroupElementList;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupElementQueryOption hostgroupElementQueryOption(USER_ID_SYSTEM);

	db.getEventInfoList(eventInfoList, eventsQueryOption);
	db.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	db.getItemInfoList(itemInfoList, itemsQueryOption);
	db.getHostgroupInfoList(hostgroupInfoList, hostgroupsQueryOption);
	db.getHostgroupElementList(hostgroupElementList,
	                           hostgroupElementQueryOption);

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgroupInfoList.empty());
	cppcut_assert_equal(false, hostgroupElementList.empty());

	cppcut_assert_equal(false, eventInfoList.empty());
	cppcut_assert_equal(false, triggerInfoList.empty());
	cppcut_assert_equal(false, itemInfoList.empty());
}

void test_oneProcWithCopyOnDemandEnabled()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testSetCopyOnDemandEnabled(true);
	armZbxApiTestee.testMainThreadOneProc();

	DBClientHatohol db;
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupInfoList hostgroupInfoList;
	HostgroupElementList hostgroupElementList;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupElementQueryOption hostgroupElementQueryOption(USER_ID_SYSTEM);

	db.getEventInfoList(eventInfoList, eventsQueryOption);
	db.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	db.getItemInfoList(itemInfoList, itemsQueryOption);
	db.getHostgroupInfoList(hostgroupInfoList, hostgroupsQueryOption);
	db.getHostgroupElementList(hostgroupElementList,
	                           hostgroupElementQueryOption);

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgroupInfoList.empty());
	cppcut_assert_equal(false, hostgroupElementList.empty());

	cppcut_assert_equal(false, eventInfoList.empty());
	cppcut_assert_equal(false, triggerInfoList.empty());
	cppcut_assert_equal(true, itemInfoList.empty());
}

void test_oneProcWithFetchItems()
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.fetchItems();
	armZbxApiTestee.testMainThreadOneProc();

	// DBClientHatoholl::getItemInfoList() function
	// needs information about hostgroup.
	armZbxApiTestee.callUpdateGroupInformation();

	DBClientHatohol db;
	EventInfoList eventInfoList;
	TriggerInfoList triggerInfoList;
	ItemInfoList itemInfoList;
	HostgroupInfoList hostgroupInfoList;
	HostgroupElementList hostgroupElementList;

	EventsQueryOption eventsQueryOption(USER_ID_SYSTEM);
	TriggersQueryOption triggersQueryOption(USER_ID_SYSTEM);
	ItemsQueryOption itemsQueryOption(USER_ID_SYSTEM);
	HostgroupsQueryOption hostgroupsQueryOption(USER_ID_SYSTEM);
	HostgroupElementQueryOption hostgroupElementQueryOption(USER_ID_SYSTEM);

	db.getEventInfoList(eventInfoList, eventsQueryOption);
	db.getTriggerInfoList(triggerInfoList, triggersQueryOption);
	db.getItemInfoList(itemInfoList, itemsQueryOption);
	db.getHostgroupInfoList(hostgroupInfoList, hostgroupsQueryOption);
	db.getHostgroupElementList(hostgroupElementList,
	                           hostgroupElementQueryOption);

	// FIXME: should check contents
	cppcut_assert_equal(false, hostgroupInfoList.empty());
	cppcut_assert_equal(false, hostgroupElementList.empty());

	cppcut_assert_equal(true, eventInfoList.empty());
	cppcut_assert_equal(true, triggerInfoList.empty());
	cppcut_assert_equal(false, itemInfoList.empty());
}

void test_makeItemVector(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	armZbxApiTestee.assertMakeItemVector();
}

void test_makeItemVectorWithNullValue(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	armZbxApiTestee.assertMakeItemVector(true);
}

void test_checkUsernamePassword(void)
{
	ArmZabbixAPITestee armZbxApiTestee(g_defaultServerInfo);
	JsonParserAgent parser(armZbxApiTestee.testInitialJsonRequest());
	string jsonUserName;
	string jsonPassword;

	cppcut_assert_equal(false, parser.hasError());
	cppcut_assert_equal(true, parser.startObject("params"));
	cppcut_assert_equal(true, parser.read("user", jsonUserName));
	cppcut_assert_equal(true, parser.read("password", jsonPassword));
	cppcut_assert_equal(g_defaultServerInfo.userName, jsonUserName);
	cppcut_assert_equal(g_defaultServerInfo.password, jsonPassword);
}

void test_checkAuthToken(void)
{
	int svId = 0;
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
	ArmZabbixAPITestee armZbxApiTestee(serverInfo);

	string firstToken,secondToken;
	firstToken = armZbxApiTestee.testAuthToken();
	secondToken = armZbxApiTestee.testAuthToken();
	cppcut_assert_equal(firstToken, secondToken);
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

void test_getLastEventId(void)
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();
	cppcut_assert_equal((uint64_t)8697, armZbxApiTestee.getLastEventId());
}

void test_verifyEventsObtanedBySplitWay(void)
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();

	ItemTablePtr expectTable = armZbxApiTestee.testMakeEventsItemTable();
	armZbxApiTestee.callUpdateEvents();
	cppcut_assert_equal(true,
	                    armZbxApiTestee.assertItemTable(expectTable,
	                      ItemTablePtr(armZbxApiTestee.m_actualEventTablePtr)));
	const uint64_t upperLimitOfEventsAtOneTime = 
	  armZbxApiTestee.testGetMaximumNumberGetEventPerOnce();
	list<uint64_t>::iterator itr =
	   armZbxApiTestee.m_numbersOfGotEvents.begin();
	for (; itr != armZbxApiTestee.m_numbersOfGotEvents.end(); ++itr)
		cppcut_assert_equal(true, *itr <= upperLimitOfEventsAtOneTime);
}

void test_verifyGroupsAndHostsGroups(void)
{
	ArmZabbixAPITestee armZbxApiTestee(setupServer());
	armZbxApiTestee.testOpenSession();

	ItemTablePtr expectGroupsTablePtr;
	ItemTablePtr expectHostsGroupsTablePtr;
	ItemTablePtr actualGroupsTablePtr;
	ItemTablePtr actualHostsGroupsTablePtr;
	ItemTablePtr dummyHostsTablePtr;

	armZbxApiTestee.testMakeGroupsItemTable(expectGroupsTablePtr);
	armZbxApiTestee.testMakeMapHostsHostgroupsItemTable
	                  (expectHostsGroupsTablePtr);
	armZbxApiTestee.getGroups(actualGroupsTablePtr);
	armZbxApiTestee.getHosts(dummyHostsTablePtr, actualHostsGroupsTablePtr);

	armZbxApiTestee.assertItemTable(expectGroupsTablePtr,
	                                actualGroupsTablePtr);
	armZbxApiTestee.assertItemTable(expectHostsGroupsTablePtr,
	                                actualHostsGroupsTablePtr);
}
} // namespace testArmZabbixAPI
