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
DataStoreNagios::DataStoreNagios(const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
	m_ctx->armNDO.start();
}

DataStoreNagios::~DataStoreNagios()
{
	if (m_ctx)
		delete m_ctx;
}

void DataStoreNagios::update(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	m_ctx->armNDO.forceUpdate();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
