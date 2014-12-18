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

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name", id);
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

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name\n"
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name", id0, id1);
	const string statement = "SELECT * FROM server_host_def ORDER BY id";
	assertDBContent(&dbHost.getDBAgent(), statement, expect);
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id0);
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

	DECLARE_DBTABLES_HOST(dbHost);
	GenericIdType id0 = dbHost.upsertServerHostDef(svHostDef);
	svHostDef.id = id0;
	svHostDef.name = "dog-dog-dog";
	GenericIdType id1 = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog", id1);
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

	DECLARE_DBTABLES_HOST(dbHost);
	svHostDef.name = "dog-dog-dog";
	GenericIdType id = dbHost.upsertServerHostDef(svHostDef);
	const string expect = StringUtils::sprintf(
	  "%" FMT_GEN_ID "|12345|15|8023455|dog-dog-dog", id);
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
	const ServerHostDef &targetServerHostDef = testServerHostDef[0];
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

void data_getVirtualMachines(void)
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

} // namespace testDBTablesHost
