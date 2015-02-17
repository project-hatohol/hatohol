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
#include "ArmUtils.h"

using namespace std;
using namespace mlpl;

static const char *SERVER_SELF_MONITORING_SUFFIX = "_SELF";

void ArmUtils::createTrigger(
  const MonitoringServerInfo &svInfo, const ArmTrigger &armTrigger,
  TriggerInfoList &triggerInfoList)
{
	TriggerInfo triggerInfo;

	triggerInfo.serverId = svInfo.id;
	triggerInfo.lastChangeTime =
	  SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	triggerInfo.globalHostId = MONITORING_SERVER_SELF_ID;
	triggerInfo.hostName = StringUtils::sprintf(
	  "%s%s", svInfo.hostName.c_str(), SERVER_SELF_MONITORING_SUFFIX);
	triggerInfo.id       = armTrigger.triggerId;
	triggerInfo.brief    = armTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status   = armTrigger.status;

	triggerInfoList.push_back(triggerInfo);
}

void ArmUtils::createEvent(
  const MonitoringServerInfo &svInfo,
  const ArmTrigger &armTrigger, EventInfoList &eventInfoList)
{
	EventInfo eventInfo;

	eventInfo.serverId = svInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	eventInfo.globalHostId = MONITORING_SERVER_SELF_ID;
	eventInfo.triggerId = armTrigger.triggerId;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.status = armTrigger.status;
	if (armTrigger.status == TRIGGER_STATUS_OK)
		eventInfo.type = EVENT_TYPE_GOOD;
	else
		eventInfo.type = EVENT_TYPE_BAD;

	eventInfoList.push_back(eventInfo);
}

