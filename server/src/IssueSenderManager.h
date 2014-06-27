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

#ifndef IssueSenderManager_h
#define IssueSenderManager_h

#include "Params.h"
#include "DBClientHatohol.h"
#include "IssueSender.h"

class IssueSenderManager
{
public:
	static IssueSenderManager &getInstance(void);

	void queue(const IssueTrackerIdType &trackerId,
		   const EventInfo &info,
		   IssueSender::StatusCallback callback = NULL,
		   void *userData = NULL);
	bool isIdling(void);

protected:
	IssueSenderManager(void);
	virtual ~IssueSenderManager();

	IssueSender *getSender(const IssueTrackerIdType &id,
			       bool autoCreate = false);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // IssuesSenderManager_h
