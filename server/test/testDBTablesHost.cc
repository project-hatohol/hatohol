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

#include <cppcutter.h>
#include <gcutter.h>
#include "DBTablesTest.h"
#include "DBHatohol.h"
#include "DBTablesHost.h"
#include "Hatohol.h"
#include "Helpers.h"
using namespace std;
using namespace mlpl;

namespace testDBTablesHost {

#define DECLARE_DBTABLES_HOST(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesHost &VAR_NAME = _dbHatohol.getDBTablesHost();

static const char *testHostName = "FOO FOO FOO";

static void assertUpsertHost(const HostIdType &hostId)
{
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	ServerHostDef serverHostDef;
	serverHostDef.id = AUTO_INCREMENT_VALUE;
	serverHostDef.hostId = hostId;
	serverHostDef.serverId = 10;
	serverHostDef.hostIdInServer= "123456";
	serverHostDef.name = testHostName;
	serverHostDef.status = HOST_STAT_NORMAL;
	HostIdType retHostId = dbHost.upsertHost(serverHostDef);

	string statement = "SELECT * FROM host_list";
	HostIdType expectHostId =
	  (hostId == UNKNOWN_HOST_ID) ? retHostId : hostId;
	string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s",
	  expectHostId, serverHostDef.name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	statement = "SELECT * FROM server_host_def";
	expect = StringUtils::sprintf(
	  "1|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
	  expectHostId, serverHostDef.serverId,
	  serverHostDef.hostIdInServer.c_str(),
	  serverHostDef.name.c_str(), serverHostDef.status);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}



void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	cppcut_assert_not_null(&dbHost);
	assertDBTablesVersion(dbHatohol.getDBAgent(),
	                      DB_TABLES_ID_HOST, DBTablesHost::TABLES_VERSION);
}

