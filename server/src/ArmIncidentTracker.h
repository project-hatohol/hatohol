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
#include "ArmBase.h"
#include "DBTablesConfig.h"

class ArmIncidentTracker : public ArmBase
{
public:
	ArmIncidentTracker(const std::string &name,
			   const IncidentTrackerInfo &trackerInfo);
	virtual ~ArmIncidentTracker();

	virtual void startIfNeeded(void) = 0;
	virtual bool isFetchItemsSupported(void) const override;

	static ArmIncidentTracker *create(
	  const IncidentTrackerInfo &trackerInfo);

protected:
	const IncidentTrackerInfo getIncidentTrackerInfo(void);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

