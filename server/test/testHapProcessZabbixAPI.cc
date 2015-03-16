/*
 * Copyright (C) 2014 Project Hatohol
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
#include <SimpleSemaphore.h>
#include "Hatohol.h"
#include "HapProcessZabbixAPI.h"
#include "ZabbixAPIEmulator.h"
#include "HatoholArmPluginTestPair.h"

using namespace mlpl;
using namespace qpid::messaging;

namespace testHapProcessZabbixAPI {

static const guint EMULATOR_PORT = 33333;
static ZabbixAPIEmulator *g_apiEmulator = NULL;

class HapProcessZabbixAPITest :
  public HapProcessZabbixAPI, public HapiTestHelper {
public:
	HapProcessZabbixAPITest(void *params)
	: HapProcessZabbixAPI(0, NULL),
	  m_readySem(0),
	  m_numGotEvents(0)
	{
	}

	void assertWaitReady(void)
	{
		assertWaitSemaphore(m_readySem);
	}

	void callUpdateAuthTokenIfNeeded(void)
	{
		setMonitoringServerInfo();
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
		HapProcessZabbixAPI::onInitiated();
		HapiTestHelper::onInitiated();
	}

	void onReady(const MonitoringServerInfo &serverInfo) override
	{
		HapProcessZabbixAPI::onReady(serverInfo);
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
	delete g_apiEmulator;
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
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
	loadTestDBServer();

	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	arg.serverIpAddr = "127.0.0.1";
	arg.serverPort   = EMULATOR_PORT;
	HatoholArmPluginTestPair<HapProcessZabbixAPITest> pair(arg);
	pair.gate->loadHostInfoCacheForEmulator();

	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnHostsAndHostgroups();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOSTS);
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOST_GROUP_ELEMENTS);

	pair.plugin->callWorkOnTriggers();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_ALL_TRIGGERS);

	pair.plugin->callWorkOnHostsAndHostgroups();
	pair.plugin->callWorkOnTriggers();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_UPDATED_TRIGGERS);

	// TODO: check the pattern of "HAPI_CMD_SEND_UPDATED_TRIGGERS".

	// TODO: check the DB content
}

void test_getHostgroups(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	arg.serverIpAddr = "127.0.0.1";
	arg.serverPort   = EMULATOR_PORT;
	HatoholArmPluginTestPair<HapProcessZabbixAPITest> pair(arg);

	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnHostgroups();
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_HOST_GROUPS);

	// TODO: check the DB content
}

void test_getEvents(void)
{
	HatoholArmPluginTestPairArg arg(MONITORING_SYSTEM_HAPI_TEST_PASSIVE);
	arg.serverIpAddr = "127.0.0.1";
	arg.serverPort   = EMULATOR_PORT;
	HatoholArmPluginTestPair<HapProcessZabbixAPITest> pair(arg);

	pair.plugin->assertWaitReady();
	pair.plugin->callUpdateAuthTokenIfNeeded();
	pair.plugin->callWorkOnEvents();
	const size_t numGotEvents = pair.plugin->getNumGotEvents();
	// We expect that the events are sent multitimes.
	cppcut_assert_equal(true, numGotEvents >= 1);
	pair.gate->assertWaitHandledCommand(HAPI_CMD_SEND_UPDATED_EVENTS,
	                                    numGotEvents);

	// TODO: check the DB content
}

} // namespace testHapProcessZabbixAPI
