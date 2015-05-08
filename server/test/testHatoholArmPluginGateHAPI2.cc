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

#include <Hatohol.h>
#include <HatoholArmPluginGateHAPI2.h>
#include "Helpers.h"

namespace testHatoholArmPluginGateHAPI2 {

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
}

void test_new(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(serverInfo), false);
	cut_assert_not_null(gate);
}

void test_procedureHandlerMonitoringServerInfo(void)
{
	MonitoringServerInfo serverInfo;
	initServerInfo(serverInfo);
	HatoholArmPluginGateHAPI2Ptr gate(
	  new HatoholArmPluginGateHAPI2(serverInfo), false);
	std::string params = "";
	std::string result =
		gate->procedureHandlerExchangeProfile(HAPI2_EXCHANGE_PROFILE, params);
	std::string expected =
		"{\"jsonrpc\":\"2.0\",\"result\":{\"procedures\":"
		  "[\"getMonitoringServerInfo\",\"getLastInfo\",\"putItems\","
		  "\"putHistory\",\"updateHosts\",\"updateHostGroups\","
		  "\"updateHostGroupMembership\",\"updateTriggers\","
		  "\"updateEvents\",\"updateHostParent\",\"updateArmInfo\""
		"]},\"name\":\"exampleName\",\"id\":1}";
	cppcut_assert_equal(result, expected);
}
}
