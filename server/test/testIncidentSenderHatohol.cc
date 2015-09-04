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
#include <gcutter.h>
#include <Hatohol.h>
#include <IncidentSenderHatohol.h>
#include <ThreadLocalDBCache.h>
#include "DBTablesTest.h"
#include "Helpers.h"

using namespace std;
using namespace mlpl;

namespace testIncidentSenderHatohol {

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
}

void cut_teardown(void)
{
}

static void assertDBNotChanged(void)
{
	string expected;
	for (size_t i = 0; i < NumTestIncidentInfo; i++)
		expected += makeIncidentOutput(testIncidentInfo[i]);
	DBHatohol dbHatohol;
	DBTablesMonitoring &dbMonitoring = dbHatohol.getDBTablesMonitoring();
	assertDBContent(&dbMonitoring.getDBAgent(),
			"select * from incidents",
			expected);
}

void test_send(void)
{
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[0];
	EventInfo eventInfo = testEventInfo[0];
	eventInfo.unifiedId = 931;
	IncidentSenderHatohol sender(tracker);
	HatoholError err = sender.send(eventInfo);

	cppcut_assert_equal(HTERR_OK, err.getCode());

	string expected =
	  "^1\\|3\\|1\\|2\\|931\\|\\|NONE\\|\\|\\d+\\|\\d+\\|\\d+\\|\\d+\\|\\|0\\|931$";
	DBHatohol dbHatohol;
	DBTablesMonitoring &dbMonitoring = dbHatohol.getDBTablesMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents;");
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_updateStatus(void)
{
	loadTestDBIncidents();
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[4];
	IncidentInfo incidentInfo = testIncidentInfo[2];
	incidentInfo.status = "IN PROGRESS";
	IncidentSenderHatohol sender(tracker);
	HatoholError err = sender.send(incidentInfo, "");

	cppcut_assert_equal(HTERR_OK, err.getCode());

	string expected =
	  "^5\\|2\\|2\\|3\\|123\\|\\|IN PROGRESS\\|\\|1412957360\\|0\\|\\d+\\|\\d+\\|\\|0\\|123$";
	DBHatohol dbHatohol;
	DBTablesMonitoring &dbMonitoring = dbHatohol.getDBTablesMonitoring();
	string actual = execSQL(&dbMonitoring.getDBAgent(),
				"select * from incidents"
				" where tracker_id=5 AND identifier='123';");
	cut_assert_match(expected.c_str(), actual.c_str());
}

void test_updateUnknownIncident(void)
{
	loadTestDBIncidents();
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[4];
	IncidentInfo incidentInfo = testIncidentInfo[2];
	incidentInfo.identifier = "Unknown incident";
	incidentInfo.status = "IN PROGRESS";
	IncidentSenderHatohol sender(tracker);
	HatoholError err = sender.send(incidentInfo, "");

	cppcut_assert_equal(HTERR_NOT_FOUND_TARGET_RECORD, err.getCode());
	assertDBNotChanged();
}

void test_updateUnmatchedIncidentTracker(void)
{
	loadTestDBIncidents();
	const IncidentTrackerInfo &tracker = testIncidentTrackerInfo[4];
	IncidentInfo incidentInfo = testIncidentInfo[1];
	incidentInfo.identifier = "Unknown incident";
	incidentInfo.status = "IN PROGRESS";
	IncidentSenderHatohol sender(tracker);
	HatoholError err = sender.send(incidentInfo, "");

	cppcut_assert_equal(HTERR_INVALID_PARAMETER, err.getCode());
	assertDBNotChanged();
}

}
