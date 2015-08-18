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
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

static const char *SERVER_SELF_MONITORING_SUFFIX = "_SELF";

struct ArmUtils::Impl
{
	ArmUtils                   *obj;
	const MonitoringServerInfo &serverInfo;
	ArmTrigger                 *armTriggers;
	const size_t                numArmTriggers;
	HostIdType                  globalHostId;

	Impl(ArmUtils *_obj, const MonitoringServerInfo &_serverInfo,
	     ArmTrigger *_armTriggers, const size_t &_numArmTriggers)
	: obj(_obj),
	  serverInfo(_serverInfo),
	  armTriggers(_armTriggers),
	  numArmTriggers(_numArmTriggers),
	  globalHostId(INVALID_HOST_ID)
	{
		initializeArmTriggers();
	}

	void initializeArmTriggers(void)
	{
		for (size_t i = 0; i < numArmTriggers; i++)
			armTriggers[i].status = TRIGGER_STATUS_ALL;
	}

	bool isCollectingOK(const size_t &idx)
	{
		return (idx == numArmTriggers + 1);
	}

	bool isUsed(const size_t &idx)
	{
		return (armTriggers[idx].status != TRIGGER_STATUS_ALL);
	}

	void updateTriggerStatus(
	  const size_t &triggerIdx, const TriggerStatusType &status)
	{
		UpdateTriggerArg arg = {triggerIdx, status};

		if (isCollectingOK(triggerIdx))
			updateTriggersForCollectingOK(arg);
		else
			updateTriggersForCollectingFailure(arg);

		if (!arg.triggerInfoList.empty()) {
			ThreadLocalDBCache cache;
			cache.getMonitoring().addTriggerInfoList(
			  arg.triggerInfoList);
		}
		if (!arg.eventInfoList.empty()) {
			UnifiedDataStore *dataStore =
			  UnifiedDataStore::getInstance();
			dataStore->addEventList(arg.eventInfoList);
		}
	}

private:
	struct UpdateTriggerArg {
		const size_t            triggerIdx;
		const TriggerStatusType status;
		TriggerInfoList         triggerInfoList;
		EventInfoList           eventInfoList;
	};

	void updateTriggersForCollectingOK(UpdateTriggerArg &arg)
	{
		// Since collecting has successed, all the self triggers
		// should be OK.
		for (size_t i = 0; i < numArmTriggers; i++) {
			if (!isUsed(i))
				continue;
			setTriggerToStatusOK(arg, armTriggers[i]);
		}
	}

	void updateTriggersForCollectingFailure(UpdateTriggerArg &arg)
	{
		if (!isUsed(arg.triggerIdx)) {
			MLPL_WARN("Try to update the unused trigger: %zd\n",
			          arg.triggerIdx);
			return;
		}

		ArmTrigger &armTrigger = armTriggers[arg.triggerIdx];
		if (armTrigger.status == arg.status)
			return;

		// update the target trigger
		armTrigger.status = arg.status;
		obj->createTrigger(armTrigger, arg.triggerInfoList);
		if (armTrigger.status != TRIGGER_STATUS_UNKNOWN)
			obj->createEvent(armTrigger, arg.eventInfoList);

		updateLowerTriggers(arg);
		updateUpperTriggers(arg);
	}

	void updateLowerTriggers(UpdateTriggerArg &arg)
	{
		for (size_t i = arg.triggerIdx + 1; i < numArmTriggers; i++) {
			if (!isUsed(i))
				continue;
			setTriggerToStatusOK(arg, armTriggers[i]);
		}
	}

	void updateUpperTriggers(UpdateTriggerArg &arg)
	{
		for (size_t i = 0; i < arg.triggerIdx; i++) {
			if (!isUsed(i))
				continue;
			setTriggerToStatusUnknown(arg, armTriggers[i]);
		}
	}

	void setTriggerToStatusOK(UpdateTriggerArg &arg, ArmTrigger &armTrigger)
	{
		const TriggerStatusType prevStat = armTrigger.status;
		if (armTrigger.status != TRIGGER_STATUS_OK) {
			armTrigger.status = TRIGGER_STATUS_OK;
			obj->createTrigger(armTrigger, arg.triggerInfoList);
		}
		if (prevStat == TRIGGER_STATUS_PROBLEM)
			obj->createEvent(armTrigger, arg.eventInfoList);
	}

	void setTriggerToStatusUnknown(UpdateTriggerArg &arg,
	                               ArmTrigger &armTrigger)
	{
		if (armTrigger.status == TRIGGER_STATUS_OK) {
			armTrigger.status = TRIGGER_STATUS_UNKNOWN;
			obj->createTrigger(armTrigger, arg.triggerInfoList);
		}
	}
};

ArmUtils::ArmUtils(const MonitoringServerInfo &serverInfo,
                   ArmTrigger *armTriggers, const size_t &numArmTriggers)
: m_impl(new Impl(this, serverInfo, armTriggers, numArmTriggers))
{
}

ArmUtils::~ArmUtils()
{
}

void ArmUtils::initializeArmTriggers(void)
{
	m_impl->initializeArmTriggers();
}

bool ArmUtils::isArmTriggerUsed(const size_t &triggerIdx)
{
	return m_impl->isUsed(triggerIdx);
}

void ArmUtils::createTrigger(
  const ArmTrigger &armTrigger, TriggerInfoList &triggerInfoList)
{
	TriggerInfo triggerInfo;

	triggerInfo.serverId = m_impl->serverInfo.id;
	triggerInfo.lastChangeTime =
	  SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	triggerInfo.globalHostId = m_impl->globalHostId;
	triggerInfo.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
	triggerInfo.hostName = StringUtils::sprintf(
	  "%s%s", m_impl->serverInfo.hostName.c_str(),
	  SERVER_SELF_MONITORING_SUFFIX);
	triggerInfo.id       = armTrigger.triggerId;
	triggerInfo.brief    = armTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status   = armTrigger.status;
	triggerInfo.validity  = TRIGGER_VALID_SELF_MONITORING;

	triggerInfoList.push_back(triggerInfo);
}

void ArmUtils::createEvent(
  const ArmTrigger &armTrigger, EventInfoList &eventInfoList)
{
	EventInfo eventInfo;

	eventInfo.serverId = m_impl->serverInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	eventInfo.globalHostId = m_impl->globalHostId;
	eventInfo.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
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
	svHostDef.hostIdInServer = MONITORING_SELF_LOCAL_HOST_ID;
	svHostDef.name = StringUtils::sprintf("%s%s",
	  m_impl->serverInfo.hostName.c_str(), SERVER_SELF_MONITORING_SUFFIX);
	svHostDef.status = HOST_STAT_SELF_MONITOR;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HatoholError err = dataStore->upsertHost(svHostDef,
	                                         &m_impl->globalHostId);
	if (err != HTERR_OK) {
		MLPL_ERR("Failed to register a host for self monitoring: "
		         "(%d) %s.", err.getCode(), err.getCodeName().c_str());
	}
}

void ArmUtils::updateTriggerStatus(
  const size_t &triggerIdx, const TriggerStatusType &status)
{
	m_impl->updateTriggerStatus(triggerIdx, status);
}
