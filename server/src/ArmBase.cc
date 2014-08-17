/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <Logger.h>
#include <AtomicValue.h>
#include "ArmBase.h"
#include "HatoholException.h"
#include "DBClientHatohol.h"
#include "UnifiedDataStore.h"

using namespace std;
using namespace mlpl;

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

	DBClientHatohol      dbClientHatohol;

	oneProcEndTrigger    oneProcTriggerTable[COLLECT_NG_KIND_NUM];

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

	Signal updatedSignal;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmBase::ArmBase(
  const string &name, const MonitoringServerInfo &serverInfo)
: m_impl(new Impl(name, serverInfo))
{
	setInitialTrrigerTable();
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

void ArmBase::setUseTrigger(OneProcEndType type)
{
	m_impl->oneProcTriggerTable[type].statusType = TRIGGER_STATUS_UNKNOWN;
}

void ArmBase::setInitialTrrigerTable(void)
{
	m_impl->oneProcTriggerTable[COLLECT_NG_PERSER_ERROR].statusType =
		TRIGGER_STATUS_ALL;
	m_impl->oneProcTriggerTable[COLLECT_NG_PERSER_ERROR].triggerId =
		PARSER_ERROR_SERVER_TRIGGERID_NG;
	m_impl->oneProcTriggerTable[COLLECT_NG_PERSER_ERROR].msg = 
		StringUtils::sprintf("Connection failure due to parse error");

	m_impl->oneProcTriggerTable[COLLECT_NG_DISCONNECT].statusType = 
		TRIGGER_STATUS_ALL;
	m_impl->oneProcTriggerTable[COLLECT_NG_DISCONNECT].triggerId = 
		DISCONNECT_SERVER_TRIGGERID_NG;
	m_impl->oneProcTriggerTable[COLLECT_NG_DISCONNECT].msg = 
		StringUtils::sprintf("Connection failure due to disconnection");

	m_impl->oneProcTriggerTable[COLLECT_NG_INTERNAL_ERROR].statusType =
		TRIGGER_STATUS_ALL;
	m_impl->oneProcTriggerTable[COLLECT_NG_INTERNAL_ERROR].triggerId =
		INTERNAL_ERROR_SERVER_TRIGGERID_NG;
	m_impl->oneProcTriggerTable[COLLECT_NG_INTERNAL_ERROR].msg = 
		StringUtils::sprintf("Connection failure due to internal error");
}

void ArmBase::setInitialTrrigerStaus(void)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfo triggerInfo;
	TriggerInfoList triggerInfoList;

	for (int i=0;i<COLLECT_NG_KIND_NUM;i++){
		if (TRIGGER_STATUS_UNKNOWN == m_impl->oneProcTriggerTable[i].statusType){
			triggerInfo.serverId = svInfo.id; 
			clock_gettime(CLOCK_REALTIME,&triggerInfo.lastChangeTime);
			triggerInfo.hostId = svInfo.id;
			triggerInfo.hostName = svInfo.hostName.c_str();
			triggerInfo.id = m_impl->oneProcTriggerTable[i].triggerId;
			triggerInfo.status = TRIGGER_STATUS_OK;
			triggerInfo.severity = TRIGGER_SEVERITY_INFO;
			triggerInfo.brief = m_impl->oneProcTriggerTable[i].msg;
		
			triggerInfoList.push_back(triggerInfo);
		}
	}
	m_impl->dbClientHatohol.addTriggerInfoList(triggerInfoList);
}

void ArmBase::createTriggerInfo(int i,TriggerInfoList &triggerInfoList)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	TriggerInfo triggerInfo;

	triggerInfo.serverId = svInfo.id;
	clock_gettime(CLOCK_REALTIME,&triggerInfo.lastChangeTime);
	triggerInfo.hostId = svInfo.id;
	triggerInfo.hostName = svInfo.hostName.c_str();
	triggerInfo.id = m_impl->oneProcTriggerTable[i].triggerId;
	triggerInfo.brief = m_impl->oneProcTriggerTable[i].msg;
	triggerInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	triggerInfo.status = m_impl->oneProcTriggerTable[i].statusType;

	triggerInfoList.push_back(triggerInfo);
}

