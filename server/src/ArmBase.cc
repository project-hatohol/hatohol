/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <Logger.h>
#include <AtomicValue.h>
#include "ArmBase.h"
#include "HatoholException.h"
#include "DBTablesMonitoring.h"
#include "UnifiedDataStore.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

const char *SERVER_SELF_MONITORING_SUFFIX = "_SELF";

struct ArmBase::Impl
{
	string               name;
	MonitoringServerInfo serverInfo; // we have the copy.
	timespec             lastPollingTime;
	sem_t                sleepSemaphore;
	AtomicValue<bool>    exitRequest;
	ArmBase::UpdateType  updateType;
	bool                 isCopyOnDemandEnabled;
	ReadWriteLock        rwlock;
	ArmStatus            armStatus;
	string               lastFailureComment;
	ArmWorkingStatus     lastFailureStatus;

	ArmResultTriggerInfo ArmResultTriggerTable[NUM_COLLECT_NG_KIND];

	Impl(const string &_name,
	               const MonitoringServerInfo &_serverInfo)
	: name(_name),
	  serverInfo(_serverInfo),
	  exitRequest(false),
	  updateType(UPDATE_POLLING),
	  isCopyOnDemandEnabled(false),
	  lastFailureStatus(ARM_WORK_STAT_FAILURE)
	{
		static const int PSHARED = 1;
		HATOHOL_ASSERT(sem_init(&sleepSemaphore, PSHARED, 0) == 0,
		             "Failed to sem_init(): %d\n", errno);
		lastPollingTime.tv_sec = 0;
		lastPollingTime.tv_nsec = 0;
	}

	virtual ~Impl()
	{
		if (sem_destroy(&sleepSemaphore) != 0)
			MLPL_ERR("Failed to call sem_destroy(): %d\n", errno);
	}

	void stampLastPollingTime(void)
	{
		if (getUpdateType() != UPDATE_POLLING)
			return;

		int result = clock_gettime(CLOCK_REALTIME,
					   &lastPollingTime);
		if (result == 0) {
			MLPL_DBG("lastPollingTime: %ld\n",
				 lastPollingTime.tv_sec);
		} else {
			MLPL_ERR("Failed to call clock_gettime: %d\n",
				 errno);
			lastPollingTime.tv_sec = 0;
			lastPollingTime.tv_nsec = 0;
		}
	}

	int getSecondsToNextPolling(void)
	{
		time_t interval = serverInfo.pollingIntervalSec;
		timespec currentTime;

		int result = clock_gettime(CLOCK_REALTIME, &currentTime);
		if (result == 0 && lastPollingTime.tv_sec != 0) {
			time_t elapsed
			  = currentTime.tv_sec - lastPollingTime.tv_sec;
			if (elapsed < interval)
				interval -= elapsed;
		}

		if (interval <= 0 || interval > serverInfo.pollingIntervalSec)
			interval = serverInfo.pollingIntervalSec;

		return interval;
	}

	UpdateType getUpdateType(void)
	{
		rwlock.readLock();
		ArmBase::UpdateType type = updateType;
		rwlock.unlock();
		return type;
	}

	void setUpdateType(ArmBase::UpdateType type)
	{
		rwlock.writeLock();
		updateType = type;
		rwlock.unlock();
	}

	void setInitialTriggerTable(void)
	{
		for (int i = 0; i < NUM_COLLECT_NG_KIND; i++)
			ArmResultTriggerTable[i].statusType = TRIGGER_STATUS_ALL;
	}

	Signal updatedSignal;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmBase::ArmBase(
  const string &name, const MonitoringServerInfo &serverInfo)
: m_impl(new Impl(name, serverInfo))
{
	m_impl->setInitialTriggerTable();
}

ArmBase::~ArmBase()
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	MLPL_INFO("%s [%d:%s]: destruction: completed.\n",
	          getName().c_str(), svInfo.id, svInfo.hostName.c_str());
}

void ArmBase::start(void)
{
	HatoholThreadBase::start();
	m_impl->armStatus.setRunningStatus(true);
}

void ArmBase::waitExit(void)
{
	HatoholThreadBase::waitExit();
	m_impl->armStatus.setRunningStatus(false);
}

bool ArmBase::isFetchItemsSupported(void) const
{
	return true;
}

