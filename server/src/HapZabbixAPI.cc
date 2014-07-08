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

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;

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

void HapZabbixAPI::workOnHostsAndHostgroupElements(void)
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

void HapZabbixAPI::workOnEvents(void)
{
	// TODO: Should we consider the case in which the last event in
	// Zabbix server changes during the execution of the following loop ?
	const uint64_t lastEventIdOfZbxSv = ZabbixAPI::getLastEventId();
	// TOOD: we should exit immediately if the Zabbix server does not
	// have any events at all.

	uint64_t lastEventIdOfHatohol = HatoholArmPluginBase::getLastEventId();
	uint64_t eventIdOffset = 0;
	if (lastEventIdOfHatohol != EVENT_NOT_FOUND)
		eventIdOffset = lastEventIdOfHatohol + 1;

	while (eventIdOffset < lastEventIdOfZbxSv) {
		const uint64_t eventIdTill =
		  eventIdOffset + NUMBER_OF_GET_EVENT_PER_ONCE;
		ItemTablePtr eventsTablePtr =
		  getEvents(eventIdOffset, eventIdTill);
		onGotNewEvents(eventsTablePtr);
		sendTable(HAPI_CMD_SEND_UPDATED_EVENTS, eventsTablePtr);

		// prepare for the next loop
		eventIdOffset = eventIdTill + 1;
	}
}

void HapZabbixAPI::parseReplyGetMonitoringServerInfoOnInitiated(
  MonitoringServerInfo &serverInfo, const SmartBuffer &replyBuf)
{
	if (parseReplyGetMonitoringServerInfo(serverInfo, replyBuf))
		return;
	THROW_HATOHOL_EXCEPTION(
	  "Failed to parse the reply for monitoring server information.\n");
}

void HapZabbixAPI::onInitiated(void)
{
	HatoholArmPluginBase::onInitiated();

	struct Callback : public CommandCallbacks {
		HapZabbixAPI *obj;

		Callback(HapZabbixAPI *_obj)
		: obj(_obj)
		{
		}

		virtual void onGotReply(
		  const SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			MonitoringServerInfo serverInfo;
			obj->parseReplyGetMonitoringServerInfoOnInitiated(
			  serverInfo, replyBuf);
			obj->setMonitoringServerInfo(serverInfo);
			obj->onReady(serverInfo);
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			THROW_HATOHOL_EXCEPTION(
			  "Failed to get monitoring server information. "
			  "This process will try it when an initiation "
			  "happens again.\n");
		}
	} *cb = new Callback(this);
	sendCmdGetMonitoringServerInfo(cb);
}

void HapZabbixAPI::onReady(const MonitoringServerInfo &serverInfo)
{
}

void HapZabbixAPI::onGotNewEvents(ItemTablePtr eventsTablePtr)
{
}
