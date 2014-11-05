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

#include <cppcutter.h>
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
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name"
	  "%" FMT_GEN_ID "|12345|15|8023455|test host name", id0, id1);
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
	cppcut_assert_not_equal((GenericIdType)AUTO_INCREMENT_VALUE, id);
}

} // namespace testDBTablesHost
