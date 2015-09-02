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

#include "IncidentSenderHatohol.h"

using namespace std;
using namespace mlpl;

struct IncidentSenderHatohol::Impl
{
	Impl(IncidentSenderHatohol &sender)
	: m_sender(sender)
	{
	}
	virtual ~Impl()
	{
	}

	IncidentSenderHatohol &m_sender;
};

IncidentSenderHatohol::IncidentSenderHatohol(const IncidentTrackerInfo &tracker)
: IncidentSender(tracker),
  m_impl(new Impl(*this))
{
}

IncidentSenderHatohol::~IncidentSenderHatohol()
{
}

HatoholError IncidentSenderHatohol::send(const EventInfo &event)
{
	return HTERR_NOT_IMPLEMENTED;
}

HatoholError IncidentSenderHatohol::send(const IncidentInfo &incident,
					 const std::string &comment)
{
	return HTERR_NOT_IMPLEMENTED;
}
