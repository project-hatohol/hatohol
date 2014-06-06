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

#ifndef IssueSender_h
#define IssueSender_h

#include "HatoholError.h"
#include "DBClientConfig.h"
#include "DBClientHatohol.h"
#include "HatoholThreadBase.h"

class IssueSender : public HatoholThreadBase
{
public:
	typedef enum {
		JOB_QUEUED,
		JOB_STARTED,
		JOB_WAITING_RETRY,
		JOB_RETRYING,
		JOB_SUCCEEDED,
		JOB_FAILED,
	} JobStatus;

	typedef void (*StatusCallback)(const EventInfo &info,
				       const JobStatus &status,
				       void *userData);

	IssueSender(const IssueTrackerInfo &tracker);
	virtual ~IssueSender();

	virtual void waitExit(void) override;

	/**
	 * Send an EventInfo as an issue to the specified issue tracking system
	 * synchronously. It will be done by the same thread with the caller.
	 * You don't need start the thread by start() when you use this
	 * function directly.
	 *
	 * @param event
	 * An EventInfo to send as an issue.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	virtual HatoholError send(const EventInfo &event) = 0;

	/**
	 * Queue an EventInfo to send it as an issue to the specified issue
	 * tracking system asynchronously. Sending an issue will be done by a
	 * dedicated thread (it will call send() internally). You must start
	 * the thread by calling start() before or after calling this function.
	 *
	 * @param event
	 * An EventInfo to send as an issue.
	 *
	 * @param callback
	 * A callback function which will be called when the status of
	 * sending job is changed.
	 *
	 * @param userData
	 * A user data which will be passed to the callback function.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	void queue(const EventInfo &eventInfo,
		   StatusCallback callback = NULL,
		   void *userData = NULL);

	/**
	 * Set max retry count of sending an issue on failing it.
	 * It affects only to queue(). send() function never retry to send
	 * automatically.
	 *
	 * @param limit
	 * A max count of retrying to send an issue.
	 */
	void setRetryLimit(const size_t &limit);

	/**
	 * Set the interval time of retrying to send an issue
	 *
	 * @param msec
	 * Interval time of retrying to send an issue [msec].
	 */
	void setRetryInterval(const unsigned int &msec);

	/**
	 * Check whether all queued sending jobs are finished or not.
	 *
	 * @return
	 * true when there is no running job and no queued job, otherwise false.
	 */
	bool isIdling(void);

protected:
	const IssueTrackerInfo &getIssueTrackerInfo(void);
	bool getServerInfo(const EventInfo &event,
			   MonitoringServerInfo &server);
	std::string buildTitle(const EventInfo &event,
			       const MonitoringServerInfo *server);
	std::string buildDescription(const EventInfo &event,
				     const MonitoringServerInfo *server);

	virtual gpointer mainThread(HatoholThreadArg *arg) override;

private:
	struct Job;
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // IssueSender_h

