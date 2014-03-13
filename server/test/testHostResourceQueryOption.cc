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

using namespace std;
using namespace mlpl;

namespace testHostResourceQueryOption {

// TODO: I want to remove these, which are too denpendent on the implementation
// NOTE: The same definitions are in testDBClientHatohol.cc
static const string serverIdColumnName = "server_id";
static const string hostGroupIdColumnName = "host_group_id";
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
		HostResourceQueryOption opt(dqCtxPtr);
		cppcut_assert_equal((DataQueryContext *)dqCtxPtr,
		                    &opt.getDataQueryContext());
		cppcut_assert_equal(2, dqCtxPtr->getUsedCount());
	}
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
}

void test_copyConstructor(void)
{
	HostResourceQueryOption opt0;
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
	setupTestDBConfig(true, true);
	HostResourceQueryOption option(USER_ID_SYSTEM);
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
	HostResourceQueryOption option;
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
	setupTestDBConfig(true, true);
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

} // namespace testHostResourceQueryOption
