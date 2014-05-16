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

#ifndef IssueSenderRedmine_h
#define IssueSenderRedmine_h

#include "IssueSender.h"

class IssueSenderRedmine : public IssueSender
{
public:
	IssueSenderRedmine(const IssueTrackerInfo &tracker);
	virtual ~IssueSenderRedmine();

	virtual HatoholError send(const EventInfo &event); // override

protected:
	std::string buildJson(const EventInfo &event);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // IssueSenderRedmine_h