void ArmBase::fetchItems(ClosureBase *closure)
{
	setUpdateType(UPDATE_ITEM_REQUEST);
	m_impl->updatedSignal.connect(closure);
	if (sem_post(&m_impl->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
}

void ArmBase::setPollingInterval(int sec)
{
	m_impl->serverInfo.pollingIntervalSec = sec;
}

int ArmBase::getPollingInterval(void) const
{
	return m_impl->serverInfo.pollingIntervalSec;
}

int ArmBase::getRetryInterval(void) const
{
	return m_impl->serverInfo.retryIntervalSec;
}

const string &ArmBase::getName(void) const
{
	return m_impl->name;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ArmBase::requestExitAndWait(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	
	MLPL_INFO("%s [%d:%s]: requested to exit.\n",
	          getName().c_str(), svInfo.id, svInfo.hostName.c_str());

	// wait for the finish of the thread
	requestExit();
	waitExit();
}

bool ArmBase::hasExitRequest(void) const
{
	return m_impl->exitRequest;
}

void ArmBase::requestExit(void)
{
	m_impl->exitRequest = true;

	// to return immediately from the waiting.
	if (sem_post(&m_impl->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
}

const MonitoringServerInfo &ArmBase::getServerInfo(void) const
{
	return m_impl->serverInfo;
}

const ArmStatus &ArmBase::getArmStatus(void) const
{
	return m_impl->armStatus;
}

void ArmBase::sleepInterruptible(int sleepTime)
{
	// sleep with timeout
	timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		sleep(10); // to avoid burnup
		return;
	}
	ts.tv_sec += sleepTime;
retry:
	int result = sem_timedwait(&m_impl->sleepSemaphore, &ts);
	if (result == -1) {
		if (errno == ETIMEDOUT)
			; // This is normal case
		else if (errno == EINTR)
			goto retry;
		else
			MLPL_ERR("sem_timedwait(): errno: %d\n", errno);
	}
	// The up of the semaphore is done only from the destructor.
}

ArmBase::UpdateType ArmBase::getUpdateType(void) const
{
	return m_impl->getUpdateType();
}

void ArmBase::setUpdateType(UpdateType updateType)
{
	m_impl->setUpdateType(updateType);
}

bool ArmBase::getCopyOnDemandEnabled(void) const
{
	return m_impl->isCopyOnDemandEnabled;
}

void ArmBase::setCopyOnDemandEnabled(bool enable)
{
	m_impl->isCopyOnDemandEnabled = enable;
}

void ArmBase::registerAvailableTrigger(const ArmPollingResult &type,
				       const TriggerIdType  &trrigerId,
				       const HatoholError   &hatoholError)
{
	m_impl->ArmResultTriggerTable[type].statusType = TRIGGER_STATUS_UNKNOWN;
	m_impl->ArmResultTriggerTable[type].triggerId = trrigerId;
	m_impl->ArmResultTriggerTable[type].msg = hatoholError.getMessage().c_str();
}

void ArmBase::setInitialTrrigerStatus(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	const MonitoringServerInfo &svInfo = getServerInfo();

	HostInfo hostInfo;
	hostInfo.serverId = svInfo.id;
	hostInfo.id = MONITORING_SERVER_SELF_ID;
	hostInfo.hostName = 
		StringUtils::sprintf("%s%s", svInfo.hostName.c_str(),
				     SERVER_SELF_MONITORING_SUFFIX);
	dbMonitoring.addHostInfo(&hostInfo);

	TriggerInfo triggerInfo;
	TriggerInfoList triggerInfoList;

	for (int i = 0; i < NUM_COLLECT_NG_KIND; i++) {
		ArmResultTriggerInfo &trgInfo = m_impl->ArmResultTriggerTable[i];
		if (TRIGGER_STATUS_UNKNOWN == trgInfo.statusType) {
			createTriggerInfo(trgInfo, triggerInfoList);
		}
	}
	dbMonitoring.addTriggerInfoList(triggerInfoList);
}

void ArmBase::createTriggerInfo(const ArmResultTriggerInfo &resTrigger,
				TriggerInfoList &triggerInfoList)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfo triggerInfo;

	triggerInfo.serverId = svInfo.id;
	triggerInfo.lastChangeTime = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	triggerInfo.hostId = MONITORING_SERVER_SELF_ID;
	triggerInfo.hostName = 
		StringUtils::sprintf("%s%s", svInfo.hostName.c_str(),
				     SERVER_SELF_MONITORING_SUFFIX);
	triggerInfo.id = resTrigger.triggerId;
	triggerInfo.brief = resTrigger.msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status = resTrigger.statusType;

	triggerInfoList.push_back(triggerInfo);
}

void ArmBase::createEventInfo(const ArmResultTriggerInfo &resTrigger,
			      EventInfoList &eventInfoList)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	EventInfo eventInfo;

	eventInfo.serverId = svInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENT_ID;
	eventInfo.time = SmartTime(SmartTime::INIT_CURR_TIME).getAsTimespec();
	eventInfo.hostId = MONITORING_SERVER_SELF_ID;
	eventInfo.triggerId = resTrigger.triggerId;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.status = resTrigger.statusType;
	if (resTrigger.statusType == TRIGGER_STATUS_OK) {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else {
		eventInfo.type = EVENT_TYPE_BAD;
	}
	eventInfoList.push_back(eventInfo);
}

void ArmBase::setServerConnectStatus(const ArmPollingResult &type)
{
	TriggerInfoList triggerInfoList;
	EventInfoList eventInfoList;

	if (type == COLLECT_OK) {
		for (int i = 0; i < NUM_COLLECT_NG_KIND; i++) {
			ArmResultTriggerInfo &trgInfo = m_impl->ArmResultTriggerTable[i];
			if (trgInfo.statusType == TRIGGER_STATUS_PROBLEM) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(trgInfo, triggerInfoList);
				createEventInfo(trgInfo, eventInfoList);
			} else if (trgInfo.statusType == TRIGGER_STATUS_UNKNOWN) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(trgInfo, triggerInfoList);
			}				
		}
	}
	else {
		if (m_impl->ArmResultTriggerTable[type].statusType == TRIGGER_STATUS_PROBLEM)
			return;

		m_impl->ArmResultTriggerTable[type].statusType = TRIGGER_STATUS_PROBLEM;
		createTriggerInfo(m_impl->ArmResultTriggerTable[type], triggerInfoList);
		createEventInfo(m_impl->ArmResultTriggerTable[type], eventInfoList);

		for (int i = static_cast<int>(type) + 1; i < NUM_COLLECT_NG_KIND; i++) {
			ArmResultTriggerInfo &trgInfo = m_impl->ArmResultTriggerTable[i];
			if (trgInfo.statusType == TRIGGER_STATUS_PROBLEM) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(trgInfo, triggerInfoList);
				createEventInfo(trgInfo, eventInfoList);
			} else if (trgInfo.statusType == TRIGGER_STATUS_UNKNOWN) {
				trgInfo.statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(trgInfo, triggerInfoList);
			}
		}
		for (int i = 0; i < type; i++) {
			ArmResultTriggerInfo &trgInfo = m_impl->ArmResultTriggerTable[i];
			if (trgInfo.statusType == TRIGGER_STATUS_OK) {
				trgInfo.statusType = TRIGGER_STATUS_UNKNOWN;
				createTriggerInfo(trgInfo, triggerInfoList);
			}
		}
	}

	if (!triggerInfoList.empty()) {
		ThreadLocalDBCache cache;
		cache.getMonitoring().addTriggerInfoList(triggerInfoList);
	}
	if (!eventInfoList.empty()) {
		UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
		dataStore->addEventList(eventInfoList);
	}
}

