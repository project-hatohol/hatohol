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

#include "IncidentSender.h"
#include "StringUtils.h"
#include "LabelUtils.h"
#include "CacheServiceDBClient.h"
#include <Mutex.h>
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
	EventInfo eventInfo;
	StatusCallback callback;
	void *userData;

	Job(const EventInfo &_eventInfo, StatusCallback _callback = NULL,
	    void *_userData = NULL)
	: eventInfo(_eventInfo), callback(_callback), userData(_userData)
	{
	}

	void notifyStatus(const JobStatus &status) const
	{
		if (callback)
			callback(eventInfo, status, userData);
	}
};

struct IncidentSender::PrivateContext
{
	IncidentSender &sender;
	IncidentTrackerInfo incidentTrackerInfo;
	Mutex            queueLock;
	std::queue<Job*> queue;
	AtomicValue<Job*> runningJob;
	SimpleSemaphore jobSemaphore;
	size_t retryLimit;
	unsigned int retryIntervalMSec;

	PrivateContext(IncidentSender &_sender)
	: sender(_sender), runningJob(NULL), jobSemaphore(0),
	  retryLimit(DEFAULT_RETRY_LIMIT),
	  retryIntervalMSec(DEFAULT_RETRY_INTERVAL_MSEC)
	{
	}
 
	~PrivateContext()
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
		job->notifyStatus(JOB_QUEUED);
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
		job->notifyStatus(JOB_STARTED);
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

	HatoholError trySend(const Job &job) {
		HatoholError result;
		for (size_t i = 0; i <= retryLimit; i++) {
			result = sender.send(job.eventInfo);
			if (result == HTERR_OK)
				break;
			if (i == retryLimit)
				break;
			if (sender.isExitRequested())
				break;

			job.notifyStatus(JOB_WAITING_RETRY);
			usleep(retryIntervalMSec * 1000);

			if (sender.isExitRequested())
				break;
			job.notifyStatus(JOB_RETRYING);
		}
		if (result == HTERR_OK)
			job.notifyStatus(JOB_SUCCEEDED);
		else
			job.notifyStatus(JOB_FAILED);
		return result;
	}
};

IncidentSender::IncidentSender(const IncidentTrackerInfo &tracker)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(*this);
	m_ctx->incidentTrackerInfo = tracker;
}

IncidentSender::~IncidentSender()
{
	exitSync();
	delete m_ctx;
}

void IncidentSender::waitExit(void)
{
	m_ctx->jobSemaphore.post();
	HatoholThreadBase::waitExit();
}

void IncidentSender::queue(const EventInfo &eventInfo,
			   StatusCallback callback, void *userData)
{
	Job *job = new Job(eventInfo, callback, userData);
	m_ctx->pushJob(job);
}

void IncidentSender::setRetryLimit(const size_t &limit)
{
	m_ctx->retryLimit = limit;
}

void IncidentSender::setRetryInterval(const unsigned int &msec)
{
	m_ctx->retryIntervalMSec = msec;
}

bool IncidentSender::isIdling(void)
{
	AutoMutex autoMutex(&m_ctx->queueLock);
	if (!m_ctx->queue.empty())
		return false;
	return !m_ctx->runningJob;
}

const IncidentTrackerInfo &IncidentSender::getIncidentTrackerInfo(void)
{
	return m_ctx->incidentTrackerInfo;
}

bool IncidentSender::getServerInfo(const EventInfo &event,
				MonitoringServerInfo &server)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	ServerQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(event.serverId);
	MonitoringServerInfoList servers;
	dbConfig->getTargetServers(servers, option);
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
	if (server)
		desc += StringUtils::sprintf(
			  "    Hostname:   \"%s\"\n"
			  "    IP Address: \"%s\"\n"
			  "    Nickname:   \"%s\"\n",
			  server->hostName.c_str(),
			  server->ipAddress.c_str(),
			  server->nickname.c_str());
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Host ID: %" FMT_HOST_ID "\n"
		  "    Hostname:   \"%s\"\n",
		  event.hostId, event.hostName.c_str());
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Event ID: %" FMT_EVENT_ID "\n"
		  "    Time:       \"%ld.%09ld (%s)\"\n"
		  "    Type:       \"%d (%s)\"\n"
		  "    Brief:      \"%s\"\n",
		  event.id,
		  event.time.tv_sec, event.time.tv_nsec, timeString,
		  event.type,
		  LabelUtils::getEventTypeLabel(event.type).c_str(),
		  event.brief.c_str());
	desc += "\n";
	desc += StringUtils::sprintf(
		  "Trigger ID: %" FMT_TRIGGER_ID "\n"
		  "    Status:     \"%d (%s)\"\n"
		  "    Severity:   \"%d (%s)\"\n",
		  event.triggerId,
		  event.status,
		  LabelUtils::getTriggerStatusLabel(event.status).c_str(),
		  event.severity,
		  LabelUtils::getTriggerSeverityLabel(event.severity).c_str());
	return desc;
}

gpointer IncidentSender::mainThread(HatoholThreadArg *arg)
{
	const IncidentTrackerInfo &tracker = m_ctx->incidentTrackerInfo;
	Job *job;
	MLPL_INFO("Start IncidentSender thread for %" FMT_INCIDENT_TRACKER_ID ":%s\n",
		  tracker.id, tracker.nickname.c_str());
	while ((job = m_ctx->waitNextJob())) {
		if (!isExitRequested())
			m_ctx->trySend(*job);
		m_ctx->runningJob = NULL;
		delete job;
	}
	MLPL_INFO("Exited IncidentSender thread for %" FMT_INCIDENT_TRACKER_ID ":%s\n",
		  tracker.id, tracker.nickname.c_str());
	return NULL;
}
