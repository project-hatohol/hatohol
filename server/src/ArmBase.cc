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
#include <queue>
#include <Logger.h>
#include <AtomicValue.h>
#include "ArmUtils.h"
#include "ArmBase.h"
#include "HatoholException.h"
#include "DBTablesMonitoring.h"
#include "UnifiedDataStore.h"
#include "ThreadLocalDBCache.h"

using namespace std;
using namespace mlpl;

typedef enum {
	UPDATE_POLLING,
	UPDATE_ITEM_REQUEST,
	UPDATE_HISTORY_REQUEST,
	UPDATE_TRIGGER_REQUEST,
} UpdateType;

struct FetcherJob
{
	UpdateType   updateType;
	ClosureBase *closure;

	struct HistoryQuery {
		ItemInfo itemInfo;
		time_t beginTime;
		time_t endTime;
		HistoryQuery(const ItemInfo _itemInfo,
			     const time_t &_beginTime, const time_t &_endTime)
		: itemInfo(_itemInfo), beginTime(_beginTime), endTime(_endTime)
		{
		}
	} *historyQuery;

	FetcherJob(Closure0 *_closure, const UpdateType &type)
	: updateType(type), closure(_closure),
	  historyQuery(NULL)
	{
	}

	FetcherJob(Closure1<HistoryInfoVect> *_closure,
		   const ItemInfo &_itemInfo,
		   const time_t &_beginTime, const time_t &_endTime)
	: updateType(UPDATE_HISTORY_REQUEST), closure(_closure),
	  historyQuery(new HistoryQuery(_itemInfo, _beginTime, _endTime))
	{
	}

	~FetcherJob()
	{
		delete closure;
		delete historyQuery;
	}

	void run(void)
	{
		HATOHOL_ASSERT(updateType == UPDATE_ITEM_REQUEST,
			       "Invalid updateType: %d\n", updateType);

		if (!closure)
			return;
		Closure0 *fetchItemClosure =
		  dynamic_cast<Closure0*>(closure);
		HATOHOL_ASSERT(fetchItemClosure, "Invalid closure\n");

		(*fetchItemClosure)();
	}

	void run(const HistoryInfoVect &historyInfoVect)
	{
		HATOHOL_ASSERT(updateType == UPDATE_HISTORY_REQUEST,
			       "Invalid updateType: %d\n", updateType);

		Closure1<HistoryInfoVect> *fetchHistoryClosure =
		  dynamic_cast<Closure1<HistoryInfoVect> *>(closure);
		HATOHOL_ASSERT(fetchHistoryClosure, "Invalid closure\n");

		(*fetchHistoryClosure)(historyInfoVect);
	}

	void run(const UpdateType &updateType)
	{
		HATOHOL_ASSERT(updateType == UPDATE_TRIGGER_REQUEST,
			       "Invalid updateType: %d\n", updateType);

		if (!closure)
			return;
		if (updateType == UPDATE_TRIGGER_REQUEST) {
			Closure0 *fetchItemClosure =
				dynamic_cast<Closure0*>(closure);
			HATOHOL_ASSERT(fetchItemClosure, "Invalid closure\n");
			
			(*fetchItemClosure)();
		}
	}
};

struct ArmBase::Impl
{
	string               name;
	MonitoringServerInfo serverInfo; // we have the copy.
	ArmUtils             utils;
	timespec             lastPollingTime;
	sem_t                sleepSemaphore;
	AtomicValue<bool>    exitRequest;
	bool                 isCopyOnDemandEnabled;
	ReadWriteLock        rwlock;
	ArmStatus            armStatus;
	string               lastFailureComment;
	ArmWorkingStatus     lastFailureStatus;
	queue<FetcherJob *>  jobQueue;

	ArmUtils::ArmTrigger armTriggers[NUM_COLLECT_NG_KIND];

