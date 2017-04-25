/*
 * Copyright (C) 2014,2016 Project Hatohol
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
#include <StringUtils.h>
#include <SeparatorInjector.h>
#include "Hatohol.h"
#include "HostResourceQueryOption.h"
#include "TestHostResourceQueryOption.h"
#include "DBTablesMonitoring.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "DBAgentSQLite3.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

static const char *TEST_PRIMARY_TABLE_NAME = "test_table_name";
static const ColumnDef COLUMN_DEF_TEST[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"flower",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_SERVER_ID,
	IDX_TEST_TABLE_HOST_ID,
	IDX_TEST_TABLE_FLOWER,
	IDX_TEST_TABLE_HOSTNAME,
	NUM_IDX_TEST_TABLE,
};

const DBAgent::TableProfile tableProfileTest(
  TEST_PRIMARY_TABLE_NAME, COLUMN_DEF_TEST,
  NUM_IDX_TEST_TABLE
);

static const char *TEST_HGRP_TABLE_NAME = "test_hgrp_table_name";
static const ColumnDef COLUMN_DEF_TEST_HGRP[] = {
{
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	"host_group_name",                 // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

enum {
	IDX_TEST_HGRP_TABLE_ID,
	IDX_TEST_HGRP_TABLE_SERVER_ID,
	IDX_TEST_HGRP_TABLE_HOST_ID,
	IDX_TEST_HGRP_TABLE_HOST_GROUP_ID,
	IDX_TEST_HGRP_TABLE_HOST_GROUP_NAME,
	NUM_IDX_TEST_HGRP_TABLE,
};

const DBAgent::TableProfile tableProfileTestHGrp(
  TEST_HGRP_TABLE_NAME, COLUMN_DEF_TEST_HGRP,
  NUM_IDX_TEST_HGRP_TABLE
);

static const HostResourceQueryOption::Synapse TEST_SYNAPSE(
  tableProfileTest,
  IDX_TEST_TABLE_ID, IDX_TEST_TABLE_SERVER_ID,
  tableProfileTest, IDX_TEST_TABLE_HOST_ID,
  true,
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_SERVER_ID, IDX_TEST_HGRP_TABLE_HOST_ID,
  IDX_TEST_HGRP_TABLE_HOST_GROUP_ID,
  IDX_TEST_TABLE_HOSTNAME, IDX_TEST_HGRP_TABLE_HOST_GROUP_NAME);

static const HostResourceQueryOption::Synapse TEST_SYNAPSE_HGRP(
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_ID, IDX_TEST_HGRP_TABLE_SERVER_ID,
  tableProfileTestHGrp, IDX_TEST_HGRP_TABLE_HOST_ID,
  false,
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_SERVER_ID, IDX_TEST_HGRP_TABLE_HOST_ID,
  IDX_TEST_HGRP_TABLE_HOST_GROUP_ID,
  IDX_TEST_TABLE_HOSTNAME, IDX_TEST_HGRP_TABLE_HOST_GROUP_NAME);

// TODO: I want to remove these, which are too denpendent on the implementation
// NOTE: The same definitions are in testDBClientHatohol.cc
static const string serverIdColumnName = "server_id";
static const string hostgroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

static string makeInOpContent(const HostgroupIdSet &hostgrpIdSet)
{
	string s;
	SeparatorInjector commaSeparator(",");
	HostgroupIdSetConstIterator hostgrpIdItr = hostgrpIdSet.begin();
	for (;hostgrpIdItr != hostgrpIdSet.end(); ++hostgrpIdItr) {
		commaSeparator(s);
		s += "'";
		s += *hostgrpIdItr;
		s += "'";
	}
	return s;
}

namespace testHostResourceQueryOption {

static string makeExpectedConditionForUser(
  UserIdType userId, OperationPrivilegeFlag flags)
{
	size_t index = userId - 1;
	// Shouldn't build them dynamically by the test.
	// Because:
	// 1. Easy to read. It will make easy to debug.
	// 2. Avoid modifying them unexpectedly.
	// 3. When the testee or the test data is changed, we should update them
	//    manually to recognize the change.
	string expected[] = {
		// UserId: 1
		"(test_table_name.server_id=1 AND"
		" test_hgrp_table_name.host_group_id IN ('0','1'))",
		// UserId: 2
		"",
		// UserId: 3
		"(test_table_name.server_id=1 OR"
		" (test_table_name.server_id=2 AND"
		" test_hgrp_table_name.host_group_id IN ('1','2'))"
		" OR (test_table_name.server_id=4 AND"
		" test_hgrp_table_name.host_group_id IN ('1'))"
		" OR (test_table_name.server_id=211 AND"
		" test_hgrp_table_name.host_group_id IN ('123')))",
		// UserId: 4
		"0",
		// UserId: 5
		"server_id=1",
		// UserId: 6
		"",
		// UserId: 7
		"(test_table_name.server_id=1 AND"
		" test_hgrp_table_name.host_group_id IN ('1','2'))",
		// UserId: 8
		"0",
		// UserId: 9
		"0",
		// UserId: 10
		"",
	};

	if (index >= 0 && index < ARRAY_SIZE(expected)) {
		return expected[index];
	} else {
		return "";
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
void data_makeSelectConditionUserAdmin(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_makeSelectConditionUserAdmin(gconstpointer data)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	string expect = "";
	fixupForFilteringDefunctServer(data, expect, option);
	string actual = option.getCondition();
	cppcut_assert_equal(expect, actual);
}

void data_makeSelectConditionAllEvents(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_makeSelectConditionAllEvents(gconstpointer data)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setFlags(OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER));
	string expect = "";
	fixupForFilteringDefunctServer(data, expect, option);
	string actual = option.getCondition();
	cppcut_assert_equal(expect, actual);
}

void test_makeSelectConditionNoneUser(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	string actual = option.getCondition();
	string expect = DBHatohol::getAlwaysFalseCondition();
	cppcut_assert_equal(expect, actual);
}

void data_makeSelectCondition(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_makeSelectCondition(gconstpointer data)
{
	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setExcludeDefunctServers(excludeDefunctServers);
	for (size_t i = 0; i < NumTestUserInfo; i++) {
		UserIdType userId = i + 1;
		option.setUserId(userId);
		string actual = option.getCondition();
		string expect = makeExpectedConditionForUser(
		                  userId, testUserInfo[i].flags);
		if (expect != "0" && excludeDefunctServers)
			insertValidServerCond(expect, option);
		cppcut_assert_equal(expect, actual);
	}
}

void test_getFromClauseWithAllHostgroup(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(string(TEST_PRIMARY_TABLE_NAME),
	                    option.getFromClause());
}

void test_getFromClauseWithSpecificHostgroup(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId("5");
	const string expect = 
	  "test_table_name INNER JOIN test_hgrp_table_name ON "
	  "((test_table_name.server_id=test_hgrp_table_name.server_id) AND "
	  "(test_table_name.host_id=test_hgrp_table_name.host_id))";
	cppcut_assert_equal(expect, option.getFromClause());
}

void data_isHostgroupUsed(void)
{
	gcut_add_datum("Not use hostgroup", "useHostgroup",
	               G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Use hostgroup", "useHostgroup",
	               G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_isHostgroupUsed(gconstpointer data)
{
	const bool useHostgroup = gcut_data_get_boolean(data, "useHostgroup");
	HostResourceQueryOption option(TEST_SYNAPSE);
	if (useHostgroup)
		option.setTargetHostgroupId("5");
	cppcut_assert_equal(useHostgroup, option.isHostgroupUsed());
}

void test_isHostgroupUsedForHostgroupTable(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE_HGRP);
	option.setTargetHostgroupId("5");
	// It shall always be false.
	cppcut_assert_equal(false, option.isHostgroupUsed());
}

void test_getColumnName(void)
{
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(string(COLUMN_DEF_TEST[idx].columnName),
	                           option.getColumnName(idx));
}

void test_getColumnNameWithTableName(void)
{
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId("5");
	string expect = TEST_PRIMARY_TABLE_NAME;
	expect += ".";
	expect += COLUMN_DEF_TEST[idx].columnName;
	cppcut_assert_equal(expect, option.getColumnName(idx));
}

void test_getColumnNameFull(void)
{
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId("5");
	string expect = TEST_PRIMARY_TABLE_NAME;
	expect += ".";
	expect += COLUMN_DEF_TEST[idx].columnName;
	cppcut_assert_equal(expect, option.getColumnName(idx));
}

void test_getColumnNameWithUseTableNameAlways(void)
{
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTableNameAlways();
	string expect = TEST_PRIMARY_TABLE_NAME;
	expect += ".";
	expect += COLUMN_DEF_TEST[idx].columnName;
	cppcut_assert_equal(expect, option.getColumnName(idx));
}

void data_getHostgroupColumnNameWithTableName(void)
{
	gcut_add_datum("Not use hostgroup", "useHostgroup",
	               G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Use hostgroup", "useHostgroup",
	               G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_getHostgroupColumnNameWithTableName(gconstpointer data)
{
	const bool useHostgroup = gcut_data_get_boolean(data, "useHostgroup");
	const size_t idx = IDX_TEST_HGRP_TABLE_SERVER_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	if (useHostgroup)
		option.setTargetHostgroupId("5");
	string expect;
	if (useHostgroup) {
		expect = TEST_HGRP_TABLE_NAME;
		expect += ".";
	}
	expect += COLUMN_DEF_TEST_HGRP[idx].columnName;
	cppcut_assert_equal(expect, option.getHostgroupColumnName(idx));
}

void test_getDBTermCodec(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	cppcut_assert_equal(typeid(*dbMonitoring.getDBAgent().getDBTermCodec()),
	                    typeid(*option.getDBTermCodec()));
}

void test_selectPluralServers(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	ServerIdSet serverIdSet;
	serverIdSet.insert(3);
	serverIdSet.insert(4);
	serverIdSet.insert(211);
	option.setSelectedServerIds(serverIdSet);
	string expect("server_id IN (1,2,3,4,5,211,222,301)"
		      " AND (server_id=3 OR server_id=4 OR server_id=211)");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_excludeServers(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	ServerIdSet serverIdSet;
	serverIdSet.insert(3);
	serverIdSet.insert(4);
	serverIdSet.insert(211);
	option.setExcludedServerIds(serverIdSet);
	string expect("server_id IN (1,2,3,4,5,211,222,301)"
		      " AND (server_id<>3 AND server_id<>4 AND server_id<>211)");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_excludeServersWithoutPrivilege(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	ServerIdSet serverIdSet;
	serverIdSet.insert(3);
	serverIdSet.insert(4);
	serverIdSet.insert(211);
	option.setExcludedServerIds(serverIdSet);
	string expect("0");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_targetNamesCondition(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	option.setTargetHostname("hostX1");
	option.setTargetHostgroupName("hostgroup Y1");
	string expect("server_id IN (1,2,3,4,211,222,301)"
		      " AND hostname='hostX1' AND host_group_name='hostgroup Y1'");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_selectPluralHostgroups(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	HostgroupIdSet hostgroupIdSet1, hostgroupIdSet2;
	hostgroupIdSet1.insert("101");
	hostgroupIdSet1.insert("102");
	hostgroupIdSet2.insert("103");
	hostgroupIdSet2.insert("104");
	ServerHostGrpSetMap map;
	map[5] = hostgroupIdSet1;
	map[6] = hostgroupIdSet2;
	option.setSelectedHostgroupIds(map);
	string expect("test_table_name.server_id IN (1,2,3,4,5,211,222,301)"
		      " AND "
		      "((test_table_name.server_id=5 AND"
		      " test_hgrp_table_name.host_group_id='101') OR"
		      " (test_table_name.server_id=5 AND"
		      " test_hgrp_table_name.host_group_id='102') OR"
		      " (test_table_name.server_id=6 AND"
		      " test_hgrp_table_name.host_group_id='103') OR"
		      " (test_table_name.server_id=6 AND"
		      " test_hgrp_table_name.host_group_id='104'))");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_excludePluralHostgroups(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	HostgroupIdSet hostgroupIdSet1, hostgroupIdSet2;
	hostgroupIdSet1.insert("101");
	hostgroupIdSet1.insert("102");
	hostgroupIdSet2.insert("103");
	hostgroupIdSet2.insert("104");
	ServerHostGrpSetMap map;
	map[5] = hostgroupIdSet1;
	map[6] = hostgroupIdSet2;
	option.setExcludedHostgroupIds(map);
	string expect("test_table_name.server_id IN (1,2,3,4,5,211,222,301)"
		      " AND "
		      "(NOT (test_table_name.server_id=5 AND"
		      " test_hgrp_table_name.host_group_id='101') AND"
		      " NOT (test_table_name.server_id=5 AND"
		      " test_hgrp_table_name.host_group_id='102') AND"
		      " NOT (test_table_name.server_id=6 AND"
		      " test_hgrp_table_name.host_group_id='103') AND"
		      " NOT (test_table_name.server_id=6 AND"
		      " test_hgrp_table_name.host_group_id='104'))");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_selectPluralHosts(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1, hostIdSet2;
	hostIdSet1.insert("101");
	hostIdSet1.insert("102");
	hostIdSet2.insert("103");
	hostIdSet2.insert("104");
	ServerHostSetMap map;
	map[5] = hostIdSet1;
	map[6] = hostIdSet2;
	option.setSelectedHostIds(map);
	string expect("server_id IN (1,2,3,4,5,211,222,301)"
		      " AND "
		      "((server_id=5 AND host_id='101') OR"
		      " (server_id=5 AND host_id='102') OR"
		      " (server_id=6 AND host_id='103') OR"
		      " (server_id=6 AND host_id='104'))");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_excludePluralHosts(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	LocalHostIdSet hostIdSet1, hostIdSet2;
	hostIdSet1.insert("101");
	hostIdSet1.insert("102");
	hostIdSet2.insert("103");
	hostIdSet2.insert("104");
	ServerHostSetMap hostsMap;
	hostsMap[5] = hostIdSet1;
	hostsMap[6] = hostIdSet2;
	option.setExcludedHostIds(hostsMap);
	string expect("server_id IN (1,2,3,4,5,211,222,301)"
		      " AND "
		      "(NOT (server_id=5 AND host_id='101') AND"
		      " NOT (server_id=5 AND host_id='102') AND"
		      " NOT (server_id=6 AND host_id='103') AND"
		      " NOT (server_id=6 AND host_id='104'))");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_selectPluralServerAndGroupsAndHosts(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	ServerIdSet serverIdSet;
	serverIdSet.insert(1);
	serverIdSet.insert(2);
	serverIdSet.insert(211);

	HostgroupIdSet hostgroupIdSet1, hostgroupIdSet2;
	hostgroupIdSet1.insert("101");
	hostgroupIdSet1.insert("102");
	hostgroupIdSet2.insert("103");
	hostgroupIdSet2.insert("104");
	ServerHostGrpSetMap groupsMap;
	groupsMap[3] = hostgroupIdSet1;
	groupsMap[4] = hostgroupIdSet2;

	LocalHostIdSet hostIdSet1, hostIdSet2;
	hostIdSet1.insert("1001");
	hostIdSet1.insert("1002");
	hostIdSet2.insert("1003");
	hostIdSet2.insert("1004");

	ServerHostSetMap hostsMap;
	hostsMap[5] = hostIdSet1;
	hostsMap[6] = hostIdSet2;
	option.setSelectedServerIds(serverIdSet);
	option.setSelectedHostgroupIds(groupsMap);
	option.setSelectedHostIds(hostsMap);

	string expect("test_table_name.server_id IN (1,2,3,4,5,211,222,301)"
		      " AND ("
		      "(test_table_name.server_id=1 OR"
		      " test_table_name.server_id=2 OR"
		      " test_table_name.server_id=211)"
		      " OR "
		      "((test_table_name.server_id=3 AND"
		      " test_hgrp_table_name.host_group_id='101') OR"
		      " (test_table_name.server_id=3 AND"
		      " test_hgrp_table_name.host_group_id='102') OR"
		      " (test_table_name.server_id=4 AND"
		      " test_hgrp_table_name.host_group_id='103') OR"
		      " (test_table_name.server_id=4 AND"
		      " test_hgrp_table_name.host_group_id='104'))"
		      " OR "
		      "((test_table_name.server_id=5 AND"
		      " test_table_name.host_id='1001') OR"
		      " (test_table_name.server_id=5 AND"
		      " test_table_name.host_id='1002') OR"
		      " (test_table_name.server_id=6 AND"
		      " test_table_name.host_id='1003') OR"
		      " (test_table_name.server_id=6 AND"
		      " test_table_name.host_id='1004'))"
		      ")");
	cppcut_assert_equal(expect, option.getCondition());
}

} // namespace testHostResourceQueryOption

namespace testHostResourceQueryOptionWithoutDBSetup {

static void _assertAllowedServersAndHostgroups(
  const string &expect, const ServerHostGrpSetMap &srvHostGrpSetMap,
  const ServerIdType &targetServerId = ALL_SERVERS,
  const HostgroupIdType &targetHostgroupId = ALL_HOST_GROUPS,
  const LocalHostIdType &targetHostId = ALL_LOCAL_HOSTS)
{
	// TEST_SYNAPSE requires DB setup so we use TEST_SYNAPSE_HGRP here
	TestHostResourceQueryOption option(TEST_SYNAPSE_HGRP);
	option.callSetAllowedServersAndHostgroups(&srvHostGrpSetMap);
	option.setTargetServerId(targetServerId);
	option.setTargetHostId(targetHostId);
	option.setTargetHostgroupId(targetHostgroupId);
	string cond = option.callMakeConditionAllowedHosts();
	cppcut_assert_equal(expect, cond);
}
#define assertAllowedServersAndHostgroups(M, ...) \
  cut_trace(_assertAllowedServersAndHostgroups(M, ##__VA_ARGS__))

void test_constructorDataQueryContext(void)
{
	const UserIdType userId = USER_ID_SYSTEM;
	DataQueryContextPtr dqCtxPtr =
	  DataQueryContextPtr(new DataQueryContext(userId), false);
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
	{
		HostResourceQueryOption opt(TEST_SYNAPSE, dqCtxPtr);
		cppcut_assert_equal((DataQueryContext *)dqCtxPtr,
		                    &opt.getDataQueryContext());
		cppcut_assert_equal(2, dqCtxPtr->getUsedCount());
	}
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
}

void test_copyConstructor(void)
{
	HostResourceQueryOption opt0(TEST_SYNAPSE);
	cppcut_assert_equal(1, opt0.getDataQueryContext().getUsedCount());
	{
		HostResourceQueryOption opt1(opt0);
		cppcut_assert_equal(&opt0.getDataQueryContext(),
		                    &opt1.getDataQueryContext());
		cppcut_assert_equal(2,
			            opt0.getDataQueryContext().getUsedCount());
	}
	cppcut_assert_equal(1, opt0.getDataQueryContext().getUsedCount());
}

void test_copyConstructorForImplContent(void)
{
	// check the members
	const ServerIdType    targetServerId = 3;
	const LocalHostIdType targetHostId = "TestHostId";
	const HostgroupIdType targetHostgroupId = "TestHostGRP";
	const bool            excludeDefunctServers = true;
	const ServerIdSet     selectedServerIdSet({5, 8, 10});
	const ServerIdSet     excludedServerIdSet({7, 9});
	const ServerHostGrpSetMap selectedServerHostgroupSetMap({
	  {4, {"G1", "G2"}}, {6, {"G10"}}
	});
	const ServerHostGrpSetMap excludedServerHostgroupSetMap({
	  {5, {"EG1"}}, {10, {"EG2", "EG3"}}
	});
	const ServerHostSetMap selectedServerHostSetMap({
	  {11, {"Host1", "HostX"}}, {100, {"ABC"}}
	});
	const ServerHostSetMap excludedServerHostSetMap({
	  {20, {"Fish"}},
	});

	HostResourceQueryOption opt0(TEST_SYNAPSE);
	opt0.setTargetServerId(targetServerId);
	opt0.setTargetHostId(targetHostId);
	opt0.setTargetHostgroupId(targetHostgroupId);
	opt0.setExcludeDefunctServers(excludeDefunctServers);
	opt0.setSelectedServerIds(selectedServerIdSet);
	opt0.setExcludedServerIds(excludedServerIdSet);
	opt0.setSelectedHostgroupIds(selectedServerHostgroupSetMap);
	opt0.setExcludedHostgroupIds(excludedServerHostgroupSetMap);
	opt0.setSelectedHostIds(selectedServerHostSetMap);
	opt0.setExcludedHostIds(excludedServerHostSetMap);

	HostResourceQueryOption opt1(opt0);
	cppcut_assert_equal(targetServerId, opt1.getTargetServerId());
	cppcut_assert_equal(targetHostId, opt1.getTargetHostId());
	cppcut_assert_equal(targetHostgroupId, opt1.getTargetHostgroupId());
	assertEqual(selectedServerIdSet, opt1.getSelectedServerIds());
	assertEqual(excludedServerIdSet, opt1.getExcludedServerIds());
	assertEqual(selectedServerHostgroupSetMap,
	            opt1.getSelectedHostgroupIds());
	assertEqual(excludedServerHostgroupSetMap,
	            opt1.getExcludedHostgroupIds());
	assertEqual(selectedServerHostSetMap, opt1.getSelectedHostIds());
	assertEqual(excludedServerHostSetMap, opt1.getExcludedHostIds());
}

void test_makeConditionServer(void)
{
	const string serverIdColumnName = "cat";
	const ServerIdType serverIds[] = {5, 15, 105, 1080};
	const size_t numServerIds = ARRAY_SIZE(serverIds);

	ServerIdSet svIdSet;
	for (size_t i = 0; i < numServerIds; i++)
		svIdSet.insert(serverIds[i]);

	TestHostResourceQueryOption option(TEST_SYNAPSE);
	string actual =
	  option.callMakeConditionServer(svIdSet, serverIdColumnName);

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
		  StringUtils::sprintf("%" FMT_SERVER_ID, *expectId);
		cppcut_assert_equal(expect, actualIds[i]);
	}
}

void test_makeConditionServerWithEmptyIdSet(void)
{
	ServerIdSet svIdSet;
	TestHostResourceQueryOption option(TEST_SYNAPSE);
	string actual = option.callMakeConditionServer(svIdSet, "meet");
	cppcut_assert_equal(DBHatohol::getAlwaysFalseCondition(), actual);
}

void test_getPrimaryTableName(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(TEST_PRIMARY_TABLE_NAME,
	                    option.getPrimaryTableName());
}

void test_defaultValueOfExcludeDefunctServers(void)
{
	HostResourceQueryOption opt(TEST_SYNAPSE);
	cppcut_assert_equal(true, opt.getExcludeDefunctServers());
}

void data_setAndGetExcludeDefunctServers(void)
{
	gcut_add_datum("Disable filtering",
	               "enable", G_TYPE_BOOLEAN, FALSE, NULL);
	gcut_add_datum("Eanble filtering",
	               "enable", G_TYPE_BOOLEAN, TRUE, NULL);
}

void test_setAndGetExcludeDefunctServers(gconstpointer data)
{
	HostResourceQueryOption opt(TEST_SYNAPSE);
	bool enable = gcut_data_get_boolean(data, "enable");
	opt.setExcludeDefunctServers(enable);
	cppcut_assert_equal(enable, opt.getExcludeDefunctServers());
}

void test_makeConditionEmpty(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	string expect = DBHatohol::getAlwaysFalseCondition();
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionAllServers(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[ALL_SERVERS].insert(ALL_HOST_GROUPS);
	string expect = "";
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionAllServersWithOthers(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert("1");
	srvHostGrpSetMap[1].insert("2");
	srvHostGrpSetMap[3].insert("1");
	srvHostGrpSetMap[ALL_SERVERS].insert(ALL_HOST_GROUPS);
	string expect = "";
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionAllServersWithSpecifiedHostgroup(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[ALL_SERVERS].insert("1");
	string expect = "";
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionOneServerAllHostGrp(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(ALL_HOST_GROUPS);
	string expect =
	  StringUtils::sprintf("%s=1", serverIdColumnName.c_str());
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionOneServerAndOneHostgroup(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert("3");
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN ('3'))",
	  serverIdColumnName.c_str(),
	  hostgroupIdColumnName.c_str());
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionOneServerAndHostgroups(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert("3");
	srvHostGrpSetMap[1].insert("1003");
	srvHostGrpSetMap[1].insert("2048");
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN (%s))",
	  serverIdColumnName.c_str(),
	  hostgroupIdColumnName.c_str(),
	  makeInOpContent(srvHostGrpSetMap[1]).c_str());
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
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
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_makeConditionWithTargetServer(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	string expect = StringUtils::sprintf("%s=14",
	  serverIdColumnName.c_str());
	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap, 14);
}

void test_makeConditionWithUnauthorizedTargetServer(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	assertAllowedServersAndHostgroups("0", srvHostGrpSetMap, 7);
}

void test_makeConditionWithTargetServerAndHost(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert(ALL_HOST_GROUPS);
	// makeAllowedServerAndHostgroups() ignores hostId.
	// getCondition() adds it instead.
	string expect = StringUtils::sprintf("%s=14",
					     serverIdColumnName.c_str());
	assertAllowedServersAndHostgroups(
	  expect, srvHostGrpSetMap, 14, ALL_HOST_GROUPS, "21");
}

void data_conditionForAdminWithTargetServerAndHost(void)
{
	prepareTestDataExcludeDefunctServers();
}

void test_conditionForAdminWithTargetServerAndHost(gconstpointer data)
{
	const bool excludeDefunctServers =
	  gcut_data_get_boolean(data, "excludeDefunctServers");
	string expect;
	TestHostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	ServerIdSet validServerIdSet;
	if (excludeDefunctServers) {
		validServerIdSet.insert(1);
		validServerIdSet.insert(26);
		option.callSetValidServerIdSet(&validServerIdSet);
		expect = StringUtils::sprintf("%s IN (1,26)"
					      " AND "
					      "(%s=26 AND %s='32')",
					      serverIdColumnName.c_str(),
					      serverIdColumnName.c_str(),
					      hostIdColumnName.c_str());
	} else {
		expect = StringUtils::sprintf("(%s=26 AND %s='32')",
					      serverIdColumnName.c_str(),
					      hostIdColumnName.c_str());
	}
	option.setExcludeDefunctServers(excludeDefunctServers);
	option.setTargetServerId(26);
	option.setTargetHostId("32");
	cppcut_assert_equal(expect, option.getCondition());
}

void test_makeConditionComplicated(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[5].insert("205");
	srvHostGrpSetMap[5].insert("800");
	srvHostGrpSetMap[14].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[768].insert("817");
	srvHostGrpSetMap[768].insert("818");
	srvHostGrpSetMap[768].insert("12817");
	srvHostGrpSetMap[2000].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[2001].insert(ALL_HOST_GROUPS);
	srvHostGrpSetMap[8192].insert("4096");

	string expect = StringUtils::sprintf(
	  "((%s=5 AND %s IN (%s)) OR ",
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str(),
	  makeInOpContent(srvHostGrpSetMap[5]).c_str());

	expect += StringUtils::sprintf(
	  "%s=14 OR "
	  "(%s=768 AND %s IN (%s)) OR ",
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str(),
	  makeInOpContent(srvHostGrpSetMap[768]).c_str());

	expect += StringUtils::sprintf(
	  "%s=2000 OR "
	  "%s=2001 OR "
	  "(%s=8192 AND %s IN (%s)))",
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str(),
	  makeInOpContent(srvHostGrpSetMap[8192]).c_str());

	assertAllowedServersAndHostgroups(expect, srvHostGrpSetMap);
}

void test_systemUserHasPrivilegeGettingAllServers(void)
{
	ServerHostGrpSetMap allowedServersAndHostgroupsMap;
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	cppcut_assert_equal(true, option.has(OPPRVLG_GET_ALL_SERVER));
}

} // namespace testHostResourceQueryOptionWithoutDBSetup
