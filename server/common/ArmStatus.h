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
#include <string>
#include <memory>
#include <SmartTime.h>

#include "Params.h"

enum ArmWorkingStatus {
	ARM_WORK_STAT_INIT,
	ARM_WORK_STAT_OK,
	ARM_WORK_STAT_FAILURE,
};

struct ArmInfo
{
	bool             running;

	ArmWorkingStatus stat;
	mlpl::SmartTime  statUpdateTime;
	std::string      failureComment;

	mlpl::SmartTime  lastSuccessTime;
	mlpl::SmartTime  lastFailureTime;

	size_t           numUpdate;
	size_t           numFailure;
	
	// Constructor
	ArmInfo(void);
};

class ArmStatus {
public:
	ArmStatus(void);
	virtual ~ArmStatus();

	/**
	 * Get Information about Arm, which is a component to get monitoring
	 * data from a monitoring system such as Zabbix and Nagios.
	 *
	 * @return An instance of ArmInfo.
	 */
	ArmInfo getArmInfo(void) const;

	void setRunningStatus(const bool &running);

	void logSuccess(void);
	void logFailure(const std::string &comment = "",
	                const ArmWorkingStatus &status = ARM_WORK_STAT_FAILURE);
	void setArmInfo(const ArmInfo &armInfo);

protected:

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

