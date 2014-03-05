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

#include "DataStoreFake.h"

#include <cstdio>

struct DataStoreFake::PrivateContext
{
	ArmFake  armFake;

	PrivateContext(const MonitoringServerInfo &serverInfo)
	: armFake(serverInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreFake::DataStoreFake(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	if (autoStart)
		m_ctx->armFake.start();
}

DataStoreFake::~DataStoreFake()
{
	if (m_ctx)
		delete m_ctx;
}

ArmBase &DataStoreFake::getArmBase(void)
{
	return m_ctx->armFake;
}

void DataStoreFake::setCopyOnDemandEnable(bool enable)
{
	m_ctx->armFake.setCopyOnDemandEnabled(enable);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
