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
#include "Helpers.h"
#include "RestResourceHost.h"
using namespace std;
using namespace mlpl;

// ---------------------------------------------------------------------------
// testFaceRestNoInit
//
// This namespace contains test cases that don't need to call hatoholInit(),
// which takes a little time.
// ---------------------------------------------------------------------------
namespace testFaceRestNoInit {

class TestFaceRestNoInit : public RestResourceHost {
public:
	static HatoholError callParseEventParameter(
		EventsQueryOption &option,
		GHashTable *query)
	{
		return parseEventParameter(option, query);
	}
};

template<typename PARAM_TYPE>
void _assertParseEventParameterTempl(
  const PARAM_TYPE &expectValue, const string &fmt, const string &paramName,
  PARAM_TYPE (EventsQueryOption::*valueGetter)(void) const,
  const HatoholErrorCode &expectCode = HTERR_OK,
  const string &forceValueStr = "")
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);

	string expectStr;
	if (forceValueStr.empty())
		expectStr =StringUtils::sprintf(fmt.c_str(), expectValue);
	else
		expectStr = forceValueStr;
	g_hash_table_insert(query,
	                    (gpointer) paramName.c_str(),
	                    (gpointer) expectStr.c_str());
	assertHatoholError(
	  expectCode,
	  TestFaceRestNoInit::callParseEventParameter(option, query));
	if (expectCode != HTERR_OK)
		return;
	cppcut_assert_equal(expectValue, (option.*valueGetter)());
}
#define assertParseEventParameterTempl(T, E, FM, PN, GT, ...) \
cut_trace(_assertParseEventParameterTempl<T>(E, FM, PN, GT, ##__VA_ARGS__))

void _assertParseEventParameterTargetServerId(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  ServerIdType, expectValue, "%" FMT_SERVER_ID, "serverId",
	  &EventsQueryOption::getTargetServerId, expectCode, forceValueStr);
}
#define assertParseEventParameterTargetServerId(E, ...) \
cut_trace(_assertParseEventParameterTargetServerId(E, ##__VA_ARGS__))

void _assertParseEventParameterTargetHostgroupId(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  HostgroupIdType, expectValue, "%" FMT_HOST_GROUP_ID,
	  "hostgroupId", &EventsQueryOption::getTargetHostgroupId,
	  expectCode, forceValueStr);
}
#define assertParseEventParameterTargetHostgroupId(E, ...) \
cut_trace(_assertParseEventParameterTargetHostgroupId(E, ##__VA_ARGS__))

void _assertParseEventParameterTargetHostId(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  HostIdType, expectValue, "%" FMT_HOST_ID, "hostId",
	  &EventsQueryOption::getTargetHostId, expectCode, forceValueStr);
}
#define assertParseEventParameterTargetHostId(E, ...) \
cut_trace(_assertParseEventParameterTargetHostId(E, ##__VA_ARGS__))

void _assertParseEventParameterSortOrderDontCare(
  const DataQueryOption::SortDirection &sortDirection,
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  DataQueryOption::SortDirection, sortDirection, "%d", "sortOrder",
	  &EventsQueryOption::getSortDirection, expectCode);
}
#define assertParseEventParameterSortOrderDontCare(O, ...) \
cut_trace(_assertParseEventParameterSortOrderDontCare(O, ##__VA_ARGS__))

void _assertParseEventParameterMaximumNumber(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  size_t, expectValue, "%zd", "maximumNumber",
	  &HostResourceQueryOption::getMaximumNumber, expectCode, forceValueStr);
}
#define assertParseEventParameterMaximumNumber(E, ...) \
cut_trace(_assertParseEventParameterMaximumNumber(E, ##__VA_ARGS__))

void _assertParseEventParameterOffset(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  size_t, expectValue, "%zd", "offset",
	  &HostResourceQueryOption::getOffset, expectCode, forceValueStr);
}
#define assertParseEventParameterOffset(E, ...) \
cut_trace(_assertParseEventParameterOffset(E, ##__VA_ARGS__))

void _assertParseEventParameterLimitOfUnifiedId(
  const size_t &expectValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  uint64_t, expectValue, "%" PRIu64, "limitOfUnifiedId",
	  &EventsQueryOption::getLimitOfUnifiedId, expectCode, forceValueStr);
}
#define assertParseEventParameterLimitOfUnifiedId(E, ...) \
cut_trace(_assertParseEventParameterLimitOfUnifiedId(E, ##__VA_ARGS__))

void _assertParseEventParameterSortType(
  const EventsQueryOption::SortType &expectedSortType,
  const string &sortTypeString,
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  EventsQueryOption::SortType, expectedSortType, "%d", "sortType",
	  &EventsQueryOption::getSortType, expectCode, sortTypeString);
}
#define assertParseEventParameterSortType(O, ...) \
cut_trace(_assertParseEventParameterSortType(O, ##__VA_ARGS__))

void _assertParseEventParameterMinimumSeverity(
  const TriggerSeverityType &expectedValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  TriggerSeverityType, expectedValue, "%d", "minimumSeverity",
	  &EventsQueryOption::getMinimumSeverity, expectCode, forceValueStr);
}
#define assertParseEventParameterMinimumSeverity(O, ...) \
cut_trace(_assertParseEventParameterMinimumSeverity(O, ##__VA_ARGS__))

void _assertParseEventParameterTriggerStatus(
  const TriggerStatusType &expectedValue, const string &forceValueStr = "",
  const HatoholErrorCode &expectCode = HTERR_OK)
{
	assertParseEventParameterTempl(
	  TriggerStatusType, expectedValue, "%d", "status",
	  &EventsQueryOption::getTriggerStatus, expectCode, forceValueStr);
}
#define assertParseEventParameterTriggerStatus(O, ...) \
cut_trace(_assertParseEventParameterTriggerStatus(O, ##__VA_ARGS__))

GHashTable *g_query = NULL;

void cut_teardown(void)
{
	if (g_query) {
		g_hash_table_unref(g_query);
		g_query = NULL;
	}
}

// ---------------------------------------------------------------------------
// test cases
// ---------------------------------------------------------------------------
void test_parseEventParameterWithNullQueryParameter(void)
{
	EventsQueryOption orig;
	EventsQueryOption option(orig);
	GHashTable *query = NULL;
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	// we confirm that the content is not changed.
	cppcut_assert_equal(true, option == orig);
}

void test_parseEventParameterDefaultSortOrder(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(DataQueryOption::SORT_DESCENDING,
	                    option.getSortDirection());
}

void test_parseEventParameterSortOrderDontCare(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_DONT_CARE);
}

void test_parseEventParameterSortOrderAscending(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_ASCENDING);
}

void test_parseEventParameterSortOrderDescending(void)
{
	assertParseEventParameterSortOrderDontCare(
	  DataQueryOption::SORT_DESCENDING);
}

void test_parseEventParameterSortInvalidValue(void)
{
	assertParseEventParameterSortOrderDontCare(
	  (DataQueryOption::SortDirection)-1, HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoSortType(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(EventsQueryOption::SORT_TIME, option.getSortType());
}

void test_parseEventParameterSortType(void)
{
	assertParseEventParameterSortType(
	  EventsQueryOption::SORT_UNIFIED_ID, "unifiedId");
}

void test_parseEventParameterSortTypeInvalidInput(void)
{
	assertParseEventParameterSortType(
	  EventsQueryOption::SORT_TIME, "event_value",
	  HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterMaximumNumberNotFound(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal((size_t)0, option.getMaximumNumber());
}

void test_parseEventParameterMaximumNumber(void)
{
	assertParseEventParameterMaximumNumber(100);
}

void test_parseEventParameterMaximumNumberInvalidInput(void)
{
	assertParseEventParameterMaximumNumber(0, "lion",
	                                       HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoOffset(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal((size_t)0, option.getOffset());
}

void test_parseEventParameterOffset(void)
{
	assertParseEventParameterOffset(150);
}

void test_parseEventParameterOffsetInvalidInput(void)
{
	assertParseEventParameterOffset(0, "cat",
					HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoLimitOfUnifiedId(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal((uint64_t)0, option.getLimitOfUnifiedId());
}

void test_parseEventParameterLimitOfUnifiedId(void)
{
	assertParseEventParameterLimitOfUnifiedId(345678);
}

void test_parseEventParameterLimitOfUnifiedIdInvalidInput(void)
{
	assertParseEventParameterLimitOfUnifiedId(
	  0, "orca", HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoTargetServerId(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(ALL_SERVERS, option.getTargetServerId());
}

void test_parseEventParameterTargetServerId(void)
{
	assertParseEventParameterTargetServerId(123);
}

void test_parseEventParameterInvalidTargetServerId(void)
{
	assertParseEventParameterTargetServerId(
	  0, "serverid", HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoTargetHostId(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(ALL_HOSTS, option.getTargetHostId());
}

void test_parseEventParameterTargetHostId(void)
{
	assertParseEventParameterTargetHostId(456);
}

void test_parseEventParameterInvalidTargetHostId(void)
{
	assertParseEventParameterTargetHostId(
	  0, "hostid", HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoTargetHostgroupId(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(ALL_HOST_GROUPS, option.getTargetHostgroupId());
}

void test_parseEventParameterTargetHostgroupId(void)
{
	assertParseEventParameterTargetHostgroupId(456);
}

void test_parseEventParameterInvalidTargetHostgroupId(void)
{
	assertParseEventParameterTargetHostgroupId(
	  0, "hostgroupid", HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoMinimumSeverity(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(TRIGGER_SEVERITY_UNKNOWN, option.getMinimumSeverity());
}

void test_parseEventParameterMinimumSeverity(void)
{
	assertParseEventParameterMinimumSeverity(TRIGGER_SEVERITY_WARNING);
}

void test_parseEventParameterInvalidMinimumSeverity(void)
{
	assertParseEventParameterMinimumSeverity(
	  TRIGGER_SEVERITY_UNKNOWN, "warning",
	  HTERR_INVALID_PARAMETER);
}

void test_parseEventParameterNoTriggerStatus(void)
{
	EventsQueryOption option;
	GHashTable *query = g_hash_table_new(g_str_hash, g_str_equal);
	assertHatoholError(
	  HTERR_OK, TestFaceRestNoInit::callParseEventParameter(option, query));
	cppcut_assert_equal(TRIGGER_STATUS_ALL, option.getTriggerStatus());
}

void test_parseEventParameterTriggerStatus(void)
{
	assertParseEventParameterTriggerStatus(TRIGGER_STATUS_PROBLEM);
}

void test_parseEventParameterInvalidTriggerStatus(void)
{
	assertParseEventParameterTriggerStatus(
	  TRIGGER_STATUS_OK, "problem",
	  HTERR_INVALID_PARAMETER);
}

} // namespace testFaceRestNoInit
