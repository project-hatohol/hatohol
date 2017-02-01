/*
 * Copyright (C) 2015 Project Hatohol
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
#include "IncidentSender.h"

class IncidentSenderHatohol : public IncidentSender
{
public:
	static const std::string STATUS_NONE;
	static const std::string STATUS_HOLD;
	static const std::string STATUS_IN_PROGRESS;
	static const std::string STATUS_DONE;

	IncidentSenderHatohol(const IncidentTrackerInfo &tracker,
			      bool shouldRecordIncidentHistory = true);
	virtual ~IncidentSenderHatohol();

	virtual HatoholError send(const EventInfo &event,
				  IncidentInfo *incident = NULL) override;

	virtual HatoholError send(const IncidentInfo &incident,
				  const std::string &comment) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

