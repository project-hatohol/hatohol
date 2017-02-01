/*
 * Copyright (C) 2014 Project Hatohol
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
#include "ArmIncidentTracker.h"
#include "DBTablesConfig.h"

class ArmRedmine : public ArmIncidentTracker
{
public:
	ArmRedmine(const IncidentTrackerInfo &trackerInfo);
	virtual ~ArmRedmine();

	virtual void startIfNeeded(void) override;

protected:
	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg) override;
	virtual ArmBase::ArmPollingResult mainThreadOneProc(void) override;

	std::string getURL(void);
	std::string getQuery(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

