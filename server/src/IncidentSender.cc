/*
 * Copyright (C) 2014-2016 Project Hatohol
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

#include "IncidentSender.h"
#include "StringUtils.h"
#include "LabelUtils.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include <mutex>
#include "SimpleSemaphore.h"
#include "Reaper.h"
#include "AtomicValue.h"
#include <unistd.h>
#include <time.h>
#include <queue>

using namespace std;
using namespace mlpl;

static const size_t DEFAULT_RETRY_LIMIT = 3;
static const unsigned int DEFAULT_RETRY_INTERVAL_MSEC = 5000;

struct IncidentSender::Job
{
	UserIdType userId;
	EventInfo *eventInfo;
	IncidentInfo *incidentInfo;
	string comment;
	CreateIncidentCallback createCallback;
	UpdateIncidentCallback updateCallback;
	void *userData;

	Job(const EventInfo &_eventInfo,
	    CreateIncidentCallback _callback = NULL,
	    void *_userData = NULL,
	    const UserIdType userId = USER_ID_SYSTEM)
	: userId(userId),
	  eventInfo(new EventInfo(_eventInfo)), incidentInfo(new IncidentInfo()),
	  createCallback(_callback), updateCallback(NULL),
	  userData(_userData)
	{
	}

	Job(const IncidentInfo &_incidentInfo,
	    const string &_comment,
	    UpdateIncidentCallback _callback = NULL,
	    void *_userData = NULL,
	    const UserIdType userId = USER_ID_SYSTEM)
	: userId(userId),
	  eventInfo(NULL), incidentInfo(new IncidentInfo(_incidentInfo)),
	  comment(_comment),
	  createCallback(NULL), updateCallback(_callback),
	  userData(_userData)
	{
	}

	virtual ~Job()
	{
		delete eventInfo;
		delete incidentInfo;
	}

	void notifyStatus(const IncidentSender &sender,
			  const JobStatus &status) const
	{
		if (eventInfo && createCallback)
			createCallback(sender, *eventInfo, status, userData);
		else if (incidentInfo && updateCallback)
			updateCallback(sender, *incidentInfo, status, userData);
	}

	HatoholError send(IncidentSender &sender) const
	{
		HatoholError err(HTERR_NOT_IMPLEMENTED);
		if (eventInfo) {
			err = sender.send(*eventInfo, incidentInfo);
		} else if (incidentInfo) {
			err = sender.send(*incidentInfo, comment);
		}
		sender.setLastResult(err);
		return sender.getLastResult();
	}
};

struct IncidentSender::Impl
{
	IncidentSender &sender;
	IncidentTrackerInfo incidentTrackerInfo;
	std::mutex          queueLock;
	std::queue<Job*> queue;
	AtomicValue<Job*> runningJob;
	SimpleSemaphore jobSemaphore;
	size_t retryLimit;
	unsigned int retryIntervalMSec;
	AtomicValue<bool> trackerChanged;
	std::mutex   trackerLock;
	HatoholError lastResult;
	bool shouldRecordIncidentHistory;

	Impl(IncidentSender &_sender)
	: sender(_sender), runningJob(NULL), jobSemaphore(0),
	  retryLimit(DEFAULT_RETRY_LIMIT),
	  retryIntervalMSec(DEFAULT_RETRY_INTERVAL_MSEC),
	  shouldRecordIncidentHistory(false)
	{
	}

	~Impl()
	{
		queueLock.lock();
		while (!queue.empty()) {
			Job *job = queue.front();
			queue.pop();
			delete job;
		}
		queueLock.unlock();
	}

	void pushJob(Job *job)
	{
		queueLock.lock();
		queue.push(job);
		job->notifyStatus(sender, JOB_QUEUED);
		jobSemaphore.post();
		queueLock.unlock();
	}

	Job *popJob(void)
	{
		Job *job = NULL;
		queueLock.lock();
		if (!queue.empty()) {
			job = queue.front();
			queue.pop();
		}
		runningJob = job;
		if (job)
			job->notifyStatus(sender, JOB_STARTED);
		queueLock.unlock();
		return job;
	}

	Job *waitNextJob(void)
	{
		jobSemaphore.wait();
		if (sender.isExitRequested())
			return NULL;
		else
			return popJob();
	}

	void saveIncidentHistory(const Job &job)
	{
		SmartTime now(SmartTime::INIT_CURR_TIME);
		UnifiedDataStore *store = UnifiedDataStore::getInstance();
		IncidentHistory history;
		if (!job.incidentInfo) {
			MLPL_ERR("No incidentInto to save history!");
			return;
		}
		history.id = AUTO_INCREMENT_VALUE;
		history.unifiedEventId = job.incidentInfo->unifiedEventId;
		history.userId = job.userId;
		history.status = job.incidentInfo->status;
		history.comment = job.comment;
		history.createdAt.tv_sec = now.getAsTimespec().tv_sec;
		history.createdAt.tv_nsec = now.getAsTimespec().tv_nsec;
		store->addIncidentHistory(history);
	}

	HatoholError trySend(const Job &job) {
		HatoholError result;
		for (size_t i = 0; i <= retryLimit; i++) {
			result = job.send(sender);
			if (result == HTERR_OK)
				break;
			if (result != HTERR_FAILED_TO_SEND_INCIDENT)
				break;
			if (i == retryLimit)
				break;
			if (sender.isExitRequested())
				break;

			job.notifyStatus(sender, JOB_WAITING_RETRY);
			usleep(retryIntervalMSec * 1000);

			if (sender.isExitRequested())
				break;
			job.notifyStatus(sender, JOB_RETRYING);
		}
		if (result == HTERR_OK) {
			if (shouldRecordIncidentHistory)
				saveIncidentHistory(job);
			job.notifyStatus(sender, JOB_SUCCEEDED);
		} else {
			job.notifyStatus(sender, JOB_FAILED);
		}
		return result;
	}
};

IncidentSender::IncidentSender(
  const IncidentTrackerInfo &tracker, bool shouldRecordIncidentHistory)
: m_impl(new Impl(*this))
{
	m_impl->incidentTrackerInfo = tracker;
	m_impl->shouldRecordIncidentHistory = shouldRecordIncidentHistory;
}

IncidentSender::~IncidentSender()
{
	exitSync();
}

void IncidentSender::waitExit(void)
{
	m_impl->jobSemaphore.post();
	HatoholThreadBase::waitExit();
}

void IncidentSender::queue(const EventInfo &eventInfo,
			   CreateIncidentCallback callback, void *userData,
			   const UserIdType userId)
{
	Job *job = new Job(eventInfo, callback, userData, userId);
	m_impl->pushJob(job);
}

void IncidentSender::queue(const IncidentInfo &incidentInfo,
			   const string &comment,
			   UpdateIncidentCallback callback, void *userData,
			   const UserIdType userId)
{
	Job *job = new Job(incidentInfo, comment, callback, userData, userId);
	m_impl->pushJob(job);
}

void IncidentSender::setRetryLimit(const size_t &limit)
{
	m_impl->retryLimit = limit;
}

void IncidentSender::setRetryInterval(const unsigned int &msec)
{
	m_impl->retryIntervalMSec = msec;
}

bool IncidentSender::isIdling(void)
{
	lock_guard<mutex> lock(m_impl->queueLock);
	if (!m_impl->queue.empty())
		return false;
	return !m_impl->runningJob;
}

const IncidentTrackerInfo IncidentSender::getIncidentTrackerInfo(void)
{
	lock_guard<mutex> lock(m_impl->trackerLock);

	if (!m_impl->trackerChanged)
		return m_impl->incidentTrackerInfo;

	IncidentTrackerInfo trackerInfo;
	IncidentTrackerIdType trackerId = m_impl->incidentTrackerInfo.id;
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	if (dataStore->getIncidentTrackerInfo(trackerId, trackerInfo)) {
		m_impl->incidentTrackerInfo = trackerInfo;
		m_impl->trackerChanged = false;
		return m_impl->incidentTrackerInfo;
	}

	return m_impl->incidentTrackerInfo;
}

void IncidentSender::setOnChangedIncidentTracker(void)
{
	m_impl->trackerChanged = true;
}

bool IncidentSender::getServerInfo(const EventInfo &event,
				MonitoringServerInfo &server)
{
	ThreadLocalDBCache cache;
	DBTablesConfig &dbConfig = cache.getConfig();
	ServerQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(event.serverId);
	MonitoringServerInfoList servers;
	dbConfig.getTargetServers(servers, option);
	if (servers.empty())
		return false;
	server = *(servers.begin());
	return true;
}

static string getServerLabel(const EventInfo &event,
			     const MonitoringServerInfo *server = NULL)
{
	if (server)
		return server->getDisplayName();
	else
		return StringUtils::sprintf("Unknown:%" FMT_SERVER_ID,
					    event.serverId);
}

string IncidentSender::buildTitle(const EventInfo &event,
				  const MonitoringServerInfo *server)
{
	return StringUtils::sprintf("[%s %s] %s",
				    getServerLabel(event, server).c_str(),
				    event.hostName.c_str(),
				    event.brief.c_str());
}

string IncidentSender::buildDescription(const EventInfo &event,
					const MonitoringServerInfo *server)
{
	string desc;
	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);
	desc += StringUtils::sprintf(
		  "Server ID: %" FMT_SERVER_ID "\n",
		  event.serverId);
	if (server) {
		desc += StringUtils::sprintf(
			  "    Hostname:   \"%s\"\n"
			  "    IP Address: \"%s\"\n"
			  "    Nickname:   \"%s\"\n",
			  server->hostName.c_str(),
			  server->ipAddress.c_str(),
			  server->nickname.c_str());
	}
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Host ID: '%" FMT_LOCAL_HOST_ID "'\n"
		  "    Hostname:   \"%s\"\n",
		  event.hostIdInServer.c_str(), event.hostName.c_str());
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Event ID: %" FMT_EVENT_ID "\n"
		  "    Time:       \"%ld.%09ld (%s)\"\n"
		  "    Type:       \"%d (%s)\"\n"
		  "    Brief:      \"%s\"\n",
		  event.id.c_str(),
		  event.time.tv_sec, event.time.tv_nsec, timeString,
		  event.type,
		  LabelUtils::getEventTypeLabel(event.type).c_str(),
		  event.brief.c_str());
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Trigger ID: %" FMT_TRIGGER_ID "\n"
		  "    Status:     \"%d (%s)\"\n"
		  "    Severity:   \"%d (%s)\"\n",
		  event.triggerId.c_str(),
		  event.status,
		  LabelUtils::getTriggerStatusLabel(event.status).c_str(),
		  event.severity,
		  LabelUtils::getTriggerSeverityLabel(event.severity).c_str());
	return desc;
}

const HatoholError &IncidentSender::getLastResult(void) const
{
	return m_impl->lastResult;
}

void IncidentSender::setLastResult(const HatoholError &err)
{
	m_impl->lastResult = err;
}

gpointer IncidentSender::mainThread(HatoholThreadArg *arg)
{
	const IncidentTrackerInfo &tracker = m_impl->incidentTrackerInfo;
	Job *job;
	MLPL_INFO("Start IncidentSender thread for %" FMT_INCIDENT_TRACKER_ID ":%s\n",
		  tracker.id, tracker.nickname.c_str());
	while ((job = m_impl->waitNextJob())) {
		if (!isExitRequested())
			m_impl->trySend(*job);
		m_impl->runningJob = NULL;
		delete job;
	}
	MLPL_INFO("Exited IncidentSender thread for %" FMT_INCIDENT_TRACKER_ID ":%s\n",
		  tracker.id, tracker.nickname.c_str());
	return NULL;
}
