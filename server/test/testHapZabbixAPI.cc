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
#include <SimpleSemaphore.h>
#include "Hatohol.h"
#include "HapZabbixAPI.h"
#include "ZabbixAPIEmulator.h"
#include "HatoholArmPluginTestPair.h"

using namespace mlpl;
using namespace qpid::messaging;

namespace testHapZabbixAPI {

static const ServerIdType DEFAULT_SERVER_ID = 5;
static const guint EMULATOR_PORT = 33333;
static ZabbixAPIEmulator *g_apiEmulator = NULL;

class HapZabbixAPITest :
  public HapZabbixAPI, public HapiTestHelper {
public:
	HapZabbixAPITest(void)
	: m_readySem(0),
	  m_numGotEvents(0)
	{
	}

	void assertWaitReady(void)
	{
		assertWaitSemaphore(m_readySem);
	}

	void callUpdateAuthTokenIfNeeded(void)
	{
		updateAuthTokenIfNeeded();
	}

	void callWorkOnTriggers(void)
	{
		workOnTriggers();
	}

	void callWorkOnHostsAndHostgroups()
	{
		workOnHostsAndHostgroupElements();
	}

	void callWorkOnHostgroups()
	{
		workOnHostgroups();
	}

	void callWorkOnEvents()
	{
		workOnEvents();
	}

	size_t getNumGotEvents(void) const
	{
		return m_numGotEvents;
	}

protected:
	void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}

	void onInitiated(void) override
	{
		HapZabbixAPI::onInitiated();
		HapiTestHelper::onInitiated();
	}

	void onReady(void) override
	{
		m_readySem.post();
	}

	void onGotNewEvents(ItemTablePtr eventsTablePtr) override
	{
		m_numGotEvents++;
	}

private:
	SimpleSemaphore m_readySem;
	size_t          m_numGotEvents;
};

void cut_startup(void)
{
	g_apiEmulator = new ZabbixAPIEmulator();
}

void cut_shutdown(void)
{
	if (g_apiEmulator)
		delete g_apiEmulator;
}

void cut_setup(void)
{
	hatoholInit();
	if (!g_apiEmulator->isRunning())
		g_apiEmulator->start(EMULATOR_PORT);
	else
		g_apiEmulator->setOperationMode(OPE_MODE_NORMAL);
}

void cut_teardown(void)
{
	g_apiEmulator->reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getHostsAndTriggers(void)
{
	deleteDBClientHatoholDB();
	const ServerIdType serverId =
	  getTestArmPluginInfo(MONITORING_SYSTEM_HAPI_TEST_PASSIVE).serverId;
	HatoholArmPluginTestPair<HapZabbixAPITest> pair(
	  serverId, "127.0.0.1", EMULATOR_PORT);

	// TODO: Suppress warning.
	// We get host data before triggers since the host name is needed
	// when the trigger is saved on the Hatohol DB.
	// However, the following warnigs are shown.
	//     [WARN] <HatoholArmPluginGate.cc:348> Ignored a trigger whose host name was not found: server: 5, host: 10010
	// The reason why is the mismatch of host data and trigger data.
	// ZabbixAPIEmulator generates trigger data with
	// zabbix-api-res-triggers-003-hosts.json.  But the host data is
	// beased on zabbix-api-res-hosts-002.json.
	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnHostsAndHostgroups();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOSTS);
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOST_GROUP_ELEMENTS);

	pair.plugin->callWorkOnTriggers();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_UPDATED_TRIGGERS);

	// TODO: check the DB content
}

void test_getHostgroups(void)
{
	const ServerIdType serverId =
	  getTestArmPluginInfo(MONITORING_SYSTEM_HAPI_TEST_PASSIVE).serverId;
	deleteDBClientHatoholDB();
	HatoholArmPluginTestPair<HapZabbixAPITest> pair(
	  serverId, "127.0.0.1", EMULATOR_PORT);

	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnHostgroups();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOST_GROUPS);

	// TODO: check the DB content
}

void test_getEvents(void)
{
	const ServerIdType serverId =
	  getTestArmPluginInfo(MONITORING_SYSTEM_HAPI_TEST_PASSIVE).serverId;
	setupTestDBAction();
	deleteDBClientHatoholDB();
	HatoholArmPluginTestPair<HapZabbixAPITest> pair(
	  serverId, "127.0.0.1", EMULATOR_PORT);

	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnEvents();
	const size_t numGotEvents = pair.plugin->getNumGotEvents();
	// We expect that the events are sent multitimes.
	cppcut_assert_equal(true, numGotEvents >= 2);
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_UPDATED_EVENTS,
	                                    numGotEvents);

	// TODO: check the DB content
}

} // namespace testHapZabbixAPI
