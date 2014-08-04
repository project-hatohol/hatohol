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

#ifndef IncidentSenderManager_h
#define IncidentSenderManager_h

#include "Params.h"
#include "DBClientHatohol.h"
#include "IncidentSender.h"

class IncidentSenderManager
{
public:
	static IncidentSenderManager &getInstance(void);

	void queue(const IncidentTrackerIdType &trackerId,
		   const EventInfo &info,
		   IncidentSender::StatusCallback callback = NULL,
		   void *userData = NULL);
	bool isIdling(void);

protected:
	IncidentSenderManager(void);
	virtual ~IncidentSenderManager();

	IncidentSender *getSender(const IncidentTrackerIdType &id,
				  bool autoCreate = false);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // IncidentSenderManager_h
