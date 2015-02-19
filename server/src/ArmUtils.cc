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

#include <SmartTime.h>
#include "ArmUtils.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

static const char *SERVER_SELF_MONITORING_SUFFIX = "_SELF";

struct ArmUtils::Impl
{
	const MonitoringServerInfo &serverInfo;

	Impl(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo)
	{
	}
};

ArmUtils::ArmUtils(const MonitoringServerInfo &serverInfo)
:  m_impl(new Impl(serverInfo))
{
}

void ArmUtils::createTrigger(
  const ArmTrigger &armTrigger, TriggerInfoList &triggerInfoList)
{
	TriggerInfo triggerInfo;

	triggerInfo.serverId = m_impl->serverInfo.id;
	triggerInfo.lastChangeTime =
	  SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	triggerInfo.hostId   = MONITORING_SERVER_SELF_ID;
	triggerInfo.hostName = StringUtils::sprintf(
	  "%s%s", m_impl->serverInfo.hostName.c_str(),
	  SERVER_SELF_MONITORING_SUFFIX);
	triggerInfo.id       = armTrigger.triggerId;
	triggerInfo.brief    = armTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status   = armTrigger.status;

	triggerInfoList.push_back(triggerInfo);
}

void ArmUtils::createEvent(
  const ArmTrigger &armTrigger, EventInfoList &eventInfoList)
{
	EventInfo eventInfo;

	eventInfo.serverId = m_impl->serverInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	eventInfo.hostId = MONITORING_SERVER_SELF_ID;
	eventInfo.triggerId = armTrigger.triggerId;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.status = armTrigger.status;
	if (armTrigger.status == TRIGGER_STATUS_OK)
		eventInfo.type = EVENT_TYPE_GOOD;
	else
		eventInfo.type = EVENT_TYPE_BAD;

	eventInfoList.push_back(eventInfo);
}

void ArmUtils::registerSelfMonitoringHost(void)
{
	ServerHostDef svHostDef;
	svHostDef.id = AUTO_INCREMENT_VALUE;
	svHostDef.hostId = AUTO_ASSIGNED_ID;
	svHostDef.serverId = m_impl->serverInfo.id;
	// TODO: Use a more readable string host name.
	svHostDef.hostIdInServer =
	  StringUtils::sprintf("%" FMT_HOST_ID, MONITORING_SERVER_SELF_ID);
	svHostDef.name = StringUtils::sprintf("%s%s",
	   m_impl->serverInfo.hostName.c_str(), SERVER_SELF_MONITORING_SUFFIX);
	svHostDef.status = HOST_STAT_SELF_MONITOR;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->upsertHost(svHostDef);
	if (err != HTERR_OK) {
		MLPL_ERR("Failed to register a host for self monitoring: "
		         "(%d) %s.", err.getCode(), err.getCodeName().c_str());
	}
}

