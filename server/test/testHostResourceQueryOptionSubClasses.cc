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

using namespace std;
using namespace mlpl;

namespace testHostResourceQueryOptionSubClasses {

// TODO: I want to remove these, which are too denpendent on the implementation
// NOTE: The same definitions are in testDBClientHatohol.cc
static const string serverIdColumnName = "server_id";
static const string hostgroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

static void initParamChecker(
  gconstpointer data, HostResourceQueryOption &option)
{
	option.setTargetServerId(2);
	string expected = "server_id=2";
	if (typeid(option) != typeid(HostgroupsQueryOption)) {
		option.setTargetHostId(4);
		expected += " AND host_id=4";
	}
	// TODO: call setHostgroupId()
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(expected, option.getCondition());
}

void _assertPrimaryTableName(const HostResourceQueryOption &option)
{
	const type_info &typeinfo = typeid(option);
	string expectedTableName;
	if (typeinfo == typeid(EventsQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_EVENTS;
	else if (typeinfo == typeid(TriggersQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_TRIGGERS;
	else if (typeinfo == typeid(ItemsQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_ITEMS;
	else if (typeinfo == typeid(HostsQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_HOSTS;
	else if (typeinfo == typeid(HostgroupsQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_HOSTGROUPS;
	else if (typeinfo == typeid(HostgroupElementQueryOption))
		expectedTableName = DBClientHatohol::TABLE_NAME_MAP_HOSTS_HOSTGROUPS;
	else
		cut_fail("Unknown type name: %s\n", typeinfo.name());
}
#define assertPrimaryTableName(TI) cut_trace(_assertPrimaryTableName(TI))

//
// For the constructor with a User ID.
//
template <class T>
static void _basicQueryOptionConstructorWithUserId(
  gconstpointer data,
  void (*checkFunc)(gconstpointer, HostResourceQueryOption &))
{
	T option(USER_ID_SYSTEM);
	assertPrimaryTableName(option);
	cppcut_assert_equal(1, option.getDataQueryContext().getUsedCount());
	(*checkFunc)(data, option);
}

#define basicQueryOptionConstructorWithUserId(T, D, O) \
cut_trace(_basicQueryOptionConstructorWithUserId<T>(D, O))

template <class T>
static void _assertQueryOptionConstructorWithUserId(gconstpointer data)
{
	basicQueryOptionConstructorWithUserId(T, data, initParamChecker);
}
#define assertQueryOptionConstructorWithUserId(T, D) \
cut_trace(_assertQueryOptionConstructorWithUserId<T>(D))

//
// For the constructor with a DataQueryContext pointer
//
template <class T>
static void _basicQueryOptionFromDataQueryContext(
  gconstpointer data,
  void (*checkFunc)(gconstpointer, HostResourceQueryOption &))
{
	DataQueryContextPtr dqCtxPtr =
	  DataQueryContextPtr(new DataQueryContext(USER_ID_SYSTEM), false);
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
	{
		T option(dqCtxPtr);
		assertPrimaryTableName(option);
		cppcut_assert_equal((DataQueryContext *)dqCtxPtr,
		                    &option.getDataQueryContext());
		cppcut_assert_equal(2, dqCtxPtr->getUsedCount());
		(*checkFunc)(data, option);
	}
	cppcut_assert_equal(1, dqCtxPtr->getUsedCount());
}
#define basicQueryOptionFromDataQueryContext(T, D, O) \
cut_trace(_basicQueryOptionFromDataQueryContext<T>(D, O))

template <class T>
static void _assertQueryOptionFromDataQueryContext(gconstpointer data)
{
	basicQueryOptionFromDataQueryContext(T, data, initParamChecker);
}
#define assertQueryOptionFromDataQueryContext(T, D) \
cut_trace(_assertQueryOptionFromDataQueryContext<T>(D))

//
// For the copy constructor
//
template <class T>
static void _assertQueryOptionCopyConstructor(gconstpointer data)
{
	T srcOpt(USER_ID_SYSTEM);
	DataQueryContext *srcDqCtx = &srcOpt.getDataQueryContext();
	cppcut_assert_equal(1, srcOpt.getDataQueryContext().getUsedCount());
	{
		T option(srcOpt);
		assertPrimaryTableName(option);
		cppcut_assert_equal(srcDqCtx, &option.getDataQueryContext());
		cppcut_assert_equal(2, srcDqCtx->getUsedCount());
		initParamChecker(data, option);
	}
	cppcut_assert_equal(1, srcDqCtx->getUsedCount());
}
#define assertQueryOptionCopyConstructor(T,D) \
cut_trace(_assertQueryOptionCopyConstructor<T>(D))

void cut_setup(void)
{
	hatoholInit();
	setupTestDBConfig(true, true);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

//
// TriggersQueryOption
//
void data_triggersQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(TriggersQueryOption, data);
}

void data_triggersQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(TriggersQueryOption, data);
}

void data_triggersQueryOptionCopyConstructor(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionCopyConstructor(gconstpointer data)
{
	assertQueryOptionCopyConstructor(TriggersQueryOption, data);
}

void data_triggersQueryOptionWithTargetId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionWithTargetId(gconstpointer data)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	TriggerIdType expectedId = 634;
	option.setTargetId(expectedId);
	string expected = StringUtils::sprintf(
		"triggers.id=%" FMT_TRIGGER_ID, expectedId);
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(expectedId, option.getTargetId());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_triggersQueryOptionDefaultMinimumSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionDefaultMinimumSeverity(gconstpointer data)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	string expected =  "";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_SEVERITY_UNKNOWN,
			    option.getMinimumSeverity());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_triggersQueryOptionWithMinimumSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionWithMinimumSeverity(gconstpointer data)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setMinimumSeverity(TRIGGER_SEVERITY_CRITICAL);
	string expected =  "triggers.severity>=4";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_SEVERITY_CRITICAL,
			    option.getMinimumSeverity());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_triggersQueryOptionDefaultStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionDefaultStatus(gconstpointer data)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	string expected =  "";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_STATUS_ALL,
			    option.getTriggerStatus());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_triggersQueryOptionWithStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_triggersQueryOptionWithStatus(gconstpointer data)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTriggerStatus(TRIGGER_STATUS_PROBLEM);
	string expected =  "triggers.status=1";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_STATUS_PROBLEM,
			    option.getTriggerStatus());
	cppcut_assert_equal(expected, option.getCondition());
}

//
// EventQueryOption
//
void data_eventQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(EventsQueryOption, data);
}

void data_eventQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(EventsQueryOption, data);
}

void data_eventQueryOptionCopyConstructor(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionCopyConstructor(gconstpointer data)
{
	assertQueryOptionCopyConstructor(EventsQueryOption, data);
}

void data_eventQueryOptionDefaultMinimumSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionWithNoSortType(void)
{
	EventsQueryOption option;
	const string expected = "";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortTypeId(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
			   DataQueryOption::SORT_DESCENDING);
	const string expected = "unified_id DESC";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortDontCare(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_UNIFIED_ID,
			   DataQueryOption::SORT_DONT_CARE);
	const string expected = "";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionWithSortTypeTime(void)
{
	EventsQueryOption option;
	option.setSortType(EventsQueryOption::SORT_TIME,
			   DataQueryOption::SORT_ASCENDING);
	const string expected =  "time_sec ASC, time_ns ASC, unified_id ASC";
	cppcut_assert_equal(expected, option.getOrderBy());
}

void test_eventQueryOptionDefaultMinimumSeverity(gconstpointer data)
{
	EventsQueryOption option(USER_ID_SYSTEM);
	string expected = "";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_SEVERITY_UNKNOWN,
			    option.getMinimumSeverity());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_eventQueryOptionWithMinimumSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionWithMinimumSeverity(gconstpointer data)
{
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setMinimumSeverity(TRIGGER_SEVERITY_CRITICAL);
	string expected =  "triggers.severity>=4";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_SEVERITY_CRITICAL,
			    option.getMinimumSeverity());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_eventQueryOptionDefaultTriggerStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionDefaultTriggerStatus(gconstpointer data)
{
	EventsQueryOption option(USER_ID_SYSTEM);
	string expected =  "";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_STATUS_ALL,
			    option.getTriggerStatus());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_eventQueryOptionWithTriggerStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionWithTriggerStatus(gconstpointer data)
{
	EventsQueryOption option(USER_ID_SYSTEM);
	option.setTriggerStatus(TRIGGER_STATUS_PROBLEM);
	string expected = "status=1";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(TRIGGER_STATUS_PROBLEM,
			    option.getTriggerStatus());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_eventQueryOptionGetServerIdColumnName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_eventQueryOptionGetServerIdColumnName(gconstpointer data)
{
	EventsQueryOption option(USER_ID_SYSTEM);
	const string hostgroupTableAlias = "map_hosts_hostgroups";
	option.setTargetServerId(26);
	option.setTargetHostgroupId(48);
	option.setTargetHostId(32);
	string expect = StringUtils::sprintf(
	                  "%s.%s=26 AND %s.%s=32 AND %s.%s=48",
			  DBClientHatohol::TABLE_NAME_EVENTS,
			  serverIdColumnName.c_str(),
			  DBClientHatohol::TABLE_NAME_TRIGGERS,
			  hostIdColumnName.c_str(),
			  hostgroupTableAlias.c_str(),
			  hostgroupIdColumnName.c_str());
	fixupForFilteringDefunctServer(data, expect, option);
	cppcut_assert_equal(expect, option.getCondition());
}

//
// ItemsQueryOption
//
void data_itemsQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemsQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(ItemsQueryOption, data);
}

void data_itemsQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemsQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(ItemsQueryOption, data);
}

void data_itemsQueryOptionCopyConstructor(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemsQueryOptionCopyConstructor(gconstpointer data)
{
	assertQueryOptionCopyConstructor(ItemsQueryOption, data);
}

void data_itemsQueryOptionWithTargetId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemsQueryOptionWithTargetId(gconstpointer data)
{
	ItemsQueryOption option(USER_ID_SYSTEM);
	ItemIdType expectedId = 436;
	option.setTargetId(expectedId);
	string expected = StringUtils::sprintf(
		"items.id=%" FMT_ITEM_ID, expectedId);
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(expectedId, option.getTargetId());
	cppcut_assert_equal(expected, option.getCondition());
}

void data_itemsQueryOptionWithItemGroupName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemsQueryOptionWithItemGroupName(gconstpointer data)
{
	ItemsQueryOption option(USER_ID_SYSTEM);
	string itemGroupName = "It's test items";
	option.setTargetItemGroupName(itemGroupName);
	string expected = "items.item_group_name='It''s test items'";
	fixupForFilteringDefunctServer(data, expected, option);
	cppcut_assert_equal(itemGroupName, option.getTargetItemGroupName());
	cppcut_assert_equal(expected, option.getCondition());
}

//
// HostsQueryOption
//
void data_hostsQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostsQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(HostsQueryOption, data);
}

void data_hostsQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostsQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(HostsQueryOption, data);
}

//
// HostgroupsQueryOption
//
void data_hostgroupsQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostgroupsQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(HostgroupsQueryOption, data);
}

void data_hostgroupsQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostgroupsQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(HostgroupsQueryOption, data);
}

void test_hostgroupsQueryOptionCallGetConditionFromUserWithoutAllServers(void)
{
	setupTestDBUser(true, true);
	const UserIdType userId = findUserWithout(OPPRVLG_GET_ALL_SERVER);
	HostgroupsQueryOption option(userId);
	option.setFilterForDataOfDefunctServers(false);
	const string actual = option.getCondition();
	const string expect = "(server_id=1 AND host_group_id IN (0,1))";
	cppcut_assert_equal(expect, actual);
}

//
// HostgroupElementQueryOption
//
void data_hostgroupElementQueryOptionConstructorWithUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostgroupElementQueryOptionConstructorWithUserId(gconstpointer data)
{
	assertQueryOptionConstructorWithUserId(HostgroupElementQueryOption,
	                                       data);
}

void data_hostgroupElementQueryOptionFromDataQueryContext(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_hostgroupElementQueryOptionFromDataQueryContext(gconstpointer data)
{
	assertQueryOptionFromDataQueryContext(HostgroupElementQueryOption,
	                                      data);
}

} // namespace testHostResourceQueryOptionSubClasses
