/*
 * Copyright (C) 2013 Project Hatohol
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
#include "Hatohol.h"
#include "ArmNagiosNDOUtils.h"
#include "Helpers.h"

namespace testArmNagiosNDOUtils {

class ArmNagiosNDOUtilsTestee : public ArmNagiosNDOUtils {
public:
	ArmNagiosNDOUtilsTestee(const MonitoringServerInfo &serverInfo)
	: ArmNagiosNDOUtils(serverInfo)
	{
	}

	void getTrigger(void)
	{
		ArmNagiosNDOUtils::getTrigger();
	} 

	void getEvent(void)
	{
		ArmNagiosNDOUtils::getEvent();
	} 
};

static ArmNagiosNDOUtils *g_armNagi = NULL;
static ArmNagiosNDOUtilsTestee *g_armNagiTestee = NULL;

template<class T>
static void createGlobalInstance(void)
{
	MonitoringServerInfo serverInfo;
	serverInfo.id = 1;
	serverInfo.type = MONITORING_SYSTEM_NAGIOS;
	serverInfo.hostName = "localhost";
	serverInfo.ipAddress = "127.0.0.1";
	serverInfo.nickname = "My PC";
	serverInfo.port = 0; // default port
	serverInfo.pollingIntervalSec = 10;
	serverInfo.retryIntervalSec = 30;
	serverInfo.userName = "ndoutils";
	serverInfo.password = "admin";
	serverInfo.dbName   = "ndoutils";
	bool gotException = false;
	try {
		g_armNagi = new T(serverInfo);
	} catch (const exception &e) {
		gotException = true;
		cut_fail("Got exception: %s", e.what());
	}
	cppcut_assert_equal(false, gotException);

	// If T is ArmNagiosNDOUtils, the following cast will return NULL.
	g_armNagiTestee = dynamic_cast<ArmNagiosNDOUtilsTestee *>(g_armNagi);
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDBAction();
}

void cut_teardown(void)
{
	if (g_armNagi) {
		delete g_armNagi;
		g_armNagi = NULL;
		g_armNagiTestee = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_create(void)
{
	createGlobalInstance<ArmNagiosNDOUtils>();
	cppcut_assert_not_null(g_armNagi);
}

void test_getTrigger(void)
{
	createGlobalInstance<ArmNagiosNDOUtilsTestee>();
	g_armNagiTestee->getTrigger();
}

void test_getEvent(void)
{
	createGlobalInstance<ArmNagiosNDOUtilsTestee>();
	g_armNagiTestee->getEvent();
}

} // namespace testArmNagiosNDOUtils

