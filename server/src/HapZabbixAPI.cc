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

#include "HapZabbixAPI.h"
#include "ItemTablePtr.h"

using namespace mlpl;

struct HapZabbixAPI::PrivateContext {
	PrivateContext(void)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapZabbixAPI::HapZabbixAPI(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	// TODO: we have to call setMonitoringServerInfo(serverInfo)
	//       before the actual communication.
}

HapZabbixAPI::~HapZabbixAPI()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HapZabbixAPI::workOnTriggers(void)
{
	SmartTime lastTriggerTime = getTimestampOfLastTrigger();
	// TODO: getTrigger() should accept SmartTime directly.
	// TODO: We should add a way to get newly added triggers.
	//       Their timestamp are 0 in UNIX time. So the following way
	//       cannot retrieve them.
	const int requestSince = lastTriggerTime.getAsTimespec().tv_sec;
	sendTable(HAPI_CMD_SEND_UPDATED_TRIGGERS, getTrigger(requestSince));
}

void HapZabbixAPI::workOnHostsAndHostgroups(void)
{
	ItemTablePtr hostTablePtr, hostGroupsTablePtr;
	getHosts(hostTablePtr, hostGroupsTablePtr);
	sendTable(HAPI_CMD_SEND_HOSTS, hostTablePtr);
	sendTable(HAPI_CMD_SEND_HOST_GROUP_ELEMENTS, hostGroupsTablePtr);
}

void HapZabbixAPI::workOnHostgroups(void)
{
	ItemTablePtr hostgroupsTablePtr;
	getGroups(hostgroupsTablePtr);
	sendTable(HAPI_CMD_SEND_HOST_GROUPS, hostgroupsTablePtr);
}
