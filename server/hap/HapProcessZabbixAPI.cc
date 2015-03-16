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

#include "HapProcessZabbixAPI.h"

using namespace std;
using namespace mlpl;

static const uint64_t NUMBER_OF_GET_EVENT_PER_ONCE  = 1000;

static const HatoholArmPluginWatchType hapiZabbixErrorList[] =
{
	COLLECT_NG_PARSER_ERROR,
	COLLECT_NG_DISCONNECT_ZABBIX,
	COLLECT_NG_PLGIN_INTERNAL_ERROR,
};

static const int numHapZabbixSelfError = ARRAY_SIZE(hapiZabbixErrorList);


// ---------------------------------------------------------------------------
// PrivateContext
// ---------------------------------------------------------------------------
struct HapProcessZabbixAPI::PrivateContext {

	PrivateContext(void)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcessZabbixAPI::HapProcessZabbixAPI(int argc, char *argv[])
: HapProcessStandard(argc, argv),
  m_ctx(NULL)
{
	initGLib();
	m_ctx = new PrivateContext();
}

HapProcessZabbixAPI::~HapProcessZabbixAPI()
{
	delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HapProcessZabbixAPI::setMonitoringServerInfo(void)
{
	ZabbixAPI::setMonitoringServerInfo(getMonitoringServerInfo());
}

void HapProcessZabbixAPI::workOnTriggers(void)
{
	int requestSince;
	const bool hostsChanged = wasHostsInServerDBChanged();
	if (hostsChanged) {
		SmartTime lastTriggerTime = getTimestampOfLastTrigger();
		// TODO: getTrigger() should accept SmartTime directly.
		// TODO: We should add a way to get newly added triggers.
		//       Their timestamp are 0 in UNIX time. So the following way
		//       cannot retrieve them.
		requestSince = lastTriggerTime.getAsTimespec().tv_sec;
	} else {
		requestSince = 0;
	}
	ItemTablePtr triggers = getTrigger(requestSince);
	ItemTablePtr expandedDescriptions =
		getTriggerExpandedDescription(requestSince);
	ItemTablePtr mergedTriggers =
		mergePlainTriggersAndExpandedDescriptions(triggers, expandedDescriptions);

	if (hostsChanged) {
		sendTable(HAPI_CMD_SEND_UPDATED_TRIGGERS, mergedTriggers);
	} else {
		sendTable(HAPI_CMD_SEND_ALL_TRIGGERS, mergedTriggers);
	}
}

void HapProcessZabbixAPI::workOnHostsAndHostgroupElements(void)
{
	ItemTablePtr hostTablePtr, hostGroupsTablePtr;
	getHosts(hostTablePtr, hostGroupsTablePtr);
	sendTable(HAPI_CMD_SEND_HOSTS, hostTablePtr);
	sendTable(HAPI_CMD_SEND_HOST_GROUP_ELEMENTS, hostGroupsTablePtr);
}

void HapProcessZabbixAPI::workOnHostgroups(void)
{
	ItemTablePtr hostgroupsTablePtr;
	getGroups(hostgroupsTablePtr);
	sendTable(HAPI_CMD_SEND_HOST_GROUPS, hostgroupsTablePtr);
}

void HapProcessZabbixAPI::workOnEvents(void)
{
	// TODO: Should we consider the case in which the last event in
	// Zabbix server changes during the execution of the following loop ?
	const uint64_t lastEventIdOfZbxSv = ZabbixAPI::getEndEventId(false);
	// TOOD: we should exit immediately if the Zabbix server does not
	// have any events at all.

	const EventIdType lastEventIdOfHatohol =
	  HatoholArmPluginBase::getLastEventId();
	uint64_t eventIdOffset = 0;
	if (lastEventIdOfHatohol != EVENT_NOT_FOUND) {
		eventIdOffset = Utils::sum(lastEventIdOfHatohol, 1);
	} else {
		if (!shouldLoadOldEvent())
			eventIdOffset = lastEventIdOfZbxSv;
	}

	while (eventIdOffset <= lastEventIdOfZbxSv) {
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

void HapProcessZabbixAPI::parseReplyGetMonitoringServerInfoOnInitiated(
  MonitoringServerInfo &serverInfo, const SmartBuffer &replyBuf)
{
	if (parseReplyGetMonitoringServerInfo(serverInfo, replyBuf))
		return;
	THROW_HATOHOL_EXCEPTION(
	  "Failed to parse the reply for monitoring server information.\n");
}

HatoholError HapProcessZabbixAPI::acquireData(const MessagingContext &msgCtx,
					      const SmartBuffer &cmdBuf)
{
	// TODO:
	setMonitoringServerInfo();
	updateAuthTokenIfNeeded();
	workOnHostsAndHostgroupElements();
	workOnHostgroups();
	workOnTriggers();
	workOnEvents();

	return HTERR_OK;
}

HatoholError HapProcessZabbixAPI::fetchItem(const MessagingContext &msgCtx,
					    const SmartBuffer &cmdBuf)
{
	ItemTablePtr items = getItems();
	ItemTablePtr applications = getApplications(items);

	SmartBuffer resBuf;
	setupResponseBuffer<void>(resBuf, 0, HAPI_RES_ITEMS, &msgCtx);
	appendItemTable(resBuf, items);
	appendItemTable(resBuf, applications);
	reply(msgCtx, resBuf);

	return HTERR_OK;
}

HatoholError HapProcessZabbixAPI::fetchHistory(const MessagingContext &msgCtx,
					       const SmartBuffer &cmdBuf)
{
	HapiParamReqFetchHistory *params =
	  getCommandBody<HapiParamReqFetchHistory>(cmdBuf);

	// TODO:
	// ZabbixAPI::fromItemValueType() can't recognize "log" and "text".
	ZabbixAPI::ValueType valueType =
	  ZabbixAPI::fromItemValueType(
	    static_cast<ItemInfoValueType>(LtoN(params->valueType)));
	const char *itemId = HatoholArmPluginInterface::getString(
	                       cmdBuf, params,
	                       params->itemIdOffset, params->itemIdLength);
	ItemTablePtr items = getHistory(itemId, valueType,
	                       static_cast<time_t>(LtoN(params->beginTime)),
	                       static_cast<time_t>(LtoN(params->endTime)));
	SmartBuffer resBuf;
	setupResponseBuffer<void>(resBuf, 0, HAPI_RES_HISTORY, &msgCtx);
	appendItemTable(resBuf, items);
	reply(msgCtx, resBuf);

	return HTERR_OK;
}

HatoholError HapProcessZabbixAPI::fetchTrigger(const MessagingContext &msgCtx,
					       const SmartBuffer &cmdBuf)
{
	ItemTablePtr triggers = getTrigger(0);
	ItemTablePtr expandedDescriptions = getTriggerExpandedDescription(0);
	ItemTablePtr mergedTriggers =
	  mergePlainTriggersAndExpandedDescriptions(triggers, expandedDescriptions);

	SmartBuffer resBuf;
	setupResponseBuffer<void>(resBuf, 0, HAPI_RES_TRIGGERS, &msgCtx);
	appendItemTable(resBuf, mergedTriggers);
	reply(msgCtx, resBuf);

	return HTERR_OK;
}

HatoholArmPluginWatchType HapProcessZabbixAPI::getHapWatchType(
  const HatoholError &err)
{
	if (err == HTERR_OK)
		return COLLECT_OK;
	else if (err == HTERR_FAILED_CONNECT_ZABBIX)
		return COLLECT_NG_DISCONNECT_ZABBIX;
	else if (err == HTERR_FAILED_TO_PARSE_JSON_DATA)
		return COLLECT_NG_PARSER_ERROR;
	return COLLECT_NG_HATOHOL_INTERNAL_ERROR;
}

void HapProcessZabbixAPI::onGotNewEvents(ItemTablePtr eventsTablePtr)
{
}

//
// The following method is executed on the AMQP packet receiver thread
// as contrasted with the above virtual methods called on a GLib Event loop.
//
void HapProcessZabbixAPI::onInitiated(void)
{
	HapProcessStandard::onInitiated();
	sendHapSelfTriggers(numHapZabbixSelfError, hapiZabbixErrorList);
}
