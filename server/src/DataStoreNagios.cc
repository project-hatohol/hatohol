/*
 * Copyright (C) 2013 Project Hatohol
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

#include "DataStoreNagios.h"

#include <cstdio>

struct DataStoreNagios::PrivateContext
{
	ArmNagiosNDOUtils  armNDO;

	PrivateContext(const MonitoringServerInfo &serverInfo)
	: armNDO(serverInfo)
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreNagios::DataStoreNagios(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	if (autoStart)
		m_ctx->armNDO.start();
}

DataStoreNagios::~DataStoreNagios()
{
	delete m_ctx;
}

ArmBase &DataStoreNagios::getArmBase(void)
{
	return m_ctx->armNDO;
}

void DataStoreNagios::setCopyOnDemandEnable(bool enable)
{
	m_ctx->armNDO.setCopyOnDemandEnabled(enable);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