void test_addHost(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	const string name = "FOO";
	HostIdType hostId = dbHost.addHost(name);
	const string statement = "SELECT * FROM host_list";
	const string expect = StringUtils::sprintf("%" FMT_HOST_ID "|%s",
	                                           hostId, name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_addHostWithDuplicatedName(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	const string name = "DOG";
	HostIdType hostId0 = dbHost.addHost(name);
	HostIdType hostId1 = dbHost.addHost(name);
	const string statement = "SELECT * FROM host_list";
	const string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s\n%" FMT_HOST_ID "|%s",
	  hostId0, name.c_str(), hostId1, name.c_str());
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_upsertServerHostDef(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name|%d",
	  id, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertServerHostDefAutoIncrement(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name|%d",
	  id0, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_equal(id0, id1);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id1);
}

void test_upsertServerHostDefUpdate(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	svHostDef.id = id0;
	svHostDef.name = "dog-dog-dog";
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog|%d",
	  id1, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertServerHostDefUpdateByServerAndHost(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = 12345;
	svHostDef.serverId = 15;
	svHostDef.hostIdInServer = "8023455";
	svHostDef.name = "test host name";
	svHostDef.status = HOST_STAT_NORMAL;

	DECLARE_DBTABLES_HOST(dbHost);
	svHostDef.name = "dog-dog-dog";
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog|%d",
	  id, HOST_STAT_NORMAL);
	const string statement = "SELECT * FROM server_host_def";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostAccess(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id);
	const string statement = "SELECT * FROM host_access";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostAccessAutoIncrement(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostAccess(hostAccess);
	GenericIdType id1 = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000\n"
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id0, id1);
	const string statement = "SELECT * FROM host_access ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertHostAccessUpdate(void)
{
	HostAccess hostAccess;
	hostAccess.id = AUTO_INCREMENT_VALUE;
	hostAccess.hostId = 12345;
	hostAccess.ipAddrOrFQDN = "192.168.100.5";
	hostAccess.priority = 1000;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostAccess(hostAccess);
	hostAccess.id = id0;
	GenericIdType id1 = dbHost.upsertHostAccess(hostAccess);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|192.168.100.5|1000", id1);
	const string statement = "SELECT * FROM host_access ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertVMInfo(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433", id);
	const string statement = "SELECT * FROM vm_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertVMInfoAutoIncrement(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertVMInfo(vmInfo);
	GenericIdType id1 = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433\n"
	  "%" FMT_GEN_ID "|12345|7766554433", id0, id1);
	const string statement = "SELECT * FROM vm_list ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertVMInfoUpdate(void)
{
	VMInfo vmInfo;
	vmInfo.id = AUTO_INCREMENT_VALUE;
	vmInfo.hostId = 12345;
	vmInfo.hypervisorHostId = 7766554433;

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertVMInfo(vmInfo);
	vmInfo.id = id0;
	GenericIdType id1 = dbHost.upsertVMInfo(vmInfo);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|7766554433", id1);
	const string statement = "SELECT * FROM vm_list ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertHostgroupList(void)
{
	Hostgroup hostgroup;
	hostgroup.id = AUTO_INCREMENT_VALUE;
	hostgroup.serverId = 52;
	hostgroup.idInServer = "88664422";
	hostgroup.name = "foo-group";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostgroup(hostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|foo-group", id);
	const string statement = "SELECT * FROM hostgroup_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostgroupListUpdate(void)
{
	Hostgroup hostgroup;
	hostgroup.id = AUTO_INCREMENT_VALUE;
	hostgroup.serverId = 52;
	hostgroup.idInServer = "88664422";
	hostgroup.name = "new group";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostgroup(hostgroup);
	hostgroup.id = id0;
	GenericIdType id1 = dbHost.upsertHostgroup(hostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|new group", id1);
	const string statement = "SELECT * FROM hostgroup_list";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_upsertHostHostgroup(void)
{
	HostHostgroup hostHostgroup;
	hostHostgroup.id = AUTO_INCREMENT_VALUE;
	hostHostgroup.serverId = 52;
	hostHostgroup.hostIdInServer = "88664422";
	hostHostgroup.hostgroupIdInServer = "1133";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertHostHostgroup(hostHostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133", id);
	const string statement = "SELECT * FROM host_hostgroup";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

void test_upsertHostHostgroupAutoIncrement(void)
{
	HostHostgroup hostHostgroup;
	hostHostgroup.id = AUTO_INCREMENT_VALUE;
	hostHostgroup.serverId = 52;
	hostHostgroup.hostIdInServer = "88664422";
	hostHostgroup.hostgroupIdInServer = "1133";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostHostgroup(hostHostgroup);
	GenericIdType id1 = dbHost.upsertHostHostgroup(hostHostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133\n"
	  "%" FMT_GEN_ID "|52|88664422|1133", id0, id1);
	const string statement = "SELECT * FROM host_hostgroup ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_not_equal(id0, id1);
}

void test_upsertHostHostgroupUpdate(void)
{
	HostHostgroup hostHostgroup;
	hostHostgroup.id = AUTO_INCREMENT_VALUE;
	hostHostgroup.serverId = 52;
	hostHostgroup.hostIdInServer = "88664422";
	hostHostgroup.hostgroupIdInServer = "1133";

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertHostHostgroup(hostHostgroup);
	hostHostgroup.id = id0;
	GenericIdType id1 = dbHost.upsertHostHostgroup(hostHostgroup);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|52|88664422|1133", id1);
	const string statement = "SELECT * FROM host_hostgroup";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
	cppcut_assert_equal(id0, id1);
}

void test_getHypervisor(void)
{
	loadTestDBVMInfo();

	DECLARE_DBTABLES_HOST(dbHost);
	const int targetIdx = 0;
	const HostIdType hostId = testVMInfo[targetIdx].hostId;
	HostIdType hypervisorId = INVALID_HOST_ID;
	HostQueryOption option(USER_ID_SYSTEM);
	HatoholError err = dbHost.getHypervisor(hypervisorId, hostId, option);
	assertHatoholError(HTERR_OK, err);
	cppcut_assert_equal(testVMInfo[targetIdx].hypervisorHostId,
	                    hypervisorId);
}

void test_getHypervisorWithInvalidUser(void)
{
	DECLARE_DBTABLES_HOST(dbHost);
	HostIdType hypervisorId = INVALID_HOST_ID;
	HostQueryOption option;
	HatoholError err = dbHost.getHypervisor(hypervisorId, 0, option);
	assertHatoholError(HTERR_INVALID_USER, err);
}

void data_getHypervisorWithUserWhoCanAccessAllHostgroup(void)
{
	gcut_add_datum("Allowed user",
	               "allowed", G_TYPE_BOOLEAN, TRUE, NULL);
	gcut_add_datum("Not allowed user",
	               "allowed", G_TYPE_BOOLEAN, FALSE, NULL);
}

void test_getHypervisorWithUserWhoCanAccessAllHostgroup(gconstpointer data)
{
	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBVMInfo();
	loadTestDBHostHostgroup();

	const gboolean allowedUser = gcut_data_get_boolean(data, "allowed");
	// TODO: This raw index is too unreadable !!
	const ServerHostDef &targetServerHostDef = testServerHostDef[11];
	DECLARE_DBTABLES_HOST(dbHost);
	HostIdType hypervisorId = INVALID_HOST_ID;

	// Select the suitable test data
	AccessInfo *targetAccessInfo = NULL;
	for (size_t i = 0; i < NumTestAccessInfo; i++) {
		AccessInfo *ai = &testAccessInfo[i];
		if (ai->hostgroupId == ALL_HOST_GROUPS &&
		    ai->serverId == targetServerHostDef.serverId) {
			targetAccessInfo = ai;
			break;
		}
	}
	cppcut_assert_not_null(targetAccessInfo);

	// Find the expected test data
	const VMInfo *expectedVMInfo = NULL;
	for (size_t i = 0; i < NumTestVMInfo; i++) {
		const VMInfo *vminfo = &testVMInfo[i];
		if (vminfo->hypervisorHostId == targetServerHostDef.hostId) {
			expectedVMInfo = vminfo;
			break;
		}
	}
	cppcut_assert_not_null(expectedVMInfo);

	UserIdType userId = targetAccessInfo->userId;
	if (!allowedUser) {
		userId = 3;
		cppcut_assert_not_equal(userId, targetAccessInfo->userId);
	}

	// Try to find the hypervisor
	HostQueryOption option(userId);
	HatoholError err = dbHost.getHypervisor(hypervisorId,
	                                        expectedVMInfo->hostId, option);
	if (allowedUser) {
		assertHatoholError(HTERR_OK, err);
		cppcut_assert_equal(expectedVMInfo->hypervisorHostId, hypervisorId);
	} else {
		assertHatoholError(HTERR_NOT_FOUND_HYPERVISOR, err);
	}
}

static void prepareForAllUserIds(void)
{
	UserIdSet userIdSet;
	userIdSet.insert(USER_ID_SYSTEM);
	for (size_t i = 0; i < NumTestAccessInfo; ++i)
		userIdSet.insert(testAccessInfo[i].userId);

	UserIdSetConstIterator userIdItr = userIdSet.begin();
	for (; userIdItr != userIdSet.end(); ++userIdItr) {
		string uidStr = StringUtils::sprintf("uid:%d", *userIdItr);
		gcut_add_datum(uidStr.c_str(),
		               "userId", G_TYPE_INT, *userIdItr, NULL);
	}
}

void data_getVirtualMachines(void)
{
	prepareForAllUserIds();
}

void test_getVirtualMachines(gconstpointer data)
{
	typedef map<HostIdType, HostIdSet> HypervisorVMMap;
	typedef HypervisorVMMap::iterator  HypervisorVMMapIterator;

	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBVMInfo();
	loadTestDBHostHostgroup();
	DECLARE_DBTABLES_HOST(dbHost);

	const UserIdType userId = gcut_data_get_int(data, "userId");

	// Collect the Hypervisors
	HypervisorVMMap hypervisorVMMap;
	size_t hostCount = 0;
	for (size_t i = 0; i < NumTestVMInfo; i++) {
		const VMInfo &vminf = testVMInfo[i];
		if (!isAuthorized(userId, vminf.hostId))
			continue;
		hypervisorVMMap[vminf.hypervisorHostId].insert(vminf.hostId);
		hostCount++;
	}
	if (isVerboseMode())
		cut_notify("<<Number of Hosts>>: %zd", hostCount);

	// Call the test method
	HostQueryOption option(userId);
	HypervisorVMMapIterator mapItr = hypervisorVMMap.begin();
	HypervisorVMMap actualHypervisorVMMap;
	for (; mapItr != hypervisorVMMap.end(); ++mapItr) {
		HostIdVector virtualMachines;
		const HostIdType &hypervisorHostId = mapItr->first;
		HatoholError err = dbHost.getVirtualMachines(
		                     virtualMachines, hypervisorHostId, option);
		assertHatoholError(HTERR_OK, err);
		for (size_t i = 0; i < virtualMachines.size(); i++) {
			const HostIdType &hid = virtualMachines[i];
			actualHypervisorVMMap[hypervisorHostId].insert(hid);
		}
	}

	// Check it
	mapItr = hypervisorVMMap.begin();
	for (; mapItr != hypervisorVMMap.end(); ++mapItr) {
		const HostIdType &expectHypervisorId = mapItr->first;
		HypervisorVMMapIterator actMapItr =
		  actualHypervisorVMMap.find(expectHypervisorId);
		cppcut_assert_equal(
		  true, actMapItr != actualHypervisorVMMap.end());
		HostIdSet &actualHostIds = actMapItr->second;

		const HostIdSet &expectHostIds = mapItr->second;
		HostIdSetConstIterator expHostIdItr = expectHostIds.begin();
		for (; expHostIdItr != expectHostIds.end(); ++expHostIdItr) {
			HostIdSetIterator actHostItr =
			  actualHostIds.find(*expHostIdItr);
			cppcut_assert_equal(
			  true, actHostItr != actualHostIds.end());
			actualHostIds.erase(actHostItr);
		}
		cppcut_assert_equal(true, actualHostIds.empty());
		actualHypervisorVMMap.erase(actMapItr);
	}
	cppcut_assert_equal(true, actualHypervisorVMMap.empty());
}

void data_upsertHost(void)
{
	gcut_add_datum("UNKNOWN_HOST_ID",
	               "hostId", G_TYPE_UINT64, UNKNOWN_HOST_ID, NULL);
	gcut_add_datum("Specific Host ID",
	               "hostId", G_TYPE_UINT64, 224466, NULL);
}

void test_upsertHost(gconstpointer data)
{
	const HostIdType hostId = gcut_data_get_boolean(data, "hostId");
	assertUpsertHost(hostId);
}

void test_upsertHostUpdate(void)
{
	assertUpsertHost(UNKNOWN_HOST_ID);

	DBHatohol dbHatohol;
	DBTablesHost &dbHost = dbHatohol.getDBTablesHost();
	ServerHostDef serverHostDef;
	serverHostDef.id = AUTO_INCREMENT_VALUE;
	serverHostDef.hostId = UNKNOWN_HOST_ID;
	serverHostDef.serverId = 10;
	serverHostDef.hostIdInServer= "123456";
	serverHostDef.name = "GO GO GO GO";
	serverHostDef.status = HOST_STAT_REMOVED;
	HostIdType hostId = dbHost.upsertHost(serverHostDef);

	string statement = "SELECT * FROM host_list";
	string expect = StringUtils::sprintf(
	  "%" FMT_HOST_ID "|%s", hostId, testHostName);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	statement = "SELECT * FROM server_host_def";
	expect = StringUtils::sprintf(
	  "1|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
	  hostId, serverHostDef.serverId,
	  serverHostDef.hostIdInServer.c_str(),
	  serverHostDef.name.c_str(), serverHostDef.status);
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void test_upsertHosts(void)
{
	ServerHostDefVect svHostDefs;
	for (size_t i = 0; i < NumTestServerHostDef; i++)
		svHostDefs.push_back(testServerHostDef[i]);
	DECLARE_DBTABLES_HOST(dbHost);
	dbHost.upsertHosts(svHostDefs);

	// check

	string statement = "SELECT * FROM host_list";
	string expect;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		expect += StringUtils::sprintf(
		  "%" FMT_HOST_ID "|%s",
		  testServerHostDef[i].hostId,
		  testServerHostDef[i].name.c_str());
		if (i <  NumTestServerHostDef - 1)
			expect += "\n";
	}
	assertDBContent(&dbHost.getDBAgent(), statement, expect);

	statement = "SELECT * FROM server_host_def";
	expect.clear();
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		const ServerHostDef &serverHostDef = testServerHostDef[i];
		expect += StringUtils::sprintf(
		  "%zd|%" FMT_HOST_ID "|%" FMT_SERVER_ID "|%s|%s|%d",
		  i + 1,
		  serverHostDef.hostId, serverHostDef.serverId,
		  serverHostDef.hostIdInServer.c_str(),
		  serverHostDef.name.c_str(), serverHostDef.status);
		if (i <  NumTestServerHostDef - 1)
			expect += "\n";
	}
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
}

void data_getServerHostDefs(void)
{
	prepareForAllUserIds();
}

void test_getServerHostDefs(gconstpointer data)
{
	const UserIdType userId = gcut_data_get_int(data, "userId");
	loadTestDBUser();
	loadTestDBAccessList();
	loadTestDBServerHostDef();
	loadTestDBHostHostgroup();
	DECLARE_DBTABLES_HOST(dbHost);

	ServerHostDefVect svHostDefVect;
	HostQueryOption option(userId);
	assertHatoholError(HTERR_OK,
	                   dbHost.getServerHostDefs(svHostDefVect, option));

	// make the expected hosts
	set<size_t> expectIds;
	for (size_t i = 0; i < NumTestServerHostDef; i++) {
		if (isAuthorized(userId, testServerHostDef[i].hostId))
			expectIds.insert(i + 1);
	}

	// Check it
	cppcut_assert_equal(expectIds.size(), svHostDefVect.size());
	for (size_t i = 0; i < svHostDefVect.size(); i++) {
		const ServerHostDef &act = svHostDefVect[i];
		set<size_t>::const_iterator it = expectIds.find(act.id);
		cppcut_assert_equal(true, it != expectIds.end());
		expectIds.erase(it);
		const size_t expectedIdx = *it - 1;
		const ServerHostDef &exp = testServerHostDef[expectedIdx];
		cppcut_assert_equal(exp.hostId, act.hostId);
		cppcut_assert_equal(exp.serverId, act.serverId);
		cppcut_assert_equal(exp.hostIdInServer, act.hostIdInServer);
		cppcut_assert_equal(exp.name, act.name);
	}
	cppcut_assert_equal(true, expectIds.empty());
}

} // namespace testDBTablesHost
