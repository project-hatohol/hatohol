/*
 * Copyright (C) 2014 Project Hatohol
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

#include <string>
#include <gcutter.h>
#include "Helpers.h"
#include "ZabbixAPITestUtils.h"

using namespace std;

namespace testZabbixAPI {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getAPIVersion_2_2_0(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getAPIVersion(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION);
}

void data_checkAPIVersion(void)
{
	gcut_add_datum("Equal",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower micro version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 3,
		       NULL);
	gcut_add_datum("Higher micro version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 5,
		       NULL);
	gcut_add_datum("Higher minor version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 1,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower major version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 1,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Higher major version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 3,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
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
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
}

void test_getAuthToken(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);

	string firstToken, secondToken;
	firstToken = zbxApiTestee.callAuthToken();
	secondToken = zbxApiTestee.callAuthToken();
	cppcut_assert_equal(firstToken, secondToken);
}

void test_verifyGroupsAndHostsGroups(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();

	ItemTablePtr expectGroupsTablePtr;
	ItemTablePtr expectHostsGroupsTablePtr;
	ItemTablePtr actualGroupsTablePtr;
	ItemTablePtr actualHostsGroupsTablePtr;
	ItemTablePtr dummyHostsTablePtr;

	zbxApiTestee.makeGroupsItemTable(expectGroupsTablePtr);
	zbxApiTestee.makeMapHostsHostgroupsItemTable(expectHostsGroupsTablePtr);
	zbxApiTestee.callGetGroups(actualGroupsTablePtr);
	zbxApiTestee.callGetHosts(dummyHostsTablePtr,
	                          actualHostsGroupsTablePtr);

	assertItemTable(expectGroupsTablePtr, actualGroupsTablePtr);
	assertItemTable(expectHostsGroupsTablePtr, actualHostsGroupsTablePtr);
}

void test_getLastEventId(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
	cppcut_assert_equal((uint64_t)8697, zbxApiTestee.callGetLastEventId());
}

} // namespace testZabbixAPI
