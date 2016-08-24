/*
 * Copyright (C) 2015 Project Hatohol
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
#include <StringUtils.h>
#include "Helpers.h"
#include "Hatohol.h"
#include "DBTablesTest.h"
#include "SelfMonitor.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

namespace testSelfMonitor {

static string expectedTriggerDBContent(
  const ServerIdType &serverId, const TriggerIdType &triggerId,
  const string &brief,
  const TriggerStatusType &expectedStatus = TRIGGER_STATUS_UNKNOWN)
{
	string expect;
	expect += StringUtils::sprintf(
	            "%" FMT_SERVER_ID "|%" FMT_TRIGGER_ID,
	            serverId, triggerId.c_str());
	expect += StringUtils::sprintf(
	            "|%d|%d|%s|%s|%" FMT_HOST_ID,
	            expectedStatus, TRIGGER_SEVERITY_UNKNOWN,
	            DBCONTENT_MAGIC_CURR_TIME,
	            DBCONTENT_MAGIC_ANY, // ns
	            INAPPLICABLE_HOST_ID);
	expect += StringUtils::sprintf(
	            "|__SELF_MONITOR|%s|%s||%d\n",
	            SelfMonitor::DEFAULT_SELF_MONITOR_HOST_NAME,
	            brief.c_str(), TRIGGER_VALID_SELF_MONITORING);
	return expect;
}

static string expectedServerHostDefDBContent(
  const ServerIdType &serverId)
{
	string expect;
	const GenericIdType expectedId = 1;
	const HostIdType expectedHostId = 1;
	expect += StringUtils::sprintf(
	            "%" FMT_GEN_ID "|%" FMT_HOST_ID "|%" FMT_SERVER_ID,
	            expectedId, expectedHostId, serverId);
	expect += StringUtils::sprintf(
	            "|%" FMT_LOCAL_HOST_ID "|%s|%d",
	            MONITORING_SELF_LOCAL_HOST_ID.c_str(),
	            SelfMonitor::DEFAULT_SELF_MONITOR_HOST_NAME,
	            HOST_STAT_SELF_MONITOR);
	return expect;
}

static void trigerStatusUpdateDoubleForeach(
  SelfMonitor &monitor,
  function<void(const TriggerStatusType &, const TriggerStatusType &)> func1,
  function<void(const TriggerStatusType &, const TriggerStatusType &)> func2)
{
	Utils::foreachTriggerStatusDouble([&](const TriggerStatusType &prev,
	                                      const TriggerStatusType &curr) {
		monitor.update(prev);
		func1(prev, curr);
		monitor.update(curr);
		func2(prev, curr);
	});
}

static void _assertTriggerDB(const string &expect)
{
	ThreadLocalDBCache cache;
	string statement = "select * from ";
	statement += DBTablesMonitoring::TABLE_NAME_TRIGGERS;
	statement += " ORDER BY last_change_time_sec ASC, last_change_time_ns ASC";
	assertDBContent(&cache.getMonitoring().getDBAgent(), statement, expect);
}

#define assertTriggerDB(E,...) \
cut_trace(_assertTriggerDB(E,##__VA_ARGS__))

static void _assertHostDB(const string &expect)
{
	ThreadLocalDBCache cache;
	assertDBContent(&cache.getMonitoring().getDBAgent(),
	                "select * from server_host_def", expect);
}

#define assertHostDB(E,...) cut_trace(_assertHostDB(E,##__VA_ARGS__))


void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	const ServerIdType serverId = 100;
	const TriggerIdType triggerId = "TEST_TRIGGER_ID";
	const string brief = "Test trigger for self monitoring.";
	SelfMonitor monitor(serverId, triggerId, brief);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief));
	assertHostDB(expectedServerHostDefDBContent(serverId));
}

void test_constructor_define_same_multiple()
{
	test_constructor();
	test_constructor();
}

void test_constructorWithStatelessTrigger(void)
{
	const ServerIdType serverId = 1035;
	const string brief = "Test stateless trigger for self monitoring.";
	SelfMonitor monitor(serverId, STATELESS_MONITOR, brief);
	assertTriggerDB("");
	assertHostDB(expectedServerHostDefDBContent(serverId));
}

void test_constructor_define_different_multiple()
{
	struct {
		ServerIdType  serverId;
		TriggerIdType triggerId;
		string        brief;
	} const dataArray[] = {
		{100, "TEST_TRIGGER_ID", "Test trigger for self monitoring."},
		{100, "Cat cafe", "I'm  a cat."},
		{99, "Tree", "Time and tide wait for no man."},
	};

	string expected;
	for (auto &d: dataArray) {
		SelfMonitor monitor(d.serverId, d.triggerId, d.brief);
		expected += expectedTriggerDBContent(d.serverId, d.triggerId,
		                                     d. brief);
	}
	assertTriggerDB(expected);
}

void test_setEventGenerator(void)
{
	const ServerIdType serverId = 5;
	const TriggerIdType triggerId = "TEST_TRIGGER_ID";
	const string brief = "Test trigger for setEventGenerator.";
	SelfMonitor monitor(serverId, triggerId, brief);

	bool called = false;
	monitor.setEventGenerator(TRIGGER_STATUS_OK, TRIGGER_STATUS_PROBLEM,
	  [&](SelfMonitor &m, ...) {
		cppcut_assert_equal(&monitor, &m);
		called = true;
	});

	trigerStatusUpdateDoubleForeach(monitor,
	  [&](...) { called = false; },
	  [&](const TriggerStatusType prev, const TriggerStatusType curr) {
		const bool condP = (prev == TRIGGER_STATUS_OK);
		const bool condC = (curr == TRIGGER_STATUS_PROBLEM);
		cppcut_assert_equal(condP && condC, called,
		  cut_message("prev: %d, curr: %d", prev, curr));
	});
}

void test_setEventGeneratorWithAllTriggerStatusPrev(void)
{
	bool called = false;
	SelfMonitor monitor(3, "TriggerId", "Brief");
	monitor.setEventGenerator(TRIGGER_STATUS_ALL, TRIGGER_STATUS_PROBLEM,
	  [&](SelfMonitor &m, ...) { called = true; });

	trigerStatusUpdateDoubleForeach(monitor,
	  [&](...) { called = false; },
	  [&](const TriggerStatusType prev, const TriggerStatusType curr) {
		cppcut_assert_equal(curr == TRIGGER_STATUS_PROBLEM, called);
	});
}

void test_setEventGeneratorWithAllTriggerStatusNew(void)
{
	bool called = false;
	SelfMonitor monitor(4, "TriggerId", "Brief");
	monitor.setEventGenerator(TRIGGER_STATUS_OK, TRIGGER_STATUS_ALL,
	  [&](SelfMonitor &m, ...) { called = true; });

	trigerStatusUpdateDoubleForeach(monitor,
	  [&](...) { called = false; },
	  [&](const TriggerStatusType prev, const TriggerStatusType curr) {
		cppcut_assert_equal(prev == TRIGGER_STATUS_OK, called);
	});
}

void test_setEventGeneratorWithAllTriggerStatusBoth(void)
{
	bool called = false;
	SelfMonitor monitor(4, "TriggerId", "Brief");
	monitor.setEventGenerator(TRIGGER_STATUS_ALL, TRIGGER_STATUS_ALL,
	  [&](SelfMonitor &m, ...) { called = true; });

	trigerStatusUpdateDoubleForeach(monitor,
	  [&](...) { called = false; },
	  [&](const TriggerStatusType prev, const TriggerStatusType curr) {
		cppcut_assert_equal(true, called);
	});
}

void test_setWorkable(void)
{
	const ServerIdType serverId = 5;
	const TriggerIdType triggerId = "TRIGGER !!!";
	const string brief = "Brieeeeeeeeeeeeeeeeef";
	SelfMonitor monitor(serverId, triggerId, brief);
	monitor.update(TRIGGER_STATUS_OK);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_OK));
	monitor.setWorkable(false);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_UNKNOWN));
	monitor.setWorkable(true);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_OK));
}

void test_setWorkableChangeStatusInUnworkable(void)
{
	const ServerIdType serverId = 5;
	const TriggerIdType triggerId = "TRIGGER !!!";
	const string brief = "Brieeeeeeeeeeeeeeeeef";
	SelfMonitor monitor(serverId, triggerId, brief);
	monitor.update(TRIGGER_STATUS_OK);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_OK));
	monitor.setWorkable(false);
	monitor.update(TRIGGER_STATUS_PROBLEM);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_UNKNOWN));
	monitor.setWorkable(true);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_PROBLEM));
	monitor.setWorkable(false);
	monitor.update(TRIGGER_STATUS_OK);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_UNKNOWN));
	monitor.setWorkable(true);
	assertTriggerDB(expectedTriggerDBContent(serverId, triggerId, brief,
	                                         TRIGGER_STATUS_OK));
}

void test_addNotificationListener(void)
{
	struct RcvMonitor : public SelfMonitor {
		SelfMonitor srcMonitor;
		bool called;

		RcvMonitor()
		: SelfMonitor(2, "TRIG2", "Receiver"),
		  srcMonitor(1, "TRIG1", "Source"),
		  called(false)
		{
		}

		void onNotified(const SelfMonitor &src,
		                const TriggerStatusType &prevStatus,
		                const TriggerStatusType &newStatus) override
		{
			cppcut_assert_equal(&srcMonitor, &src);
			cppcut_assert_equal(TRIGGER_STATUS_UNKNOWN, prevStatus);
			cppcut_assert_equal(TRIGGER_STATUS_OK, newStatus);
			called = true;
		}
	};
	shared_ptr<RcvMonitor> rcvMonitorPtr(new RcvMonitor());
	SelfMonitor &srcMonitor = rcvMonitorPtr->srcMonitor;
	srcMonitor.addNotificationListener(rcvMonitorPtr);
	srcMonitor.update(TRIGGER_STATUS_OK);
	cppcut_assert_equal(true, rcvMonitorPtr->called);
}

void test_setNotificationHandler(void)
{
	SelfMonitor srcMonitor(1, "TRIG1", "Source");
	SelfMonitorPtr rcvMonitorPtr(new SelfMonitor(2, "TRIG2", "Receiver"));

	bool called = false;
	auto handler = [&](SelfMonitor &m, const SelfMonitor &src,
	                   const TriggerStatusType &prevStatus,
	                   const TriggerStatusType &newStatus)
	{
		cppcut_assert_equal(&srcMonitor, &src);
		cppcut_assert_equal(rcvMonitorPtr.get(), &m);
		cppcut_assert_equal(TRIGGER_STATUS_UNKNOWN, prevStatus);
		cppcut_assert_equal(TRIGGER_STATUS_OK, newStatus);
		called = true;
	};
	srcMonitor.addNotificationListener(rcvMonitorPtr);
	rcvMonitorPtr->setNotificationHandler(handler);
	srcMonitor.update(TRIGGER_STATUS_OK);
	cppcut_assert_equal(true, called);
}

void test_eraseDeletedListener(void)
{
	struct RcvMonitor : public SelfMonitor {
		bool called;

		RcvMonitor(const TriggerIdType &triggerId)
		: SelfMonitor(1, triggerId, "Receiver"),
		  called(false)
		{
		}

		void onNotified(const SelfMonitor &,
		  const TriggerStatusType &, const TriggerStatusType &) override
		{
			called = true;
		}
	};

	SelfMonitor srcMonitor(1, "TRIG1", "Source");
	shared_ptr<RcvMonitor> rcvMonitor1Ptr(new RcvMonitor("R1"));
	{
		shared_ptr<RcvMonitor> rcvMonitor2Ptr(new RcvMonitor("R2"));
		srcMonitor.addNotificationListener(rcvMonitor2Ptr);
		srcMonitor.addNotificationListener(rcvMonitor1Ptr);

		srcMonitor.update(TRIGGER_STATUS_OK);
		cppcut_assert_equal(true, rcvMonitor1Ptr->called);
		cppcut_assert_equal(true, rcvMonitor2Ptr->called);
	}
	rcvMonitor1Ptr->called = false;
	srcMonitor.update(TRIGGER_STATUS_PROBLEM);
	cppcut_assert_equal(true, rcvMonitor1Ptr->called);
}

void test_getLastKnownStatus(void)
{
	SelfMonitor m(1, "TRIG1", "Monitor");
	cppcut_assert_equal(TRIGGER_STATUS_UNKNOWN, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_OK);
	cppcut_assert_equal(TRIGGER_STATUS_OK, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_UNKNOWN);
	cppcut_assert_equal(TRIGGER_STATUS_OK, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_PROBLEM);
	cppcut_assert_equal(TRIGGER_STATUS_PROBLEM, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_UNKNOWN);
	cppcut_assert_equal(TRIGGER_STATUS_PROBLEM, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_OK);
	cppcut_assert_equal(TRIGGER_STATUS_OK, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_PROBLEM);
	cppcut_assert_equal(TRIGGER_STATUS_PROBLEM, m.getLastKnownStatus());

	m.update(TRIGGER_STATUS_OK);
	cppcut_assert_equal(TRIGGER_STATUS_OK, m.getLastKnownStatus());
}

void test_saveEvent(void)
{
	const ServerIdType serverId = 123;
	const TriggerIdType triggerId = "TRIG1";
	const string trigBrief = "Test self monitor.";
	const TriggerSeverityType severity = TRIGGER_SEVERITY_ERROR;

	auto expectedContent = [&](
	  UnifiedEventIdType &unifiedEventId,
	  const EventType &expectedEventType,
	  const TriggerStatusType expectedStatus,
	  const string &expectedEventBrief) {
		string s;
		s += StringUtils::sprintf(
		  "%" FMT_UNIFIED_EVENT_ID "|%" FMT_SERVER_ID "||",
		  unifiedEventId, serverId);
		s += StringUtils::sprintf(
		  "%s|%s|%d|%" FMT_TRIGGER_ID "|%d|%d|" ,
		  DBCONTENT_MAGIC_CURR_TIME, DBCONTENT_MAGIC_ANY, // ns
		  expectedEventType, triggerId.c_str(),
		  expectedStatus, severity);
		s += StringUtils::sprintf(
		  "%" FMT_HOST_ID "|__SELF_MONITOR|%s|%s|",
		  INAPPLICABLE_HOST_ID,
		  SelfMonitor::DEFAULT_SELF_MONITOR_HOST_NAME,
		  expectedEventBrief.c_str());
		s += "\n";
		unifiedEventId++;
		return s;
	};

	SelfMonitor monitor(serverId, triggerId, trigBrief, severity);
	const string evtBrief = "This is the test of the event.";
	const EventType evtType = EVENT_TYPE_GOOD;
	monitor.saveEvent(evtBrief, evtType);

	string statement = "select * from ";
	statement += DBTablesMonitoring::TABLE_NAME_EVENTS;
	statement += " ORDER BY time_sec ASC, time_ns ASC";
	UnifiedEventIdType uevtid = 1;
	string expect;
	expect += expectedContent(uevtid, evtType,
	                          TRIGGER_STATUS_UNKNOWN, evtBrief);

	ThreadLocalDBCache cache;
	assertDBContent(&cache.getMonitoring().getDBAgent(), statement, expect);

	// EVENT_TYPE_AUTO
	const string evtBrief2 = "The 2nd Event message.";
	monitor.update(TRIGGER_STATUS_PROBLEM);
	monitor.saveEvent(evtBrief2);
	expect += expectedContent(uevtid, EVENT_TYPE_BAD,
	                          TRIGGER_STATUS_PROBLEM, evtBrief2);
	assertDBContent(&cache.getMonitoring().getDBAgent(), statement, expect);

	monitor.update(TRIGGER_STATUS_OK);
	monitor.saveEvent(evtBrief2);
	expect += expectedContent(uevtid, EVENT_TYPE_GOOD,
	                          TRIGGER_STATUS_OK, evtBrief2);
	assertDBContent(&cache.getMonitoring().getDBAgent(), statement, expect);

	// event brief is empty
	monitor.saveEvent();
	expect += expectedContent(uevtid, EVENT_TYPE_GOOD,
	                          TRIGGER_STATUS_OK, trigBrief);
	assertDBContent(&cache.getMonitoring().getDBAgent(), statement, expect);
}

}; // namespace testSelfMonitor