	Impl(const string &_name,
	               const MonitoringServerInfo &_serverInfo)
	: name(_name),
	  serverInfo(_serverInfo),
	  utils(serverInfo, armTriggers, NUM_COLLECT_NG_KIND),
	  exitRequest(false),
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
		clearJob();
		if (sem_destroy(&sleepSemaphore) != 0)
			MLPL_ERR("Failed to call sem_destroy(): %d\n", errno);
	}

	void stampLastPollingTime(void)
	{
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

	void pushJob(FetcherJob *job)
	{
		rwlock.writeLock();
		jobQueue.push(job);
		rwlock.unlock();
	}

	FetcherJob *popJob(void)
	{
		FetcherJob *job = NULL;
		rwlock.writeLock();
		if (!jobQueue.empty()) {
			job = jobQueue.front();
			jobQueue.pop();
		}
		rwlock.unlock();
		return job;
	}

	void clearJob(void)
	{
		rwlock.writeLock();
		while (!jobQueue.empty()) {
			FetcherJob *job = jobQueue.front();
			if (job->updateType == UPDATE_ITEM_REQUEST) {
				job->run();
			} else if (job->updateType == UPDATE_HISTORY_REQUEST) {
				HistoryInfoVect historyInfoVect;
				job->run(historyInfoVect);
			} else 	if (job->updateType == UPDATE_TRIGGER_REQUEST) {
				job->run(job->updateType);
			}
			jobQueue.pop();
			delete job;
		}
		rwlock.unlock();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ArmBase::ArmBase(
  const string &name, const MonitoringServerInfo &serverInfo)
: m_impl(new Impl(name, serverInfo))
{
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

void ArmBase::fetchItems(Closure0 *closure)
{
	m_impl->pushJob(new FetcherJob(closure, UPDATE_ITEM_REQUEST));
	if (sem_post(&m_impl->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
}

void ArmBase::fetchTriggers(Closure0 *closure)
{
	m_impl->pushJob(new FetcherJob(closure, UPDATE_TRIGGER_REQUEST));
	if (sem_post(&m_impl->sleepSemaphore) == -1)
		MLPL_ERR("Failed to call sem_post: %d\n", errno);
}

void ArmBase::fetchHistory(const ItemInfo &itemInfo,
			   const time_t &beginTime,
			   const time_t &endTime,
			   Closure1<HistoryInfoVect> *closure)
{
	m_impl->pushJob(new FetcherJob(closure, itemInfo, beginTime, endTime));
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
	m_impl->armTriggers[type].status    = TRIGGER_STATUS_UNKNOWN;
	m_impl->armTriggers[type].triggerId = trrigerId;
	m_impl->armTriggers[type].msg       = hatoholError.getMessage();
}

bool ArmBase::hasTrigger(const ArmPollingResult &type)
{
	return m_impl->utils.isArmTriggerUsed(type);
}

void ArmBase::setInitialTriggerStatus(void)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();

	TriggerInfoList triggerInfoList;

	for (int i = 0; i < NUM_COLLECT_NG_KIND; i++) {
		if (!m_impl->utils.isArmTriggerUsed(i))
			continue;
		ArmUtils::ArmTrigger &armTrigger = m_impl->armTriggers[i];
		if (armTrigger.status != TRIGGER_STATUS_UNKNOWN)
			continue;
		m_impl->utils.createTrigger(armTrigger, triggerInfoList);
	}

	if (!triggerInfoList.empty()) {
		m_impl->utils.registerSelfMonitoringHost();
		dbMonitoring.addTriggerInfoList(triggerInfoList);
	}
}

void ArmBase::setServerConnectStatus(const ArmPollingResult &type)
{
	TriggerStatusType status;
	if (type == COLLECT_OK)
		status = TRIGGER_STATUS_OK;
	else
		status = TRIGGER_STATUS_PROBLEM;
	m_impl->utils.updateTriggerStatus(type, status);
}

gpointer ArmBase::mainThread(HatoholThreadArg *arg)
{
	ArmWorkingStatus previousArmWorkStatus = ARM_WORK_STAT_INIT;
	while (!hasExitRequest()) {
		FetcherJob *job = m_impl->popJob();
		UpdateType updateType = job ? job->updateType : UPDATE_POLLING;
		int sleepTime = m_impl->getSecondsToNextPolling();

		ArmPollingResult armPollingResult;
		if (updateType == UPDATE_ITEM_REQUEST) {
			HATOHOL_ASSERT(job, "Invalid FetcherJob");
			armPollingResult = mainThreadOneProcFetchItems();
			job->run();
		} else if (updateType == UPDATE_HISTORY_REQUEST) {
			HATOHOL_ASSERT(job && job->historyQuery,
				       "Invalid FetcherJob");
			HistoryInfoVect historyInfoVect;
			FetcherJob::HistoryQuery &query = *job->historyQuery;
			armPollingResult =
			  mainThreadOneProcFetchHistory(
			    historyInfoVect, query.itemInfo,
			    query.beginTime, query.endTime);
			job->run(historyInfoVect);
		} else 	if (updateType == UPDATE_TRIGGER_REQUEST) {
			HATOHOL_ASSERT(job, "Invalid FetcherJob");
			armPollingResult = mainThreadOneProcFetchTriggers();
			job->run(updateType);
		} else {
			armPollingResult = mainThreadOneProc();
		}
		delete job;

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
			setInitialTriggerStatus();
		}
		if (m_impl->lastFailureStatus != ARM_WORK_STAT_OK) {
			setServerConnectStatus(armPollingResult);
		} else {
			if (previousArmWorkStatus != m_impl->lastFailureStatus)
				setServerConnectStatus(armPollingResult);
		}
		previousArmWorkStatus = m_impl->lastFailureStatus;

		if (updateType == UPDATE_POLLING)
			m_impl->stampLastPollingTime();

		if (hasExitRequest())
			break;
		sleepInterruptible(sleepTime);
	}
	return NULL;
}

ArmBase::ArmPollingResult ArmBase::mainThreadOneProcFetchItems(void)
{
	return COLLECT_OK;
}

ArmBase::ArmPollingResult ArmBase::mainThreadOneProcFetchHistory(
  HistoryInfoVect &historyInfoVect,
 const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime)
{
	return COLLECT_OK;
}

ArmBase::ArmPollingResult ArmBase::mainThreadOneProcFetchTriggers(void)
{
	return COLLECT_OK;
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
