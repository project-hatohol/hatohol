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
#include "Params.h"
#include "DBTablesMonitoring.h"
#include "IncidentSender.h"

class IncidentSenderManager
{
public:
	static IncidentSenderManager &getInstance(void);

	/**
	 * Queue a job to register an incident which will be tied to an event.
	 *
	 * @param trackerId
	 * An ID of IncidentTracker to register an incident.
	 * @param event
	 * An event which will be tied to the registered incident.
	 * @param callback
	 * A callback function which will be called when the IncidentSender
	 * succeed or fail to register an incident.
	 * @param userData
	 * A data which is passed to the callback function.
	 * @param userId
	 * The user ID of the operator.
	 *
	 * @return A HatoholError instance.
	 */
	HatoholError queue(const IncidentTrackerIdType &trackerId,
			   const EventInfo &event,
			   IncidentSender::CreateIncidentCallback callback = NULL,
			   void *userData = NULL,
			   const UserIdType userId = USER_ID_SYSTEM);

	/**
	 * Queue a job to update an incident.
	 *
	 * @param incidentInfo
	 * An IncidentInfo to update.
	 * @param comment
	 * A comment string to add to the incident. Note that the incident
	 * tracker may not support adding comments.
	 * @param callback
	 * A callback function which will be called when the IncidentSender
	 * succeed or fail to send an incident.
	 * @param userData
	 * A data which is passed to the callback function.
	 * @param userId
	 * The user ID of the operator.
	 *
	 * @return A HatoholError instance.
	 */
	HatoholError queue(const IncidentInfo &incidentInfo,
			   const std::string &comment,
			   IncidentSender::UpdateIncidentCallback callback = NULL,
			   void *userData = NULL,
			   const UserIdType userId = USER_ID_SYSTEM);

	bool isIdling(void);
	void setOnChangedIncidentTracker(const IncidentTrackerIdType id);
	void deleteIncidentTracker(const IncidentTrackerIdType id);

protected:
	IncidentSenderManager(void);
	virtual ~IncidentSenderManager();

	IncidentSender *getSender(const IncidentTrackerIdType &id,
				  bool autoCreate = false);
	void deleteSender(const IncidentTrackerIdType &id);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

