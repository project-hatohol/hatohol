/*
 * Copyright (C) 2014 Project Hatohol
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

#include <string>
#include <fstream>
#include "Synchronizer.h"
#include "ZabbixAPIEmulator.h"
#include "ZabbixAPITestUtils.h"

using namespace std;

// TODO: unify these variables in testArmZabbixAPI
static ZabbixAPIEmulator g_apiEmulator;

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

static const guint EMULATOR_PORT = 33345;

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

// ---------------------------------------------------------------------------
// ZabbixAPITestee
// ---------------------------------------------------------------------------
ZabbixAPITestee::ZabbixAPITestee(const MonitoringServerInfo &serverInfo)
{
	setMonitoringServerInfo(serverInfo);
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);

}

ZabbixAPITestee::~ZabbixAPITestee()
{
	g_apiEmulator.reset();
}

const string &ZabbixAPITestee::errorMessage(void) const
{
	return m_errorMessage;
}

bool ZabbixAPITestee::run(
  const GetTestType &type, const ZabbixAPIEmulator::APIVersion &expectedVersion)
{
	bool succeeded = false;
	if (type == GET_TEST_TYPE_API_VERSION) {
		succeeded = launch(&ZabbixAPITestee::testProcVersion);
	} else {
		cut_fail("Unknown get test type: %d\n", type);
	}
	return succeeded;
}

void ZabbixAPITestee::testCheckAPIVersion(
  bool expected, int major, int minor, int micro)
{
	const string version = getAPIVersion();
	cppcut_assert_equal(string("2.0.4"), version);
	cppcut_assert_equal(expected,
	                    checkAPIVersion(major, minor, micro));
}

void ZabbixAPITestee::testOpenSession(void)
{
	cppcut_assert_equal(true, openSession());
}

void ZabbixAPITestee::initServerInfoWithDefaultParam(
  MonitoringServerInfo &serverInfo)
{
	int svId = 0;
	serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();
}

string ZabbixAPITestee::callAuthToken(void)
{
	return getAuthToken();
}

void ZabbixAPITestee::callGetHosts(ItemTablePtr &hostsTablePtr,
                                   ItemTablePtr &hostsGroupsTablePtr)
{
	getHosts(hostsTablePtr, hostsGroupsTablePtr);
}

void ZabbixAPITestee::callGetGroups(ItemTablePtr &groupsTablePtr)
{
	getGroups(groupsTablePtr);
}

uint64_t ZabbixAPITestee::callGetLastEventId(void)
{
	return getFirstOrLastEventId(LAST_EVENT_ID);
}

void ZabbixAPITestee::makeGroupsItemTable(ItemTablePtr &groupsTablePtr)
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
	for (int i = 0; i < numData; i++)
		parseAndPushGroupsData(parser, variableGroupsTablePtr, i);
	groupsTablePtr = ItemTablePtr(variableGroupsTablePtr);
}

void ZabbixAPITestee::makeMapHostsHostgroupsItemTable(
  ItemTablePtr &hostsGroupsTablePtr)
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
		parseAndPushHostsGroupsData(parser,
		                            variableHostsGroupsTablePtr, i);
	}
	hostsGroupsTablePtr = ItemTablePtr(variableHostsGroupsTablePtr);
}

//
// Protected
//
bool ZabbixAPITestee::launch(TestProc testProc, size_t numRepeat)
{
	for (size_t i = 0; i < numRepeat; i++) {
		if (!(this->*testProc)())
			return false;
	}
	return true;
}

bool ZabbixAPITestee::defaultTestProc(void)
{
	THROW_HATOHOL_EXCEPTION("m_testProc is not set.");
	return false;
}

bool ZabbixAPITestee::testProcVersion(void)
{
	cppcut_assert_equal(g_apiEmulator.getAPIVersionString(),
	                    getAPIVersion());
	return true;
}

// ---------------------------------------------------------------------------
// General methods
// ---------------------------------------------------------------------------
void _assertTestGet(
  const ZabbixAPITestee::GetTestType &testType,
  const ZabbixAPIEmulator::APIVersion &expectedVersion)
{
	int svId = 0;
	assertReceiveData(testType, svId, expectedVersion);
}

void _assertReceiveData(
  const ZabbixAPITestee::GetTestType &testType, const ServerIdType &svId,
  const ZabbixAPIEmulator::APIVersion &expectedVersion)
{
	MonitoringServerInfo serverInfo = g_defaultServerInfo;
	serverInfo.id = svId;
	serverInfo.port = getTestPort();

	ZabbixAPITestee zbxApiTestee(serverInfo);
	g_apiEmulator.setAPIVersion(expectedVersion);
	cppcut_assert_equal(
	  true, zbxApiTestee.run(testType, expectedVersion),
	  cut_message("%s\n", zbxApiTestee.errorMessage().c_str()));
}

void _assertCheckAPIVersion(
  bool expected, int major, int minor , int micro)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testCheckAPIVersion(expected, major, minor, micro);
}
