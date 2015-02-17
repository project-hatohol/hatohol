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
	triggerInfo.hostId   = MONITORING_SERVER_SELF_ID;
	triggerInfo.hostName = StringUtils::sprintf(
	  "%s%s", svInfo.hostName.c_str(), SERVER_SELF_MONITORING_SUFFIX);
	triggerInfo.id       = armTrigger.triggerId;
	triggerInfo.brief    = armTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status   = armTrigger.status;

	triggerInfoList.push_back(triggerInfo);
}

