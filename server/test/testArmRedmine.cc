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
#include "Hatohol.h"
#include "RedmineAPIEmulator.h"
#include "ArmRedmine.h"
#include "DBTablesTest.h"

using namespace std;
using namespace mlpl;

namespace testArmRedmine {

struct ArmRedmineTestee : public ArmRedmine {
	ArmRedmineTestee(const IncidentTrackerInfo &info)
	:ArmRedmine(info)
	{
	}

	~ArmRedmineTestee()
	{
	}

	string callGetURL(void)
	{
		return getURL();
	}

	string callGetQuery(void)
	{
		return getQuery();
	}

	bool callOneProc(void)
	{
		return mainThreadOneProc();
	}
};

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
}

void cut_teardown(void)
{
	g_redmineEmulator.reset();
}

void test_getURL(void)
{
	IncidentTrackerInfo &tracker = testIncidentTrackerInfo[0];
	ArmRedmineTestee arm(tracker);
	cppcut_assert_equal(string("http://localhost/issues.json"),
			    arm.callGetURL());
}

void test_baseQueryString(void)
{
	IncidentTrackerInfo &tracker = testIncidentTrackerInfo[0];
	ArmRedmineTestee arm(tracker);
	string expected =
		"f%5B%5D=status%5Fid&op%5Bstatus%5Fid%5D=%2A&"
		"limit=100&project%5Fid=1&tracker%5Fid=3&"
		"sort=updated%5Fon%3Adesc";
	cppcut_assert_equal(expected, arm.callGetQuery());
}

void test_oneProc(void)
{
	IncidentTrackerInfo &tracker = testIncidentTrackerInfo[2];
	ArmRedmineTestee arm(tracker);
	cppcut_assert_equal(true, arm.callOneProc());
}

}
