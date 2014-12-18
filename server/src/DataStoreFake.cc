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

#include "DataStoreFake.h"

#include <cstdio>

struct DataStoreFake::Impl
{
	ArmFake  armFake;

	Impl(const MonitoringServerInfo &serverInfo)
	: armFake(serverInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreFake::DataStoreFake(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: m_impl(new Impl(serverInfo))
{
	if (autoStart)
		m_impl->armFake.start();
}

DataStoreFake::~DataStoreFake()
{
}

ArmBase &DataStoreFake::getArmBase(void)
{
	return m_impl->armFake;
}

void DataStoreFake::setCopyOnDemandEnable(bool enable)
{
	m_impl->armFake.setCopyOnDemandEnabled(enable);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