void ArmBase::createEventInfo(int i,EventInfoList &eventInfoList)
{
	const MonitoringServerInfo &svInfo = getServerInfo();
	EventInfo eventInfo;

	eventInfo.serverId = svInfo.id;
	eventInfo.id = DISCONNECT_SERVER_EVENTID_TYPE;
	clock_gettime(CLOCK_REALTIME,&eventInfo.time);
	eventInfo.hostId = svInfo.id;
	eventInfo.triggerId = m_impl->oneProcTriggerTable[i].triggerId;
	eventInfo.severity = TRIGGER_SEVERITY_EMERGENCY;
	eventInfo.status = m_impl->oneProcTriggerTable[i].statusType;
	if (m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_OK) {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else {
		eventInfo.type = EVENT_TYPE_BAD;
	}
	eventInfoList.push_back(eventInfo);
}

void ArmBase::setServerConnectStaus(bool enable,OneProcEndType type)
{
	TriggerInfoList triggerInfoList;
	EventInfoList eventInfoList;

	if (!enable) {
		if (m_impl->oneProcTriggerTable[type].statusType == TRIGGER_STATUS_PROBLEM)
			return;
	}

	if (enable){
		for (int i=0;i<COLLECT_NG_KIND_NUM;i++){
			if (m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_PROBLEM ){
				m_impl->oneProcTriggerTable[i].statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(i,triggerInfoList);
				createEventInfo(i,eventInfoList);
			} else if (m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_UNKNOWN ){
				m_impl->oneProcTriggerTable[i].statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(i,triggerInfoList);
			}				
		}
	}
	else {
		m_impl->oneProcTriggerTable[type].statusType = TRIGGER_STATUS_PROBLEM;
		createTriggerInfo(static_cast<int>(type),triggerInfoList);
		createEventInfo(static_cast<int>(type),eventInfoList);

		for (int i=static_cast<int>(type)+1;i<COLLECT_NG_KIND_NUM;i++){
			if (m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_PROBLEM ){
				m_impl->oneProcTriggerTable[i].statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(i,triggerInfoList);
				createEventInfo(i,eventInfoList);
			} else if ( m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_UNKNOWN ){
				m_impl->oneProcTriggerTable[i].statusType = TRIGGER_STATUS_OK;
				createTriggerInfo(i,triggerInfoList);
			}
		}
		
		for (int i=0;i<type;i++){
			if (m_impl->oneProcTriggerTable[i].statusType == TRIGGER_STATUS_OK){
				m_impl->oneProcTriggerTable[i].statusType = TRIGGER_STATUS_UNKNOWN;
				createTriggerInfo(i,triggerInfoList);
			}
		}
	}

	m_impl->dbClientHatohol.addTriggerInfoList(triggerInfoList);
	
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	dataStore->addEventList(eventInfoList);
}

gpointer ArmBase::mainThread(HatoholThreadArg *arg)
{
	ArmWorkingStatus beforeFailureStatus = ARM_WORK_STAT_INIT;
	while (!hasExitRequest()) {
		int sleepTime = m_impl->getSecondsToNextPolling();
		OneProcEndType oneProcEndType = mainThreadOneProc();
		if (oneProcEndType == COLLECT_OK) {
			m_impl->armStatus.logSuccess();
			m_impl->lastFailureStatus = ARM_WORK_STAT_OK;
		} else {
			sleepTime = getRetryInterval();
			m_impl->armStatus.logFailure(m_impl->lastFailureComment,
			                            m_impl->lastFailureStatus);
			m_impl->lastFailureComment.clear();
			m_impl->lastFailureStatus = ARM_WORK_STAT_FAILURE;
		}
		if (beforeFailureStatus == ARM_WORK_STAT_INIT ){
			setInitialTrrigerStaus();
			beforeFailureStatus = ARM_WORK_STAT_OK;
		}
		if ( m_impl->lastFailureStatus != ARM_WORK_STAT_OK ){
			setServerConnectStaus(false,oneProcEndType);
		}else{
			if (beforeFailureStatus != m_impl->lastFailureStatus)
				setServerConnectStaus(true,oneProcEndType);
		}
		beforeFailureStatus = m_impl->lastFailureStatus;

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