gpointer ArmBase::mainThread(HatoholThreadArg *arg)
{
	ArmWorkingStatus previousArmWorkStatus = ARM_WORK_STAT_INIT;
	while (!hasExitRequest()) {
		int sleepTime = m_impl->getSecondsToNextPolling();
		ArmPollingResult armPollingResult = mainThreadOneProc();
		if (armPollingResult == COLLECT_OK) {
			m_impl->armStatus.logSuccess();
			m_impl->lastFailureStatus = ARM_WORK_STAT_OK;
		} else {
			sleepTime = getRetryInterval();
			m_impl->armStatus.logFailure(m_impl->lastFailureComment,
			                            m_impl->lastFailureStatus);
			m_impl->lastFailureComment.clear();
			m_impl->lastFailureStatus = ARM_WORK_STAT_FAILURE;
		}
		if (previousArmWorkStatus == ARM_WORK_STAT_INIT) {
			setInitialTrrigerStatus();
		}
		if (m_impl->lastFailureStatus != ARM_WORK_STAT_OK) {
			setServerConnectStatus(armPollingResult);
		} else {
			if (previousArmWorkStatus != m_impl->lastFailureStatus)
				setServerConnectStatus(armPollingResult);
		}
		previousArmWorkStatus = m_impl->lastFailureStatus;

		m_impl->stampLastPollingTime();
		m_impl->setUpdateType(UPDATE_POLLING);
		m_impl->updatedSignal();
		m_impl->updatedSignal.clear();

		if (hasExitRequest())
			break;
		sleepInterruptible(sleepTime);
	}
	return NULL;
}

void ArmBase::getArmStatus(ArmStatus *&armStatus)
{
	armStatus = &m_impl->armStatus;
}

void ArmBase::setFailureInfo(const std::string &comment,
                             const ArmWorkingStatus &status)
{
	m_impl->lastFailureComment = comment;
	m_impl->lastFailureStatus = status;
}
