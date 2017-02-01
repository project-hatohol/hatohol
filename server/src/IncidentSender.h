/*
 * Copyright (C) 2014-2015 Project Hatohol
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

#pragma once
#include "HatoholError.h"
#include "DBTablesConfig.h"
#include "DBTablesMonitoring.h"
#include "HatoholThreadBase.h"

class IncidentSender : public HatoholThreadBase
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

	typedef void (*CreateIncidentCallback)(const IncidentSender &sender,
					       const EventInfo &info,
					       const JobStatus &status,
					       void *userData);
	typedef void (*UpdateIncidentCallback)(const IncidentSender &sender,
					       const IncidentInfo &info,
					       const JobStatus &status,
					       void *userData);

	IncidentSender(const IncidentTrackerInfo &tracker,
		       bool shouldRecordIncidentHistory);
	virtual ~IncidentSender();

	virtual void waitExit(void) override;

	/**
	 * Send an EventInfo as an incident to the specified incident tracking
	 * system synchronously. It will be done by the same thread with the
	 * caller. You don't need start the thread by start() when you use this
	 * function directly.
	 *
	 * @param event
	 * An EventInfo to send as an incident.
	 *
	 * @param incident
	 * A generated IncidentInfo. It's filled only when the function returns
	 * HTERR_OK.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	virtual HatoholError send(const EventInfo &event,
				  IncidentInfo *incident = NULL) = 0;

	/**
	 * Send an IncidentInfo to update existing one. It will be done by the
	 * same thread with the caller. You don't need start the thread by
	 * start() when you use this function directly.
	 *
	 * @param incident
	 * An IncidentInfo to update.
	 *
	 * @param comment
	 * A string to post as an additional comment for the incident.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	virtual HatoholError send(const IncidentInfo &incident,
				  const std::string &comment) = 0;

	/**
	 * Queue an EventInfo to send it as an incident to the specified
	 * incident tracking system asynchronously. Sending an incident will be
	 * done by a dedicated thread (it will call send() internally). You must
	 * start the thread by calling start() before or after calling this
	 * function.
	 *
	 * @param event
	 * An EventInfo to send as an incident.
	 *
	 * @param callback
	 * A callback function which will be called when the status of
	 * sending job is changed.
	 *
	 * @param userData
	 * A user data which will be passed to the callback function.
	 *
	 * @param userId
	 * The user ID of the operator.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	void queue(const EventInfo &eventInfo,
		   CreateIncidentCallback callback = NULL,
		   void *userData = NULL,
		   const UserIdType userId = USER_ID_SYSTEM);

	/**
	 * Queue an IncidentInfo to update existing one. Sending an incident
	 * will be done by a dedicated thread (it will call send() internally).
	 * You must start the thread by calling start() before or after calling
	 * this function.
	 *
	 * @param incident
	 * An IncidentInfo to update.
	 *
	 * @param comment
	 * A string to post as an additional comment for the incident.
	 *
	 * @param callback
	 * A callback function which will be called when the status of
	 * sending job is changed.
	 *
	 * @param userData
	 * A user data which will be passed to the callback function.
	 *
	 * @param userId
	 * The user ID of the operator.
	 *
	 * @return HTERR_OK on succeeded to send. Otherwise an error.
	 */
	void queue(const IncidentInfo &incidentInfo,
		   const std::string &comment,
		   UpdateIncidentCallback callback = NULL,
		   void *userData = NULL,
		   const UserIdType userId = USER_ID_SYSTEM);

	/**
	 * Set max retry count of sending an incident on failing it.
	 * It affects only to queue(). send() function never retry to send
	 * automatically.
	 *
	 * @param limit
	 * A max count of retrying to send an incident.
	 */
	void setRetryLimit(const size_t &limit);

	/**
	 * Set the interval time of retrying to send an incident
	 *
	 * @param msec
	 * Interval time of retrying to send an incident [msec].
	 */
	void setRetryInterval(const unsigned int &msec);

	/**
	 * Check whether all queued sending jobs are finished or not.
	 *
	 * @return
	 * true when there is no running job and no queued job, otherwise false.
	 */
	bool isIdling(void);

	const IncidentTrackerInfo getIncidentTrackerInfo(void);
	void setOnChangedIncidentTracker(void);

	const HatoholError &getLastResult(void) const;

protected:
	bool getServerInfo(const EventInfo &event,
			   MonitoringServerInfo &server);
	virtual std::string buildTitle(
	  const EventInfo &event,
	  const MonitoringServerInfo *server);
	virtual std::string buildDescription(
	  const EventInfo &event,
	  const MonitoringServerInfo *server);
	void setLastResult(const HatoholError &err);

	virtual gpointer mainThread(HatoholThreadArg *arg) override;

private:
	struct Job;
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

