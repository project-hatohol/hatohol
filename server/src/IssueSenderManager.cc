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

#include <queue>
#include "MutexLock.h"
#include "IssueSenderManager.h"
#include "HatoholThreadBase.h"

using namespace std;
using namespace mlpl;

struct IssueSenderManager::Job
{
	IssueTrackerIdType trackerId;
	EventInfo          eventInfo;
	Job(const IssueTrackerIdType &_trackerId,
	    const EventInfo &_eventInfo)
	: trackerId(_trackerId), eventInfo(_eventInfo)
	{
	}
};

struct IssueSenderManager::PrivateContext
{
	static IssueSenderManager instance;
	MutexLock queueLock;
	std::queue<Job*> queue;

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
};

IssueSenderManager IssueSenderManager::PrivateContext::instance;

class IssueSenderManager::Worker : public HatoholThreadBase
{
public:
	Worker(void);
	~Worker();

protected:
	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		return NULL;
	}
};

IssueSenderManager &IssueSenderManager::getInstance(void)
{
	return PrivateContext::instance;
}

void IssueSenderManager::queue(
  const IssueTrackerIdType &trackerId, const EventInfo &eventInfo)
{
	Job *job = new Job(trackerId, eventInfo);
	m_ctx->queueLock.lock();
	m_ctx->queue.push(job);
	m_ctx->queueLock.unlock();
}

IssueSenderManager::IssueSenderManager(void)
{
	m_ctx = new PrivateContext();
}

IssueSenderManager::~IssueSenderManager()
{
	delete m_ctx;
}
