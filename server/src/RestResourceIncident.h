/*
 * Copyright (C) 2016 Project Hatohol
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
#include "FaceRestPrivate.h"

struct RestResourceIncident : public RestResourceMemberHandler
{
	typedef void (RestResourceIncident::*HandlerFunc)(void);

	static void registerFactories(FaceRest *faceRest);

	RestResourceIncident(FaceRest *faceRest, HandlerFunc handler);
	virtual ~RestResourceIncident();

	void handlerIncident(void);
	void handlerGetIncident(void);
	void handlerGetIncidentHistory(const UnifiedEventIdType unifiedEventId);
	void handlerPostIncident(void);
	void handlerPutIncident(void);
	void createIncidentAsync(const UnifiedEventIdType &eventId,
				 const IncidentTrackerIdType &trackerId);

	void handlerIncidentComment(void);
	void handlerPutIncidentComment(void);
	void handlerDeleteIncidentComment(void);

	static const char *pathForIncident;
	static const char *pathForIncidentComment;

 private:
	bool getRequestedIncidentHistory(IncidentHistory &history);
	void updateIncidentHistory(IncidentHistory &history);
};

