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
#include <gcutter.h>
#include <StringUtils.h>
#include "Hatohol.h"
#include "HostResourceQueryOption.h"
#include "TestHostResourceQueryOption.h"
#include "DBClientHatohol.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "DBAgentSQLite3.h"

using namespace std;
using namespace mlpl;

namespace testHostResourceQueryOption {

static const char *TEST_PRIMARY_TABLE_NAME = "test_table_name";
static const char *TEST_ALT_TABLE_NAME     = "test_alt_table";
static const ColumnDef COLUMN_DEF_TEST[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_PRIMARY_TABLE_NAME,           // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_PRIMARY_TABLE_NAME,           // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_PRIMARY_TABLE_NAME,           // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_ALT_TABLE_NAME,               // tableName
	"flower",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_SERVER_ID,
	IDX_TEST_TABLE_HOST_ID,
	IDX_TEST_TABLE_FLOWER,
	NUM_IDX_TEST_TABLE,
};

const size_t NUM_COLUMNS_TEST = sizeof(COLUMN_DEF_TEST) / sizeof(ColumnDef);
const DBAgent::TableProfile tableProfileTest(
  TEST_PRIMARY_TABLE_NAME, COLUMN_DEF_TEST,
  sizeof(COLUMN_DEF_TEST), NUM_IDX_TEST_TABLE
);

static const char *TEST_HGRP_TABLE_NAME = "test_hgrp_table_name";
static const ColumnDef COLUMN_DEF_TEST_HGRP[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_HGRP_TABLE_NAME,              // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_HGRP_TABLE_NAME,              // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_HGRP_TABLE_NAME,              // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},{
	ITEM_ID_NOT_SET,                   // itemId
	TEST_HGRP_TABLE_NAME,              // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_IDX,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

enum {
	IDX_TEST_HGRP_TABLE_ID,
	IDX_TEST_HGRP_TABLE_SERVER_ID,
	IDX_TEST_HGRP_TABLE_HOST_ID,
	IDX_TEST_HGRP_TABLE_HOST_GROUP_ID,
	NUM_IDX_TEST_HGRP_TABLE,
};

const size_t NUM_COLUMNS_HGRP_TEST =
  sizeof(COLUMN_DEF_TEST_HGRP) / sizeof(ColumnDef);
const DBAgent::TableProfile tableProfileTestHGrp(
  TEST_HGRP_TABLE_NAME, COLUMN_DEF_TEST_HGRP,
  sizeof(COLUMN_DEF_TEST_HGRP), NUM_IDX_TEST_HGRP_TABLE
);

static const HostResourceQueryOption::Synapse TEST_SYNAPSE(
  tableProfileTest,
  IDX_TEST_TABLE_ID, IDX_TEST_TABLE_SERVER_ID, IDX_TEST_TABLE_HOST_ID,
  INVALID_COLUMN_IDX,
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_SERVER_ID, IDX_TEST_HGRP_TABLE_HOST_ID,
  IDX_TEST_HGRP_TABLE_HOST_GROUP_ID);

static const HostResourceQueryOption::Synapse TEST_SYNAPSE_HGRP(
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_ID, IDX_TEST_HGRP_TABLE_SERVER_ID,
  IDX_TEST_HGRP_TABLE_HOST_ID, IDX_TEST_HGRP_TABLE_HOST_GROUP_ID,
  tableProfileTestHGrp,
  IDX_TEST_HGRP_TABLE_SERVER_ID, IDX_TEST_HGRP_TABLE_HOST_ID,
  IDX_TEST_HGRP_TABLE_HOST_GROUP_ID);

// TODO: I want to remove these, which are too denpendent on the implementation
// NOTE: The same definitions are in testDBClientHatohol.cc
static const string serverIdColumnName = "server_id";
static const string hostgroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

// FIXME: Change order of parameter.
static void _assertMakeCondition(
  const ServerHostGrpSetMap &srvHostGrpSetMap, const string &expect,
  const ServerIdType targetServerId = ALL_SERVERS,
  const HostIdType targetHostId = ALL_HOSTS,
  const HostgroupIdType targetHostgroupId = ALL_HOST_GROUPS)
{
	string cond = TestHostResourceQueryOption::callMakeCondition(
			srvHostGrpSetMap,
			serverIdColumnName,
			hostgroupIdColumnName,
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
	struct {
		bool useFullColumnName;
		string operator()(const string &tableName,
		                  const string &columnName) {
			return useFullColumnName ?
			       tableName + "." + columnName : columnName;
		}
	} nameBuilder;
	nameBuilder.useFullColumnName = false;

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
		srvHostGrpSetMap[accInfo.serverId].insert(accInfo.hostgroupId);
		if (accInfo.hostgroupId != ALL_HOST_GROUPS)
			nameBuilder.useFullColumnName = true;
	}

	string svIdColName =
	  nameBuilder(TEST_PRIMARY_TABLE_NAME, serverIdColumnName);
	string hgrpIdColName =
	  nameBuilder(TEST_HGRP_TABLE_NAME, hostgroupIdColumnName);
	string hostIdColName =
	  nameBuilder(TEST_PRIMARY_TABLE_NAME, hostIdColumnName);
	// TODO: consider that the following way (using a part of test
	//       target method) is good.
	const string exp = TestHostResourceQueryOption::callMakeCondition(
	  srvHostGrpSetMap, svIdColName, hgrpIdColName, hostIdColName);
	return exp;
}

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
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

void test_getPrimaryTableName(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(TEST_PRIMARY_TABLE_NAME,
	                    option.getPrimaryTableName());
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
		  StringUtils::sprintf("%" FMT_SERVER_ID, *expectId);
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
	HostResourceQueryOption opt(TEST_SYNAPSE);
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
	HostResourceQueryOption opt(TEST_SYNAPSE);
	bool enable = gcut_data_get_boolean(data, "enable");
	opt.setFilterForDataOfDefunctServers(enable);
	cppcut_assert_equal(enable, opt.getFilterForDataOfDefunctServers());
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

void test_makeConditionAllServersWithSpecifiedHostgroup(void)
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

void test_makeConditionOneServerAndOneHostgroup(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(3);
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN (3))",
	  serverIdColumnName.c_str(),
	  hostgroupIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void test_makeConditionOneServerAndHostgroups(void)
{
	ServerHostGrpSetMap srvHostGrpSetMap;
	srvHostGrpSetMap[1].insert(3);
	srvHostGrpSetMap[1].insert(1003);
	srvHostGrpSetMap[1].insert(2048);
	string expect =
	  StringUtils::sprintf("(%s=1 AND %s IN (3,1003,2048))",
	  serverIdColumnName.c_str(),
	  hostgroupIdColumnName.c_str());
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
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	option.setFilterForDataOfDefunctServers(filterForDataOfDefunctSv);
	option.setTargetServerId(26);
	option.setTargetHostId(32);
	string expect = StringUtils::sprintf("%s=26 AND %s=32",
					     serverIdColumnName.c_str(),
					     hostIdColumnName.c_str());
	cppcut_assert_equal(expect, option.getCondition());
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
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(),
	  serverIdColumnName.c_str(), hostgroupIdColumnName.c_str());
	assertMakeCondition(srvHostGrpSetMap, expect);
}

void data_makeSelectConditionUserAdmin(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_makeSelectConditionUserAdmin(gconstpointer data)
{
	setupTestDBConfig(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE, USER_ID_SYSTEM);
	string expect = "";
	fixupForFilteringDefunctServer(data, expect, option);
	string actual = option.getCondition();
	cppcut_assert_equal(actual, expect);
}

void data_makeSelectConditionAllEvents(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_makeSelectConditionAllEvents(gconstpointer data)
{
	setupTestDBConfig(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setFlags(OperationPrivilege::makeFlag(OPPRVLG_GET_ALL_SERVER));
	string expect = "";
	fixupForFilteringDefunctServer(data, expect, option);
	string actual = option.getCondition();
	cppcut_assert_equal(actual, expect);
}

void test_makeSelectConditionNoneUser(void)
{
	setupTestDBConfig(true, true);
	setupTestDBUser(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE);
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
	setupTestDBConfig(true, true);
	setupTestDBUser(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE);
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

void test_getFromClauseWithAllHostgroup(void)
{
	setupTestDBUser(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(string(TEST_PRIMARY_TABLE_NAME),
	                    option.getFromClause());
}

void test_getFromClauseWithSpecificHostgroup(void)
{
	setupTestDBUser(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId(5);
	const string expect = 
	  "test_table_name INNER JOIN test_hgrp_table_name ON "
	  "((test_table_name.server_id=test_hgrp_table_name.server_id) AND "
	  "(test_table_name.host_id=test_hgrp_table_name.host_id))";
	cppcut_assert_equal(expect, option.getFromClause());
}

void data_isJoinNeeded(void)
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
		option.setTargetHostgroupId(5);
	cppcut_assert_equal(useHostgroup, option.isHostgroupUsed());
}

void test_isHostgroupUsedForHostgroupTable(void)
{
	setupTestDBUser(true, true);
	HostResourceQueryOption option(TEST_SYNAPSE_HGRP);
	option.setTargetHostgroupId(5);
	// It shall always be false.
	cppcut_assert_equal(false, option.isHostgroupUsed());
}

void test_getColumnName(void)
{
	setupTestDBUser(true, true);
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	cppcut_assert_equal(string(COLUMN_DEF_TEST[idx].columnName),
	                           option.getColumnName(idx));
}

void data_getColumnNameWithTableName(void)
{
	gcut_add_datum("The same table name", "idx", G_TYPE_INT,
	               IDX_TEST_TABLE_HOST_ID, NULL);
	gcut_add_datum("Different table name", "idx", G_TYPE_INT,
	               IDX_TEST_TABLE_FLOWER, NULL);
}

void test_getColumnNameWithTableName(gconstpointer data)
{
	setupTestDBUser(true, true);
	const size_t idx = gcut_data_get_boolean(data, "idx");
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId(5);
	string expect = COLUMN_DEF_TEST[idx].tableName;
	expect += ".";
	expect += COLUMN_DEF_TEST[idx].columnName;
	cppcut_assert_equal(expect, option.getColumnName(idx));
}

void test_getColumnNameFull(void)
{
	setupTestDBUser(true, true);
	const size_t idx = IDX_TEST_TABLE_HOST_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	option.setTargetHostgroupId(5);
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
	setupTestDBUser(true, true);
	const bool useHostgroup = gcut_data_get_boolean(data, "useHostgroup");
	const size_t idx = IDX_TEST_HGRP_TABLE_SERVER_ID;
	HostResourceQueryOption option(TEST_SYNAPSE);
	if (useHostgroup)
		option.setTargetHostgroupId(5);
	string expect;
	if (useHostgroup) {
		expect = COLUMN_DEF_TEST_HGRP[idx].tableName;
		expect += ".";
	}
	expect += COLUMN_DEF_TEST_HGRP[idx].columnName;
	cppcut_assert_equal(expect, option.getHostgroupColumnName(idx));
}

void test_getDBTermCodec(void)
{
	HostResourceQueryOption option(TEST_SYNAPSE);
	DBClientHatohol dbHatohol;
	cppcut_assert_equal(typeid(*dbHatohol.getDBAgent()->getDBTermCodec()),
	                    typeid(*option.getDBTermCodec()));
}

} // namespace testHostResourceQueryOption
